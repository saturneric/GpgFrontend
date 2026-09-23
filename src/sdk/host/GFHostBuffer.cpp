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

#include <cstdint>
#include <new>

#include "GFHostImpl.h"
#include "core/model/GFBuffer.h"
#include "core/utils/MemoryUtils.h"
#include "private/GFHostTransfer.h"
#include "private/GFSDKHandleRegistry.h"
#include "private/GFSDKHandleSweep.h"
#include "private/GFSDKPrivate.h"

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

namespace gf_host {

namespace {

/// Wipe and free one buffer handle. The single place that knows how, so
/// release and the unload sweep cannot diverge about what destruction means.
void DestroyBuffer(GFBufferImpl* impl) {
  impl->magic = 0;        // scrub, so corruption is distinguishable from reuse
  impl->~GFBufferImpl();  // wipes the payload: GFBuffer frees via SMASecFree
  GpgFrontend::SMAFree(impl);
}

void ReportStaleHandle(const void* handle, const char* what) {
  LOG_W().nospace()
      << what
      << ": stale handle (already released, or never issued by the host): "
      << handle;
#ifdef DEBUG
  qFatal("%s: stale handle %p", what, handle);
#endif
}

/// Validate then dereference, never the other way around.
auto ResolveLive(GFBufferView buf, const char* what) -> const GFBufferImpl* {
  if (buf == nullptr) return nullptr;
  const auto state =
      GFHandleRegistry<GFBufferImpl>::Instance().Check(buf, what);
  if (state != GFHandleState::kLive) {
    if (state == GFHandleState::kStale) ReportStaleHandle(buf, what);
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
    GFHandleRegistry<GFBufferImpl>::Instance().Register(
        impl, gf_sdk_internal::CurrentModuleId(), "GFBufferNewFromBytes");
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
  const auto state =
      GFHandleRegistry<GFBufferImpl>::Instance().Take(buf, "buffer.release");
  if (state != GFHandleState::kLive) {
    if (state == GFHandleState::kStale) {
      ReportStaleHandle(buf, "buffer.release");
    }
    return;
  }

  DestroyBuffer(buf);
}

}  // namespace gf_host

// The file-local helpers above are inside gf_host; these definitions are
// not, because their declarations are at global scope.
using namespace gf_host;  // NOLINT(build/namespaces)

namespace gf_sdk_internal {

auto TakeBufferForTransfer(GFBufferRef buf, const char* what)
    -> std::optional<GpgFrontend::GFBuffer> {
  if (buf == nullptr) return std::nullopt;
  const auto state = GFHandleRegistry<GFBufferImpl>::Instance().Take(buf, what);
  if (state != GFHandleState::kLive) {
    if (state == GFHandleState::kStale) ReportStaleHandle(buf, what);
    return std::nullopt;
  }
  auto buffer = buf->buf;  // shares the storage; nothing is copied
  DestroyBuffer(buf);      // drops this handle's share only
  return buffer;
}

auto NewBufferFor(GpgFrontend::GFBuffer buffer, const QString& module_id,
                  const char* origin) -> GFBufferRef {
  try {
    auto* mem = GpgFrontend::SMAMalloc(sizeof(GFBufferImpl));
    if (mem == nullptr) return nullptr;
    auto* impl = new (mem) GFBufferImpl(std::move(buffer));
    GFHandleRegistry<GFBufferImpl>::Instance().Register(impl, module_id,
                                                        origin);
    return impl;
  } catch (...) {
    LOG_E() << origin << ": allocation failed";
    return nullptr;
  }
}

auto ReadBuffer(GFBufferView buf, const char* what)
    -> std::optional<GpgFrontend::GFBuffer> {
  const auto* impl = ResolveLive(buf, what);
  if (impl == nullptr) return std::nullopt;
  return impl->buf;
}

auto SweepBufferHandles(const QString& module_id) -> QList<const char*> {
  QList<const char*> origins;
  for (const auto& entry :
       GFHandleRegistry<GFBufferImpl>::Instance().TakeAllFor(module_id)) {
    origins.append(entry.second);
    DestroyBuffer(entry.first);
  }
  return origins;
}

}  // namespace gf_sdk_internal

namespace gf_host {

auto GFBufferOutstandingCount(const char* module_id) -> size_t {
  return GFHandleRegistry<GFBufferImpl>::Instance().Count(
      module_id == nullptr ? QString() : QString::fromUtf8(module_id));
}

}  // namespace gf_host