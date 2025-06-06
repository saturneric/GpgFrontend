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

#include <optional>

#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

class GF_CORE_EXPORT GFBuffer {
 public:
  GFBuffer();

  explicit GFBuffer(size_t size);

  explicit GFBuffer(const QByteArray& buffer);

  explicit GFBuffer(const QString& str);

  explicit GFBuffer(const char* str);

  ~GFBuffer();

  auto operator==(const GFBuffer& o) const -> bool;

  auto operator==(const char* str) const -> bool;

  auto operator!=(const char* str) const -> bool;

  [[nodiscard]] auto Data() -> char*;

  [[nodiscard]] auto Data() const -> const char*;

  void Resize(ssize_t size);

  [[nodiscard]] auto Size() const -> size_t;

  [[nodiscard]] auto Empty() const -> bool;

  void Append(const GFBuffer&);

  void Append(const char*, ssize_t);

  [[nodiscard]] auto ConvertToQByteArray() const -> QByteArray;

  [[nodiscard]] auto ConvertToQString() const -> QString;

  [[nodiscard]] auto Mid(ssize_t pos, ssize_t len) const -> GFBuffer;

  [[nodiscard]] auto Left(ssize_t len) const -> GFBuffer;

  [[nodiscard]] auto Right(ssize_t len) const -> GFBuffer;

  void Zeroize();

 private:
  struct Impl;
  QSharedPointer<Impl> impl_;
};

using GFBufferOrNone = std::optional<GFBuffer>;

}  // namespace GpgFrontend