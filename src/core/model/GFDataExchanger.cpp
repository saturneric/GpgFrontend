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

#include <algorithm>

namespace GpgFrontend {

auto GFDataExchanger::Write(const std::byte* buffer, size_t size) -> ssize_t {
  if (close_) return -1;
  if (size == 0) return 0;

  // Bulk between waits, not a wait per byte. Both loops here used to evaluate
  // a condition-variable predicate and consider a notify for every single
  // byte, which put this pipe at single-digit megabytes a second -- so reading
  // a 46 MiB archive through it cost nearly six seconds of pure
  // synchronisation to do a fifth of a second of work.
  //
  // The queue stays a queue of bytes: its element type is what gives the
  // contents the wiping allocator, and that is the reason this class exists.
  // What changed is only how often the two threads talk to each other.
  std::unique_lock<std::mutex> lock(mutex_);
  ssize_t write_bytes = 0;
  size_t written = 0;

  while (written < size) {
    not_full_.wait(lock, [this] {
      return queue_.size() < static_cast<unsigned long>(queue_max_size_) ||
             close_;
    });
    if (close_) return -1;

    const auto room = static_cast<size_t>(queue_max_size_) - queue_.size();
    const auto n = std::min(room, size - written);

    try {
      for (size_t i = 0; i < n; ++i) queue_.push(buffer[written + i]);
    } catch (...) {
      FLOG_W(
          "gf data exchanger caught exception when it writes to queue, "
          "abort...");
      return write_bytes;
    }

    written += n;
    write_bytes += static_cast<ssize_t>(n);
    not_empty_.notify_all();
  }

  return write_bytes;
}

auto GFDataExchanger::Read(std::byte* buffer, size_t size) -> ssize_t {
  std::unique_lock<std::mutex> lock(mutex_);
  if (size == 0 || (close_ && queue_.empty())) return 0;

  ssize_t read_bytes = 0;
  size_t filled = 0;

  while (filled < size) {
    not_empty_.wait(lock, [this] { return !queue_.empty() || close_; });

    // End of stream part-way through filling the caller's buffer. What has
    // already been copied is real data and must be reported: returning 0 here
    // discarded the tail of every stream whose length was not a whole multiple
    // of the reader's buffer, which is almost all of them.
    if (queue_.empty()) return read_bytes;

    const auto n = std::min(queue_.size(), size - filled);
    for (size_t i = 0; i < n; ++i) {
      buffer[filled + i] = queue_.front();
      queue_.pop();
    }

    filled += n;
    read_bytes += static_cast<ssize_t>(n);
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

auto CreateStandardGFDataExchanger() -> QSharedPointer<GFDataExchanger> {
  return SecureCreateSharedObject<GFDataExchanger>(kSecBufferSizeForFile);
}
}  // namespace GpgFrontend