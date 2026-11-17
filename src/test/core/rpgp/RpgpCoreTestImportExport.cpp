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

#include "RpgpCoreTest.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

TEST_F(RpgpCoreTest, CoreExportKeyTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ExportKey(key, false, true, false);

  ASSERT_TRUE(err.Ok());
  ASSERT_FALSE(gf_buffer.Empty());
}

TEST_F(RpgpCoreTest, CoreExportKeySecretTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ExportKey(key, true, true, false);

  ASSERT_TRUE(err.Ok());
  ASSERT_FALSE(gf_buffer.Empty());
}

TEST_F(RpgpCoreTest, CoreExportPublicKeyTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3BEDAB48EAAAA195006330414DD9733454846D0C");
  ASSERT_TRUE(key != nullptr);

  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ExportKey(key, false, true, false);

  ASSERT_TRUE(err.Ok());
  ASSERT_FALSE(gf_buffer.Empty());
}

// The rPGP engine ignored the export flags: a binary export came out armored,
// and an SSH one as an OpenPGP key under an SSH file name.
TEST_F(RpgpCoreTest, AnExportIsWhatWasAskedFor) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3BEDAB48EAAAA195006330414DD9733454846D0C");
  ASSERT_TRUE(key != nullptr);
  auto& op = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest);

  auto [armored_err, armored] = op.ExportKey(key, false, true, false);
  ASSERT_TRUE(armored_err.Ok());
  EXPECT_TRUE(armored.ConvertToQByteArray().startsWith("-----BEGIN PGP"));

  auto [binary_err, binary] = op.ExportKey(key, false, false, false);
  ASSERT_TRUE(binary_err.Ok());
  ASSERT_FALSE(binary.Empty());
  EXPECT_FALSE(binary.ConvertToQByteArray().startsWith("-----BEGIN"));
  // An OpenPGP packet: the tag octet always has its top bit set.
  EXPECT_NE(static_cast<unsigned char>(binary.ConvertToQByteArray().at(0)) &
                0x80U,
            0U);

  auto [minimal_err, minimal] = op.ExportKey(key, false, true, true);
  ASSERT_TRUE(minimal_err.Ok());
  EXPECT_LE(minimal.Size(), armored.Size());

  auto [ssh_err, ssh] = op.ExportKey(key, false, true, false, true);
  EXPECT_EQ(ssh_err.code, GFErrorCode::kNOT_SUPPORTED);
  EXPECT_TRUE(ssh.Empty());
}

// rPGP counted every saved key as newly imported, a key it already had
// included, and gave no status bits. Re-importing a key now reads as GnuPG
// reports it: considered, unchanged, nothing imported.
TEST_F(RpgpCoreTest, ReimportingAKeyCountsItUnchanged) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3BEDAB48EAAAA195006330414DD9733454846D0C");
  ASSERT_TRUE(key != nullptr);
  auto& op = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest);
  auto [err, block] = op.ExportKey(key, false, true, false);
  ASSERT_TRUE(err.Ok());

  auto info = op.ImportKey(block);
  ASSERT_TRUE(info != nullptr);
  EXPECT_EQ(info->considered, 1);
  EXPECT_EQ(info->imported, 0);
  EXPECT_EQ(info->unchanged, 1);
  EXPECT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported_keys.size(), 1);
  EXPECT_EQ(info->imported_keys.front().import_status, 0);
}

