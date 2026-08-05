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

#include "ArchiveFileOperator.h"

#include <archive.h>
#include <archive_entry.h>
#include <sys/fcntl.h>

#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/AsyncUtils.h"

namespace {

/// POSIX set-user-id bit, as stored in an archive entry's permissions.
constexpr unsigned kPermSetUid = 04000;
/// POSIX set-group-id bit, as stored in an archive entry's permissions.
constexpr unsigned kPermSetGid = 02000;

/**
 * @brief Copy one entry's data, counting what actually lands on disk.
 *
 * The count comes from the data blocks rather than from the header's declared
 * size: a hostile archive is free to declare one byte and deliver a gigabyte,
 * and the ceiling has to hold against the bytes that are really written.
 */
auto CopyData(struct archive *ar, struct archive *aw, qint64 max_entry_bytes,
              qint64 remaining_total, qint64 &written) -> int {
  int r;
  const void *buff;
  size_t size;
  int64_t offset;

  written = 0;
  for (;;) {
    r = archive_read_data_block(ar, &buff, &size, &offset);
    if (r == ARCHIVE_EOF) return ARCHIVE_OK;
    if (r != ARCHIVE_OK) {
      LOG_W() << "archive_read_data_block() failed: "
              << archive_error_string(ar);
      return r;
    }

    written += static_cast<qint64>(size);
    if (max_entry_bytes >= 0 && written > max_entry_bytes) {
      LOG_W() << "archive entry exceeds the per-entry limit, aborting; limit: "
              << max_entry_bytes;
      return ARCHIVE_FATAL;
    }
    if (remaining_total >= 0 && written > remaining_total) {
      LOG_W() << "archive exceeds the total extraction limit, aborting";
      return ARCHIVE_FATAL;
    }

    r = archive_write_data_block(aw, buff, size, offset);
    if (r != ARCHIVE_OK) {
      LOG_W() << "archive_write_data_block() failed: "
              << archive_error_string(aw);
      return r;
    }
  }
}

/// True when the entry is one of the types no caller has any use for.
auto IsRefusedEntryType(mode_t filetype) -> bool {
  return filetype != AE_IFREG && filetype != AE_IFDIR && filetype != AE_IFLNK;
}

}  // namespace

