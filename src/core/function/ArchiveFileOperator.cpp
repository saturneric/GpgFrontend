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

#include "ArchiveFileOperator.h"

#include <archive.h>
#include <archive_entry.h>
#include <sys/fcntl.h>

#include "core/utils/AsyncUtils.h"

namespace GpgFrontend {

auto CopyData(struct archive *ar, struct archive *aw) -> int {
  int r;
  const void *buff;
  size_t size;
  int64_t offset;

  for (;;) {
    r = archive_read_data_block(ar, &buff, &size, &offset);
    if (r == ARCHIVE_EOF) return (ARCHIVE_OK);
    if (r != ARCHIVE_OK) {
      GF_CORE_LOG_ERROR("archive_read_data_block() failed: {}",
                        archive_error_string(ar));
      return (r);
    }
    r = archive_write_data_block(aw, buff, size, offset);
    if (r != ARCHIVE_OK) {
      GF_CORE_LOG_ERROR("archive_write_data_block() failed: {}",
                        archive_error_string(aw));
      return (r);
    }
  }
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

void ArchiveFileOperator::NewArchive2DataExchanger(
    const QString &target_directory, std::shared_ptr<GFDataExchanger> exchanger,
    const OperationCallback &cb) {
  RunIOOperaAsync(
      [=](const DataObjectPtr &data_object) -> GFError {
        std::array<char, 1024> buff{};
        auto ret = 0;
        const auto base_path = QDir(QDir(target_directory).absolutePath());

        auto *archive = archive_write_new();
        archive_write_add_filter_none(archive);
        archive_write_set_format_pax_restricted(archive);

        archive_write_open(archive, exchanger.get(), nullptr,
                           ArchiveWriteCallback, ArchiveCloseWriteCallback);

        auto *disk = archive_read_disk_new();
        archive_read_disk_set_standard_lookup(disk);

#ifdef WINDOWS
        auto r = archive_read_disk_open_w(
            disk, target_directory.toStdWString().c_str());
#else
        auto r = archive_read_disk_open(disk, target_directory.toUtf8());
#endif

        if (r != ARCHIVE_OK) {
          GF_CORE_LOG_ERROR("archive_read_disk_open() failed: {}, abort...",
                            archive_error_string(disk));
          archive_read_free(disk);
          archive_write_free(archive);
          return -1;
        }

        for (;;) {
          auto *entry = archive_entry_new();
          r = archive_read_next_header2(disk, entry);
          if (r == ARCHIVE_EOF) break;
          if (r != ARCHIVE_OK) {
            GF_CORE_LOG_ERROR(
                "archive_read_next_header2() failed, ret: {}, explain: {}", r,
                archive_error_string(disk));
            ret = -1;
            break;
          }

          archive_read_disk_descend(disk);

          // turn absolute path to relative path
          archive_entry_set_pathname(
              entry,
              base_path.relativeFilePath(QString(archive_entry_pathname(entry)))
                  .toUtf8());

          r = archive_write_header(archive, entry);
          if (r < ARCHIVE_OK) {
            GF_CORE_LOG_ERROR(
                "archive_write_header() failed, ret: {}, explain: {} ", r,
                archive_error_string(archive));
            continue;
          }

          if (r == ARCHIVE_FATAL) {
            GF_CORE_LOG_ERROR(
                "archive_write_header() failed, ret: {}, explain: {}, "
                "abort ...",
                r, archive_error_string(archive));
            ret = -1;
            break;
          }

          if (r > ARCHIVE_FAILED) {
            auto fd = open(archive_entry_sourcepath(entry), O_RDONLY);
            auto len = read(fd, buff.data(), buff.size());
            while (len > 0) {
              archive_write_data(archive, buff.data(), len);
              len = read(fd, buff.data(), buff.size());
            }
            close(fd);
          }
          archive_entry_free(entry);
        }

        archive_read_free(disk);
        archive_write_free(archive);
        return ret;
      },
      cb, "archive_write_new");
}

void ArchiveFileOperator::ExtractArchiveFromDataExchanger(
    std::shared_ptr<GFDataExchanger> ex, const QString &target_path,
    const OperationCallback &cb) {
  RunIOOperaAsync(
      [=](const DataObjectPtr &data_object) -> GFError {
        auto *archive = archive_read_new();
        auto *ext = archive_write_disk_new();

        auto r = archive_read_support_filter_all(archive);
        if (r != ARCHIVE_OK) {
          GF_CORE_LOG_ERROR(
              "archive_read_support_filter_all(), ret: {}, reason: {}", r,
              archive_error_string(archive));
          return r;
        }

        r = archive_read_support_format_all(archive);
        if (r != ARCHIVE_OK) {
          GF_CORE_LOG_ERROR(
              "archive_read_support_format_all(), ret: {}, reason: {}", r,
              archive_error_string(archive));
          return r;
        }

        auto rdata = ArchiveReadClientData{};
        rdata.ex = ex.get();

        r = archive_read_open(archive, &rdata, nullptr, ArchiveReadCallback,
                              nullptr);
        if (r != ARCHIVE_OK) {
          GF_CORE_LOG_ERROR("archive_read_open(), ret: {}, reason: {}", r,
                            archive_error_string(archive));
          return r;
        }

        r = archive_write_disk_set_options(ext, 0);
        if (r != ARCHIVE_OK) {
          GF_CORE_LOG_ERROR(
              "archive_write_disk_set_options(), ret: {}, reason: {}", r,
              archive_error_string(archive));
          return r;
        }

        for (;;) {
          struct archive_entry *entry;
          r = archive_read_next_header(archive, &entry);
          if (r == ARCHIVE_EOF) break;
          if (r != ARCHIVE_OK) {
            GF_CORE_LOG_ERROR("archive_read_next_header(), ret: {}, reason: {}",
                              r, archive_error_string(archive));
            break;
          }

          archive_entry_set_pathname(
              entry,
              (target_path + "/" + archive_entry_pathname(entry)).toUtf8());

          r = archive_write_header(ext, entry);
          if (r != ARCHIVE_OK) {
            GF_CORE_LOG_ERROR("archive_write_header(), ret: {}, reason: {}", r,
                              archive_error_string(archive));
          } else {
            r = CopyData(archive, ext);
          }
        }

        r = archive_read_free(archive);
        if (r != ARCHIVE_OK) {
          GF_CORE_LOG_ERROR("archive_read_free(), ret: {}, reason: {}", r,
                            archive_error_string(archive));
        }
        r = archive_write_free(ext);
        if (r != ARCHIVE_OK) {
          GF_CORE_LOG_ERROR("archive_read_free(), ret: {}, reason: {}", r,
                            archive_error_string(archive));
        }

        return 0;
      },
      cb, "archive_read_new");
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
    GF_CORE_LOG_DEBUG("File: {}", archive_entry_pathname(entry));
    GF_CORE_LOG_DEBUG("File Path: {}", archive_entry_pathname(entry));
    archive_read_data_skip(a);  // Note 2
  }
  r = archive_read_free(a);  // Note 3
  if (r != ARCHIVE_OK) return;
}

}  // namespace GpgFrontend