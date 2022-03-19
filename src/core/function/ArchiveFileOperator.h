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

#ifndef GPGFRONTEND_ARCHIVEFILEOPERATOR_H
#define GPGFRONTEND_ARCHIVEFILEOPERATOR_H

#include "core/GpgFrontendCore.h"
#include "core/function/FileOperator.h"

namespace GpgFrontend {

struct ArchiveStruct {
  struct archive *archive;
  struct archive_entry *entry;
  int fd;
  bool is_open;
  std::string name;
};

class ArchiveFileOperator {
 public:
  static void ListArchive(const std::filesystem::path &archive_path) {
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

  static void CreateArchive(
      const std::filesystem::path &base_path,
      const std::filesystem::path &archive_path,
                            int compress,
                            const std::vector<std::filesystem::path> &files);

  static void ExtractArchive(
      const std::filesystem::path &archive_path,
      const std::filesystem::path &base_path);
};
}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ARCHIVEFILEOPERATOR_H
