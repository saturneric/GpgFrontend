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

#include "GFDataExchanger.h"

namespace GpgFrontend {

auto GFDataExchanger::Write(const std::byte* buffer, size_t size) -> ssize_t {
  if (close_) return -1;
  if (size == 0) return 0;

  std::atomic<ssize_t> write_bytes = 0;
  std::unique_lock<std::mutex> lock(mutex_);
  try {
    for (size_t i = 0; i < size; i++) {
      if (queue_.size() == static_cast<unsigned long>(queue_max_size_)) {
        not_empty_.notify_all();
      }

      not_full_.wait(lock, [=] {
        return queue_.size() < static_cast<unsigned long>(queue_max_size_) ||
               close_;
      });
      if (close_) return -1;

      queue_.push(buffer[i]);
      write_bytes++;
    }
  } catch (...) {
    qCWarning(
        core,
        "gf data exchanger caught exception when it writes to queue, abort...");
  }

  if (!queue_.empty()) not_empty_.notify_all();
  return write_bytes;
}

auto GFDataExchanger::Read(std::byte* buffer, size_t size) -> ssize_t {
  std::unique_lock<std::mutex> lock(mutex_);
  if (size == 0 || (close_ && queue_.empty())) return 0;

  std::atomic<ssize_t> read_bytes = 0;
  for (size_t i = 0; i < size; ++i) {
    if (queue_.empty()) not_full_.notify_all();
    not_empty_.wait(lock, [=] { return !queue_.empty() || close_; });

    if (close_ && queue_.empty()) return 0;
    buffer[i] = queue_.front();
    queue_.pop();
    read_bytes++;
  }

  if (queue_.size() < static_cast<unsigned long>(queue_max_size_)) {
    not_full_.notify_all();
  }
  return read_bytes;
}

void GFDataExchanger::CloseWrite() {
  std::unique_lock<std::mutex> const lock(mutex_);

  close_ = true;
  not_full_.notify_all();
  not_empty_.notify_all();
}

GFDataExchanger::GFDataExchanger(ssize_t size) : queue_max_size_(size) {}

}  // namespace GpgFrontend