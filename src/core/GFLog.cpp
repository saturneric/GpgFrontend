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
#include "GFLog.h"

namespace GpgFrontend {

auto BuildQtLoggingFilterRules(int level) -> QString {
  switch (static_cast<GFLogLevel>(level)) {
    case GFLogLevel::kDEBUG:
      return {};
    case GFLogLevel::kINFO:
      return "*.debug=false\n";
    case GFLogLevel::kWARNING:
      return "*.debug=false\n"
             "*.info=false\n";
    case GFLogLevel::kCRITICAL:
      return "*.debug=false\n"
             "*.info=false\n"
             "*.warning=false\n";
    case GFLogLevel::kFATAL:
      return "*.debug=false\n"
             "*.info=false\n"
             "*.warning=false\n"
             "*.critical=false\n";
    default:
      return "*.debug=false\n";
  }
}

void GFLogRingBuffer::Push(GFLogEntry entry) {
  QMutexLocker locker(&mutex_);

  buffer_[write_index_] = std::move(entry);
  write_index_ = (write_index_ + 1) % capacity_;

  if (size_ < capacity_) {
    ++size_;
  } else {
    full_ = true;
  }
}

auto GFLogRingBuffer::Snapshot() const -> QVector<GFLogEntry> {
  QMutexLocker locker(&mutex_);

  QVector<GFLogEntry> result;
  result.reserve(size_);

  if (size_ == 0) return result;

  const int start = full_ ? write_index_ : 0;
  for (int i = 0; i < size_; ++i) {
    const int index = (start + i) % capacity_;
    result.push_back(buffer_[index]);
  }

  return result;
}

auto GFLogRingBuffer::Size() const -> int {
  QMutexLocker locker(&mutex_);
  return size_;
}

auto GFLogRingBuffer::Capacity() const -> int { return capacity_; }

auto GFLogManager::Instance() -> GFLogManager& {
  static GFLogManager instance;
  return instance;
}

void GFLogManager::InitRingBuffer(int capacity) {
  QMutexLocker locker(&mutex_);
  ring_buffer_ = std::make_unique<GFLogRingBuffer>(capacity);
}

void GFLogManager::Push(const GFLogEntry& entry) {
  QMutexLocker locker(&mutex_);
  if (ring_buffer_ != nullptr) {
    ring_buffer_->Push(entry);
  }
}

auto GFLogManager::Snapshot() const -> QVector<GFLogEntry> {
  QMutexLocker locker(&mutex_);
  if (ring_buffer_ == nullptr) return {};
  return ring_buffer_->Snapshot();
}

}  // namespace GpgFrontend
