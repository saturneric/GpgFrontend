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

#include "GlobalSettingStation.h"

#include "core/function/FileOperator.h"

void GpgFrontend::GlobalSettingStation::SyncSettings() noexcept {
  using namespace libconfig;
  try {
    ui_cfg_.writeFile(ui_config_path_.u8string().c_str());
    SPDLOG_DEBUG("updated ui configuration successfully written to {}",
                 ui_config_path_.u8string());

  } catch (const FileIOException &fioex) {
    SPDLOG_ERROR("i/o error while writing ui configuration file: {}",
                 ui_config_path_.u8string());
  }
}

GpgFrontend::GlobalSettingStation::GlobalSettingStation(int channel) noexcept
    : SingletonFunctionObject<GlobalSettingStation>(channel) {
  using namespace std::filesystem;
  using namespace libconfig;

  SPDLOG_INFO("app path: {}", app_path_.u8string());
  SPDLOG_INFO("app configure path: {}", app_configure_path_.u8string());
  SPDLOG_INFO("app data path: {}", app_data_path_.u8string());
  SPDLOG_INFO("app log path: {}", app_log_path_.u8string());
  SPDLOG_INFO("app locale path: {}", app_locale_path_.u8string());
  SPDLOG_INFO("app conf path: {}", ui_config_path_.u8string());

  if (!is_directory(app_configure_path_)) create_directory(app_configure_path_);

  if (!is_directory(app_data_path_)) create_directory(app_data_path_);

  if (!is_directory(app_log_path_)) create_directory(app_log_path_);

  if (!is_directory(ui_config_dir_path_)) create_directory(ui_config_dir_path_);

  if (!exists(ui_config_path_)) {
    try {
      this->ui_cfg_.writeFile(ui_config_path_.u8string().c_str());
      SPDLOG_DEBUG("user interface configuration successfully written to {}",
                   ui_config_path_.u8string());

    } catch (const FileIOException &fioex) {
      SPDLOG_DEBUG(
          "i/o error while writing UserInterface configuration file {}",
          ui_config_path_.u8string());
    }
  } else {
    try {
      this->ui_cfg_.readFile(ui_config_path_.u8string().c_str());
      SPDLOG_DEBUG("user interface configuration successfully read from {}",
                   ui_config_path_.u8string());
    } catch (const FileIOException &fioex) {
      SPDLOG_ERROR("i/o error while reading UserInterface configure file");
    } catch (const ParseException &pex) {
      SPDLOG_ERROR("parse error at {} : {} - {}", pex.getFile(), pex.getLine(),
                   pex.getError());
    }
  }
}

void GpgFrontend::GlobalSettingStation::init_app_secure_key() {}

GpgFrontend::GlobalSettingStation::~GlobalSettingStation() noexcept = default;
