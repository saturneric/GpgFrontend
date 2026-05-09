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

namespace GpgFrontend {
class SecureMemoryAllocator;
}

namespace {

QMutex instance_mutex;
GpgFrontend::SecureMemoryAllocator* instance = nullptr;

auto EnsureSodiumInit() -> bool {
  static const int kRc = sodium_init();
  return kRc >= 0;
}

struct AllocationInfo {
  size_t size = 0;
  bool secure = false;
};

auto SecureLevelFromApp() -> int {
  if (qApp == nullptr) return 0;
  return qApp->property("GFSecureLevel").toInt();
}

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

  auto reg_mem(void* ptr, size_t size, bool secure) -> void;
  auto take_info(void* ptr) -> std::optional<AllocationInfo>;

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

auto SecureMemoryAllocator::reg_mem(void* ptr, size_t size, bool secure)
    -> void {
  if (ptr == nullptr) return;

  QMutexLocker locker(&mutex_);
  Q_ASSERT(!allocated_.contains(ptr));

  allocated_.insert(ptr, AllocationInfo{size, secure});
}

auto SecureMemoryAllocator::take_info(void* ptr)
    -> std::optional<AllocationInfo> {
  if (ptr == nullptr) return {};

  QMutexLocker locker(&mutex_);

  if (!allocated_.contains(ptr)) {
    FLOG_W() << "this memory address was not allocated by "
                "SecureMemoryAllocator:"
             << ptr;
    return {};
  }

  auto info = allocated_.value(ptr);
  allocated_.remove(ptr);
  return info;
}

auto SecureMemoryAllocator::Allocate(size_t size) -> void* {
  if (size == 0) return nullptr;

  if (secure_level_ < 1) {
    return std::malloc(size);
  }

  auto* ptr = NormalAllocate(size);
  reg_mem(ptr, size, false);

  return ptr;
}

auto SecureMemoryAllocator::Reallocate(void* ptr, size_t size) -> void* {
  if (secure_level_ < 1) {
    if (size == 0) {
      std::free(ptr);
      return nullptr;
    }
    return std::realloc(ptr, size);
  }

  if (ptr == nullptr) return Allocate(size);

  if (size == 0) {
    Deallocate(ptr);
    return nullptr;
  }

  auto old_info = take_info(ptr);
  if (!old_info) return nullptr;

  if (old_info->secure) {
    FLOG_W()
        << "SMARealloc called for secure memory; using secure reallocation";

    auto* new_ptr = sodium_malloc(size);
    if (new_ptr == nullptr) {
      reg_mem(ptr, old_info->size, true);
      FLOG_F("sodium_malloc failed");
      return nullptr;
    }

    std::memset(new_ptr, 0, size);
    std::memcpy(new_ptr, ptr, std::min(size, old_info->size));

    reg_mem(new_ptr, size, true);
    sodium_free(ptr);
    return new_ptr;
  }

  auto* new_ptr = NormalAllocate(size);
  if (new_ptr == nullptr) {
    reg_mem(ptr, old_info->size, false);
    return nullptr;
  }

  std::memcpy(new_ptr, ptr, std::min(size, old_info->size));
  NormalDeallocate(ptr, old_info->size, true);

  reg_mem(new_ptr, size, false);
  return new_ptr;
}

void SecureMemoryAllocator::Deallocate(void* ptr) {
  if (ptr == nullptr) return;

  if (secure_level_ < 1) {
    std::free(ptr);
    return;
  }

  auto info = take_info(ptr);
  if (!info) return;

  if (info->secure) {
    FLOG_W() << "SMAFree called for secure memory; using sodium_free";
    sodium_free(ptr);
    return;
  }

  const bool wipe = secure_level_ >= 1;
  NormalDeallocate(ptr, info->size, wipe);
}

auto SecureMemoryAllocator::SecAllocate(size_t size) -> void* {
  if (size == 0) return nullptr;

  if (secure_level_ < 2) {
    return Allocate(size);
  }

  auto* ptr = sodium_malloc(size);
  if (ptr == nullptr) {
    FLOG_F("sodium_malloc failed");
    return nullptr;
  }

  std::memset(ptr, 0, size);
  reg_mem(ptr, size, true);

  return ptr;
}

auto SecureMemoryAllocator::SecReallocate(void* ptr, size_t size) -> void* {
  if (secure_level_ < 2) {
    return Reallocate(ptr, size);
  }

  if (ptr == nullptr) return SecAllocate(size);

  if (size == 0) {
    SecDeallocate(ptr);
    return nullptr;
  }

  auto old_info = take_info(ptr);
  if (!old_info) return nullptr;

  auto* new_ptr = SecAllocate(size);
  if (new_ptr == nullptr) {
    reg_mem(ptr, old_info->size, old_info->secure);
    return nullptr;
  }

  std::memcpy(new_ptr, ptr, std::min(size, old_info->size));

  if (old_info->secure) {
    sodium_free(ptr);
  } else {
    NormalDeallocate(ptr, old_info->size, secure_level_ >= 1);
  }

  return new_ptr;
}

void SecureMemoryAllocator::SecDeallocate(void* ptr) {
  if (ptr == nullptr) return;

  if (secure_level_ < 2) {
    Deallocate(ptr);
    return;
  }

  auto info = take_info(ptr);
  if (!info) return;

  if (info->secure) {
    sodium_free(ptr);
    return;
  }

  NormalDeallocate(ptr, info->size, secure_level_ >= 1);
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