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

#include <openssl/crypto.h>

#include <cstdlib>
#include <cstring>
#include <optional>

namespace GpgFrontend {
class SecureMemoryAllocator;
}

namespace {
QMutex instance_mutex;
GpgFrontend::SecureMemoryAllocator* instance = nullptr;
}  // namespace

namespace {

struct AllocationInfo {
  size_t size = 0;
  // Allocated with OPENSSL_secure_zalloc, so it has to go back through
  // OPENSSL_secure_clear_free.
  bool guarded = false;
  // Must be zeroed before the memory is released. Kept separate from
  // `guarded` because an ordinary block holding a secret has to be wiped too.
  bool wipe = false;
};

auto NormalAllocate(size_t size) -> void* {
  auto* ptr = OPENSSL_zalloc(size);
  if (ptr == nullptr) FLOG_F("OPENSSL_zalloc failed");
  return ptr;
}

void NormalDeallocate(void* ptr, size_t size, bool wipe) {
  if (wipe) {
    OPENSSL_clear_free(ptr, size);
  } else {
    OPENSSL_free(ptr);
  }
}

}  // namespace

namespace GpgFrontend {

class SecureMemoryAllocator {
 public:
  static auto GetInstance() -> SecureMemoryAllocator*;

  SecureMemoryAllocator(const SecureMemoryAllocator&) = delete;

  auto operator=(const SecureMemoryAllocator&)
      -> SecureMemoryAllocator& = delete;

  auto Allocate(size_t) -> void*;

  auto Reallocate(void*, size_t) -> void*;

  void Deallocate(void*);

  auto SecAllocate(size_t) -> void*;

  auto SecReallocate(void*, size_t) -> void*;

  void SecDeallocate(void*);

 private:
  int secure_level_;
  QMutex mutex_;
  QHash<void*, AllocationInfo> allocated_;

  explicit SecureMemoryAllocator(int secure_level);

  ~SecureMemoryAllocator();

  void reg_mem(void* ptr, size_t size, bool guarded, bool wipe);
  auto take_info_if_tracked(void* ptr) -> std::optional<AllocationInfo>;
  static void report_invalid_free(void* ptr);
};

SecureMemoryAllocator::SecureMemoryAllocator(int secure_level)
    : secure_level_(secure_level) {}

SecureMemoryAllocator::~SecureMemoryAllocator() = default;

void SecureMemoryAllocator::reg_mem(void* ptr, size_t size, bool guarded,
                                    bool wipe) {
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
  // a double free or a foreign pointer. release builds only warn and leak,
  // debug builds fail fast so the offending call site is caught.
  FLOG_W() << "this memory address was not allocated by SecureMemoryAllocator: "
           << ptr;
  Q_ASSERT_X(false, "SecureMemoryAllocator", "invalid free");
}

auto SecureMemoryAllocator::Allocate(size_t size) -> void* {
  // low secure level
  if (secure_level_ < 1) return malloc(size);

  // should not do allocate
  if (size == 0) return nullptr;

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
    if (secure_level_ < 1) return realloc(ptr, size);

    report_invalid_free(ptr);
    return nullptr;
  }

  if (info->guarded) {
    FLOG_W() << "SMARealloc called for secure memory; using secure reallocation";

    auto* new_ptr = SecAllocate(size);
    if (new_ptr == nullptr) {
      reg_mem(ptr, info->size, info->guarded, info->wipe);
      return nullptr;
    }

    std::memcpy(new_ptr, ptr, std::min(size, info->size));
    OPENSSL_secure_clear_free(ptr, info->size);
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
      free(ptr);
      return;
    }

    report_invalid_free(ptr);
    return;
  }

  if (info->guarded) {
    FLOG_W() << "SMAFree called for secure memory; using secure free";
    OPENSSL_secure_clear_free(ptr, info->size);
    return;
  }

  NormalDeallocate(ptr, info->size, info->wipe);
}

auto SecureMemoryAllocator::GetInstance() -> SecureMemoryAllocator* {
  QMutexLocker locker(&instance_mutex);

  if (instance == nullptr) {
    auto secure_level = qApp->property("GFSecureLevel").toInt();

    void* addr = nullptr;
    // low and middle secure level
    if (secure_level < 2) {
      addr = malloc(sizeof(SecureMemoryAllocator));
    } else {
      addr = OPENSSL_secure_zalloc(sizeof(SecureMemoryAllocator));
    }
    Q_ASSERT(addr != nullptr);

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

  auto* ptr = OPENSSL_secure_zalloc(size);
  if (ptr == nullptr) FLOG_F("OPENSSL_secure_zalloc failed");

  // OPENSSL_secure_clear_free zeroes the block it releases.
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
    OPENSSL_secure_clear_free(ptr, info->size);
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
    OPENSSL_secure_clear_free(ptr, info->size);
    return;
  }

  NormalDeallocate(ptr, info->size, info->wipe);
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