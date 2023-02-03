/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "GpgFrontend.h"
#include "GpgFrontendBuildInfo.h"
#include "core/function/GlobalSettingStation.h"

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
      "core", begin(sinks), end(sinks), spdlog::thread_pool());
  main_logger->set_pattern(
      "[%H:%M:%S.%e] [T:%t] [%=4n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

#ifdef DEBUG
  main_logger->set_level(spdlog::level::trace);
#elif
  core_logger->set_level(spdlog::level::info);
#endif

  // register it as default logger
  spdlog::set_default_logger(main_logger);
}