// A revocation imported over a key already here is a new signature, as GnuPG
// reports it: the key was updated, not left unchanged.
TEST_F(RpgpCoreTest, ARevocationImportedOverAKeyIsAnUpdate) {
  constexpr auto kFpr = "DF22A6F953939EF880399FAC8B5988B7A371F080";
  auto& repo = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest);
  // Another test may have brought the revoked key in already.
  repo.FlushKeyCache();
  if (auto here = repo.GetKeyPtr(kFpr); here != nullptr) {
    KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
        .DeleteKey(here);
    repo.FlushKeyCache();
  }
  ASSERT_EQ(repo.GetKeyPtr(kFpr), nullptr);

  auto& op = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest);
  auto [before_ok, before] =
      ReadFileGFBuffer(":/test/rpgp_aux_keys/aux_revoked_before.asc");
  ASSERT_TRUE(before_ok);
  auto first = op.ImportKey(before);
  ASSERT_TRUE(first != nullptr);
  EXPECT_EQ(first->imported, 1);

  auto [revoked_ok, revoked] =
      ReadFileGFBuffer(":/test/rpgp_aux_keys/aux_revoked.asc");
  ASSERT_TRUE(revoked_ok);
  auto info = op.ImportKey(revoked);
  ASSERT_TRUE(info != nullptr);
  EXPECT_EQ(info->imported, 0);
  EXPECT_EQ(info->unchanged, 0);
  EXPECT_EQ(info->new_revocations, 1);
  ASSERT_EQ(info->imported_keys.size(), 1);
  EXPECT_EQ(info->imported_keys.front().import_status & 4, 4);
  repo.FlushKeyCache();
  auto now = repo.GetKeyPtr(kFpr);
  ASSERT_TRUE(now != nullptr);
  EXPECT_TRUE(now->IsRevoked());
}

TEST_F(RpgpCoreTest, CoreImportKeyTest) {
  auto key_files = QDir(":/test/rpgp_keys").entryList();
  ASSERT_FALSE(key_files.empty());

  auto [success, gf_buffer] = ReadFileGFBuffer(
      QString(":/test/rpgp_keys") + "/" + key_files.first());
  ASSERT_TRUE(success);
  ASSERT_FALSE(gf_buffer.Empty());

  auto info =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ImportKey(gf_buffer);

  ASSERT_TRUE(info != nullptr);
  ASSERT_GE(info->imported, 0);
}

namespace {

// The binary packets inside an ASCII-armored block, as `gpg --export` writes
// them without --armor.
auto DearmorForTest(const QByteArray& armored) -> QByteArray {
  QByteArray body;
  bool in_body = false;
  for (const auto& raw : armored.split('\n')) {
    const auto line = raw.trimmed();
    if (line.startsWith("-----BEGIN PGP")) continue;
    if (line.startsWith("-----END PGP")) break;
    if (!in_body) {
      if (line.isEmpty()) in_body = true;  // end of the armor headers
      continue;
    }
    if (line.startsWith('=')) continue;  // CRC-24
    body += line;
  }
  return QByteArray::fromBase64(body);
}

auto ExportFixtureKey(bool secret) -> QByteArray {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  if (key == nullptr) return {};
  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ExportKey(key, secret, true, false);
  if (!err.Ok()) return {};
  return gf_buffer.ConvertToQByteArray();
}

void DeleteFixtureKey() {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  if (key == nullptr) return;
  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
}

}  // namespace

// GnuPG's default `--export` is binary, and "Import from file" offers *.gpg:
// the engine re-armors binary key blocks before reading them.
TEST_F(RpgpCoreTest, CoreImportBinaryKeyBlockTest) {
  const auto armored = ExportFixtureKey(true);
  ASSERT_FALSE(armored.isEmpty());
  const auto binary = DearmorForTest(armored);
  ASSERT_FALSE(binary.isEmpty());

  DeleteFixtureKey();
  auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                  .ImportKey(GFBuffer(binary));
  ASSERT_TRUE(info != nullptr);
  EXPECT_EQ(info->imported, 1);
}

