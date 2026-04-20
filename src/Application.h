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

#include "GpgFrontendContext.h"

namespace GpgFrontend {

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
  explicit GFLogRingBuffer(int capacity = 1024)
      : capacity_(capacity), buffer_(capacity) {}

  void Push(GFLogEntry entry);

  auto Snapshot() const -> QVector<GFLogEntry>;

  auto Size() const -> int;

  auto Capacity() const -> int;

 private:
  int capacity_ = 0;
  mutable QMutex mutex_;
  QVector<GFLogEntry> buffer_;
  int write_index_ = 0;
  int size_ = 0;
  bool full_ = false;
};

class GFLogManager {
 public:
  static auto Instance() -> GFLogManager&;

  void InitRingBuffer(int capacity);

  void Push(const GFLogEntry& entry);

  auto Snapshot() const -> QVector<GFLogEntry>;

 private:
  GFLogManager() = default;

  mutable QMutex mutex_;
  std::unique_ptr<GFLogRingBuffer> ring_buffer_;
};

/**
 * @brief
 *
 * @param type
 * @param context
 * @param msg
 */
void GFMessageHandler(QtMsgType type, const QMessageLogContext& context,
                      const QString& msg);

/**
 * @brief
 *
 * @param level
 * @return QString
 */
auto BuildQtLoggingFilterRules(int level) -> QString;

/**
 * @brief
 *
 * @param p_ctx
 * @return int
 */
auto StartApplication(const GFCxtWPtr& p_ctx) -> int;

}  // namespace GpgFrontend
