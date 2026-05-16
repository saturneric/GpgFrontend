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

#pragma once

#include "core/thread/Task.h"

namespace GpgFrontend::UI {

/**
 * @brief Task that reads a file in fixed-size chunks and emits each chunk as a
 * signal.
 *
 * Uses an event-loop-driven approach: each chunk triggers SignalFileBytesRead,
 * then SignalFileBytesReadNext to schedule the next read. This keeps the runner
 * thread responsive. HoldOnLifeCycle(true) prevents auto-deletion between
 * reads. SignalFileBytesReadEnd is emitted when the file is fully read or does
 * not exist.
 */
class GF_CORE_EXPORT FileReadTask : public GpgFrontend::Thread::Task {
  Q_OBJECT
 public:
  /**
   * @brief Construct a task to read the file at @p path.
   *
   * @param path absolute or relative path to the file to read
   */
  explicit FileReadTask(QString path);

  ~FileReadTask() override;

  /**
   * @brief Open the file and begin the chunked read sequence.
   *
   * Returns -1 if the file cannot be opened or is not readable.
   *
   * @return 0 on success, -1 on open failure
   */
  int Run() override;

 signals:
  /**
   * @brief Emitted for each chunk of data read from the file.
   *
   * @param bytes the chunk of bytes read (up to 8192 bytes per emission)
   */
  void SignalFileBytesRead(QByteArray bytes);

  /**
   * @brief Emitted when the entire file has been read or the file does not
   * exist.
   */
  void SignalFileBytesReadEnd();

  /**
   * @brief Emitted internally to trigger reading the next chunk.
   */
  void SignalFileBytesReadNext();

 private:
  // Absolute path of the file to read.
  QString read_file_path_;
  // File handle kept open across chunk reads.
  QFile target_file_;
  // Event loop used to interleave chunk reads with the runner's event loop.
  QEventLoop looper_;

 private slots:
  /**
   * @brief Read the next chunk and emit SignalFileBytesRead, or emit
   * SignalFileBytesReadEnd and SignalTaskShouldEnd when done.
   */
  void slot_read_bytes();
};

}  // namespace GpgFrontend::UI
