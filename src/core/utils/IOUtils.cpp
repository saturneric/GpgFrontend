/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include "core/utils/FilesystemUtils.h"

namespace GpgFrontend {

auto GetFileChecksum(const QString& file_name,
                     QCryptographicHash::Algorithm hashAlgorithm)
    -> QByteArray {
  QFile f(file_name);
  if (f.open(QFile::ReadOnly)) {
    QCryptographicHash hash(hashAlgorithm);
    if (hash.addData(&f)) {
      return hash.result();
    }
  }
  return {};
}

auto ReadFile(const QString& file_name, QByteArray& data) -> bool {
  QFile file(file_name);
  if (!file.open(QIODevice::ReadOnly)) {
    LOG_W() << "failed to open file: " << file_name;
    return false;
  }
  data = file.readAll();
  file.close();
  return true;
}

auto WriteFile(const QString& file_name, const QByteArray& data) -> bool {
  QFile file(file_name);
  if (!file.open(QIODevice::WriteOnly)) {
    LOG_W() << "failed to open file for writing: " << file_name;
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
    ss << "# " << QCoreApplication::tr("File Hash Information") << Qt::endl;
    ss << "- " << QCoreApplication::tr("Filename") << QCoreApplication::tr(": ")
       << info.fileName() << Qt::endl;

    // read all data
    ss << "- " << QCoreApplication::tr("File Size") << "(bytes)"
       << QCoreApplication::tr(": ") << QString::number(info.size())
       << Qt::endl;

    ss << "- " << QCoreApplication::tr("File Size")
       << QCoreApplication::tr(": ") << GetHumanFriendlyFileSize(info.size())
       << Qt::endl;

    // md5
    ss << "- " << "MD5" << QCoreApplication::tr(": ")
       << GetFileChecksum(file_path, QCryptographicHash::Md5).toHex()
       << Qt::endl;

    // sha1
    ss << "- " << "SHA1" << QCoreApplication::tr(": ")
       << GetFileChecksum(file_path, QCryptographicHash::Sha1).toHex()
       << Qt::endl;

    // sha1
    ss << "- " << "SHA256" << QCoreApplication::tr(": ")
       << GetFileChecksum(file_path, QCryptographicHash::Sha256).toHex()
       << Qt::endl;

    ss << Qt::endl;

  } else {
    ss << "# " << QCoreApplication::tr("Error: cannot read target file")
       << Qt::endl;
    ss << "- " << QCoreApplication::tr("Filename") << QCoreApplication::tr(": ")
       << info.fileName() << Qt::endl;
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

auto CreateTempFileAndWriteData(const GFBuffer& data) -> QString {
  auto temp_file = GetTempFilePath();
  WriteFile(temp_file, data.ConvertToQByteArray());
  return temp_file;
}

auto TargetFilePreCheck(const QString& path,
                        bool read) -> std::tuple<bool, QString> {
  QFileInfo const file_info(path);

  if (read) {
    if (!file_info.exists()) {
      return {false, QCoreApplication::tr("target path doesn't exists")};
    }
  } else {
    QFileInfo const path_info(file_info.absolutePath());
    if (!path_info.isWritable()) {
      return {false,
              QCoreApplication::tr("do NOT have permission to write path")};
    }
  }

  if (read ? !file_info.isReadable() : false) {
    return {false,
            QCoreApplication::tr("do NOT have permission to read/write file")};
  }

  return {true, QCoreApplication::tr("Success")};
}

auto GetFullExtension(const QString& path) -> QString {
  QString const filename = QFileInfo(path).fileName();

  auto const dot_index = filename.indexOf('.');
  if (dot_index == -1) return {};

  return filename.mid(dot_index);
}

auto CalculateBinaryChacksum(const QString& path) -> QString {
  // check file info and access rights
  QFileInfo info(path);
  if (!info.exists() || !info.isFile() || !info.isReadable()) {
    LOG_W() << "get info for file: " << info.filePath()
            << " error, exists: " << info.exists();
    return {};
  }

  // open and read file
  QFile f(info.filePath());
  if (!f.open(QIODevice::ReadOnly)) {
    LOG_W() << "open " << path
            << "to calculate checksum error: " << f.errorString();
    return {};
  }

  QCryptographicHash hash_sha(QCryptographicHash::Sha256);

  // read data by chunks
  const qint64 buffer_size = 8192;  // Define a suitable buffer size
  while (!f.atEnd()) {
    QByteArray const buffer = f.read(buffer_size);
    if (buffer.isEmpty()) {
      LOG_W() << "error reading file: " << path
              << " during checksum calculation";
      return {};
    }
    hash_sha.addData(buffer);
  }

  // close the file
  f.close();

  // return the SHA-256 hash of the file
  return hash_sha.result().toHex();
}

}  // namespace GpgFrontend