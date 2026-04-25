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

#include "GpgFrontendTest.h"

#include <qglobal.h>

#include "core/GpgConstants.h"
#include "core/GpgCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/IOUtils.h"

Q_LOGGING_CATEGORY(test, "test")

auto GF_TEST_EXPORT GFTestValidateSymbol() -> int { return 0; }

namespace {
auto GenerateRandomString(size_t length) -> QString {
  const QString characters =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(0, characters.size() - 1);

  QString random_string;
  for (size_t i = 0; i < length; ++i) {
    random_string += characters[distribution(generator)];
  }

  return random_string;
}

void ImportPrivateKeys() {
  auto key_files = QDir(":/test/key").entryList();

  for (const auto& key_file : key_files) {
    auto [success, gf_buffer] =
        GpgFrontend::ReadFileGFBuffer(QString(":/test/key") + "/" + key_file);

    if (success) {
      auto info = GpgFrontend::KeyImportExportOperation::GetInstance(
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

  GpgFrontend::AbstractKeyRepository::GetInstance().FlushCache();
  GpgFrontend::AbstractKeyRepository::GetInstance().Fetch();
}

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

auto TestStatusCb(void* hook, const char* keyword, const char* args)
    -> gpgme_error_t {
  FLOG_D("keyword %s", keyword);
  return GPG_ERR_NO_ERROR;
}

}  // namespace

namespace GpgFrontend::Test {

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
};

void SetupGlobalTestEnv() {
  auto app_path = GlobalSettingStation::GetInstance().GetAppDir();
  auto test_path = app_path + "/test";
  auto test_config_path = test_path + "/conf/test.ini";
  auto test_data_path = test_path + "/data";

  LOG_I() << "test config file path: " << test_config_path;
  LOG_I() << "test data file path: " << test_data_path;

  ImportPrivateKeys();
}

auto ExecuteAllTestCase(GpgFrontendContext args) -> int {
  ConfigureGpgContext();
  SetupGlobalTestEnv();

  testing::InitGoogleTest(&args.argc, args.argv);
  return RUN_ALL_TESTS();
}

auto WaitFor(std::function<bool()> cond, int timeout_ms) -> bool {
  QEventLoop loop;
  QTimer timer;
  timer.setSingleShot(true);

  bool matched = false;

  QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

  QTimer check_timer;
  check_timer.setInterval(20);

  QObject::connect(&check_timer, &QTimer::timeout, [&]() {
    if (cond()) {
      matched = true;
      loop.quit();
    }
  });

  timer.start(timeout_ms);
  check_timer.start();

  if (cond()) {
    matched = true;
  } else {
    loop.exec();
  }

  return matched;
}
}  // namespace GpgFrontend::Test