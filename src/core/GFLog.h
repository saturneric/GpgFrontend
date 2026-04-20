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
 * @brief
 *
 * @param level
 * @return QString
 */
auto GF_CORE_EXPORT BuildQtLoggingFilterRules(int level) -> QString;

enum class GFLogLevel : uint8_t {
  kDEBUG = 0,
  kINFO = 1,
  kWARNING = 2,
  kCRITICAL = 3,
  kFATAL = 4
};

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
   * @param capacity
   */
  explicit GFLogRingBuffer(int capacity = 1024)
      : capacity_(capacity), buffer_(capacity) {}

  /**
   * @brief
   *
   * @param entry
   */
  void Push(GFLogEntry entry);

  /**
   * @brief
   *
   * @return QVector<GFLogEntry>
   */
  auto Snapshot() const -> QVector<GFLogEntry>;

  /**
   * @brief
   *
   * @return int
   */
  auto Size() const -> int;

  /**
   * @brief
   *
   * @return int
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
   * @brief
   *
   * @return GFLogManager&
   */
  static auto Instance() -> GFLogManager&;

  /**
   * @brief
   *
   * @param capacity
   */
  void InitRingBuffer(int capacity);

  /**
   * @brief
   *
   * @param entry
   */
  void Push(const GFLogEntry& entry);

  /**
   * @brief
   *
   * @return QVector<GFLogEntry>
   */
  auto Snapshot() const -> QVector<GFLogEntry>;

 private:
  GFLogManager() = default;

  mutable QMutex mutex_;
  std::unique_ptr<GFLogRingBuffer> ring_buffer_;
};

}  // namespace GpgFrontend
