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

#include <boost/filesystem.hpp>
#include <utility>

namespace GpgFrontend::UI {

FileReadThread::FileReadThread(std::string path) : path_(std::move(path)) {
  qRegisterMetaType<std::string>("std::string");
}

void FileReadThread::run() {
  LOG(INFO) << "started";
  boost::filesystem::path read_file_path(this->path_);
  if (is_regular_file(read_file_path)) {
    LOG(INFO) << "read open";

    auto fp = fopen(read_file_path.string().c_str(), "rb");
    size_t read_size;
    LOG(INFO) << "thread start reading";

    char buffer[4096];
    while ((read_size = fread(buffer, sizeof(char), sizeof buffer, fp)) > 0) {
      // Check isInterruptionRequested
      if (QThread::currentThread()->isInterruptionRequested()) {
        LOG(INFO) << "thread is interruption requested ";
        fclose(fp);
        return;
      }
      LOG(INFO) << "block size " << read_size;
      std::string buffer_str(buffer, read_size);

      emit SignalSendReadBlock(buffer_str);
#ifdef RELEASE
      QThread::msleep(32);
#else
      QThread::msleep(128);
#endif
    }
    fclose(fp);
    emit SignalReadDone();
    LOG(INFO) << "thread end reading";
  }
}

}  // namespace GpgFrontend::UI