namespace GpgFrontend {

auto ArchiveExtractPolicy::Permissive() -> ArchiveExtractPolicy { return {}; }

auto ArchiveExtractPolicy::Strict(qint64 max_total_bytes, int max_entries)
    -> ArchiveExtractPolicy {
  ArchiveExtractPolicy p;
  p.allow_symlinks = false;
  p.allow_hardlinks = false;
  p.max_total_bytes = max_total_bytes;
  p.max_entry_bytes = max_total_bytes;
  p.max_entries = max_entries;
  p.max_path_length = 1024;
  p.max_depth = 32;
  p.require_empty_destination = true;
  return p;
}

auto ArchiveEntryVerdictToString(ArchiveEntryVerdict v) -> const char * {
  switch (v) {
    case ArchiveEntryVerdict::kACCEPT:
      return "accepted";
    case ArchiveEntryVerdict::kREJECT_ABSOLUTE:
      return "absolute path";
    case ArchiveEntryVerdict::kREJECT_DOTDOT:
      return "path escapes the destination";
    case ArchiveEntryVerdict::kREJECT_EMPTY:
      return "empty path";
    case ArchiveEntryVerdict::kREJECT_PATH_TOO_LONG:
      return "path too long";
    case ArchiveEntryVerdict::kREJECT_TOO_DEEP:
      return "path too deep";
  }
  return "unknown";
}

auto ValidateArchiveEntryPath(const QString &path_name,
                              const ArchiveExtractPolicy &policy,
                              QString &normalised) -> ArchiveEntryVerdict {
  normalised.clear();

  if (path_name.isEmpty()) return ArchiveEntryVerdict::kREJECT_EMPTY;
  if (policy.max_path_length >= 0 &&
      path_name.length() > policy.max_path_length) {
    return ArchiveEntryVerdict::kREJECT_PATH_TOO_LONG;
  }

  // a backslash separates on Windows and is a legal filename character
  // elsewhere, but an archive written on either platform may be extracted on
  // the other, so it is a separator everywhere here rather than a character
  // that smuggles "..\\.." past a forward-slash-only check.
  auto path = path_name;
  path.replace('\\', '/');

  if (path.startsWith('/')) return ArchiveEntryVerdict::kREJECT_ABSOLUTE;

  // "C:/..." and "C:" both name a drive; "//host/share" is a UNC root.
  if (path.length() >= 2 && path[1] == ':' && path[0].isLetter()) {
    return ArchiveEntryVerdict::kREJECT_ABSOLUTE;
  }

  QStringList kept;
  const auto parts = path.split('/', Qt::SkipEmptyParts);
  for (const auto &part : parts) {
    if (part == ".") continue;
    if (part == "..") return ArchiveEntryVerdict::kREJECT_DOTDOT;
    kept.append(part);
  }

  if (kept.isEmpty()) return ArchiveEntryVerdict::kREJECT_EMPTY;
  if (policy.max_depth >= 0 && kept.size() > policy.max_depth) {
    return ArchiveEntryVerdict::kREJECT_TOO_DEEP;
  }

  normalised = kept.join('/');
  return ArchiveEntryVerdict::kACCEPT;
}

struct ArchiveReadClientData {
  GFDataExchanger *ex;
  std::array<std::byte, 1024> buf;
  const std::byte *p_buf = buf.data();
};

auto ArchiveReadCallback(struct archive *, void *client_data,
                         const void **buffer) -> ssize_t {
  auto *rdata = static_cast<ArchiveReadClientData *>(client_data);
  *buffer = reinterpret_cast<const void *>(rdata->p_buf);
  return rdata->ex->Read(rdata->buf.data(), rdata->buf.size());
}

auto ArchiveWriteCallback(struct archive *, void *client_data,
                          const void *buffer, size_t length) -> ssize_t {
  auto *ex = static_cast<GFDataExchanger *>(client_data);
  return ex->Write(static_cast<const std::byte *>(buffer), length);
}

auto ArchiveCloseWriteCallback(struct archive *, void *client_data) -> int {
  auto *ex = static_cast<GFDataExchanger *>(client_data);
  ex->CloseWrite();
  return 0;
}

auto ArchiveFileOperator::NewArchive2DataExchangerSync(
    const QString &target_directory,
    const QSharedPointer<GFDataExchanger> &exchanger,
    ArchiveCompression compression, const ArchiveEntryFilter &filter)
    -> GFError {
  {
    {
      auto ret = 0;
      const auto base_path = QDir(QDir(target_directory).absolutePath());

      auto *archive = archive_write_new();
      if (compression == ArchiveCompression::kGZIP) {
        archive_write_add_filter_gzip(archive);
      } else {
        archive_write_add_filter_none(archive);
      }
      archive_write_set_format_pax_restricted(archive);
      archive_write_set_format_option(archive, "pax", "hdrcharset", "BINARY");

      archive_write_open(archive, exchanger.get(), nullptr,
                         ArchiveWriteCallback, ArchiveCloseWriteCallback);

      auto *disk = archive_read_disk_new();
      archive_read_disk_set_standard_lookup(disk);

#ifdef Q_OS_WINDOWS
      auto target_directory_utf16_wstr = std::wstring(
          reinterpret_cast<const wchar_t *>((target_directory).utf16()));
      auto r =
          archive_read_disk_open_w(disk, target_directory_utf16_wstr.c_str());
#else
      auto r = archive_read_disk_open(disk, target_directory.toUtf8());
#endif

      if (r != ARCHIVE_OK) {
        FLOG_W("archive_read_disk_open() failed: %s, abort...",
               archive_error_string(disk));
        archive_read_free(disk);
        archive_write_free(archive);
        return -1;
      }

      for (;;) {
        auto *entry = archive_entry_new();
        r = archive_read_next_header2(disk, entry);
        if (r == ARCHIVE_EOF) {
          archive_entry_free(entry);
          break;
        }
        if (r != ARCHIVE_OK) {
          FLOG_W("archive_read_next_header2() failed, ret: %d, explain: %s", r,
                 archive_error_string(disk));
          archive_entry_free(entry);
          ret = -1;
          break;
        }

#ifdef Q_OS_WINDOWS
        auto source_path =
            QString::fromUtf16(reinterpret_cast<const char16_t *>(
                archive_entry_pathname_w(entry)));
#else
        auto source_path = QString::fromUtf8(archive_entry_pathname(entry));
#endif

        const auto relative_path_name = base_path.relativeFilePath(source_path);
        const auto filetype = archive_entry_filetype(entry);
        const auto is_dir = filetype == AE_IFDIR;

        // the traversal starts at the root itself, which is the archive's
        // frame of reference rather than a member of it
        if (relative_path_name.isEmpty() || relative_path_name == ".") {
          archive_read_disk_descend(disk);
          archive_entry_free(entry);
          continue;
        }

        // deciding before descending is what makes excluding a directory
        // cost nothing: the whole subtree is never walked
        if (filter && !filter(relative_path_name)) {
          archive_entry_free(entry);
          continue;
        }

        archive_read_disk_descend(disk);

        archive_entry_set_pathname(entry, relative_path_name.toUtf8());

        if (is_dir) {
          // a directory carries no data, and saying so keeps libarchive from
          // reserving space for a body that never arrives
          archive_entry_set_size(entry, 0);
          r = archive_write_header(archive, entry);
          if (r < ARCHIVE_OK) {
            FLOG_W("archive_write_header() failed for dir %s, explain: %s",
                   qPrintable(relative_path_name),
                   archive_error_string(archive));
          }
          if (r == ARCHIVE_FATAL) {
            ret = -1;
            archive_entry_free(entry);
            break;
          }
          archive_write_finish_entry(archive);
          archive_entry_free(entry);
          continue;
        }

        QFile file(source_path);
        if (file.open(QIODevice::ReadOnly)) {
#ifdef Q_OS_WINDOWS
          auto source_path_utf16_wstr = std::wstring(
              reinterpret_cast<const wchar_t *>(source_path.utf16()));
          archive_entry_copy_sourcepath_w(entry,
                                          source_path_utf16_wstr.c_str());
#else
          archive_entry_copy_sourcepath(entry, source_path.toUtf8());
#endif

          r = archive_write_header(archive, entry);
          if (r == ARCHIVE_FATAL) {
            FLOG_W(
                "archive_write_header() failed, ret: %d, explain: %s, "
                "abort ...",
                r, archive_error_string(archive));
            ret = -1;
            archive_entry_free(entry);
            break;
          }

          if (r < ARCHIVE_OK) {
            FLOG_W("archive_write_header() failed, ret: %d, explain: %s", r,
                   archive_error_string(archive));
            archive_entry_free(entry);
            continue;
          }

          if (r > ARCHIVE_FAILED) {
            auto buffer = file.read(1024);
            while (!buffer.isEmpty()) {
              archive_write_data(archive, buffer.data(), buffer.size());
              buffer = file.read(1024);
            }
          }
        }
        archive_write_finish_entry(archive);
        archive_entry_free(entry);
      }

      archive_read_free(disk);
      archive_write_free(archive);

      return ret;
    }
  }
}

void ArchiveFileOperator::NewArchive2DataExchanger(
    const QString &target_directory,
    const QSharedPointer<GFDataExchanger> &exchanger,
    const OperationCallback &cb, ArchiveCompression compression,
    const ArchiveEntryFilter &filter) {
  auto *task =
      new Thread::Task{[=](const DataObjectPtr &) -> GFError {
                         return NewArchive2DataExchangerSync(
                             target_directory, exchanger, compression, filter);
                       },
                       "new_archive_2_data_exchanger", TransferParams(), cb};

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_IO)
      ->PostTask(task);
}

