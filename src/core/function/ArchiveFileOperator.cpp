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
    if (r == ARCHIVE_EOF) return (ARCHIVE_OK);
    if (r != ARCHIVE_OK) {
      LOG(ERROR) << "archive_read_data_block() failed: "
                 << archive_error_string(ar);
      return (r);
    }
    r = archive_write_data_block(aw, buff, size, offset);
    if (r != ARCHIVE_OK) {
      LOG(ERROR) << "archive_write_data_block() failed: "
                 << archive_error_string(aw);
      return (r);
    }
  }
}

void GpgFrontend::ArchiveFileOperator::CreateArchive(
    const std::filesystem::path &base_path,
    const std::filesystem::path &archive_path, int compress,
    const std::vector<std::filesystem::path> &files) {
  LOG(INFO) << "CreateArchive: " << archive_path.u8string();

  auto current_base_path_backup = QDir::currentPath();
  QDir::setCurrent(base_path.u8string().c_str());

  auto relative_archive_path =
      std::filesystem::relative(archive_path, base_path);

  std::vector<std::filesystem::path> relative_files;
  relative_files.reserve(files.size());
  for (const auto &file : files) {
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

  auto u8_filename = relative_archive_path.u8string();

  if (!u8_filename.empty() && u8_filename == u8"-")
    throw std::runtime_error("cannot write to stdout");

#ifdef WINDOWS
  archive_write_open_filename_w(a, relative_archive_path.wstring().c_str());
#else
  archive_write_open_filename(a, u8_filename.c_str());
#endif

  for (const auto &file : relative_files) {
    struct archive *disk = archive_read_disk_new();
#ifndef NO_LOOKUP
    archive_read_disk_set_standard_lookup(disk);
#endif
    int r;

    LOG(INFO) << "reading file: " << file.u8string();

#ifdef WINDOWS
    r = archive_read_disk_open_w(disk, file.wstring().c_str());
#else
    r = archive_read_disk_open(disk, file.u8string().c_str());
#endif

    LOG(INFO) << "read file done: " << file.u8string();

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

#ifdef WINDOWS
      auto entry_path =
          QString::fromStdWString(std::wstring(archive_entry_pathname_w(entry))).toUtf8()
              .toStdString();
#else
      auto entry_path = std::string(archive_entry_pathname_utf8(entry));
#endif

      LOG(INFO) << "Adding: " << archive_entry_pathname_utf8(entry) << "size"
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
        QByteArray buff;
#ifdef WINDOWS
        FileOperator::ReadFile(
            QString::fromStdWString(archive_entry_sourcepath_w(entry)), buff);
#else
        FileOperator::ReadFile(archive_entry_sourcepath(entry), buff);
#endif
        archive_write_data(a, buff.data(), buff.size());
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
  LOG(INFO) << "ExtractArchive: " << archive_path.u8string();

  auto current_base_path_backup = QDir::currentPath();
  QDir::setCurrent(base_path.u8string().c_str());

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

  auto filename = archive_path.u8string();

  if (!filename.empty() && filename == u8"-") {
    LOG(ERROR) << "cannot read from stdin";
  }
#ifdef WINDOWS
  if (archive_read_open_filename_w(a, archive_path.wstring().c_str(),
                                        10240) != ARCHIVE_OK) {
#else
  if (archive_read_open_filename(a, archive_path.u8string().c_str(),
                                      10240) != ARCHIVE_OK) {
#endif
    LOG(ERROR) << "archive_read_open_filename() failed: "
               << archive_error_string(a);
    throw std::runtime_error("archive_read_open_filename() failed");
  }
  for (;;) {
    r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF) break;
    if (r != ARCHIVE_OK) {
      LOG(ERROR) << "archive_read_next_header() failed: "
                 << archive_error_string(a);
      throw std::runtime_error("archive_read_next_header() failed");
    }
    LOG(INFO) << "Extracting: " << archive_entry_pathname(entry) << "size"
              << archive_entry_size(entry) << " bytes"
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

void GpgFrontend::ArchiveFileOperator::ListArchive(
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
    LOG(INFO) << "File: " << archive_entry_pathname(entry);
    LOG(INFO) << "File Path: " << archive_entry_pathname(entry);
    archive_read_data_skip(a);  // Note 2
  }
  r = archive_read_free(a);  // Note 3
  if (r != ARCHIVE_OK) return;
}
