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
    GF_CORE_LOG_ERROR("failed to open file: {}", file_name);
    return false;
  }
  data = file.readAll();
  file.close();
  return true;
}

auto WriteFile(const QString& file_name, const QByteArray& data) -> bool {
  QFile file(file_name);
  if (!file.open(QIODevice::WriteOnly)) {
    GF_CORE_LOG_ERROR("failed to open file: {}", file_name);
    return false;
  }
  file.write(data);
  file.close();
  return true;
}

auto ReadFileGFBuffer(const QString& file_name) -> std::tuple<bool, GFBuffer> {
  QByteArray byte_data;
  const bool res = ReadFile(file_name, byte_data);

  return {res, GFBuffer(byte_data)};
}

auto WriteFileGFBuffer(const QString& file_name, GFBuffer data) -> bool {
  return WriteFile(file_name, data.ConvertToQByteArray());
}

auto CalculateHash(const QString& file_path) -> QString {
  // Returns empty QByteArray() on failure.
  QFileInfo const info(file_path);
  QString buffer;
  QTextStream ss(&buffer);

  if (info.isFile() && info.isReadable()) {
    ss << "[#] " << _("File Hash Information") << Qt::endl;
    ss << "    " << _("filename") << _(": ") << info.fileName() << Qt::endl;

    QFile f(info.filePath());
    if (f.open(QFile::ReadOnly)) {
      // read all data
      auto buffer = f.readAll();
      ss << "    " << _("file size(bytes)") << _(": ") << buffer.size()
         << Qt::endl;

      // md5
      auto hash_md5 = QCryptographicHash(QCryptographicHash::Md5);
      hash_md5.addData(buffer);
      auto md5 = hash_md5.result().toHex();
      GF_CORE_LOG_DEBUG("md5 {}", md5);
      ss << "    "
         << "md5" << _(": ") << md5 << Qt::endl;

      // sha1
      auto hash_sha1 = QCryptographicHash(QCryptographicHash::Sha1);
      hash_sha1.addData(buffer);
      auto sha1 = hash_sha1.result().toHex();
      GF_CORE_LOG_DEBUG("sha1 {}", sha1);
      ss << "    "
         << "sha1" << _(": ") << sha1 << Qt::endl;

      // sha1
      auto hash_sha256 = QCryptographicHash(QCryptographicHash::Sha256);
      hash_sha256.addData(buffer);
      auto sha256 = hash_sha256.result().toHex();
      GF_CORE_LOG_DEBUG("sha256 {}", sha256);
      ss << "    "
         << "sha256" << _(": ") << sha256 << Qt::endl;

      ss << Qt::endl;
    }
  } else {
    ss << "[#] " << _("Error in Calculating File Hash ") << Qt::endl;
  }

  return ss.readAll();
}

auto GetTempFilePath() -> QString {
  QString const temp_dir = QDir::tempPath();
  QString const filename = QUuid::createUuid().toString() + ".data";
  return temp_dir + "/" + filename;
}

auto CreateTempFileAndWriteData(const QString& data) -> QString {
  auto temp_file = GetTempFilePath();
  WriteFile(temp_file, data.toUtf8());
  return temp_file;
}

auto TargetFilePreCheck(const QString& path, bool read)
    -> std::tuple<bool, QString> {
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

  return {true, _("Success")};
}

auto GetFullExtension(const QString& path) -> QString {
  QString const filename = QFileInfo(path).fileName();

  auto const dot_index = filename.indexOf('.');
  if (dot_index == -1) return {};

  return filename.mid(dot_index);
}

}  // namespace GpgFrontend