auto ArchiveFileOperator::ExtractArchiveFromDataExchangerSync(
    const QSharedPointer<GFDataExchanger> &ex, const QString &target_path,
    const ArchiveExtractPolicy &policy) -> GFError {
  {
    {
      // only ever true for a destination this call is responsible for, which
      // is what makes removing it on failure safe rather than destructive
      auto may_remove_destination = false;

      if (policy.require_empty_destination) {
        QDir dir(target_path);
        if (!dir.exists()) {
          if (!QDir().mkpath(target_path)) {
            FLOG_W("cannot create extraction destination: %s",
                   qPrintable(target_path));
            return -1;
          }
        } else if (!dir.isEmpty(QDir::AllEntries | QDir::NoDotAndDotDot |
                                QDir::Hidden | QDir::System)) {
          FLOG_W("refusing to extract into a non-empty destination: %s",
                 qPrintable(target_path));
          return -1;
        }
        may_remove_destination = true;
      }

      int ret = 0;
      auto *archive = archive_read_new();
      auto *ext = archive_write_disk_new();

      auto fail = [&](int code) -> GFError {
        archive_read_free(archive);
        archive_write_free(ext);
        if (may_remove_destination) QDir(target_path).removeRecursively();
        return code;
      };

      auto r = archive_read_support_filter_all(archive);
      if (r != ARCHIVE_OK) {
        FLOG_W("archive_read_support_filter_all(), ret: %d, reason: %s", r,
               archive_error_string(archive));
        return fail(r);
      }

      r = archive_read_support_format_all(archive);
      if (r != ARCHIVE_OK) {
        FLOG_W("archive_read_support_format_all(), ret: %d, reason: %s", r,
               archive_error_string(archive));
        return fail(r);
      }

      auto rdata = ArchiveReadClientData{};
      rdata.ex = ex.get();

      r = archive_read_open(archive, &rdata, nullptr, ArchiveReadCallback,
                            nullptr);

      if (r != ARCHIVE_OK) {
        FLOG_W("archive_read_open(), ret: %d, reason: %s", r,
               archive_error_string(archive));
        return fail(r);
      }

      // SECURE_SYMLINKS covers what this loop cannot see: a symlink
      // appearing in the destination path between the check and the write.
      //
      // SECURE_NODOTDOT and SECURE_NOABSOLUTEPATHS are deliberately absent.
      // They are applied to the pathname handed to archive_write_header(),
      // which is the destination-prefixed one and therefore always absolute
      // — turning them on rejects every entry. The entry names the archive
      // actually carries are checked by ValidateArchiveEntryPath() above,
      // which is strictly stronger: it also rejects Windows drive letters
      // and treats a backslash as a separator on every platform.
      r = archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_SECURE_SYMLINKS);
      if (r != ARCHIVE_OK) {
        FLOG_W("archive_write_disk_set_options(), ret: %d, reason: %s", r,
               archive_error_string(archive));
        return fail(r);
      }

      qint64 total_written = 0;
      int entry_count = 0;

      for (;;) {
        struct archive_entry *entry;
        r = archive_read_next_header(archive, &entry);
        if (r == ARCHIVE_EOF) break;
        if (r != ARCHIVE_OK) {
          FLOG_W("archive_read_next_header(), ret: %d, reason: %s", r,
                 archive_error_string(archive));
          ret = r;
          break;
        }

        if (policy.max_entries >= 0 && ++entry_count > policy.max_entries) {
          FLOG_W("archive exceeds the entry limit (%d), aborting",
                 policy.max_entries);
          ret = -1;
          break;
        }

        const auto path_name = QString::fromUtf8(archive_entry_pathname(entry));

        QString relative_path;
        const auto verdict =
            ValidateArchiveEntryPath(path_name, policy, relative_path);
        if (verdict != ArchiveEntryVerdict::kACCEPT) {
          FLOG_W("refusing archive entry '%s': %s", qPrintable(path_name),
                 ArchiveEntryVerdictToString(verdict));
          ret = -1;
          break;
        }

        const auto filetype = archive_entry_filetype(entry);
        const auto is_hardlink = archive_entry_hardlink(entry) != nullptr;

        if (IsRefusedEntryType(filetype)) {
          FLOG_W("refusing archive entry '%s': unsupported entry type",
                 qPrintable(path_name));
          ret = -1;
          break;
        }
        if (filetype == AE_IFLNK && !policy.allow_symlinks) {
          FLOG_W("refusing archive entry '%s': symbolic links not allowed",
                 qPrintable(path_name));
          ret = -1;
          break;
        }
        if (is_hardlink && !policy.allow_hardlinks) {
          FLOG_W("refusing archive entry '%s': hard links not allowed",
                 qPrintable(path_name));
          ret = -1;
          break;
        }

        // a set-user-id bit inside an archive is never something the
        // recipient asked for. the bits live in the archive's stored
        // permissions, not in the host's file mode, so they are masked by
        // their fixed POSIX values rather than via <sys/stat.h> — mingw
        // declares neither S_ISUID nor S_ISGID.
        const auto perm = archive_entry_perm(entry);
        archive_entry_set_perm(entry, static_cast<decltype(perm)>(
                                          perm & ~(kPermSetUid | kPermSetGid)));

        // reject before writing, on the declared size, so an obviously
        // oversized entry never starts landing on disk at all
        if (archive_entry_size_is_set(entry)) {
          const auto declared = static_cast<qint64>(archive_entry_size(entry));
          if (policy.max_entry_bytes >= 0 &&
              declared > policy.max_entry_bytes) {
            FLOG_W(
                "refusing archive entry '%s': declared size %lld exceeds "
                "the per-entry limit",
                qPrintable(path_name), static_cast<long long>(declared));
            ret = -1;
            break;
          }
          if (policy.max_total_bytes >= 0 &&
              total_written + declared > policy.max_total_bytes) {
            FLOG_W(
                "refusing archive entry '%s': would exceed the total "
                "extraction limit",
                qPrintable(path_name));
            ret = -1;
            break;
          }
        }

        const auto target_path_name = target_path + "/" + relative_path;

#ifdef Q_OS_WINDOWS
        auto target_path_utf16_wstr = std::wstring(
            reinterpret_cast<const wchar_t *>((target_path_name).utf16()));
        archive_entry_copy_pathname_w(entry, target_path_utf16_wstr.c_str());
#else
        archive_entry_set_pathname(entry, target_path_name.toUtf8());
#endif

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
          FLOG_W("archive_write_header(), ret: %d, reason: %s", r,
                 archive_error_string(ext));
          continue;
        }

        const auto remaining = policy.max_total_bytes < 0
                                   ? qint64{-1}
                                   : policy.max_total_bytes - total_written;

        qint64 written = 0;
        r = CopyData(archive, ext, policy.max_entry_bytes, remaining, written);
        total_written += written;
        if (r != ARCHIVE_OK) {
          ret = -1;
          break;
        }
      }

      if (ret != 0) return fail(ret);

      r = archive_read_free(archive);
      if (r != ARCHIVE_OK) {
        FLOG_W("archive_read_free(), ret: %d, reason: %s", r,
               archive_error_string(archive));
      }
      r = archive_write_free(ext);
      if (r != ARCHIVE_OK) {
        FLOG_W("archive_write_free(), ret: %d, reason: %s", r,
               archive_error_string(ext));
      }

      return ret;
    }
  }
}

void ArchiveFileOperator::ExtractArchiveFromDataExchanger(
    const QSharedPointer<GFDataExchanger> &ex, const QString &target_path,
    const OperationCallback &cb, const ArchiveExtractPolicy &policy) {
  auto *task = new Thread::Task{
      [=](const DataObjectPtr &) -> GFError {
        return ExtractArchiveFromDataExchangerSync(ex, target_path, policy);
      },
      "extract_archive_from_data_exchanger", TransferParams(), cb};

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_IO)
      ->PostTask(task);
}

void ArchiveFileOperator::ListArchive(const QString &archive_path) {
  struct archive *a;
  struct archive_entry *entry;
  int r;

  a = archive_read_new();
  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);
  r = archive_read_open_filename(a, archive_path.toUtf8(),
                                 10240);  // Note 1
  if (r != ARCHIVE_OK) return;
  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    FLOG_D("File: %s", archive_entry_pathname(entry));
    FLOG_D("File Path: %s", archive_entry_pathname(entry));
    archive_read_data_skip(a);  // Note 2
  }
  r = archive_read_free(a);  // Note 3
  if (r != ARCHIVE_OK) return;
}

}  // namespace GpgFrontend
