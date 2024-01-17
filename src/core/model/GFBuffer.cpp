/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

GFBuffer::GFBuffer()
    : buffer_(SecureCreateSharedObject<std::vector<std::byte>>()) {}

GFBuffer::GFBuffer(QByteArray buffer)
    : buffer_(SecureCreateSharedObject<std::vector<std::byte>>()) {
  std::transform(buffer.begin(), buffer.end(), std::back_inserter(*buffer_),
                 [](const char c) { return static_cast<std::byte>(c); });
}

GFBuffer::GFBuffer(QString str)
    : buffer_(SecureCreateSharedObject<std::vector<std::byte>>()) {
  std::transform(
      str.begin(), str.end(), std::back_inserter(*buffer_),
      [](const QChar c) { return static_cast<std::byte>(c.unicode()); });
}

auto GFBuffer::operator==(const GFBuffer& o) const -> bool {
  return equal(buffer_->begin(), buffer_->end(), o.buffer_->begin());
}

auto GFBuffer::Data() const -> std::byte* { return buffer_->data(); }

void GFBuffer::Resize(size_t size) { buffer_->resize(size); }

auto GFBuffer::Size() const -> size_t { return buffer_->size(); }

auto GFBuffer::ConvertToQByteArray() const -> QByteArray {
  return QByteArray::fromRawData(reinterpret_cast<const char*>(Data()),
                                 static_cast<qsizetype>(Size()));
}

auto GFBuffer::Empty() const -> bool { return this->Size() == 0; }

}  // namespace GpgFrontend