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

#include "GpgCoreTest.h"
#include "core/GpgConstants.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

// Key Details imported whatever key a chosen revocation certificate named.
// The target is read first; only a single key revocation signature has one.
TEST(RevocationTargetTest, ReadsTheOneKeyRevocationSignature) {
  const QString rev =
      ":signature packet: algo 22, keyid A11849836F435C61\n"
      "\tversion 4, created 1794922902, md5len 0, sigclass 0x20\n"
      "\tdigest algo 10, begin of digest f3 67\n"
      "\thashed subpkt 33 len 21 (issuer fpr v4 "
      "714d1ae12d40c76faeae5e7ca11849836f435c61)\n"
      "\thashed subpkt 29 len 1 (revocation reason 0x00 ())\n"
      "\tsubpkt 16 len 8 (issuer key ID A11849836F435C61)\n";
  EXPECT_EQ(RevocationTargetFromPacketList(rev),
            "714D1AE12D40C76FAEAE5E7CA11849836F435C61");

  // Without the fingerprint subpacket, the key ID it was made by.
  QString no_fpr = rev;
  no_fpr.remove(QRegularExpression(".*issuer fpr.*\n"));
  EXPECT_EQ(RevocationTargetFromPacketList(no_fpr), "A11849836F435C61");

  // A certification, not a revocation.
  QString certification = rev;
  certification.replace("sigclass 0x20", "sigclass 0x13");
  EXPECT_TRUE(RevocationTargetFromPacketList(certification).isEmpty());

  // Anything beside the signature: a key, or a second signature.
  EXPECT_TRUE(
      RevocationTargetFromPacketList(":public key packet:\n" + rev).isEmpty());
  EXPECT_TRUE(RevocationTargetFromPacketList(rev + rev).isEmpty());
  EXPECT_TRUE(RevocationTargetFromPacketList({}).isEmpty());
}

TEST_F(GpgCoreTest, CoreRevocationTargetIsTheRevokedKey) {
  // gpg's own revocation certificate of a throwaway key, made for this test.
  const auto cert = GFBuffer(QString(
      "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
      "Comment: This is a revocation certificate\n"
      "\n"
      "iHgEIBYKACAWIQRxTRrhLUDHb66uXnyhGEmDb0NcYQUCavxZlgIdAAAKCRChGEmD\n"
      "b0NcYfNnAP93Ogf/KrbYzIxvMS7zg3EJsnnx9+cOwYrAwkV65Z6G8AEAtdARArJw\n"
      "itz2N1sNh+jdFh3QFoOW/kMZ3bheuPs8eQY=\n"
      "=AXNd\n"
      "-----END PGP PUBLIC KEY BLOCK-----\n"));
  EXPECT_EQ(GpgKeyImportExporter::GetInstance().RevocationCertificateTarget(
                cert),
            "714D1AE12D40C76FAEAE5E7CA11849836F435C61");
}

TEST_F(GpgCoreTest, CoreRevocationTargetOfSomethingElseIsEmpty) {
  auto& importer = GpgKeyImportExporter::GetInstance();
  EXPECT_TRUE(importer.RevocationCertificateTarget({}).isEmpty());
  EXPECT_TRUE(
      importer.RevocationCertificateTarget(GFBuffer(QString("not a cert")))
          .isEmpty());

  // A public key block revokes nothing, though gpg reads it fine.
  auto key = GpgKeyGetter::GetInstance().GetPubkeyPtr(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(key != nullptr);
  auto [err, block] = importer.ExportKey(key, false, true, false);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_TRUE(importer.RevocationCertificateTarget(block).isEmpty());
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestA) {
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "F89C95A05088CC93", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestB) {
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "F89C95A05088CC93!", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestC) {
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "CFF986E51BBC2F46064C2136F89C95A05088CC93", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

TEST_F(GpgCoreTest, CoreExportSubkeyTestD) {
  auto [err, gf_buffer] = GpgKeyImportExporter::GetInstance().ExportSubkey(
      "CFF986E51BBC2F46064C2136F89C95A05088CC93!", true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
  ASSERT_EQ(
      QCryptographicHash::hash(
          gf_buffer.ConvertToQByteArray().replace("\r\n", "\n"),
          QCryptographicHash::Sha256)
          .toHex(),
      QByteArray(
          "6e3375060aa889d9eb61e2966eabb31eb6b5359a7742ee7adeedec09e6afa36a"));
}

}  // namespace GpgFrontend::Test