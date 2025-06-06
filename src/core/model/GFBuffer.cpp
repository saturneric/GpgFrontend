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

#include <openssl/crypto.h>

namespace GpgFrontend {

struct GFBuffer::Impl {
  void* sec_ptr_ = nullptr;
  size_t sec_size_ = 0;

  explicit Impl() = default;

  explicit Impl(size_t size) {
    sec_ptr_ = SMASecMalloc(size);
    sec_size_ = size;
  }

  ~Impl() {
    if (sec_ptr_ != nullptr) {
      SMASecFree(sec_ptr_);
      sec_ptr_ = nullptr;
    }
    sec_size_ = 0;
  }

  Impl(const Impl&) = delete;

  auto operator=(const Impl&) -> Impl& = delete;

  Impl(Impl&& other) noexcept
      : sec_ptr_(other.sec_ptr_), sec_size_(other.sec_size_) {
    other.sec_ptr_ = nullptr;
    other.sec_size_ = 0;
  }

  auto operator=(Impl&& other) noexcept -> Impl& {
    if (this != &other) {
      if (sec_ptr_ != nullptr) SMASecFree(sec_ptr_);
      sec_ptr_ = other.sec_ptr_;
      sec_size_ = other.sec_size_;
      other.sec_ptr_ = nullptr;
      other.sec_size_ = 0;
    }
    return *this;
  }
};

GFBuffer::GFBuffer() : impl_(SecureCreateSharedObject<Impl>()) {}

GFBuffer::GFBuffer(size_t size) : impl_(SecureCreateSharedObject<Impl>(size)) {}

GFBuffer::~GFBuffer() = default;

GFBuffer::GFBuffer(const QByteArray& buffer)
    : impl_(SecureCreateSharedObject<Impl>(buffer.size())) {
  std::memcpy(impl_->sec_ptr_, buffer.constData(), impl_->sec_size_);
}

GFBuffer::GFBuffer(const QString& str) {
  auto b = str.toUtf8();
  impl_ = SecureCreateSharedObject<Impl>(b.size());
  std::memcpy(impl_->sec_ptr_, b.constData(), impl_->sec_size_);
}

auto GFBuffer::operator==(const GFBuffer& o) const -> bool {
  return Size() == o.Size() &&
         (Size() == 0 || std::memcmp(Data(), o.Data(), Size()) == 0);
}

auto GFBuffer::Data() -> char* { return static_cast<char*>(impl_->sec_ptr_); }

auto GFBuffer::Data() const -> const char* {
  return static_cast<const char*>(impl_->sec_ptr_);
}

void GFBuffer::Resize(ssize_t size) {
  impl_->sec_ptr_ = SMASecRealloc(impl_->sec_ptr_, size);
  impl_->sec_size_ = size;
}

auto GFBuffer::Size() const -> size_t { return impl_->sec_size_; }

auto GFBuffer::ConvertToQByteArray() const -> QByteArray {
  Q_ASSERT(impl_);
  Q_ASSERT(impl_->sec_ptr_ != nullptr);
  return QByteArray{static_cast<const char*>(impl_->sec_ptr_),
                    static_cast<qsizetype>(impl_->sec_size_)};
}

auto GFBuffer::Empty() const -> bool { return this->Size() == 0; }

void GFBuffer::Append(const GFBuffer& o) {
  if (o.Empty()) return;

  if (&o == this) {
    const GFBuffer& copy(o);
    Append(copy);
    return;
  }

  const auto old_size = impl_->sec_size_;
  Resize(static_cast<ssize_t>(impl_->sec_size_ + o.impl_->sec_size_));
  memcpy(static_cast<char*>(impl_->sec_ptr_) + old_size, o.impl_->sec_ptr_,
         o.impl_->sec_size_);
}

void GFBuffer::Append(const char* buffer, ssize_t size) {
  if (size == 0) return;

  const auto old_size = impl_->sec_size_;
  Resize(static_cast<ssize_t>(impl_->sec_size_ + size));
  memcpy(static_cast<char*>(impl_->sec_ptr_) + old_size, buffer, size);
}

auto GFBuffer::Left(ssize_t len) const -> GFBuffer {
  if (len <= 0) return {};

  len = std::min(len, static_cast<ssize_t>(impl_->sec_size_));
  auto ret = GFBuffer(len);
  memcpy(ret.impl_->sec_ptr_, impl_->sec_ptr_, len);
  return ret;
}

auto GFBuffer::Mid(ssize_t pos, ssize_t len) const -> GFBuffer {
  if (pos < 0 || len <= 0 || pos >= impl_->sec_size_) return {};

  len = std::min(len, static_cast<ssize_t>(impl_->sec_size_ - pos));
  auto ret = GFBuffer(len);
  memcpy(ret.impl_->sec_ptr_,
         reinterpret_cast<void*>(static_cast<char*>(impl_->sec_ptr_) + pos),
         len);
  return ret;
}

auto GFBuffer::Right(ssize_t len) const -> GFBuffer {
  if (len <= 0) return {};

  len = std::min(len, static_cast<ssize_t>(impl_->sec_size_));
  auto ret = GFBuffer(len);
  memcpy(ret.impl_->sec_ptr_,
         reinterpret_cast<void*>(static_cast<char*>(impl_->sec_ptr_) +
                                 impl_->sec_size_ - len),
         len);
  return ret;
}

void GFBuffer::Zeroize() {
  if (impl_ && (impl_->sec_ptr_ != nullptr) && impl_->sec_size_ > 0) {
    OPENSSL_cleanse(impl_->sec_ptr_, impl_->sec_size_);
  }
}
}  // namespace GpgFrontend