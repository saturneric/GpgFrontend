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

#include "core/function/GFBufferFactory.h"
#include "core/utils/FilesystemUtils.h"

namespace {

auto GetFileHashQt(const QString& file_path,
                   QCryptographicHash::Algorithm algorithm) -> QByteArray {
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) return {};

  QCryptographicHash hash(algorithm);

  GpgFrontend::GFBuffer buffer(GpgFrontend::kSecBufferSize);
  const auto buffer_size = static_cast<qint64>(buffer.Size());

  while (!file.atEnd()) {
    const auto n = file.read(buffer.Data(), buffer_size);
    if (n < 0) return {};
    if (n == 0) break;

    hash.addData(QByteArray::fromRawData(buffer.Data(), n));
  }

  return hash.result();
}

// Prefix of the work directories staged outputs are written into. Each one is
// created fresh, private and with an unpredictable suffix, so two GpgFrontend
// instances never share one and nobody can create one in advance.
auto SafeOutputWorkDirPrefix() -> QString {
  return QStringLiteral(".gpgfrontend.tmp.");
}

// An unpredictable suffix for staging names that cannot live in a private
// work directory.
auto UnpredictableSuffix() -> QString {
  return QUuid::createUuid().toString(QUuid::Id128);
}

// Give the owner write access to every directory of a tree, so the entries
// in it can be unlinked. Links are skipped: changing a mode follows a link.
void MakeTreeRemovable(const QString& root) {
  const auto add_owner_rwx = [](const QString& dir) {
    QFile::setPermissions(dir,
                          QFile::permissions(dir) | QFileDevice::ReadOwner |
                              QFileDevice::WriteOwner | QFileDevice::ExeOwner);
  };

  add_owner_rwx(root);
  QDirIterator it(
      root, QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
      QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const auto dir = it.next();
    if (!QFileInfo(dir).isSymLink()) add_owner_rwx(dir);
  }
}

// Remove a staged output, file or directory, with everything in it. A tree
// with read-only directories in it (an extracted archive can carry them) is
// made writable first rather than left behind with plaintext inside.
auto RemoveStagedPath(const QString& path) -> bool {
  const QFileInfo info(path);
  if (info.isDir() && !info.isSymLink()) {
    if (QDir(path).removeRecursively()) return true;

    MakeTreeRemovable(path);
    return QDir(path).removeRecursively();
  }
  return !info.exists() && !info.isSymLink() ? true : QFile::remove(path);
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
  // QSaveFile writes through a link to its target. Secret material must land
  // at the path that was chosen, not wherever a link there points.
  if (QFileInfo(file_name).isSymLink()) {
    LOG_W() << "refusing to write through a symbolic link:" << file_name;
    return false;
  }

  // Written to a new file beside the target and renamed over it on commit,
  // so a failed or short write never leaves a truncated file, and the content
  // is never readable by others: the new file is owner-only before any byte
  // is written.
  QSaveFile file(file_name);
  if (!file.open(QIODevice::WriteOnly)) return false;

  if (!file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
    LOG_W() << "cannot restrict the permissions of:" << file_name;
    file.cancelWriting();
    return false;
  }

  const auto size = static_cast<qint64>(data.Size());
  if (file.write(data.Data(), size) != size) {
    file.cancelWriting();
    return false;
  }

  return file.commit();
}

auto CalculateFileHashInfo(const QString& file_path)
    -> QList<QPair<QString, QString>> {
  QList<QPair<QString, QString>> fields;

  QFileInfo const info(file_path);
  if (!info.isFile() || !info.isReadable()) return fields;

  // Read and hash the file exactly once; everything else is derived from these
  // fields so no consumer ever has to parse formatted text back into data.
  fields.append({QCoreApplication::tr("Filename"), info.fileName()});
  fields.append({QCoreApplication::tr("File Size") + QStringLiteral("(bytes)"),
                 QString::number(info.size())});
  fields.append({QCoreApplication::tr("File Size"),
                 GetHumanFriendlyFileSize(info.size())});
  fields.append(
      {QStringLiteral("MD5"),
       QString::fromLatin1(
           GetFileHashQt(file_path, QCryptographicHash::Md5).toHex())});
  fields.append(
      {QStringLiteral("SHA1"),
       QString::fromLatin1(
           GetFileHashQt(file_path, QCryptographicHash::Sha1).toHex())});
  fields.append(
      {QStringLiteral("SHA256"),
       QString::fromLatin1(
           GetFileHashQt(file_path, QCryptographicHash::Sha256).toHex())});

  return fields;
}

