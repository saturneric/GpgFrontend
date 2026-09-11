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

#include "SecureMemoryAllocator.h"

#include <sodium.h>

#include <QCoreApplication>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <optional>

#include "core/utils/CommonUtils.h"

namespace GpgFrontend {
class SecureMemoryAllocator;
}

namespace {

QMutex instance_mutex;
GpgFrontend::SecureMemoryAllocator* instance = nullptr;

struct AllocationInfo {
  size_t size = 0;
  // Allocated with sodium_malloc, so it has to go back through sodium_free.
  bool guarded = false;
  // Must be zeroed before the memory is released. Kept separate from
  // `guarded` because sodium_free already zeroes what it frees, while an
  // ordinary block holding a secret has to be wiped by hand.
  bool wipe = false;
};

auto NormalAllocate(size_t size) -> void* {
  if (size == 0) return nullptr;

  auto* ptr = std::calloc(1, size);
  if (ptr == nullptr) {
    FLOG_F("calloc failed");
  }

  return ptr;
}

void NormalDeallocate(void* ptr, size_t size, bool wipe) {
  if (ptr == nullptr) return;

  if (wipe && size > 0) {
    sodium_memzero(ptr, size);
  }

  std::free(ptr);
}

}  // namespace

namespace GpgFrontend {

class SecureMemoryAllocator {
 public:
  static auto GetInstance() -> SecureMemoryAllocator*;

  SecureMemoryAllocator(const SecureMemoryAllocator&) = delete;
  auto operator=(const SecureMemoryAllocator&)
      -> SecureMemoryAllocator& = delete;

  auto Allocate(size_t size) -> void*;
  auto Reallocate(void* ptr, size_t size) -> void*;
  void Deallocate(void* ptr);

  auto SecAllocate(size_t size) -> void*;
  auto SecReallocate(void* ptr, size_t size) -> void*;
  void SecDeallocate(void* ptr);

 private:
  explicit SecureMemoryAllocator(int secure_level);

  ~SecureMemoryAllocator();

  auto reg_mem(void* ptr, size_t size, bool guarded, bool wipe) -> void;
  auto take_info(void* ptr) -> std::optional<AllocationInfo>;
  auto take_info_if_tracked(void* ptr) -> std::optional<AllocationInfo>;
  static void report_invalid_free(void* ptr);

