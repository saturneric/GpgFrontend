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

#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>
#include <string>

#include "GpgFrontend.h"
#include "GpgFrontendBuildInfo.h"
#include "core/function/GlobalSettingStation.h"

#ifdef WINDOWS
int setenv(const char *name, const char *value, int overwrite) {
  if (!overwrite) {
    int errcode = 0;
    size_t envsize = 0;
    errcode = getenv_s(&envsize, NULL, 0, name);
    if (errcode || envsize) return errcode;
  }
  return _putenv_s(name, value);
}
#endif

void init_logging_system() {
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  // sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());

  // thread pool
  spdlog::init_thread_pool(1024, 2);

  // logger
  auto main_logger = std::make_shared<spdlog::async_logger>(
      "main", begin(sinks), end(sinks), spdlog::thread_pool());
  main_logger->set_pattern(
      "[%H:%M:%S.%e] [T:%t] [%=6n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

#ifdef DEBUG
  main_logger->set_level(spdlog::level::trace);
#else
  main_logger->set_level(spdlog::level::info);
#endif

  // flush policy
  main_logger->flush_on(spdlog::level::err);
  spdlog::flush_every(std::chrono::seconds(5));

  // register it as default logger
  spdlog::set_default_logger(main_logger);
}

void shutdown_logging_system() {
#ifdef WINDOWS
  // Under VisualStudio, this must be called before main finishes to workaround
  // a known VS issue
  spdlog::drop_all();
  spdlog::shutdown();
#endif
}

void init_global_path_env() {
  // read settings
  bool use_custom_gnupg_install_path =
      GpgFrontend::GlobalSettingStation::GetInstance().LookupSettings(
          "general.use_custom_gnupg_install_path", false);

  std::string custom_gnupg_install_path =
      GpgFrontend::GlobalSettingStation::GetInstance().LookupSettings(
          "general.custom_gnupg_install_path", std::string{});

  // add custom gnupg install path into env $PATH
  if (use_custom_gnupg_install_path && !custom_gnupg_install_path.empty()) {
    std::string path_value = getenv("PATH");
    SPDLOG_DEBUG("Current System PATH: {}", path_value);
    setenv("PATH",
           ((std::filesystem::path{custom_gnupg_install_path}).u8string() +
            ":" + path_value)
               .c_str(),
           1);
    std::string modified_path_value = getenv("PATH");
    SPDLOG_DEBUG("Modified System PATH: {}", modified_path_value);
  }
}
