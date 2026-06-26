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

#include "core/GFCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/rpgp/PasswordFetcher.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

bool RpgpCoreTest::is_rpgp_available = false;

namespace {

void ImportPrivateKeys() {
  auto key_files = QDir(":/test/rpgp_keys").entryList();

  for (const auto& key_file : key_files) {
    auto [success, gf_buffer] =
        ReadFileGFBuffer(QString(":/test/rpgp_keys") + "/" + key_file);

    if (success) {
      auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                      .ImportKey(gf_buffer);

      if (info == nullptr) {
        LOG_E() << "import rpgp key for unit test failed: " << key_file;
        continue;
      }

      LOG_D() << "unit test rpgp key(s) imported: " << info->imported;

      for (const auto& key : info->imported_keys) {
        LOG_D() << "(+) unit test rpgp key: " << key.fpr;
      }

    } else {
      FLOG_W() << "read from rpgp key file failed: " << key_file;
    }
  }

  AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushCache();
  AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest).Fetch();
}

auto ConfigureGpgContext() -> bool {
  auto db_path = QDir(QDir::tempPath() + "/" + GenerateRandomString(12));

  if (db_path.exists()) db_path.rmdir(".");
  db_path.mkpath(".");

  LOG_D() << "db path of unit test: " << db_path.canonicalPath();
  Q_ASSERT(db_path.exists());

  auto succ = GpgFrontend::BuildOpenPGPContext(
      kRpgpChannelForUnitTest, GpgFrontend::OpenPGPContextInitArgs{
                                   .engine = GpgFrontend::OpenPGPEngine::kRPGP,
                                   .db_name = "test-rpgp-db",
                                   .db_path = db_path.canonicalPath()});

  LOG_D() << "configure gpg context for unit test, db path: "
          << db_path.canonicalPath();

  if (!succ) {
    LOG_E() << "configure gpg context for unit test failed";
    return false;
  }
  return true;
};
}  // namespace

void RpgpCoreTest::SetUpTestSuite() {
  RpgpCoreTest::is_rpgp_available =
      GetGSS().IsEngineSupported(OpenPGPEngine::kRPGP);

  if (!RpgpCoreTest::is_rpgp_available) {
    LOG_W() << "rPGP is not available, some tests will be skipped.";
    return;
  }

  if (!ConfigureGpgContext()) {
    LOG_E() << "configure gpg context for unit test failed, some tests will be "
               "skipped.";
    RpgpCoreTest::is_rpgp_available = false;
    return;
  }

  SetChannelPasswordFetcher(
      kRpgpChannelForUnitTest,
      [](const PassphraseState&) -> GFBuffer {
        return GFBuffer(QString("123456"));
      });

  ImportPrivateKeys();
}

void RpgpCoreTest::TearDownTestSuite() {}

void RpgpCoreTest::SetUp() {
  if (!RpgpCoreTest::is_rpgp_available) {
    GTEST_SKIP() << "rPGP Engine is not available.";
  }
}

void RpgpCoreTest::TearDown() {}
}  // namespace GpgFrontend::Test
