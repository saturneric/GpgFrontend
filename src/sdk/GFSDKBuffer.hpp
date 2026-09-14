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

/**
 * @file GFSDKBuffer.hpp
 * @brief Optional C++ conveniences over the C buffer handle.
 *
 * Separate from GFSDKBuffer.h on purpose: that header is the ABI and must
 * stay compilable as C. This one is C++-only and entirely optional -- nothing
 * in the SDK requires it -- but using it is what makes the ~154 DUP/UDUP call
 * sites collapse into ordinary scoped objects.
 *
 * It lives in the SDK rather than in modules/include/ so that out-of-tree
 * modules get the safe ergonomics from the SDK itself instead of copying a
 * helper out of the modules repository.
 */

#include <QByteArray>
#include <utility>

#include "GFSDKBuffer.h"

/**
 * @brief Move-only owner of a GFBufferRef.
 *
 * A handle has exactly one owner by construction, so a copy would mean two
 * GFBufferRelease calls on one handle. Deleting the copy makes that a compile
 * error instead of a double free.
 */
class GFBuf {
 public:
  GFBuf() = default;

  /// Adopt a handle that was transferred to us (a *New* or *Take* result).
  explicit GFBuf(GFBufferRef ref) : ref_(ref) {}

  /// Copy bytes out of a Qt container into wipeable storage.
  [[nodiscard]] static auto Copy(const QByteArray& bytes) -> GFBuf {
    return GFBuf(GFBufferNewFromBytes(bytes.constData(),
                                      static_cast<size_t>(bytes.size())));
  }

  ~GFBuf() { Reset(); }

  GFBuf(const GFBuf&) = delete;
  auto operator=(const GFBuf&) -> GFBuf& = delete;

  GFBuf(GFBuf&& other) noexcept : ref_(std::exchange(other.ref_, nullptr)) {}

  auto operator=(GFBuf&& other) noexcept -> GFBuf& {
    if (this != &other) {
      Reset();
      ref_ = std::exchange(other.ref_, nullptr);
    }
    return *this;
  }

  /**
   * @brief Borrow, to pass INTO an SDK call.
   *
   * Returns the VIEW type deliberately. There is no accessor on this class
   * that yields a GFBufferRef, so a caller cannot accidentally hand an owning
   * handle to something that borrows, nor pass this to GFBufferRelease.
   */
  [[nodiscard]] auto View() const -> GFBufferView { return ref_; }

  /// Receive an out-parameter. Releases anything already held first.
  auto Out() -> GFBufferRef* {
    Reset();
    return &ref_;
  }

  /// Relinquish ownership to the caller, who must then release it.
  [[nodiscard]] auto Take() -> GFBufferRef {
    return std::exchange(ref_, nullptr);
  }

  [[nodiscard]] auto Data() const -> const char* {
    return static_cast<const char*>(GFBufferData(ref_));
  }

  [[nodiscard]] auto Size() const -> size_t { return GFBufferSize(ref_); }
  [[nodiscard]] auto Empty() const -> bool { return Size() == 0; }
  [[nodiscard]] explicit operator bool() const { return ref_ != nullptr; }

  /// Erase the contents now, before this object goes out of scope.
  void Zeroize() { GFBufferZeroize(ref_); }

  /**
   * @brief A QByteArray copy of the payload.
   *
   * Named to say what it costs. Qt containers are implicitly shared and their
   * mutating methods detach, so a copy made here CANNOT be erased afterwards:
   * this is the one place a payload escapes wipeable memory. For anything the
   * user would call a secret, keep it in the GFBuf, or use a move-only secret
   * holder, rather than reaching for this.
   */
  [[nodiscard]] auto UnwipeableCopy() const -> QByteArray {
    const auto* data = Data();
    if (data == nullptr) return {};
    return {data, static_cast<qsizetype>(Size())};
  }

  void Reset() {
    if (ref_ != nullptr) {
      GFBufferRelease(ref_);
      ref_ = nullptr;
    }
  }

 private:
  GFBufferRef ref_ = nullptr;
};
