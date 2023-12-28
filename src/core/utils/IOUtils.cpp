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

#include "IOUtils.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>

#include "GpgModel.h"

namespace GpgFrontend {

auto ReadFile(const QString& file_name, QByteArray& data) -> bool {
  QFile file(file_name);
  if (!file.open(QIODevice::ReadOnly)) {
    SPDLOG_ERROR("failed to open file: {}", file_name.toStdString());
    return false;
  }
  data = file.readAll();
  file.close();
  return true;
}

auto WriteFile(const QString& file_name, const QByteArray& data) -> bool {
  QFile file(file_name);
  if (!file.open(QIODevice::WriteOnly)) {
    SPDLOG_ERROR("failed to open file: {}", file_name.toStdString());
    return false;
  }
  file.write(data);
  file.close();
  return true;
}

auto ReadFileStd(const std::filesystem::path& file_name, std::string& data)
    -> bool {
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

auto ReadFileGFBuffer(const std::filesystem::path& file_name)
    -> std::tuple<bool, GFBuffer> {
  QByteArray byte_data;
#ifdef WINDOWS
  const bool res = ReadFile(
      QString::fromStdU16String(file_name.u16string()).toUtf8(), byte_data);
#else
  const bool res = ReadFile(
      QString::fromStdString(file_name.u8string()).toUtf8(), byte_data);
#endif

  return {res, GFBuffer(byte_data)};
}

auto WriteFileStd(const std::filesystem::path& file_name,
                  const std::string& data) -> bool {
  return WriteFile(QString::fromStdString(file_name.u8string()).toUtf8(),
                   QByteArray::fromStdString(data));
}

auto WriteFileGFBuffer(const std::filesystem::path& file_name, GFBuffer data)
    -> bool {
  return WriteFile(
      QString::fromStdString(file_name.u8string()).toUtf8(),
      QByteArray::fromRawData(reinterpret_cast<const char*>(data.Data()),
                              static_cast<qsizetype>(data.Size())));
}

auto CalculateHash(const std::filesystem::path& file_path) -> std::string {
  // Returns empty QByteArray() on failure.
  QFileInfo info(QString::fromStdString(file_path.string()));
  std::stringstream ss;

  if (info.isFile() && info.isReadable()) {
    ss << "[#] " << _("File Hash Information") << std::endl;
    ss << "    " << _("filename") << _(": ")
       << file_path.filename().u8string().c_str() << std::endl;

    QFile f(info.filePath());
    if (f.open(QFile::ReadOnly)) {
      // read all data
      auto buffer = f.readAll();
      ss << "    " << _("file size(bytes)") << _(": ") << buffer.size()
         << std::endl;

      // md5
      auto hash_md5 = QCryptographicHash(QCryptographicHash::Md5);
      hash_md5.addData(buffer);
      auto md5 = hash_md5.result().toHex().toStdString();
      SPDLOG_DEBUG("md5 {}", md5);
      ss << "    "
         << "md5" << _(": ") << md5 << std::endl;

      // sha1
      auto hash_sha1 = QCryptographicHash(QCryptographicHash::Sha1);
      hash_sha1.addData(buffer);
      auto sha1 = hash_sha1.result().toHex().toStdString();
      SPDLOG_DEBUG("sha1 {}", sha1);
      ss << "    "
         << "sha1" << _(": ") << sha1 << std::endl;

      // sha1
      auto hash_sha256 = QCryptographicHash(QCryptographicHash::Sha256);
      hash_sha256.addData(buffer);
      auto sha256 = hash_sha256.result().toHex().toStdString();
      SPDLOG_DEBUG("sha256 {}", sha256);
      ss << "    "
         << "sha256" << _(": ") << sha256 << std::endl;

      ss << std::endl;
    }
  } else {
    ss << "[#] " << _("Error in Calculating File Hash ") << std::endl;
  }

  return ss.str();
}

auto ReadAllDataInFile(const std::string& utf8_path) -> std::string {
  std::string data;
  ReadFileStd(utf8_path, data);
  return data;
}

auto WriteBufferToFile(const std::string& utf8_path,
                       const std::string& out_buffer) -> bool {
  return WriteFileStd(utf8_path, out_buffer);
}

auto ConvertPathByOS(const std::string& path) -> std::filesystem::path {
#ifdef WINDOWS
  return {QString::fromStdString(path).toStdU16String()};
#else
  return {path};
#endif
}

auto ConvertPathByOS(const QString& path) -> std::filesystem::path {
#ifdef WINDOWS
  return {path.toStdU16String()};
#else
  return {path.toStdString()};
#endif
}

auto GetTempFilePath() -> std::filesystem::path {
  std::filesystem::path const temp_dir = std::filesystem::temp_directory_path();
  boost::uuids::uuid const uuid = boost::uuids::random_generator()();
  std::string const filename = boost::uuids::to_string(uuid) + ".data";
  std::filesystem::path const temp_file = temp_dir / filename;
  return temp_dir / filename;
}

auto CreateTempFileAndWriteData(const std::string& data)
    -> std::filesystem::path {
  auto temp_file = GetTempFilePath();
  std::ofstream file_stream(temp_file, std::ios::out | std::ios::trunc);
  if (!file_stream.is_open()) {
    throw std::runtime_error("Unable to open temporary file.");
  }
  file_stream << data;
  file_stream.close();
  return temp_file.string();
}

auto TargetFilePreCheck(const std::filesystem::path& path, bool read)
    -> std::tuple<bool, std::string> {
  QFileInfo const file_info(path);

  if (read) {
    if (!file_info.exists()) {
      return {false, _("target path doesn't exists")};
    }
  } else {
    QFileInfo const path_info(file_info.absolutePath());
    if (!path_info.isWritable()) {
      return {false, _("do NOT have permission to write path")};
    }
  }

  if (read ? !file_info.isReadable() : false) {
    return {false, _("do NOT have permission to read/write file")};
  }

  return {true, _("")};
}

auto GetFullExtension(std::filesystem::path path) -> std::string {
  const auto filename = path.filename().string();
  std::string extension(std::find(filename.begin(), filename.end(), '.'),
                        filename.end());
  return extension;
}

}  // namespace GpgFrontend