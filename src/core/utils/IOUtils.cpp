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

#include <openssl/err.h>
#include <openssl/evp.h>

#include "core/utils/FilesystemUtils.h"

namespace {
auto GetFileHashOpenSSL(const QString& file_path, const EVP_MD* md_type)
    -> QByteArray {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) return {};

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (ctx == nullptr) return {};

  if (EVP_DigestInit_ex(ctx, md_type, nullptr) != 1) {
    EVP_MD_CTX_free(ctx);
    return {};
  }

  GpgFrontend::GFBuffer buffer(GpgFrontend::kSecBufferSize);
  const auto buffer_size = static_cast<qsizetype>(buffer.Size());
  while (!file.atEnd()) {
    auto n = file.read(buffer.Data(), buffer_size);
    if (n <= 0 || n > buffer_size) break;

    if (EVP_DigestUpdate(ctx, buffer.Data(), n) != 1) {
      EVP_MD_CTX_free(ctx);
      return {};
    }
  }

  std::array<unsigned char, EVP_MAX_MD_SIZE> md_value;
  unsigned int md_len = 0;
  if (EVP_DigestFinal_ex(ctx, md_value.data(), &md_len) != 1) {
    EVP_MD_CTX_free(ctx);
    return {};
  }
  EVP_MD_CTX_free(ctx);

  return {reinterpret_cast<const char*>(md_value.data()), md_len};
}
}  // namespace

namespace GpgFrontend {

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
  QFile file(file_name);
  if (!file.open(QIODevice::ReadOnly)) {
    return {false, GFBuffer()};
  }

  auto file_size = file.size();
  if (file_size <= 0) return {false, GFBuffer()};

  GFBuffer buf(static_cast<size_t>(file_size));
  auto n = file.read(buf.Data(), file_size);
  if (n <= 0 || n != file_size) return {false, GFBuffer()};

  return {true, std::move(buf)};
}

auto WriteFileGFBuffer(const QString& file_name, GFBuffer data) -> bool {
  QFile file(file_name);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }

  file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  auto n = file.write(data.Data(), static_cast<qsizetype>(data.Size()));
  file.flush();
  file.close();
  return n == static_cast<decltype(n)>(data.Size());
}

auto CalculateHash(const QString& file_path) -> QString {
  // Returns empty QByteArray() on failure.
  QFileInfo const info(file_path);
  QString buffer;
  QTextStream ss(&buffer);

  if (info.isFile() && info.isReadable()) {
    auto md5 = GetFileHashOpenSSL(file_path, EVP_md5()).toHex();
    auto sha1 = GetFileHashOpenSSL(file_path, EVP_sha1()).toHex();
    auto sha256 = GetFileHashOpenSSL(file_path, EVP_sha256()).toHex();

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
    ss << "- " << "MD5" << QCoreApplication::tr(": ") << md5 << Qt::endl;

    // sha1
    ss << "- " << "SHA1" << QCoreApplication::tr(": ") << sha1 << Qt::endl;

    // sha1
    ss << "- " << "SHA256" << QCoreApplication::tr(": ") << sha256 << Qt::endl;

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

auto TargetFilePreCheck(const QString& path, bool read)
    -> std::tuple<bool, QString> {
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