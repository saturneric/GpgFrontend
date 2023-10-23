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

#pragma once

#include "module/GpgFrontendModule.h"

namespace spdlog {
class logger;
}

using Logger = std::shared_ptr<spdlog::logger>;

/**
 * @brief
 *
 */
void GPGFRONTEND_MODULE_EXPORT InitModuleLoggingSystem();

/**
 * @brief
 *
 */
void GPGFRONTEND_MODULE_EXPORT ShutdownModuleLoggingSystem();

/**
 * @brief
 *
 */
Logger GetModuleLogger();

#define MODULE_LOG_TRACE(...) SPDLOG_LOGGER_TRACE(GetModuleLogger, ...)
#define MODULE_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(GetModuleLogger, ...)
#define MODULE_LOG_INFO(...) SPDLOG_LOGGER_INFO(GetModuleLogger, ...)
#define MODULE_LOG_WARN(...) SPDLOG_LOGGER_WARN(GetModuleLogger, ...)
#define MODULE_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(GetModuleLogger, ...)

template <typename... Args>
void ModuleLogTrace(const char* fmt, const Args&... args) {
  SPDLOG_LOGGER_TRACE(GetModuleLogger(), fmt, args...);
}

template <typename... Args>
void ModuleLogDebug(const char* fmt, const Args&... args) {
  SPDLOG_LOGGER_DEBUG(GetModuleLogger(), fmt, args...);
}

template <typename... Args>
void ModuleLogInfo(const char* fmt, const Args&... args) {
  SPDLOG_LOGGER_INFO(GetModuleLogger(), fmt, args...);
}

template <typename... Args>
void ModuleLogWarn(const char* fmt, const Args&... args) {
  SPDLOG_LOGGER_WARN(GetModuleLogger(), fmt, args...);
}

template <typename... Args>
void ModuleLogError(const char* fmt, const Args&... args) {
  SPDLOG_LOGGER_ERROR(GetModuleLogger(), fmt, args...);
}
