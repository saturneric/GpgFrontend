/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "FileReadThread.h"

#include <boost/filesystem.hpp>
#include <utility>

namespace GpgFrontend::UI {

FileReadThread::FileReadThread(std::string path) : path(std::move(path)) {}

void FileReadThread::run() {
  LOG(INFO) << "read_thread Started";
  boost::filesystem::path read_file_path(this->path);
  if (is_regular_file(read_file_path)) {
    LOG(INFO) << "read_thread Read Open";

    auto fp = fopen(read_file_path.c_str(), "r");
    size_t read_size;
    LOG(INFO) << "Thread Start Reading";
    
    char buffer[8192];
    while ((read_size = fread(buffer, sizeof(char), sizeof buffer, fp)) > 0) {
      // Check isInterruptionRequested
      if (QThread::currentThread()->isInterruptionRequested()) {
        LOG(INFO) << "Read Thread isInterruptionRequested ";
        fclose(fp);
        return;
      }
      LOG(INFO) << "Read Thread Read block size " << read_size;
      std::string buffer_str(buffer, read_size);

      emit sendReadBlock(QString::fromStdString(buffer_str));
    }
    fclose(fp);
    emit readDone();
    LOG(INFO) << "Thread End Reading";
  }
}

}  // namespace GpgFrontend::UI
