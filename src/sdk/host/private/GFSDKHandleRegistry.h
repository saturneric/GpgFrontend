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

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QString>

#include "GFSDKHandleSweep.h"
#include "GFSDKPrivate.h"
#include "core/utils/CommonUtils.h"

/**
 * @file GFSDKHandleRegistry.h
 * @brief The live-handle registry shared by every opaque SDK handle type.
 *
 * WHY A REGISTRY AT ALL: an opaque handle is a pointer, and the one question
 * every entry point must answer first is "is this pointer still mine?". The
 * tempting answer -- stash a magic word in the struct and read it -- is
 * itself undefined behavior on a released handle: it dereferences freed
 * memory, is exactly what ASan traps, and can read anything once the
 * allocator reuses the block. So validity is decided by looking the POINTER
 * VALUE up in this table, without ever touching the pointed-to memory. A
 * magic word may still live in the struct, but only as a corruption check
 * applied AFTER the handle is known live.
 *
 * This mirrors SecureMemoryAllocator::take_info_if_tracked +
 * report_invalid_free, which already decide validity from a registry rather
 * than from the block's contents, and warn in release / qFatal in debug.
 *
 * WHAT THIS DOES NOT DO: make concurrent release safe. The mutex protects
 * this hash, nothing more. Once Lookup returns and the lock is dropped,
 * another thread may release the same handle and the pointer dangles.
 * Borrowed-handle lifetime is an ownership rule, not a locking one: a handle
 * has a single owner, and views borrowed from it are valid only within that
 * owner's lifetime on the owning thread. ASan/UBSan remains the real
 * use-after-free backstop; this catches the cheap, decidable cases and turns
 * them into an attributable log line instead of heap corruption.
 */

/**
 * @brief Who asked for a handle, and what issued it.
 *
 * `origin` is the name of the SDK entry point that created it, which is a
 * string literal with static storage -- so recording it costs a pointer and
 * nothing is owned. It is not the module's own file and line: capturing that
 * would mean wrapping every creator in a macro that passes __FILE__ and
 * __LINE__, changing the public C surface of each one. What this gives
 * instead is "module X leaked two buffers from GFBufferNewFromBytes and one
 * result from GFGpgEncrypt", which narrows a leak to a call site's kind
 * rather than its line. ASan gives the exact line when that is not enough.
 */
struct GFHandleOrigin {
  QString module_id;
  const char* origin = nullptr;
};

/// What a handle lookup found.
enum class GFHandleState {
  kLive,     ///< issued, not reclaimed, and owned by the caller
  kStale,    ///< never issued, or already released or swept
  kForeign,  ///< live, but issued to a different module
};

/**
 * @brief The live-handle table for one handle type.
 *
 * @tparam T the handle's implementation struct
 */
template <typename T>
class GFHandleRegistry {
 public:
  static auto Instance() -> GFHandleRegistry& {
    static GFHandleRegistry instance;
    return instance;
  }

  /// Record a newly issued handle against the module that asked for it.
  void Register(T* impl, const QString& module_id, const char* origin) {
    if (impl == nullptr) return;
    QMutexLocker locker(&mutex_);
    live_.insert(impl, GFHandleOrigin{module_id, origin});
  }

  /**
   * @brief Is @p handle one we issued, have not reclaimed, and issued to the
   *        calling module? Decided WITHOUT dereferencing it.
   *
   * The caller is the module the current gate attributed this thread to. A
   * handle is private to the module that asked for it: one module holding
   * another's pointer does not make it that module's to read or release.
   * Host-internal calls (no attribution) and handles issued outside any
   * module are not owner-checked. A foreign handle is logged here.
   */
  [[nodiscard]] auto Check(const T* handle, const char* what) -> GFHandleState {
    if (handle == nullptr) return GFHandleState::kStale;
    QMutexLocker locker(&mutex_);
    const auto it = live_.constFind(const_cast<T*>(handle));
    if (it == live_.constEnd()) return GFHandleState::kStale;
    return OwnerStateLocked(it.value(), handle, what);
  }

  /**
   * @brief @p handle, when Check() calls it live; null otherwise.
   *
   * Validate first, dereference second -- never the other way round. A stale
   * handle is reported here: once, in one wording, for every handle type.
   */
  [[nodiscard]] auto ResolveLive(T* handle, const char* what) -> T* {
    if (handle == nullptr) return nullptr;
    const auto state = Check(handle, what);
    if (state == GFHandleState::kLive) return handle;
    if (state == GFHandleState::kStale) ReportStale(handle, what);
    return nullptr;
  }

  /// A handle that was never issued, or was already released or swept.
  static void ReportStale(const void* handle, const char* what) {
    LOG_W().nospace()
        << what
        << ": stale handle (already released, or never issued by the host): "
        << handle;
#ifdef DEBUG
    qFatal("%s: stale handle %p", what, handle);
#endif
  }

  /// Remove @p handle if Check() would call it live, reporting what it found.
  auto Take(T* handle, const char* what) -> GFHandleState {
    if (handle == nullptr) return GFHandleState::kStale;
    QMutexLocker locker(&mutex_);
    const auto it = live_.find(handle);
    if (it == live_.end()) return GFHandleState::kStale;
    const auto state = OwnerStateLocked(it.value(), handle, what);
    if (state == GFHandleState::kLive) live_.erase(it);
    return state;
  }

  /// Outstanding handles, for @p module_id or process-wide when it is null.
  [[nodiscard]] auto Count(const QString& module_id) -> size_t {
    QMutexLocker locker(&mutex_);
    if (module_id.isNull()) return static_cast<size_t>(live_.size());

    size_t n = 0;
    for (auto it = live_.constBegin(); it != live_.constEnd(); ++it) {
      if (it.value().module_id == module_id) ++n;
    }
    return n;
  }

  /**
   * @brief Remove and return every handle belonging to @p module_id.
   *
   * For the unload sweep, and only safe where the caller has already
   * established that no module code can run -- which is why it is called from
   * step 5 of teardown and from nowhere else. The handles come out of the
   * table before the caller destroys them, so a concurrent accessor sees them
   * as stale rather than following a pointer into a destructor.
   *
   * @param module_id the module whose handles to reclaim
   * @return the handles, which the caller now owns and must destroy
   */
  auto TakeAllFor(const QString& module_id) -> QList<QPair<T*, const char*>> {
    QList<QPair<T*, const char*>> taken;
    QMutexLocker locker(&mutex_);
    for (auto it = live_.begin(); it != live_.end();) {
      if (it.value().module_id != module_id) {
        ++it;
        continue;
      }
      taken.append({it.key(), it.value().origin});
      it = live_.erase(it);
    }
    return taken;
  }

 private:
  static auto OwnerStateLocked(const GFHandleOrigin& owner, const T* handle,
                               const char* what) -> GFHandleState {
    const auto caller = gf_sdk_internal::CurrentModuleId();
    if (caller.isEmpty() || owner.module_id.isEmpty() ||
        caller == owner.module_id) {
      return GFHandleState::kLive;
    }
    LOG_W().nospace() << what << ": module " << caller
                      << " presented a handle issued to module "
                      << owner.module_id
                      << "; refused: " << static_cast<const void*>(handle);
    return GFHandleState::kForeign;
  }

  QMutex mutex_;
  QHash<T*, GFHandleOrigin> live_;
};
