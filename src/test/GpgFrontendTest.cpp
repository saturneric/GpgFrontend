/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include <gtest/gtest.h>

#include "core/GpgConstants.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/ChannelObject.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

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

void ConfigureGpgContext() {
  auto db_path = QDir(QDir::tempPath() + "/" + GenerateRandomString(12));
  GF_TEST_LOG_DEBUG("setting up new database path for test case: {}",
                    db_path.path());

  if (db_path.exists()) db_path.rmdir(".");
  db_path.mkpath(".");

  GpgContext::CreateInstance(
      kGpgFrontendDefaultChannel, [=]() -> ChannelObjectPtr {
        GpgContextInitArgs args;
        args.test_mode = true;
        args.offline_mode = true;
        args.db_path = db_path.path();

        return ConvertToChannelObjectPtr<>(SecureCreateUniqueObject<GpgContext>(
            args, kGpgFrontendDefaultChannel));
      });
}

void ImportPrivateKeys(const QString& data_path,
                       const libconfig::Setting& config) {
  if (config.exists("load_keys.private_keys")) {
    auto& private_keys = config.lookup("load_keys.private_keys");
    for (auto& private_key : private_keys) {
      if (private_key.exists("filename")) {
        std::string filename;
        private_key.lookupValue("filename", filename);
        auto data_file_path =
            data_path + "/" + QString::fromStdString(filename);

        auto [success, gf_buffer] = ReadFileGFBuffer(data_file_path);
        if (success) {
          GpgKeyImportExporter::GetInstance(kGpgFrontendDefaultChannel)
              .ImportKey(gf_buffer);
        } else {
          GF_TEST_LOG_ERROR("read from file faild: {}", data_file_path);
        }
      }
    }
  }
}

void SetupGlobalTestEnv() {
  auto app_path = GlobalSettingStation::GetInstance().GetAppDir();
  auto test_path = app_path + "/test";
  auto test_config_path = test_path + "/conf/test.cfg";
  auto test_data_path = test_path + "/data";

  GF_TEST_LOG_INFO("test config file path: {}", test_config_path);
  GF_TEST_LOG_INFO("test data file path: {}", test_data_path);

  libconfig::Config cfg;
  ASSERT_NO_THROW(cfg.readFile(test_config_path.toUtf8()));

  auto& root = cfg.getRoot();
  ImportPrivateKeys(test_data_path, root);
}

auto ExecuteAllTestCase(GpgFrontendContext args) -> int {
  ConfigureGpgContext();
  SetupGlobalTestEnv();

  testing::InitGoogleTest(&args.argc, args.argv);
  return RUN_ALL_TESTS();
}

}  // namespace GpgFrontend::Test