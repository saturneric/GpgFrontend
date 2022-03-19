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

#include "FileReadThread.h"


#include <utility>

namespace GpgFrontend::UI {

FileReadThread::FileReadThread(std::string path) : path_(std::move(path)) {
  qRegisterMetaType<std::string>("std::string");
}

void FileReadThread::run() {
  LOG(INFO) << "started reading" << path_;

#ifdef WINDOWS
  std::filesystem::path read_file_path(QString::fromStdString(path_).toStdU16String());
#else
  std::filesystem::path read_file_path(QString::fromStdString(path_).toStdString());
#endif

  if (is_regular_file(read_file_path)) {
    LOG(INFO) << "read open" << read_file_path;

    QFile target_file;
    target_file.setFileName(QString::fromStdString(read_file_path.u8string()));
    target_file.open(QIODevice::ReadOnly);
    QByteArray read_buffer;
    LOG(INFO) << "thread start reading";

    const size_t buffer_size = 4096;
    if(!(target_file.isOpen() && target_file.isReadable())) {
      LOG(ERROR) << "file not open or not readable";
      if(target_file.isOpen())
        target_file.close();
      return;
    }

    while (!target_file.atEnd() && (read_buffer = target_file.read(buffer_size)).size() > 0) {
      // Check isInterruptionRequested
      if (QThread::currentThread()->isInterruptionRequested()) {
        LOG(INFO) << "thread is interruption requested ";
        target_file.close();
        return;
      }
      LOG(INFO) << "block size " << read_buffer.size();
      std::string buffer_str(read_buffer.toStdString());

      emit SignalSendReadBlock(buffer_str);
#ifdef RELEASE
      QThread::msleep(32);
#else
      QThread::msleep(128);
#endif
    }
    target_file.close();
    emit SignalReadDone();
    LOG(INFO) << "thread end reading";
  }
}

}  // namespace GpgFrontend::UI
