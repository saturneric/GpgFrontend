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

  SPDLOG_INFO("app log files total size: {}", GetLogFilesSize());
  SPDLOG_INFO("app data objects files total size: {}",
              GetDataObjectsFilesSize());

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

libconfig::Setting &
GpgFrontend::GlobalSettingStation::GetUISettings() noexcept {
  return ui_cfg_.getRoot();
}

void GpgFrontend::GlobalSettingStation::init_app_secure_key() {}

int64_t GpgFrontend::GlobalSettingStation::get_files_size_at_path(
    std::filesystem::path path, std::string filename_pattern) const {
  auto dir = QDir(QString::fromStdString(path.u8string()));
  QFileInfoList fileList = dir.entryInfoList(
      QStringList() << QString::fromStdString(filename_pattern), QDir::Files);
  qint64 totalSize = 0;

  for (const QFileInfo &fileInfo : fileList) {
    totalSize += fileInfo.size();
  }
  return totalSize;
}

std::string GpgFrontend::GlobalSettingStation::get_human_readable_size(
    int64_t size) const {
  double num = size;
  QStringList list;
  list << "KB"
       << "MB"
       << "GB"
       << "TB";

  QStringListIterator i(list);
  QString unit("bytes");

  while (num >= 1024.0 && i.hasNext()) {
    unit = i.next();
    num /= 1024.0;
  }
  return (QString().setNum(num, 'f', 2) + " " + unit).toStdString();
}

std::string GpgFrontend::GlobalSettingStation::GetLogFilesSize() const {
  return get_human_readable_size(
      get_files_size_at_path(app_log_path_, "*.log"));
}

std::string GpgFrontend::GlobalSettingStation::GetDataObjectsFilesSize() const {
  return get_human_readable_size(
      get_files_size_at_path(app_data_objs_path_, "*"));
}

void GpgFrontend::GlobalSettingStation::ClearAllLogFiles() const {
  delete_all_files(app_log_path_, "*.log");
}

void GpgFrontend::GlobalSettingStation::ClearAllDataObjects() const {
  delete_all_files(app_data_objs_path_, "*");
}

void GpgFrontend::GlobalSettingStation::delete_all_files(
    std::filesystem::path path, std::string filename_pattern) const {
  auto dir = QDir(QString::fromStdString(path.u8string()));

  // 使用name filters来只选取以.log结尾的文件
  QStringList logFiles = dir.entryList(
      QStringList() << QString::fromStdString(filename_pattern), QDir::Files);

  // 遍历并删除所有符合条件的文件
  for (const auto &file : logFiles) {
    QFile::remove(dir.absoluteFilePath(file));
  }
}

GpgFrontend::GlobalSettingStation::~GlobalSettingStation() noexcept = default;
