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
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <boost/date_time.hpp>
#include <boost/dll.hpp>
#include <filesystem>

#include "core/GpgCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgContext.h"
#include "spdlog/spdlog.h"

namespace GpgFrontend::Test {

void InitTestLoggingSystem(spdlog::level::level_enum level) {
  // get the log directory
  auto logfile_path =
      (GlobalSettingStation::GetInstance().GetLogDir() / "test");
  logfile_path.replace_extension(".log");

  // sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(
      SecureCreateSharedObject<spdlog::sinks::stderr_color_sink_mt>());
  sinks.push_back(
      SecureCreateSharedObject<spdlog::sinks::rotating_file_sink_mt>(
          logfile_path.u8string(), 1048576 * 32, 8));

  // logger
  auto test_logger = SecureCreateSharedObject<spdlog::logger>(
      "test", begin(sinks), end(sinks));
  test_logger->set_pattern(
      "[%H:%M:%S.%e] [T:%t] [%=6n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

  // set the level of logger
  test_logger->set_level(level);

  // register it as default logger
  spdlog::set_default_logger(test_logger);
}

void ShutdownTestLoggingSystem() {
#ifdef WINDOWS
  // Under VisualStudio, this must be called before main finishes to workaround
  // a known VS issue
  spdlog::drop_all();
  spdlog::shutdown();
#endif
}

auto GenerateRandomString(size_t length) -> std::string {
  const std::string characters =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(0, characters.size() - 1);

  std::string random_string;
  for (size_t i = 0; i < length; ++i) {
    random_string += characters[distribution(generator)];
  }

  return random_string;
}

void ConfigureGpgContext() {
  auto db_path =
      std::filesystem::temp_directory_path() / GenerateRandomString(12);
  SPDLOG_DEBUG("setting up new database path for test case: {}",
               db_path.string());

  if (!std::filesystem::exists(db_path)) {
    std::filesystem::create_directory(db_path);
  } else {
    std::filesystem::remove_all(db_path);
    std::filesystem::create_directory(db_path);
  }

  // GpgContext::CreateInstance(
  //     kGpgFrontendDefaultChannel, [&]() -> ChannelObjectPtr {
  //       GpgContextInitArgs args;
  //       args.test_mode = true;
  //       args.offline_mode = true;
  //       args.db_path = db_path.string();

  //       return
  //       ConvertToChannelObjectPtr<>(SecureCreateUniqueObject<GpgContext>(
  //           args, kGpgFrontendDefaultChannel));
  //     });
}

auto ExecuteAllTestCase(GpgFrontendContext args) -> int {
  ConfigureGpgContext();

  testing::InitGoogleTest(&args.argc, args.argv);
  return RUN_ALL_TESTS();
}

}  // namespace GpgFrontend::Test