auto FormatFileHashInfo(const QList<QPair<QString, QString>>& fields)
    -> QString {
  QString buffer;
  QTextStream ss(&buffer);

  ss << "# " << QCoreApplication::tr("File Hash Information") << Qt::endl;
  for (const auto& field : fields) {
    ss << "- " << field.first << QCoreApplication::tr(": ") << field.second
       << Qt::endl;
  }
  ss << Qt::endl;

  return ss.readAll();
}

auto CalculateHash(const QString& file_path) -> QString {
  QFileInfo const info(file_path);
  if (!info.isFile() || !info.isReadable()) {
    QString buffer;
    QTextStream ss(&buffer);
    ss << "# " << QCoreApplication::tr("Error: cannot read target file")
       << Qt::endl;
    ss << "- " << QCoreApplication::tr("Filename") << QCoreApplication::tr(": ")
       << info.fileName() << Qt::endl;
    return ss.readAll();
  }

  return FormatFileHashInfo(CalculateFileHashInfo(file_path));
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

auto MakeSafeOutputTempPath(const QString& final_path) -> QString {
  const QFileInfo info(final_path);
  const auto base_name = info.fileName();
  const auto final_dir = info.absolutePath();

  // mkdtemp semantics: a new directory, owner-only (0700), with a random
  // suffix. Nothing can be planted in it before the output is written.
  QTemporaryDir work_dir(
      QDir(final_dir).absoluteFilePath(SafeOutputWorkDirPrefix() + "XXXXXX"));
  if (work_dir.isValid()) {
    work_dir.setAutoRemove(false);
    return QDir(work_dir.path()).absoluteFilePath(base_name);
  }

  LOG_W() << "cannot create an output work directory in:" << final_dir
          << ", staging next to the final file instead";
  return QDir(final_dir).absoluteFilePath(
      QString(".%1.gpgfrontend.tmp.%2").arg(base_name, UnpredictableSuffix()));
}

auto CommitSafeOutputFile(const QString& temp_path, const QString& final_path)
    -> bool {
  if (!QFileInfo::exists(temp_path)) return false;

  const QFileInfo final_info(final_path);
  const auto backup_path =
      QDir(final_info.absolutePath())
          .absoluteFilePath(
              QString(".%1.gpgfrontend.backup.%2")
                  .arg(final_info.fileName(), UnpredictableSuffix()));

  bool has_backup = false;

  // QDir::rename moves files and directories alike (an archive decryption
  // stages a directory).
  if (QFileInfo::exists(final_path) || QFileInfo(final_path).isSymLink()) {
    if (!QDir().rename(final_path, backup_path)) return false;

    has_backup = true;
  }

  if (!QDir().rename(temp_path, final_path)) {
    if (has_backup) QDir().rename(backup_path, final_path);
    return false;
  }

  if (has_backup && !RemoveStagedPath(backup_path)) {
    LOG_W() << "cannot remove the replaced output:" << backup_path;
  }

  return true;
}

auto DiscardSafeOutput(const QString& temp_path) -> bool {
  if (temp_path.isEmpty()) return true;
  return RemoveStagedPath(temp_path);
}

void RemoveSafeOutputWorkDir(const QString& temp_path) {
  if (temp_path.isEmpty()) return;

  const auto dir = QFileInfo(temp_path).absolutePath();
  if (!QFileInfo(dir).fileName().startsWith(SafeOutputWorkDirPrefix())) return;

  QDir().rmdir(dir);
}

auto CalculateBinaryChacksum(QIODevice& io) -> QString {
  // Integrity digests belong to GFBufferFactory, which is where the module
  // pipeline computes every one it compares against. This kept a private
  // QCryptographicHash loop, so the digest a module was admitted on came from
  // a different library than the digest its package was signed over.
  // (GetFileHashQt() above is unrelated: it offers several algorithms for the
  // file-info panel to display, and nothing decides anything on its output.)
  return GFBufferFactory::Sha256HexOfDevice(io);
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

  return CalculateBinaryChacksum(f);
}

}  // namespace GpgFrontend