// Pasted, dropped or editor text may carry a non-ASCII armor header. The GUI
// hands it to the engine as UTF-8; Latin-1 would turn it into bytes that are
// not UTF-8, and the import would fail outright.
TEST_F(RpgpCoreTest, CoreImportKeyWithNonLatin1ArmorHeaderTest) {
  auto armored = QString::fromUtf8(ExportFixtureKey(false));
  ASSERT_FALSE(armored.isEmpty());
  const auto first_nl = armored.indexOf('\n');
  ASSERT_GT(first_nl, 0);
  armored.insert(first_nl + 1, QString::fromUtf8("Comment: \xe5\x85\xac\xe9"
                                                 "\x92\xa5 \xc3\xbc\n"));

  DeleteFixtureKey();
  auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                  .ImportKey(GFBuffer(armored.toUtf8()));
  ASSERT_TRUE(info != nullptr);
  EXPECT_EQ(info->imported, 1);
}
namespace {

constexpr auto kAuxGoodFpr = "B455DBFFB8D10C58B811AA9FB07187521D00B577";

struct TestPacket {
  int tag;
  QByteArray body;
};

// The packets of a binary key block. Only new-format headers, as sq and rPGP
// write them; an empty list if the block is anything else.
auto SplitPackets(const QByteArray& data) -> QList<TestPacket> {
  QList<TestPacket> packets;
  qsizetype pos = 0;
  const auto octet = [&](qsizetype i) {
    return static_cast<qsizetype>(static_cast<unsigned char>(data.at(i)));
  };
  while (pos < data.size()) {
    if (pos + 2 > data.size() || (octet(pos) & 0xC0) != 0xC0) return {};
    const int tag = static_cast<int>(octet(pos) & 0x3F);
    const auto first = octet(pos + 1);
    qsizetype header = 2;
    qsizetype length = first;
    if (first >= 192 && first < 224) {
      if (pos + 3 > data.size()) return {};
      header = 3;
      length = ((first - 192) << 8) + octet(pos + 2) + 192;
    } else if (first == 255) {
      if (pos + 6 > data.size()) return {};
      header = 6;
      length = (octet(pos + 2) << 24) | (octet(pos + 3) << 16) |
               (octet(pos + 4) << 8) | octet(pos + 5);
    } else if (first >= 224) {
      return {};  // partial lengths never occur in a key block
    }
    if (pos + header + length > data.size()) return {};
    packets.push_back({tag, data.mid(pos + header, length)});
    pos += header + length;
  }
  return packets;
}

auto JoinPackets(const QList<TestPacket>& packets) -> QByteArray {
  QByteArray out;
  for (const auto& packet : packets) {
    const auto size = packet.body.size();
    out.append(static_cast<char>(0xC0 | packet.tag));
    if (size < 192) {
      out.append(static_cast<char>(size));
    } else if (size < 8384) {
      out.append(static_cast<char>(((size - 192) >> 8) + 192));
      out.append(static_cast<char>((size - 192) & 0xFF));
    } else {
      out.append(static_cast<char>(0xFF));
      for (int shift = 24; shift >= 0; shift -= 8) {
        out.append(static_cast<char>((size >> shift) & 0xFF));
      }
    }
    out.append(packet.body);
  }
  return out;
}

// aux_good's packets, with the key itself no longer in the store.
auto FreshAuxGood() -> QList<TestPacket> {
  auto& repo = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest);
  repo.FlushKeyCache();
  if (auto here = repo.GetKeyPtr(kAuxGoodFpr); here != nullptr) {
    KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
        .DeleteKey(here);
    repo.FlushKeyCache();
  }
  auto [ok, armored] = ReadFileGFBuffer(":/test/rpgp_aux_keys/aux_good.asc");
  if (!ok) return {};
  return SplitPackets(DearmorForTest(armored.ConvertToQByteArray()));
}

auto ExportAuxGood() -> QByteArray {
  auto& repo = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest);
  repo.FlushKeyCache();
  auto key = repo.GetKeyPtr(kAuxGoodFpr);
  if (key == nullptr) return {};
  auto [err, block] =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ExportKey(key, false, false, false);
  return err.Ok() ? block.ConvertToQByteArray() : QByteArray{};
}

}  // namespace

