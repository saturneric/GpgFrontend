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

#include "GFBuffer.h"

namespace GpgFrontend {

GFBuffer::GFBuffer() = default;

GFBuffer::GFBuffer(size_t size) : buffer_(static_cast<qsizetype>(size), 0) {}

GFBuffer::GFBuffer(QByteArray buffer) : buffer_(std::move(buffer)) {}

GFBuffer::GFBuffer(const QString& str) : buffer_(str.toUtf8()) {}

auto GFBuffer::operator==(const GFBuffer& o) const -> bool {
  return buffer_ == o.buffer_;
}

auto GFBuffer::Data() -> char* { return buffer_.data(); }

auto GFBuffer::Data() const -> const char* { return buffer_.constData(); }

void GFBuffer::Resize(ssize_t size) { buffer_.resize(size); }

auto GFBuffer::Size() const -> size_t { return buffer_.size(); }

auto GFBuffer::ConvertToQByteArray() const -> QByteArray { return buffer_; }

auto GFBuffer::Empty() const -> bool { return this->Size() == 0; }

void GFBuffer::Append(const GFBuffer& o) { buffer_.append(o.buffer_); }

void GFBuffer::Append(const char* buffer, ssize_t size) {
  buffer_.append(buffer, size);
}
}  // namespace GpgFrontend