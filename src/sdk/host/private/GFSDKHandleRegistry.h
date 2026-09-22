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

#include "core/utils/CommonUtils.h"

/**
 * @file GFSDKHandleRegistry.h
 * @brief The live-handle registry shared by every opaque SDK handle type.
 *
 * WHY A REGISTRY AT ALL: an opaque handle is a pointer, and the one question
 * every entry point must answer first is "is this pointer still mine?". The
 * tempting answer -- stash a magic word in the struct and read it -- is
 * itself undefined behaviour on a released handle: it dereferences freed
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

  /// Is @p handle one we issued and have not reclaimed? Decided WITHOUT
  /// dereferencing it.
  [[nodiscard]] auto IsLive(const T* handle) -> bool {
    if (handle == nullptr) return false;
    QMutexLocker locker(&mutex_);
    return live_.contains(const_cast<T*>(handle));
  }

  /// Remove @p handle, reporting whether it was ours to remove.
  auto Take(T* handle) -> bool {
    if (handle == nullptr) return false;
    QMutexLocker locker(&mutex_);
    return live_.remove(handle) > 0;
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
  QMutex mutex_;
  QHash<T*, GFHandleOrigin> live_;
};