// sq canonicalizes a certificate before storing it. rPGP stored a new key
// as it came, so a User ID anyone appended (here with the key's own
// certification copied onto it, which does not verify there) was kept and
// exported again as part of the key.
TEST_F(RpgpCoreTest, ANewKeyIsStoredWithoutAUserIdItsHolderNeverCertified) {
  auto packets = FreshAuxGood();
  ASSERT_FALSE(packets.empty());
  const auto uid = std::find_if(packets.begin(), packets.end(),
                                [](const auto& p) { return p.tag == 13; });
  ASSERT_NE(uid, packets.end());
  const auto copied_certification = *std::next(uid);
  ASSERT_EQ(copied_certification.tag, 2);
  const auto first_subkey =
      std::find_if(packets.begin(), packets.end(),
                   [](const auto& p) { return p.tag == 14; });
  const auto at = std::distance(packets.begin(), first_subkey);
  packets.insert(at, {13, QByteArray("<mallory@test.example>")});
  packets.insert(at + 1, copied_certification);

  auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                  .ImportKey(GFBuffer(JoinPackets(packets)));
  ASSERT_TRUE(info != nullptr);
  EXPECT_EQ(info->imported, 1);

  const auto stored = ExportAuxGood();
  ASSERT_FALSE(stored.isEmpty());
  EXPECT_TRUE(stored.contains("<aux-good@test.example>"));
  EXPECT_FALSE(stored.contains("<mallory@test.example>"));
}

// sq takes two signatures that differ only in their unhashed area, which no
// signature covers, for one. rPGP compared every byte, so a copy whose
// unhashed area a key server or GnuPG had rewritten stored the signature
// again, and the import reported a key that had not changed as updated.
TEST_F(RpgpCoreTest, AKeyWhoseUnhashedAreaWasRewrittenIsUnchanged) {
  auto packets = FreshAuxGood();
  ASSERT_FALSE(packets.empty());
  auto& op = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest);
  auto first = op.ImportKey(GFBuffer(JoinPackets(packets)));
  ASSERT_TRUE(first != nullptr);
  ASSERT_EQ(first->imported, 1);

  // Add an Issuer Key ID subpacket (RFC 9580 §5.2.3.12), agreeing with the
  // issuer already named, to the unhashed area of every v4 signature.
  const auto issuer = QByteArray::fromHex(QByteArray(kAuxGoodFpr).right(16));
  int rewritten = 0;
  for (auto& packet : packets) {
    auto& body = packet.body;
    if (packet.tag != 2 || body.size() < 8 || body.at(0) != 4) continue;
    const auto u16 = [&](qsizetype i) {
      return (static_cast<qsizetype>(static_cast<unsigned char>(body.at(i)))
              << 8) |
             static_cast<unsigned char>(body.at(i + 1));
    };
    const auto unhashed_at = 4 + 2 + u16(4);
    const auto unhashed_len = u16(unhashed_at);
    const auto subpacket = QByteArray("\x09\x10", 2) + issuer;
    body.insert(unhashed_at + 2 + unhashed_len, subpacket);
    const auto grown = unhashed_len + subpacket.size();
    body[unhashed_at] = static_cast<char>(grown >> 8);
    body[unhashed_at + 1] = static_cast<char>(grown & 0xFF);
    ++rewritten;
  }
  ASSERT_GT(rewritten, 0);

  auto info = op.ImportKey(GFBuffer(JoinPackets(packets)));
  ASSERT_TRUE(info != nullptr);
  EXPECT_EQ(info->considered, 1);
  EXPECT_EQ(info->unchanged, 1);
  EXPECT_EQ(info->new_signatures, 0);
  ASSERT_EQ(info->imported_keys.size(), 1);
  EXPECT_EQ(info->imported_keys.front().import_status, 0);
}

