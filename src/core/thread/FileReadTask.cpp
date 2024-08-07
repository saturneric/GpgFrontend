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

#include "core/thread/FileReadTask.h"

namespace GpgFrontend::UI {

constexpr size_t kBufferSize = 8192;

FileReadTask::FileReadTask(QString path)
    : Task("file_read_task"), read_file_path_(std::move(path)) {
  HoldOnLifeCycle(true);
  connect(this, &FileReadTask::SignalFileBytesReadNext, this,
          &FileReadTask::slot_read_bytes);
}

auto FileReadTask::Run() -> int {
  if (QFileInfo(read_file_path_).isFile()) {
    target_file_.setFileName(read_file_path_);
    target_file_.open(QIODevice::ReadOnly);

    if (!(target_file_.isOpen() && target_file_.isReadable())) {
      FLOG_W("file not open or not readable");
      if (target_file_.isOpen()) target_file_.close();
      return -1;
    }

    slot_read_bytes();
  } else {
    emit SignalFileBytesReadEnd();
  }
  return 0;
}

void FileReadTask::slot_read_bytes() {
  QByteArray read_buffer;
  if (QByteArray read_buffer;
      !target_file_.atEnd() &&
      (read_buffer = target_file_.read(kBufferSize)).size() > 0) {
    emit SignalFileBytesRead(std::move(read_buffer));
  } else {
    emit SignalFileBytesReadEnd();
    // announce finish task
    emit SignalTaskShouldEnd(0);
  }
}

FileReadTask::~FileReadTask() {
  if (target_file_.isOpen()) target_file_.close();
}

}  // namespace GpgFrontend::UI
