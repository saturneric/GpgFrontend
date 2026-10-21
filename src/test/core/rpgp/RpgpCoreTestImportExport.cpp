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

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
}

TEST_F(RpgpCoreTest, CoreExportKeySecretTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ExportKey(key, true, true, false);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
}

TEST_F(RpgpCoreTest, CoreExportPublicKeyTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3BEDAB48EAAAA195006330414DD9733454846D0C");
  ASSERT_TRUE(key != nullptr);

  auto [err, gf_buffer] =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ExportKey(key, false, true, false);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(gf_buffer.Empty());
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
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return {};
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

}  // namespace GpgFrontend::Test