// keys.openpgp.org serves a certificate whose addresses are not verified
// without its User IDs. It imports as a new key, and imported over the
// stored key it keeps every stored User ID.
TEST_F(RpgpCoreTest, ACertificateWithoutUserIdsImportsAndMerges) {
  auto packets = FreshAuxGood();
  ASSERT_FALSE(packets.empty());
  QList<TestPacket> bare;
  bool in_user = false;
  for (const auto& packet : packets) {
    if (packet.tag == 13 || packet.tag == 17) {
      in_user = true;
      continue;
    }
    if (packet.tag == 14) in_user = false;
    if (in_user && packet.tag == 2) continue;
    bare.push_back(packet);
  }
  ASSERT_LT(bare.size(), packets.size());

  auto& op = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest);
  auto first = op.ImportKey(GFBuffer(JoinPackets(bare)));
  ASSERT_TRUE(first != nullptr);
  EXPECT_EQ(first->imported, 1);

  auto full = op.ImportKey(GFBuffer(JoinPackets(packets)));
  ASSERT_TRUE(full != nullptr);
  EXPECT_EQ(full->new_user_ids, 2);  // "<aux-good@test.example>", "Aux Good"

  auto again = op.ImportKey(GFBuffer(JoinPackets(bare)));
  ASSERT_TRUE(again != nullptr);
  EXPECT_EQ(again->unchanged, 1);
  const auto stored = ExportAuxGood();
  EXPECT_TRUE(stored.contains("<aux-good@test.example>"));
  EXPECT_TRUE(stored.contains("Aux Good"));
}

// More self-claimed signatures than the engine checks: those past the limit
// are not stored, and the import says so. A clean copy imported afterwards
// merges as usual and brings in what the flooded one could not.
TEST_F(RpgpCoreTest, AFloodedImportReportsTheCheckLimit) {
  auto packets = FreshAuxGood();
  ASSERT_FALSE(packets.empty());
  const auto uid_at =
      std::distance(packets.cbegin(),
                    std::find_if(packets.cbegin(), packets.cend(),
                                 [](const auto& p) { return p.tag == 13; }));
  ASSERT_LT(uid_at + 1, packets.size());
  const auto certification = packets.at(uid_at + 1);
  ASSERT_EQ(certification.tag, 2);
  ASSERT_EQ(certification.body.at(0), 4);

  // Copies of the User ID's own certification with another digest prefix
  // (RFC 9580 §5.2.3): none of them verifies.
  const auto u16 = [&](qsizetype i) {
    return (static_cast<qsizetype>(
                static_cast<unsigned char>(certification.body.at(i)))
            << 8) |
           static_cast<unsigned char>(certification.body.at(i + 1));
  };
  const auto unhashed_at = 4 + 2 + u16(4);
  const auto prefix_at = unhashed_at + 2 + u16(unhashed_at);
  QList<TestPacket> flooded = packets;
  const auto at = uid_at + 2;
  for (int i = 0; i < 4200; ++i) {
    auto copy = certification;
    copy.body[prefix_at] = static_cast<char>((i >> 8) ^ 0x5a);
    copy.body[prefix_at + 1] = static_cast<char>(i & 0xff);
    flooded.insert(at, copy);
  }

  auto& op = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest);
  auto first = op.ImportKey(GFBuffer(JoinPackets(flooded)));
  ASSERT_TRUE(first != nullptr);
  EXPECT_EQ(first->imported, 1);
  EXPECT_EQ(first->check_limit_reached, 1);

  auto clean = op.ImportKey(GFBuffer(JoinPackets(packets)));
  ASSERT_TRUE(clean != nullptr);
  EXPECT_EQ(clean->check_limit_reached, 0);
  const auto stored = ExportAuxGood();
  EXPECT_TRUE(stored.contains("<aux-good@test.example>"));
  EXPECT_TRUE(stored.contains("Aux Good"));
}

}  // namespace GpgFrontend::Test
