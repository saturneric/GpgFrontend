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

#include "FileOperator.h"

bool GpgFrontend::FileOperator::ReadFile(const QString& file_name,
                                         QByteArray& data) {
  QFile file(file_name);
  if (!file.open(QIODevice::ReadOnly)) {
    SPDLOG_ERROR("failed to open file: {}", file_name.toStdString());
    return false;
  }
  data = file.readAll();
  file.close();
  return true;
}

bool GpgFrontend::FileOperator::WriteFile(const QString& file_name,
                                          const QByteArray& data) {
  QFile file(file_name);
  if (!file.open(QIODevice::WriteOnly)) {
    SPDLOG_ERROR("failed to open file: {}", file_name.toStdString());
    return false;
  }
  file.write(data);
  file.close();
  return true;
}

bool GpgFrontend::FileOperator::ReadFileStd(
    const std::filesystem::path& file_name, std::string& data) {
  QByteArray byte_data;
#ifdef WINDOWS
  bool res = ReadFile(QString::fromStdU16String(file_name.u16string()).toUtf8(),
                      byte_data);
#else
  bool res = ReadFile(QString::fromStdString(file_name.u8string()).toUtf8(),
                      byte_data);
#endif
  data = byte_data.toStdString();
  return res;
}

bool GpgFrontend::FileOperator::WriteFileStd(
    const std::filesystem::path& file_name, const std::string& data) {
  return WriteFile(QString::fromStdString(file_name.u8string()).toUtf8(),
                   QByteArray::fromStdString(data));
}

std::string GpgFrontend::FileOperator::CalculateHash(
    const std::filesystem::path& file_path) {
  // Returns empty QByteArray() on failure.
  QFileInfo info(QString::fromStdString(file_path.string()));
  std::stringstream ss;

  if (info.isFile() && info.isReadable()) {
    ss << "[#] " << _("File Hash Information") << std::endl;
    ss << "    " << _("filename") << _(": ")
       << file_path.filename().u8string().c_str() << std::endl;

    QFile f(info.filePath());
    f.open(QFile::ReadOnly);
    auto buffer = f.readAll();
    ss << "    " << _("file size(bytes)") << _(": ") << buffer.size()
       << std::endl;
    f.close();
    if (f.open(QFile::ReadOnly)) {
      auto hash_md5 = QCryptographicHash(QCryptographicHash::Md5);
      // md5
      hash_md5.addData(buffer);
      auto md5 = hash_md5.result().toHex().toStdString();
      SPDLOG_INFO("md5 {}", md5);
      ss << "    "
         << "md5" << _(": ") << md5 << std::endl;

      auto hash_sha1 = QCryptographicHash(QCryptographicHash::Sha1);
      // sha1
      hash_sha1.addData(buffer);
      auto sha1 = hash_sha1.result().toHex().toStdString();
      SPDLOG_INFO("sha1 {}", sha1);
      ss << "    "
         << "sha1" << _(": ") << sha1 << std::endl;

      auto hash_sha256 = QCryptographicHash(QCryptographicHash::Sha256);
      // sha1
      hash_sha256.addData(buffer);
      auto sha256 = hash_sha256.result().toHex().toStdString();
      SPDLOG_INFO("sha256 {}", sha256);
      ss << "    "
         << "sha256" << _(": ") << sha256 << std::endl;

      ss << std::endl;
    }
  } else {
    ss << "[#] " << _("Error in Calculating File Hash ") << std::endl;
  }

  return ss.str();
}
