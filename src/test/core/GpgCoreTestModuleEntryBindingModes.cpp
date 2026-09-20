/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <gtest/gtest.h>

#include <QByteArray>

#include "GpgFrontendTest.h"
#include "core/module/ModuleEntryBinding.h"

/**
 * @file GpgCoreTestModuleEntryBindingModes.cpp
 * @brief The three ways a descriptor binds its entry, and why they differ.
 *
 * A full-file digest is right on Linux and wrong on the other two, because
 * Windows Authenticode signing and macOS code signing both rewrite bytes of a
 * file that is otherwise unchanged. Binding to raw bytes there would either
 * forbid normal platform signing or force the descriptor to be regenerated
 * after it -- and on macOS, regenerating it after signing is what would drag a
 * build-produced packager into a privileged signing job.
 *
 * So each platform binds what is stable on it, and the interesting assertions
 * here are the ones that say what each mode deliberately does NOT notice.
 *
 * These run on Linux, against synthesised images, which is the point: the
 * Windows and macOS paths are the ones no developer exercises by accident.
 */

namespace GpgFrontend::Test {

namespace {

void PutU16(QByteArray& b, qsizetype at, quint16 v) {
  b[at] = static_cast<char>(v & 0xFF);
  b[at + 1] = static_cast<char>((v >> 8) & 0xFF);
}

void PutU32(QByteArray& b, qsizetype at, quint32 v) {
  for (int i = 0; i < 4; ++i) {
    b[at + i] = static_cast<char>((v >> (8 * i)) & 0xFF);
  }
}

/// A minimal but structurally real PE32+ image: MZ stub, PE signature, COFF
/// header, optional header with a data directory, one section.
///
/// Synthesised rather than committed as a fixture because what is being tested
/// is which REGIONS the digest covers, and a hand-built image makes each of
/// those regions addressable by name below.
auto SyntheticPe(qsizetype section_bytes = 512) -> QByteArray {
  constexpr qsizetype kPeAt = 0x80;
  constexpr qsizetype kOptionalAt = kPeAt + 24;
  constexpr qsizetype kDirectoriesAt = kOptionalAt + 112;
  constexpr qsizetype kOptionalSize = 112 + 16 * 8;
  constexpr qsizetype kSectionTableAt = kOptionalAt + kOptionalSize;
  const qsizetype headers = 1024;

  QByteArray pe(headers + section_bytes, '\0');
  pe[0] = 'M';
  pe[1] = 'Z';
  PutU32(pe, 0x3C, static_cast<quint32>(kPeAt));

  pe[kPeAt] = 'P';
  pe[kPeAt + 1] = 'E';

  PutU16(pe, kPeAt + 6, 1);  // NumberOfSections
  PutU16(pe, kPeAt + 20, static_cast<quint16>(kOptionalSize));
  PutU16(pe, kOptionalAt, 0x20B);                               // PE32+
  PutU32(pe, kOptionalAt + 60, static_cast<quint32>(headers));  // SizeOfHeaders
  PutU32(pe, kOptionalAt + 64, 0xDEADBEEF);                     // CheckSum
  PutU32(pe, kOptionalAt + 108, 16);                            // dir count

  // One section, following the headers.
  PutU32(pe, kSectionTableAt + 16, static_cast<quint32>(section_bytes));
  PutU32(pe, kSectionTableAt + 20, static_cast<quint32>(headers));

  // Recognisable content, so a mutation below is clearly inside the section.
  for (qsizetype i = 0; i < section_bytes; ++i) {
    pe[headers + i] = static_cast<char>('A' + (i % 26));
  }

  Q_UNUSED(kDirectoriesAt)
  return pe;
}

constexpr qsizetype kPeOptionalAt = 0x80 + 24;
constexpr qsizetype kPeChecksumAt = kPeOptionalAt + 64;
constexpr qsizetype kPeCertificateEntryAt = kPeOptionalAt + 112 + 4 * 8;

/// Sign @p pe the way a real Authenticode signer does.
///
/// The earlier version of this helper appended the certificate directly, which
/// is not what happens: an attribute certificate entry must begin on an
/// EIGHT-BYTE boundary, so the signer zero-pads the file first. On a synthetic
/// image whose size was already a multiple of eight that distinction is
/// invisible -- which is exactly why the digest looked stable here and was not
/// stable in CI.
auto SignPeFaithfully(const QByteArray& pe, char filler = '\xAB',
                      qsizetype certificate_bytes = 256) -> QByteArray {
  auto signed_pe = pe;

  // The signer pads to an 8-byte boundary before the certificate table, and
  // those padding bytes fall INSIDE the hashed region.
  const auto padding = (8 - (signed_pe.size() % 8)) % 8;
  signed_pe.append(QByteArray(padding, '\0'));

  const auto certificate_at = signed_pe.size();
  const QByteArray certificate(certificate_bytes, filler);
  signed_pe.append(certificate);

  PutU32(signed_pe, kPeCertificateEntryAt,
         static_cast<quint32>(certificate_at));
  PutU32(signed_pe, kPeCertificateEntryAt + 4,
         static_cast<quint32>(certificate.size()));

  // And it recomputes the checksum.
  PutU32(signed_pe, kPeChecksumAt, 0x12345678);
  return signed_pe;
}

}  // namespace

// ------------------------------------------------------------ linux

TEST(ModuleEntryBindingModesTest, TheLinuxModeIsAWholeFileDigest) {
  // Nothing legitimately rewrites an ELF after the build, so the strictest of
  // the three is also the correct one there.
  const QByteArray a(
      "\x7f"
      "ELF and then some contents",
      26);
  auto b = a;
  b[20] = static_cast<char>(b[20] ^ 1);
  EXPECT_NE(a, b);
}

// ---------------------------------------------------------- windows

TEST(ModuleEntryBindingModesTest, ThePeDigestCoversExecutableContent) {
  QString reason;
  const auto pe = SyntheticPe();
  const auto before = Module::PeAuthenticodeDigest(pe, reason);
  ASSERT_FALSE(before.isEmpty()) << reason.toStdString();

  auto mutated = pe;
  mutated[1024 + 10] = static_cast<char>(mutated[1024 + 10] ^ 1);

  const auto after = Module::PeAuthenticodeDigest(mutated, reason);
  ASSERT_FALSE(after.isEmpty()) << reason.toStdString();
  EXPECT_NE(before, after) << "a changed instruction must change the binding";
}

TEST(ModuleEntryBindingModesTest, ThePeDigestIgnoresCertificateMaterial) {
  // THE property the whole Windows mode exists for, and the one that makes
  // the Authenticode prohibition unnecessary: a module DLL may be signed and
  // timestamped after its descriptor is final, by CI or by hand, and the
  // binding still holds. Signing writes exactly these three regions.
  QString reason;
  const auto pe = SyntheticPe();
  const auto before = Module::PeAuthenticodeDigest(pe, reason);
  ASSERT_FALSE(before.isEmpty()) << reason.toStdString();

  const auto signed_pe = SignPeFaithfully(pe);
  const auto after = Module::PeAuthenticodeDigest(signed_pe, reason);
  ASSERT_FALSE(after.isEmpty()) << reason.toStdString();
  EXPECT_EQ(before, after)
      << "Authenticode signing must be invisible to the descriptor binding";

  // And re-signing with a different certificate is equally invisible.
  EXPECT_EQ(before,
            Module::PeAuthenticodeDigest(SignPeFaithfully(pe, '\xCD'), reason));
}

TEST(ModuleEntryBindingModesTest, ThePeDigestSurvivesSigningAnUnalignedImage) {
  // The case CI found and the synthetic image hid.
  //
  // A signer pads the file to an 8-byte boundary before appending the
  // certificate table, and that padding lands inside the hashed region. An
  // implementation that hashes "everything up to EOF" when unsigned, and
  // "everything up to EOF minus the certificate size" when signed, silently
  // disagrees with itself by exactly those padding bytes -- so the descriptor
  // stops matching the DLL the moment anyone signs it.
  //
  // Sizes chosen to land on every residue mod 8, because the failure only
  // appears when the unaligned ones are exercised.
  for (const qsizetype section_bytes :
       {qsizetype{500}, qsizetype{501}, qsizetype{502}, qsizetype{503},
        qsizetype{504}, qsizetype{505}, qsizetype{506}, qsizetype{507}}) {
    QString reason;
    const auto pe = SyntheticPe(section_bytes);
    const auto before = Module::PeAuthenticodeDigest(pe, reason);
    ASSERT_FALSE(before.isEmpty()) << reason.toStdString();

    const auto after =
        Module::PeAuthenticodeDigest(SignPeFaithfully(pe), reason);
    ASSERT_FALSE(after.isEmpty()) << reason.toStdString();

    EXPECT_EQ(before, after)
        << "a " << section_bytes << "-byte section (file size " << pe.size()
        << ", " << (pe.size() % 8) << " mod 8) changed digest when signed";
  }
}

TEST(ModuleEntryBindingModesTest,
     ThePeDigestSurvivesResigningAtADifferentSize) {
  // Timestamping enlarges a signature, and re-signing with a different
  // certificate changes its size too. Neither may move the digest, or a DLL
  // signed once and timestamped afterwards would stop matching its
  // descriptor -- which §14.2 explicitly promises does not happen.
  QString reason;
  const auto pe = SyntheticPe(503);  // not 8-aligned, so padding is in play
  const auto before = Module::PeAuthenticodeDigest(pe, reason);
  ASSERT_FALSE(before.isEmpty()) << reason.toStdString();

  for (const qsizetype size :
       {qsizetype{8}, qsizetype{256}, qsizetype{1024}, qsizetype{4096}}) {
    const auto digest = Module::PeAuthenticodeDigest(
        SignPeFaithfully(pe, '\xAB', size), reason);
    EXPECT_EQ(before, digest)
        << "a " << size << "-byte certificate changed the digest";
  }
}

TEST(ModuleEntryBindingModesTest, ThePeDigestSurvivesSigningWithATrailingGap) {
  // A real PE need not end exactly where its last section does. Trailing
  // bytes are hashed, and must still be hashed identically once a certificate
  // sits behind them.
  QString reason;
  auto pe = SyntheticPe();
  pe.append(QByteArray(37, '\x5A'));  // deliberately not a multiple of 8

  const auto before = Module::PeAuthenticodeDigest(pe, reason);
  ASSERT_FALSE(before.isEmpty()) << reason.toStdString();

  const auto after = Module::PeAuthenticodeDigest(SignPeFaithfully(pe), reason);
  EXPECT_EQ(before, after)
      << "trailing data before the certificate table must hash the same way "
         "before and after signing";
}

TEST(ModuleEntryBindingModesTest, ThePeDigestRefusesWhatIsNotAPe) {
  QString reason;
  EXPECT_TRUE(
      Module::PeAuthenticodeDigest(QByteArray("not a pe at all"), reason)
          .isEmpty());
  EXPECT_FALSE(reason.isEmpty());

  // Truncated after the MZ stub: the offsets it would read are past the end.
  auto truncated = SyntheticPe();
  truncated.truncate(0x50);
  EXPECT_TRUE(Module::PeAuthenticodeDigest(truncated, reason).isEmpty());
}

// ------------------------------------------------------------ macos

TEST(ModuleEntryBindingModesTest, TheBindingIdIsDeterministic) {
  const Module::ModuleEntryBindingContext a{"com.example.module.email",
                                            "gfb1-abc", 3};
  EXPECT_EQ(Module::ModuleEntryBindingId(a), Module::ModuleEntryBindingId(a));
  EXPECT_EQ(Module::ModuleEntryBindingId(a).size(), 64);
}

TEST(ModuleEntryBindingModesTest, EveryBindingIdInputChangesIt) {
  // Three separate cases rather than one combined change: a derivation that
  // silently ignored an input would pass a test that varied them together.
  const Module::ModuleEntryBindingContext base{"com.example.module.email",
                                               "gfb1-abc", 3};
  const auto reference = Module::ModuleEntryBindingId(base);

  auto other_module = base;
  other_module.module_id = "com.example.module.other";
  EXPECT_NE(Module::ModuleEntryBindingId(other_module), reference)
      << "a dylib from another module must not satisfy this descriptor";

  auto other_build = base;
  other_build.build_id = "gfb1-def";
  EXPECT_NE(Module::ModuleEntryBindingId(other_build), reference)
      << "a module from another build must not satisfy this descriptor";

  auto other_abi = base;
  other_abi.sdk_abi = 4;
  EXPECT_NE(Module::ModuleEntryBindingId(other_abi), reference)
      << "an incompatible ABI must not satisfy this descriptor";
}

TEST(ModuleEntryBindingModesTest, TheBindingIdSeparatorsAreUnambiguous) {
  // Without separators these two would hash the same bytes. They are
  // different modules and must have different bindings.
  const Module::ModuleEntryBindingContext a{"com.example.ab", "c", 3};
  const Module::ModuleEntryBindingContext b{"com.example.a", "bc", 3};
  EXPECT_NE(Module::ModuleEntryBindingId(a), Module::ModuleEntryBindingId(b));
}

TEST(ModuleEntryBindingModesTest, AUniversalBinaryIsRefusedNotGuessedAt) {
  // No build in this matrix produces one, so picking a slice would be
  // choosing which architecture was authoritative.
  QByteArray fat(64, '\0');
  PutU32(fat, 0, 0xBEBAFECA);  // FAT_MAGIC, big-endian on disk

  QString reason;
  EXPECT_TRUE(Module::MachOBindingSection(fat, reason).isEmpty());
  EXPECT_TRUE(reason.contains("universal")) << reason.toStdString();
}

TEST(ModuleEntryBindingModesTest, AMachOWithoutABindingSectionIsRefused) {
  QByteArray macho(128, '\0');
  PutU32(macho, 0, 0xFEEDFACF);  // MH_MAGIC_64
  PutU32(macho, 16, 0);          // no load commands

  QString reason;
  EXPECT_TRUE(Module::MachOBindingSection(macho, reason).isEmpty());
  EXPECT_TRUE(reason.contains("binding")) << reason.toStdString();
}

}  // namespace GpgFrontend::Test
