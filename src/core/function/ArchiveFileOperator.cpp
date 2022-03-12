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

#include "ArchiveFileOperator.h"

int copy_data(struct archive *ar, struct archive *aw) {
  int r;
  const void *buff;
  size_t size;
  int64_t offset;

  for (;;) {
    r = archive_read_data_block(ar, &buff, &size, &offset);
    if (r == ARCHIVE_EOF)
      return (ARCHIVE_OK);
    if (r != ARCHIVE_OK) {
      LOG(ERROR) << "archive_read_data_block() failed: " << archive_error_string(ar);
      return (r);
    }
    r = archive_write_data_block(aw, buff, size, offset);
    if (r != ARCHIVE_OK) {
      LOG(ERROR) << "archive_write_data_block() failed: " << archive_error_string(aw);
      return (r);
    }
  }
}

void GpgFrontend::ArchiveFileOperator::CreateArchive(
    const std::filesystem::path &base_path,
    const std::filesystem::path &archive_path, int compress,
    const std::vector<std::filesystem::path> &files) {
  LOG(INFO) << "CreateArchive: " << archive_path.string();

  auto current_base_path_backup = QDir::currentPath();
  QDir::setCurrent(base_path.string().c_str());

  auto relative_archive_path = std::filesystem::relative(archive_path, base_path);

  std::vector<std::filesystem::path> relative_files;
  relative_files.reserve(files.size());
  for(const auto& file : files) {
    relative_files.push_back(std::filesystem::relative(file, base_path));
  }

  struct archive *a;
  struct archive_entry *entry;
  ssize_t len;
  int fd;

  LOG(INFO) << "compress: " << compress;

  a = archive_write_new();
  switch (compress) {
#ifndef NO_BZIP2_CREATE
    case 'j':
    case 'y':
      archive_write_add_filter_bzip2(a);
      break;
#endif
#ifndef NO_COMPRESS_CREATE
    case 'Z':
      archive_write_add_filter_compress(a);
      break;
#endif
#ifndef NO_GZIP_CREATE
    case 'z':
      archive_write_add_filter_gzip(a);
      break;
#endif
    default:
      archive_write_add_filter_none(a);
      break;
  }
  archive_write_set_format_ustar(a);
  archive_write_set_format_pax_restricted(a);

  auto filename = relative_archive_path.string();
  if (!filename.empty() && filename == "-")
    throw std::runtime_error("cannot write to stdout");

  archive_write_open_filename(a, filename.c_str());

  for (const auto &file : relative_files) {
    struct archive *disk = archive_read_disk_new();
#ifndef NO_LOOKUP
    archive_read_disk_set_standard_lookup(disk);
#endif
    int r;

    LOG(INFO) << "ReadFile: " << file.string();

    r = archive_read_disk_open(disk, file.string().c_str());
    if (r != ARCHIVE_OK) {
      LOG(ERROR) << "archive_read_disk_open() failed: "
                 << archive_error_string(disk);
      throw std::runtime_error("archive_read_disk_open() failed");
    }

    for (;;) {
      bool needcr = false;

      entry = archive_entry_new();
      r = archive_read_next_header2(disk, entry);
      if (r == ARCHIVE_EOF) break;
      if (r != ARCHIVE_OK) {
        LOG(ERROR) << "archive_read_next_header2() failed: "
                   << archive_error_string(disk);
        throw std::runtime_error("archive_read_next_header2() failed");
      }
      archive_read_disk_descend(disk);
      LOG(INFO) << "Adding: " << archive_entry_pathname(entry) << "size"
                << archive_entry_size(entry) << " bytes"
                << "file type" << archive_entry_filetype(entry);
      r = archive_write_header(a, entry);
      if (r < ARCHIVE_OK) {
        LOG(ERROR) << "archive_write_header() failed: "
                   << archive_error_string(a);
        throw std::runtime_error("archive_write_header() failed");
      }
      if (r == ARCHIVE_FATAL) throw std::runtime_error("archive fatal");
      if (r > ARCHIVE_FAILED) {
        ByteArray buff;
        FileOperator::ReadFileStd(archive_entry_sourcepath(entry), buff);
        archive_write_data(a, buff.c_str(), buff.size());
      }
      archive_entry_free(entry);
    }
    archive_read_close(disk);
    archive_read_free(disk);
  }
  archive_write_close(a);
  archive_write_free(a);

  QDir::setCurrent(current_base_path_backup);
}

void GpgFrontend::ArchiveFileOperator::ExtractArchive(
    const std::filesystem::path &archive_path,
    const std::filesystem::path &base_path) {

  LOG(INFO) << "ExtractArchive: " << archive_path.string();

  auto current_base_path_backup = QDir::currentPath();
  QDir::setCurrent(base_path.string().c_str());

    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int r;

    a = archive_read_new();
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, 0);
#ifndef NO_BZIP2_EXTRACT
    archive_read_support_filter_bzip2(a);
#endif
#ifndef NO_GZIP_EXTRACT
    archive_read_support_filter_gzip(a);
#endif
#ifndef NO_COMPRESS_EXTRACT
    archive_read_support_filter_compress(a);
#endif
#ifndef NO_TAR_EXTRACT
    archive_read_support_format_tar(a);
#endif
#ifndef NO_CPIO_EXTRACT
    archive_read_support_format_cpio(a);
#endif
#ifndef NO_LOOKUP
    archive_write_disk_set_standard_lookup(ext);
#endif

    auto filename = archive_path.string();

    if (!filename.empty() && filename == "-") {
      LOG(ERROR) << "cannot read from stdin";
    }
    if ((r = archive_read_open_filename(a, filename.c_str(), 10240))) {
      LOG(ERROR) << "archive_read_open_filename() failed: "
                 << archive_error_string(a);
      throw std::runtime_error("archive_read_open_filename() failed");
    }
    for (;;) {
      r = archive_read_next_header(a, &entry);
      if (r == ARCHIVE_EOF)
        break;
      if (r != ARCHIVE_OK) {
        LOG(ERROR) << "archive_read_next_header() failed: "
                   << archive_error_string(a);
        throw std::runtime_error("archive_read_next_header() failed");
      }
      LOG(INFO) << "Extracting: " << archive_entry_pathname(entry)
                << "size" << archive_entry_size(entry) << " bytes"
                << "file type" << archive_entry_filetype(entry);
      r = archive_write_header(ext, entry);
      if (r != ARCHIVE_OK) {
        LOG(ERROR) << "archive_write_header() failed: "
                   << archive_error_string(ext);
      } else {
        r = copy_data(a, ext);
        if (r != ARCHIVE_OK) {
          LOG(ERROR) << "copy_data() failed: " << archive_error_string(ext);
        }
      }
    }
    archive_read_close(a);
    archive_read_free(a);

    archive_write_close(ext);
    archive_write_free(ext);

    QDir::setCurrent(current_base_path_backup);
}
