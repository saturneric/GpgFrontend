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
#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/ChannelObject.h"
#include "core/function/gpg/GpgAbstractKeyGetter.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
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
      auto info = GpgFrontend::GpgKeyImportExporter::GetInstance(
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

  GpgFrontend::GpgAbstractKeyGetter::GetInstance().FlushCache();
  GpgFrontend::GpgAbstractKeyGetter::GetInstance().Fetch();
}

void ConfigureGpgContext() {
  auto db_path = QDir(QDir::tempPath() + "/" + GenerateRandomString(12));

  if (db_path.exists()) db_path.rmdir(".");
  db_path.mkpath(".");

  LOG_D() << "db path of unit test: " << db_path.canonicalPath();
  Q_ASSERT(db_path.exists());

  GpgFrontend::GpgContext::CreateInstance(
      GpgFrontend::kGpgFrontendDefaultChannel,
      [=]() -> GpgFrontend::ChannelObjectPtr {
        GpgFrontend::GpgContextInitArgs args;
        args.test_mode = true;
        args.offline_mode = true;
        args.db_name = "UNIT_TEST";
        args.db_path = db_path.path();

        return GpgFrontend::ConvertToChannelObjectPtr<>(
            GpgFrontend::SecureCreateUniqueObject<GpgFrontend::GpgContext>(
                args, GpgFrontend::kGpgFrontendDefaultChannel));
      });
}

};  // namespace

namespace GpgFrontend::Test {

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

  QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

  QTimer check_timer;
  check_timer.setInterval(20);
  QObject::connect(&check_timer, &QTimer::timeout, [&]() {
    if (cond()) loop.quit();
  });

  timer.start(timeout_ms);
  check_timer.start();
  loop.exec();

  return cond();
}
}  // namespace GpgFrontend::Test