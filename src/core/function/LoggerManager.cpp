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

#include "LoggerManager.h"

#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "core/function/GlobalSettingStation.h"

namespace GpgFrontend {

std::shared_ptr<spdlog::logger> LoggerManager::default_logger = nullptr;
spdlog::level::level_enum LoggerManager::default_log_level =
    spdlog::level::debug;

LoggerManager::LoggerManager(int channel)
    : SingletonFunctionObject<LoggerManager>(channel) {
  spdlog::init_thread_pool(1024, 2);
  spdlog::flush_every(std::chrono::seconds(5));
}

LoggerManager::~LoggerManager() {
#ifdef WINDOWS
  // Under VisualStudio, this must be called before main finishes to workaround
  // a known VS issue
  spdlog::drop_all();
  spdlog::shutdown();
#endif

  if (default_logger) default_logger = nullptr;
}

auto LoggerManager::GetLogger(const QString& id)
    -> std::shared_ptr<spdlog::logger> {
  auto m_it = logger_map_.find(id);
  if (m_it == logger_map_.end()) return GetDefaultLogger();
  return m_it->second;
}

auto LoggerManager::RegisterAsyncLogger(const QString& id,
                                        spdlog::level::level_enum level)
    -> std::shared_ptr<spdlog::logger> {
  // get the log directory
  auto log_file_path =
      (GlobalSettingStation::GetInstance().GetLogDir() / id.toStdString());
  log_file_path.replace_extension(".log");

  // sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(GpgFrontend::SecureCreateSharedObject<
                  spdlog::sinks::stderr_color_sink_mt>());
  sinks.push_back(GpgFrontend::SecureCreateSharedObject<
                  spdlog::sinks::rotating_file_sink_mt>(
      log_file_path.u8string(), 1048576 * 32, 8));

  // logger
  auto logger = GpgFrontend::SecureCreateSharedObject<spdlog::async_logger>(
      id.toStdString(), begin(sinks), end(sinks), spdlog::thread_pool());
  logger->set_pattern(
      "[%H:%M:%S.%e] [T:%t] [%=6n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

  // set the level of logger
  logger->set_level(level);

  // flush policy
#ifdef DEBUG
  logger->flush_on(spdlog::level::trace);
#else
  core_logger->flush_on(spdlog::level::err);
#endif

  logger_map_[id] = logger;
  return logger;
}

auto LoggerManager::RegisterSyncLogger(const QString& id,
                                       spdlog::level::level_enum level)
    -> std::shared_ptr<spdlog::logger> {
  // get the log directory
  auto log_file_path =
      (GlobalSettingStation::GetInstance().GetLogDir() / id.toStdString());
  log_file_path.replace_extension(".log");

  // sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(GpgFrontend::SecureCreateSharedObject<
                  spdlog::sinks::stderr_color_sink_mt>());
  sinks.push_back(GpgFrontend::SecureCreateSharedObject<
                  spdlog::sinks::rotating_file_sink_mt>(
      log_file_path.u8string(), 1048576 * 32, 8));

  // logger
  auto logger = GpgFrontend::SecureCreateSharedObject<spdlog::logger>(
      id.toStdString(), begin(sinks), end(sinks));
  logger->set_pattern(
      "[%H:%M:%S.%e] [T:%t] [%=6n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

  // set the level of logger
  logger->set_level(level);

  logger_map_[id] = logger;
  return logger;
}

auto LoggerManager::GetDefaultLogger() -> std::shared_ptr<spdlog::logger> {
  if (default_logger == nullptr) {
    // sinks
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(GpgFrontend::SecureCreateSharedObject<
                    spdlog::sinks::stderr_color_sink_mt>());

    // logger
    auto logger = GpgFrontend::SecureCreateSharedObject<spdlog::logger>(
        "default", begin(sinks), end(sinks));
    logger->set_pattern(
        "[%H:%M:%S.%e] [T:%t] [%=6n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

    // set the level of logger
    logger->set_level(default_log_level);
    spdlog::set_default_logger(logger);
    default_logger = logger;
  }
  return default_logger;
}

void LoggerManager::SetDefaultLogLevel(spdlog::level::level_enum level) {
  default_log_level = level;
}
}  // namespace GpgFrontend