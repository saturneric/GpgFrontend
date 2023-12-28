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

#include <fstream>

#include "core/utils/AsyncUtils.h"

namespace GpgFrontend {

struct ArchiveStruct {
  struct archive *archive;
  struct archive_entry *entry;
  int fd;
  bool is_open;
  std::string name;
};

auto CopyData(struct archive *ar, struct archive *aw) -> int {
  int r;
  const void *buff;
  size_t size;
  int64_t offset;

  for (;;) {
    r = archive_read_data_block(ar, &buff, &size, &offset);
    if (r == ARCHIVE_EOF) return (ARCHIVE_OK);
    if (r != ARCHIVE_OK) {
      SPDLOG_ERROR("archive_read_data_block() failed: {}",
                   archive_error_string(ar));
      return (r);
    }
    r = archive_write_data_block(aw, buff, size, offset);
    if (r != ARCHIVE_OK) {
      SPDLOG_ERROR("archive_write_data_block() failed: {}",
                   archive_error_string(aw));
      return (r);
    }
  }
}

void ArchiveFileOperator::NewArchive2Fd(
    const std::filesystem::path &target_directory, int fd,
    const OperationCallback &cb) {
  RunIOOperaAsync(
      [=](const DataObjectPtr &data_object) -> GFError {
        struct archive *archive;
        struct archive_entry *entry;
        std::array<char, 8192> buff{};

        archive = archive_write_new();
        archive_write_add_filter_none(archive);
        archive_write_set_format_pax_restricted(archive);
        archive_write_open_fd(archive, fd);

        for (const auto &file :
             std::filesystem::recursive_directory_iterator(target_directory)) {
          entry = archive_entry_new();

          auto file_path = file.path().string();
          archive_entry_set_pathname(entry, file_path.c_str());
          archive_entry_set_filetype(entry, AE_IFREG);
          archive_entry_set_perm(entry, 0644);

          std::ifstream target_file(file_path, std::ifstream::binary);
          if (!target_file) {
            SPDLOG_ERROR("cannot open file: {}, abort...", file_path);
            archive_entry_free(entry);
            continue;
          }

          target_file.seekg(0, std::ios::end);
          auto file_size = target_file.tellg();
          target_file.seekg(0, std::ios::beg);
          archive_entry_set_size(entry, file_size);

          archive_write_header(archive, entry);

          while (!target_file.eof()) {
            target_file.read(buff.data(), buff.size());
            std::streamsize const bytes_read = target_file.gcount();
            archive_write_data(archive, buff.data(), bytes_read);
          }

          archive_entry_free(entry);
        }

        archive_write_close(archive);
        archive_write_free(archive);
        close(fd);
        return 0;
      },
      cb, "archive_write_new");
}

void ArchiveFileOperator::ExtractArchiveFromFd(
    int fd, const std::filesystem::path &target_path,
    const OperationCallback &cb) {
  SPDLOG_DEBUG("extract archive from fd start, cuurent thread: {}",
               QThread::currentThread()->currentThreadId());
  RunIOOperaAsync(
      [=](const DataObjectPtr &data_object) -> GFError {
        SPDLOG_DEBUG("extract archive from fd processing, cuurent thread: {}",
                     QThread::currentThread()->currentThreadId());
        struct archive *archive;
        struct archive *ext;
        struct archive_entry *entry;

        archive = archive_read_new();
        ext = archive_write_disk_new();
        archive_write_disk_set_options(ext, 0);

#ifndef NO_BZIP2_EXTRACT
        archive_read_support_filter_bzip2(archive);
#endif
#ifndef NO_GZIP_EXTRACT
        archive_read_support_filter_gzip(archive);
#endif
#ifndef NO_COMPRESS_EXTRACT
        archive_read_support_filter_compress(archive);
#endif
#ifndef NO_TAR_EXTRACT
        archive_read_support_format_tar(archive);
#endif
#ifndef NO_CPIO_EXTRACT
        archive_read_support_format_cpio(archive);
#endif
#ifndef NO_LOOKUP
        archive_write_disk_set_standard_lookup(ext);
#endif

        archive_read_open_fd(archive, fd, 8192);
        SPDLOG_ERROR("archive_read_open_fd() failed: {}",
                     archive_error_string(archive));

        while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
          SPDLOG_DEBUG("add file: {}, size: {}, bytes: {}, file type: {}",
                       archive_entry_pathname_utf8(entry),
                       archive_entry_size(entry),
                       archive_entry_filetype(entry));

          auto file_path =
              std::filesystem::path(archive_entry_pathname_utf8(entry));
          auto target_file_path = target_path / file_path;
          archive_entry_set_pathname(entry, file_path.c_str());

          auto r = archive_write_header(ext, entry);
          if (r != ARCHIVE_OK) {
            SPDLOG_ERROR("archive_write_header() failed: {}",
                         archive_error_string(ext));
          }

          r = CopyData(archive, ext);
          if (r != ARCHIVE_OK) {
            SPDLOG_ERROR("copy_data() failed: {}", archive_error_string(ext));
          }
        }

        archive_read_free(archive);
        archive_write_free(ext);
        close(fd);
        return 0;
      },
      cb, "archive_read_new");
}

void ArchiveFileOperator::ListArchive(
    const std::filesystem::path &archive_path) {
  struct archive *a;
  struct archive_entry *entry;
  int r;

  a = archive_read_new();
  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);
  r = archive_read_open_filename(a, archive_path.u8string().c_str(),
                                 10240);  // Note 1
  if (r != ARCHIVE_OK) return;
  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    SPDLOG_DEBUG("File: {}", archive_entry_pathname(entry));
    SPDLOG_DEBUG("File Path: {}", archive_entry_pathname(entry));
    archive_read_data_skip(a);  // Note 2
  }
  r = archive_read_free(a);  // Note 3
  if (r != ARCHIVE_OK) return;
}

}  // namespace GpgFrontend