 private:
  int secure_level_ = 0;
  QMutex mutex_;
  QHash<void*, AllocationInfo> allocated_;
};

SecureMemoryAllocator::SecureMemoryAllocator(int secure_level)
    : secure_level_(secure_level) {
  if (!EnsureSodiumInit()) {
    FLOG_F("sodium_init failed");
  }
}

SecureMemoryAllocator::~SecureMemoryAllocator() = default;

auto SecureMemoryAllocator::reg_mem(void* ptr, size_t size, bool guarded,
                                    bool wipe) -> void {
  if (ptr == nullptr) return;

  QMutexLocker locker(&mutex_);
  Q_ASSERT(!allocated_.contains(ptr));

  allocated_.insert(ptr, AllocationInfo{size, guarded, wipe});
}

auto SecureMemoryAllocator::take_info_if_tracked(void* ptr)
    -> std::optional<AllocationInfo> {
  if (ptr == nullptr) return {};

  QMutexLocker locker(&mutex_);

  auto it = allocated_.find(ptr);
  if (it == allocated_.end()) return {};

  auto info = it.value();
  allocated_.erase(it);
  return info;
}

void SecureMemoryAllocator::report_invalid_free(void* ptr) {
  FLOG_W() << "this memory address was not allocated by "
              "SecureMemoryAllocator:"
           << ptr;
#ifdef DEBUG
  // a double free or a foreign pointer. release builds only warn and leak,
  // debug builds fail fast so the offending call site is caught.
  qFatal("SecureMemoryAllocator: invalid free of %p", ptr);
#endif
}

auto SecureMemoryAllocator::take_info(void* ptr)
    -> std::optional<AllocationInfo> {
  if (ptr == nullptr) return {};

  auto info = take_info_if_tracked(ptr);
  if (!info) report_invalid_free(ptr);
  return info;
}

auto SecureMemoryAllocator::Allocate(size_t size) -> void* {
  if (size == 0) return nullptr;

  if (secure_level_ < 1) {
    return std::malloc(size);
  }

  auto* ptr = NormalAllocate(size);
  reg_mem(ptr, size, false, true);

  return ptr;
}

auto SecureMemoryAllocator::Reallocate(void* ptr, size_t size) -> void* {
  if (ptr == nullptr) return Allocate(size);

  if (size == 0) {
    Deallocate(ptr);
    return nullptr;
  }

  // The secure tier registers its allocations at every level, so a pointer can
  // be tracked even while the normal tier is running untracked. Look before
  // reaching for realloc: a tracked block may hold a secret, and realloc would
  // copy it into a new block and hand the old one back to the heap intact.
  auto info = take_info_if_tracked(ptr);
  if (!info) {
    if (secure_level_ < 1) return std::realloc(ptr, size);

    report_invalid_free(ptr);
    return nullptr;
  }

  if (info->guarded) {
    FLOG_W()
        << "SMARealloc called for secure memory; using secure reallocation";

    auto* new_ptr = SecAllocate(size);
    if (new_ptr == nullptr) {
      reg_mem(ptr, info->size, info->guarded, info->wipe);
      return nullptr;
    }

    std::memcpy(new_ptr, ptr, std::min(size, info->size));
    sodium_free(ptr);
    return new_ptr;
  }

  auto* new_ptr = NormalAllocate(size);
  if (new_ptr == nullptr) {
    reg_mem(ptr, info->size, info->guarded, info->wipe);
    return nullptr;
  }

  std::memcpy(new_ptr, ptr, std::min(size, info->size));
  NormalDeallocate(ptr, info->size, info->wipe);

  reg_mem(new_ptr, size, false, info->wipe);
  return new_ptr;
}

void SecureMemoryAllocator::Deallocate(void* ptr) {
  if (ptr == nullptr) return;

  // Tracked first, for the same reason as Reallocate: a secret freed through
  // SMAFree must still be wiped, and must still leave the registry.
  auto info = take_info_if_tracked(ptr);
  if (!info) {
    if (secure_level_ < 1) {
      std::free(ptr);
      return;
    }

    report_invalid_free(ptr);
    return;
  }

  if (info->guarded) {
    FLOG_W() << "SMAFree called for secure memory; using sodium_free";
    sodium_free(ptr);
    return;
  }

  NormalDeallocate(ptr, info->size, info->wipe);
}

auto SecureMemoryAllocator::SecAllocate(size_t size) -> void* {
  if (size == 0) return nullptr;

  if (secure_level_ < 2) {
    // Registered even when the normal tier is running untracked. This tier is
    // the one GFBuffer and the other secret holders allocate from, so
    // SecDeallocate has to know how many bytes to wipe whatever the level is.
    auto* ptr = NormalAllocate(size);
    reg_mem(ptr, size, false, true);
    return ptr;
  }

  auto* ptr = sodium_malloc(size);
  if (ptr == nullptr) {
    FLOG_F("sodium_malloc failed");
    return nullptr;
  }

  std::memset(ptr, 0, size);
  // No wipe flag: sodium_free zeroes the guarded pages it releases.
  reg_mem(ptr, size, true, false);

  return ptr;
}

auto SecureMemoryAllocator::SecReallocate(void* ptr, size_t size) -> void* {
  if (ptr == nullptr) return SecAllocate(size);

  if (size == 0) {
    SecDeallocate(ptr);
    return nullptr;
  }

  auto info = take_info_if_tracked(ptr);
  if (!info) {
    report_invalid_free(ptr);
    return nullptr;
  }

  auto* new_ptr = SecAllocate(size);
  if (new_ptr == nullptr) {
    reg_mem(ptr, info->size, info->guarded, info->wipe);
    return nullptr;
  }

  std::memcpy(new_ptr, ptr, std::min(size, info->size));

  // Deliberately never plain realloc: growing a secret in place would copy it
  // into a new block and release the old one with the contents still in it.
  if (info->guarded) {
    sodium_free(ptr);
  } else {
    NormalDeallocate(ptr, info->size, info->wipe);
  }

  return new_ptr;
}

void SecureMemoryAllocator::SecDeallocate(void* ptr) {
  if (ptr == nullptr) return;

  auto info = take_info_if_tracked(ptr);
  if (!info) {
    report_invalid_free(ptr);
    return;
  }

  if (info->guarded) {
    sodium_free(ptr);
    return;
  }

  // Always wiped, at every secure level. Releasing this tier intact would
  // defeat the only reason it exists, so the guarantee does not depend on a
  // setting the user has no reason to have changed.
  NormalDeallocate(ptr, info->size, true);
}

auto SecureMemoryAllocator::GetInstance() -> SecureMemoryAllocator* {
  QMutexLocker locker(&instance_mutex);

  if (instance == nullptr) {
    const auto secure_level = SecureLevelFromApp();

    auto* addr = std::malloc(sizeof(SecureMemoryAllocator));
    Q_ASSERT(addr != nullptr);

    if (addr == nullptr) {
      FLOG_F("malloc SecureMemoryAllocator failed");
      return nullptr;
    }

    instance = new (addr) SecureMemoryAllocator(secure_level);
  }

  return instance;
}

auto SMAMalloc(size_t size) -> void* {
  return SecureMemoryAllocator::GetInstance()->Allocate(size);
}

auto SMARealloc(void* ptr, size_t size) -> void* {
  return SecureMemoryAllocator::GetInstance()->Reallocate(ptr, size);
}

void SMAFree(void* ptr) {
  SecureMemoryAllocator::GetInstance()->Deallocate(ptr);
}

auto SMASecMalloc(size_t size) -> void* {
  return SecureMemoryAllocator::GetInstance()->SecAllocate(size);
}

auto SMASecRealloc(void* ptr, size_t size) -> void* {
  return SecureMemoryAllocator::GetInstance()->SecReallocate(ptr, size);
}

void SMASecFree(void* ptr) {
  SecureMemoryAllocator::GetInstance()->SecDeallocate(ptr);
}

}  // namespace GpgFrontend