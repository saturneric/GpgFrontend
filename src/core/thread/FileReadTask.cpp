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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "core/thread/FileReadTask.h"

namespace GpgFrontend::UI {

FileReadTask::FileReadTask(std::string path) : Task("file_read_task") {
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
  SetFinishAfterRun(false);

  if (is_regular_file(read_file_path_)) {
    SPDLOG_DEBUG("read open file: {}", read_file_path_.u8string());

    target_file_.setFileName(
        QString::fromStdString(read_file_path_.u8string()));
    target_file_.open(QIODevice::ReadOnly);

    if (!(target_file_.isOpen() && target_file_.isReadable())) {
      SPDLOG_ERROR("file not open or not readable");
      if (target_file_.isOpen()) target_file_.close();
      return;
    }
    SPDLOG_DEBUG("started reading: {}", read_file_path_.u8string());
    read_bytes();
  } else {
    emit SignalFileBytesReadEnd();
  }
}

void FileReadTask::read_bytes() {
  QByteArray read_buffer;
  if (!target_file_.atEnd() &&
      (read_buffer = target_file_.read(buffer_size_)).size() > 0) {
    SPDLOG_DEBUG("read bytes: {}", read_buffer.size());
    emit SignalFileBytesRead(std::move(read_buffer));
  } else {
    SPDLOG_DEBUG("read bytes end");
    emit SignalFileBytesReadEnd();
    // announce finish task
    emit SignalTaskRunnableEnd(0);
  }
}

FileReadTask::~FileReadTask() {
  SPDLOG_DEBUG("close file: {}", read_file_path_.u8string());
  if (target_file_.isOpen()) target_file_.close();
}

}  // namespace GpgFrontend::UI
