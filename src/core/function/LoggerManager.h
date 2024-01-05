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

#include "core/function/basic/GpgFunctionObject.h"

namespace spdlog {
class logger;
}

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT LoggerManager
    : public SingletonFunctionObject<LoggerManager> {
 public:
  explicit LoggerManager(int channel);

  ~LoggerManager() override;

  auto RegisterAsyncLogger(const std::string& id, spdlog::level::level_enum)
      -> std::shared_ptr<spdlog::logger>;

  auto RegisterSyncLogger(const std::string& id, spdlog::level::level_enum)
      -> std::shared_ptr<spdlog::logger>;

  auto GetLogger(const std::string& id) -> std::shared_ptr<spdlog::logger>;

  static auto GetDefaultLogger() -> std::shared_ptr<spdlog::logger>;

  static void SetDefaultLogLevel(spdlog::level::level_enum);

 private:
  static spdlog::level::level_enum default_log_level;
  static std::shared_ptr<spdlog::logger> default_logger;

  std::map<std::string, std::shared_ptr<spdlog::logger>> logger_map_;
};

}  // namespace GpgFrontend