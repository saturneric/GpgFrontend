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

#include "core/GFCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

bool GpgCoreTest::is_gnupg_available = false;

namespace {

auto TestPassphraseCb(void* opaque, const char* uid_hint,
                      const char* passphrase_info, int last_was_bad, int fd)
    -> gpgme_error_t {
  QString passphrase = "abcdefg";
  auto pass_bytes = passphrase.toLatin1();
  auto pass_size = pass_bytes.size();
  const auto* p_pass_bytes = pass_bytes.constData();

  qsizetype res = 0;
  if (pass_size > 0) {
    qsizetype off = 0;
    qsizetype ret = 0;
    do {
      ret = gpgme_io_write(fd, &p_pass_bytes[off], pass_size - off);
      if (ret > 0) off += ret;
    } while (ret > 0 && off != pass_size);
    res = off;
  }

  res += gpgme_io_write(fd, "\n", 1);
  return res == pass_size + 1 ? 0 : GPG_ERR_CANCELED;
}

void ImportPrivateKeys() {
  auto key_files = QDir(":/test/key").entryList();

  for (const auto& key_file : key_files) {
    auto [success, gf_buffer] =
        ReadFileGFBuffer(QString(":/test/key") + "/" + key_file);

    if (success) {
      auto info = KeyImportExportOperation::GetInstance(
                      GpgFrontend::kGpgFrontendDefaultChannel)
                      .ImportKey(gf_buffer);

      if (info == nullptr) {
        LOG_E() << "import key for unit test failed: " << key_file;
        continue;
      }

      LOG_D() << "unit test key(s) imported: " << info->imported;

      for (const auto& key : info->imported_keys) {
        LOG_D() << "(+) unit test key: " << key.fpr;
      }

    } else {
      FLOG_W() << "read from key file failed: " << key_file;
    }
  }

  AbstractKeyRepository::GetInstance().FlushCache();
  AbstractKeyRepository::GetInstance().Fetch();
}

auto ConfigureGpgContext() -> bool {
  auto db_path = QDir(QDir::tempPath() + "/" + GenerateRandomString(12));

  if (db_path.exists()) db_path.rmdir(".");
  db_path.mkpath(".");

  LOG_D() << "db path of unit test: " << db_path.canonicalPath();
  Q_ASSERT(db_path.exists());

  auto succ = GpgFrontend::BuildOpenPGPContext(
      kGpgChannelForUnitTest, GpgFrontend::OpenPGPContextInitArgs{
                                  .engine = GpgFrontend::OpenPGPEngine::kGNUPG,
                                  .db_name = "test-db",
                                  .db_path = db_path.canonicalPath(),
                                  .offline_mode = true,
                                  .auto_import_missing_key = false,
                              });

  LOG_D() << "configure gpg context for unit test, db path: "
          << db_path.canonicalPath();

  if (!succ) {
    LOG_E() << "configure gpg context for unit test failed";
    return false;
  }

  auto& ctx =
      GpgCtx(GpgFrontend::OpenPGPContext::GetInstance(kGpgChannelForUnitTest));
  ctx.SetPassphraseCb(ctx.DefaultContext(), TestPassphraseCb);
  ctx.SetPassphraseCb(ctx.BinaryContext(), TestPassphraseCb);
  return true;
};
}  // namespace

void GpgCoreTest::SetUpTestSuite() {
  GpgCoreTest::is_gnupg_available =
      GetGSS().IsEngineSupported(OpenPGPEngine::kGNUPG);

  if (!GpgCoreTest::is_gnupg_available) {
    LOG_W() << "GnuPG is not available, some tests will be skipped.";
    return;
  }

  if (!ConfigureGpgContext()) {
    LOG_E() << "configure gpg context for unit test failed, some tests will be "
               "skipped.";
    GpgCoreTest::is_gnupg_available = false;
    return;
  }
  ImportPrivateKeys();
}

void GpgCoreTest::TearDownTestSuite() {}

void GpgCoreTest::SetUp() {
  if (!GpgCoreTest::is_gnupg_available) {
    GTEST_SKIP() << "GnuPG Engine is not available.";
  }
}

void GpgCoreTest::TearDown() {}
}  // namespace GpgFrontend::Test
