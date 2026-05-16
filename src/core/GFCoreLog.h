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

namespace GpgFrontend {

/**
 * @brief Builds Qt logging filter rules string based on the given log level,
 * suppressing all log categories below the specified threshold.
 *
 * @param level log level threshold as integer
 * @return QString filter rules string for QLoggingCategory
 */
auto GF_CORE_EXPORT BuildQtLoggingFilterRules(int level) -> QString;

enum class GFLogLevel : uint8_t {
  kDEBUG = 0,
  kINFO = 1,
  kWARNING = 2,
  kCRITICAL = 3,
  kFATAL = 4
};

/**
 * @brief Represents a single log entry with metadata such as timestamp,
 * severity type, source file, and message content.
 */
struct GFLogEntry {
  QDateTime timestamp;
  QtMsgType type;
  QString category;
  QString file;
  int line = 0;
  QString function;
  QString formatted_message;
  QString raw_message;
};

class GFLogRingBuffer {
 public:
  /**
   * @brief Construct a new GFLogRingBuffer object
   *
   * @param capacity maximum number of log entries the buffer can hold
   */
  explicit GFLogRingBuffer(int capacity = 1024)
      : capacity_(capacity), buffer_(capacity) {}

  /**
   * @brief Appends a log entry to the ring buffer, overwriting the oldest
   * entry when the buffer is full.
   *
   * @param entry log entry to store
   */
  void Push(GFLogEntry entry);

  /**
   * @brief Returns a chronological snapshot of all entries currently stored
   * in the buffer.
   *
   * @return QVector<GFLogEntry> ordered copy of buffered log entries
   */
  auto Snapshot() const -> QVector<GFLogEntry>;

  /**
   * @brief Returns the current number of entries in the buffer.
   *
   * @return int number of buffered entries
   */
  auto Size() const -> int;

  /**
   * @brief Returns the maximum capacity of the ring buffer.
   *
   * @return int max entries the buffer can hold
   */
  auto Capacity() const -> int;

 private:
  int capacity_ = 0;
  mutable QMutex mutex_;
  QVector<GFLogEntry> buffer_;
  int write_index_ = 0;
  int size_ = 0;
  bool full_ = false;
};

class GF_CORE_EXPORT GFLogManager {
 public:
  /**
   * @brief Returns the singleton instance of the log manager.
   *
   * @return GFLogManager& reference to the singleton instance
   */
  static auto Instance() -> GFLogManager&;

  /**
   * @brief Initializes the internal ring buffer with the specified capacity.
   *
   * @param capacity maximum number of log entries to buffer
   */
  void InitRingBuffer(int capacity);

  /**
   * @brief Thread-safe push of a log entry into the ring buffer.
   *
   * @param entry log entry to store
   */
  void Push(const GFLogEntry& entry);

  /**
   * @brief Thread-safe snapshot of all buffered log entries.
   *
   * @return QVector<GFLogEntry> ordered copy of buffered log entries
   */
  auto Snapshot() const -> QVector<GFLogEntry>;

 private:
  GFLogManager() = default;

  mutable QMutex mutex_;
  std::unique_ptr<GFLogRingBuffer> ring_buffer_;
};

}  // namespace GpgFrontend
