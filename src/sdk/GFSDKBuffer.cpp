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

#include "GFSDKBuffer.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <cstdint>
#include <new>

#include "core/model/GFBuffer.h"
#include "core/utils/MemoryUtils.h"
#include "private/GFSDKPrivat.h"

namespace {

constexpr uint32_t kGFBufferMagic = 0x47464246U;  // 'GFBF'

}  // namespace

/**
 * @brief What a GFBufferRef actually points at.
 *
 * Unlike the older SDK result structs -- which modules free with a raw
 * GFFreeMemory, so no destructor ever runs -- this one has a non-trivial
 * member. That is safe precisely because GFBufferRelease is the single exit
 * and calls the destructor explicitly; no module ever frees this directly.
 */
struct GFBufferImpl {
  uint32_t magic;
  GpgFrontend::GFBuffer buf;

  explicit GFBufferImpl(GpgFrontend::GFBuffer b)
      : magic(kGFBufferMagic), buf(std::move(b)) {}
};

namespace {

/// Where a handle was created, for attributing a leak at sweep time.
struct AllocationSite {
  QString module_id;
};

/**
 * @brief The live-handle registry: the authority on whether a handle is real.
 *
 * Validation is a lookup on the POINTER VALUE. It never dereferences the
 * candidate, because reading a magic word out of a released handle would
 * itself be undefined behaviour -- exactly what ASan traps, and able to read
 * anything at all once the allocator reuses the block. The magic word inside
 * the struct is only a secondary consistency check, applied after the handle
 * is already known live, and it catches corruption rather than use-after-free.
 *
 * This mirrors SecureMemoryAllocator::take_info_if_tracked +
 * report_invalid_free, which already decide validity from a registry rather
 * than from the block's contents.
 *
 * What this does NOT do: make concurrent release safe. The mutex protects the
 * hash, nothing more. Once Lookup returns and the lock is dropped, another
 * thread may release the same handle and the pointer dangles. Borrowed-view
 * lifetime is an ownership rule, not a locking one -- see GFSDKBuffer.h.
 * ASan/UBSan remains the real use-after-free backstop.
 */
class BufferRegistry {
 public:
  static auto Instance() -> BufferRegistry& {
    static BufferRegistry instance;
    return instance;
  }

  void Register(GFBufferImpl* impl, const QString& module_id) {
    QMutexLocker locker(&mutex_);
    live_.insert(impl, AllocationSite{module_id});
  }

  /// True when @p handle is a handle we issued and have not released.
  /// Decided without touching the pointed-to memory.
  [[nodiscard]] auto IsLive(const GFBufferImpl* handle) -> bool {
    if (handle == nullptr) return false;
    QMutexLocker locker(&mutex_);
    return live_.contains(const_cast<GFBufferImpl*>(handle));
  }

  /// Removes @p handle and reports whether it was ours to remove.
  auto Take(GFBufferImpl* handle) -> bool {
    if (handle == nullptr) return false;
    QMutexLocker locker(&mutex_);
    return live_.remove(handle) > 0;
  }

  [[nodiscard]] auto Count(const QString& module_id) -> size_t {
    QMutexLocker locker(&mutex_);
    if (module_id.isNull()) return static_cast<size_t>(live_.size());

    size_t n = 0;
    for (auto it = live_.constBegin(); it != live_.constEnd(); ++it) {
      if (it.value().module_id == module_id) ++n;
    }
    return n;
  }

 private:
  QMutex mutex_;
  QHash<GFBufferImpl*, AllocationSite> live_;
};

void ReportStaleHandle(const void* handle, const char* what) {
  LOG_W() << "GFSDKBuffer:" << what
          << "called on a handle that is not live (already released, or never "
             "issued by this SDK):"
          << handle;
#ifdef DEBUG
  qFatal("GFSDKBuffer: %s on stale handle %p", what, handle);
#endif
}

/// Validate then dereference, never the other way around.
auto ResolveLive(GFBufferView buf, const char* what) -> const GFBufferImpl* {
  if (buf == nullptr) return nullptr;
  if (!BufferRegistry::Instance().IsLive(buf)) {
    ReportStaleHandle(buf, what);
    return nullptr;
  }
  Q_ASSERT(buf->magic == kGFBufferMagic);
  return buf;
}

}  // namespace

auto GFBufferNewFromBytes(const void* data, size_t size) -> GFBufferRef {
  if (data == nullptr && size != 0) return nullptr;

  try {
    auto buffer =
        size == 0 ? GpgFrontend::GFBuffer()
                  : GpgFrontend::GFBuffer(static_cast<const char*>(data), size);

    auto* mem = GpgFrontend::SMAMalloc(sizeof(GFBufferImpl));
    if (mem == nullptr) return nullptr;

    auto* impl = new (mem) GFBufferImpl(std::move(buffer));
    BufferRegistry::Instance().Register(impl, QString());
    return impl;
  } catch (...) {
    // No C++ exception may cross the C ABI.
    LOG_E() << "GFBufferNewFromBytes: allocation failed";
    return nullptr;
  }
}

auto GFBufferData(GFBufferView buf) -> const void* {
  const auto* impl = ResolveLive(buf, "GFBufferData");
  if (impl == nullptr) return nullptr;
  return impl->buf.Data();
}

auto GFBufferSize(GFBufferView buf) -> size_t {
  const auto* impl = ResolveLive(buf, "GFBufferSize");
  if (impl == nullptr) return 0;
  return impl->buf.Size();
}

void GFBufferZeroize(GFBufferRef buf) {
  const auto* impl = ResolveLive(buf, "GFBufferZeroize");
  if (impl == nullptr) return;
  const_cast<GFBufferImpl*>(impl)->buf.Zeroize();
}

void GFBufferRelease(GFBufferRef buf) {
  if (buf == nullptr) return;

  // Registry first: a pointer we never issued, or already reclaimed, must not
  // be dereferenced at all.
  if (!BufferRegistry::Instance().Take(buf)) {
    ReportStaleHandle(buf, "GFBufferRelease");
    return;
  }

  buf->magic = 0;        // scrub, so corruption is distinguishable from reuse
  buf->~GFBufferImpl();  // wipes the payload: GFBuffer frees via SMASecFree
  GpgFrontend::SMAFree(buf);
}

auto GFBufferOutstandingCount(const char* module_id) -> size_t {
  return BufferRegistry::Instance().Count(
      module_id == nullptr ? QString() : QString::fromUtf8(module_id));
}
