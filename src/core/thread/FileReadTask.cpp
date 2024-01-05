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

#include "core/thread/FileReadTask.h"

namespace GpgFrontend::UI {

constexpr size_t kBufferSize = 8192;

FileReadTask::FileReadTask(std::string path) : Task("file_read_task") {
  HoldOnLifeCycle(true);
  connect(this, &FileReadTask::SignalFileBytesReadNext, this,
          &FileReadTask::read_bytes);

#ifdef WINDOWS
  std::filesystem::path read_file_path(
      QString::fromStdString(path).toStdU16String());
#else
  std::filesystem::path read_file_path(
      QString::fromStdString(path).toStdString());
#endif
  read_file_path_ = read_file_path;
}

void FileReadTask::Run() {
  if (is_regular_file(read_file_path_)) {
    GF_CORE_LOG_DEBUG("read open file: {}", read_file_path_.u8string());

    target_file_.setFileName(
        QString::fromStdString(read_file_path_.u8string()));
    target_file_.open(QIODevice::ReadOnly);

    if (!(target_file_.isOpen() && target_file_.isReadable())) {
      GF_CORE_LOG_ERROR("file not open or not readable");
      if (target_file_.isOpen()) target_file_.close();
      return;
    }
    GF_CORE_LOG_DEBUG("started reading: {}", read_file_path_.u8string());
    read_bytes();
  } else {
    emit SignalFileBytesReadEnd();
  }
}

void FileReadTask::read_bytes() {
  QByteArray read_buffer;
  if (!target_file_.atEnd() &&
      (read_buffer = target_file_.read(kBufferSize)).size() > 0) {
    GF_CORE_LOG_DEBUG("io thread read bytes: {}", read_buffer.size());
    emit SignalFileBytesRead(std::move(read_buffer));
  } else {
    GF_CORE_LOG_DEBUG("io read bytes end");
    emit SignalFileBytesReadEnd();
    // announce finish task
    emit SignalTaskShouldEnd(0);
  }
}

FileReadTask::~FileReadTask() {
  GF_CORE_LOG_DEBUG("close file: {}", read_file_path_.u8string());
  if (target_file_.isOpen()) target_file_.close();
}

}  // namespace GpgFrontend::UI
