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

#include <QByteArray>
#include <QString>
#include <utility>

#include "GFSDKBuffer.hpp"
#include "GFSDKGpgResult.h"

/**
 * @file GFSDKGpgResult.hpp
 * @brief Optional C++ conveniences over the opaque gpg result handle.
 *
 * Separate from GFSDKGpgResult.h on purpose: that header is the ABI and stays
 * compilable as C.
 */

/**
 * @brief Move-only owner of a GFGpgResultRef.
 *
 * The reason this is worth having: the old result structs had to be torn down
 * field by field, in order, on every path -- and the error paths are exactly
 * where that was got wrong, because a failed operation still allocated a
 * result. Here the destructor is the whole teardown, so an early `return` is
 * correct by construction.
 */
class GFGpgResult {
 public:
  GFGpgResult() = default;
  explicit GFGpgResult(GFGpgResultRef ref) : ref_(ref) {}
  ~GFGpgResult() { Reset(); }

  GFGpgResult(const GFGpgResult&) = delete;
  auto operator=(const GFGpgResult&) -> GFGpgResult& = delete;

  GFGpgResult(GFGpgResult&& o) noexcept
      : ref_(std::exchange(o.ref_, nullptr)) {}

  auto operator=(GFGpgResult&& o) noexcept -> GFGpgResult& {
    if (this != &o) {
      Reset();
      ref_ = std::exchange(o.ref_, nullptr);
    }
    return *this;
  }

  /// Receive the out-parameter of an operation. Releases anything held first.
  auto Out() -> GFGpgResultRef* {
    Reset();
    return &ref_;
  }

  [[nodiscard]] auto Status() const -> GFGpgResultStatus {
    return static_cast<GFGpgResultStatus>(GFGpgResultStatusOf(ref_));
  }
  [[nodiscard]] auto Ok() const -> bool { return Status() == GF_GPG_OK; }
  [[nodiscard]] explicit operator bool() const { return ref_ != nullptr; }

  [[nodiscard]] auto Error() const -> uint32_t {
    return GFGpgResultError(ref_);
  }

  /// Borrowed; dies with this object. Use TakeData() to outlive it.
  [[nodiscard]] auto Data() const -> GFBufferView {
    return GFGpgResultData(ref_);
  }

  /// The output bytes as a Qt container. See GFBuf::UnwipeableCopy on cost.
  [[nodiscard]] auto DataCopy() const -> QByteArray {
    auto view = Data();
    const auto* bytes = static_cast<const char*>(GFBufferData(view));
    if (bytes == nullptr) return {};
    return {bytes, static_cast<qsizetype>(GFBufferSize(view))};
  }

  /// Transfers the payload out, so it can outlive the result.
  [[nodiscard]] auto TakeData() -> GFBuf {
    return GFBuf(GFGpgResultTakeData(ref_));
  }

  [[nodiscard]] auto CapsuleId() const -> QString {
    return QString::fromUtf8(GFGpgResultCapsuleId(ref_));
  }
  [[nodiscard]] auto ErrorString() const -> QString {
    return QString::fromUtf8(GFGpgResultErrorString(ref_));
  }
  [[nodiscard]] auto HashAlgo() const -> QString {
    return QString::fromUtf8(GFGpgResultHashAlgo(ref_));
  }

  void Reset() {
    if (ref_ != nullptr) {
      GFGpgResultRelease(ref_);
      ref_ = nullptr;
    }
  }

 private:
  GFGpgResultRef ref_ = nullptr;
};
