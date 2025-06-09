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

namespace GpgFrontend {
class SecureMemoryAllocator;
}

namespace {
QMutex instance_mutex;
GpgFrontend::SecureMemoryAllocator* instance = nullptr;
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
  QHash<void*, size_t> allocated_;

  explicit SecureMemoryAllocator(int secure_level);

  ~SecureMemoryAllocator();
};

SecureMemoryAllocator::SecureMemoryAllocator(int secure_level)
    : secure_level_(secure_level) {}

SecureMemoryAllocator::~SecureMemoryAllocator() = default;

auto SecureMemoryAllocator::Allocate(size_t size) -> void* {
  // low secure level
  if (secure_level_ < 1) return malloc(size);

  // should not do allocate
  if (size == 0) return nullptr;

  auto* addr = OPENSSL_zalloc(size);
  if (size != 0 && addr == nullptr) FLOG_F("OPENSSL_zalloc failed");

  {
    QMutexLocker locker(&mutex_);
    Q_ASSERT(!allocated_.contains(addr));

    allocated_[addr] = size;
  }
  return addr;
}

auto SecureMemoryAllocator::Reallocate(void* ptr, size_t size) -> void* {
  // debug
  Q_ASSERT(size != 0);

  // low secure level
  if (secure_level_ < 1) return realloc(ptr, size);

  if (ptr == nullptr) return Allocate(size);

  {
    QMutexLocker locker(&mutex_);
    Q_ASSERT(allocated_.count(ptr) != 0);
    if (allocated_.count(ptr) == 0) {
      FLOG_W()
          << "this memory address was not allocated by SecureMemoryAllocator: "
          << ptr;
      return nullptr;
    }

    const auto ptr_size = allocated_.value(ptr);
    auto* addr = OPENSSL_clear_realloc(ptr, ptr_size, size);
    if (size != 0 && addr == nullptr) FLOG_F("OPENSSL_clear_realloc failed");

    allocated_.remove(ptr);
    allocated_[addr] = size;

    return addr;
  }
}

void SecureMemoryAllocator::Deallocate(void* ptr) {
  // low secure level
  if (secure_level_ < 1) {
    free(ptr);
    return;
  }

  if (ptr == nullptr) return;

  {
    QMutexLocker locker(&mutex_);

    Q_ASSERT(allocated_.contains(ptr));
    if (!allocated_.contains(ptr)) {
      FLOG_W()
          << "this memory address was not allocated by SecureMemoryAllocator: "
          << ptr;
      return;
    }

    const auto ptr_size = allocated_.value(ptr);
    OPENSSL_clear_free(ptr, ptr_size);
    allocated_.remove(ptr);
  }
}

auto SecureMemoryAllocator::GetInstance() -> SecureMemoryAllocator* {
  QMutexLocker<QMutex> locker(&instance_mutex);

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
  // middle secure level
  if (secure_level_ < 2) return Allocate(size);

  auto* addr = OPENSSL_secure_zalloc(size);
  if (addr == nullptr) FLOG_F("OPENSSL_secure_malloc failed");

  {
    QMutexLocker locker(&mutex_);
    Q_ASSERT(!allocated_.contains(addr));

    allocated_[addr] = size;
  }
  return addr;
}

auto SecureMemoryAllocator::SecReallocate(void* ptr, size_t size) -> void* {
  // middle secure level
  if (secure_level_ < 2) return Reallocate(ptr, size);

  if (CRYPTO_secure_malloc_initialized() != 1) {
    FLOG_F("CRYPTO_secure_malloc_initialized failed");
  }

  void* new_addr = SecAllocate(size);
  Q_ASSERT(new_addr != ptr);

  if (ptr != nullptr) {
    auto old_size = OPENSSL_secure_actual_size(ptr);
    std::memcpy(new_addr, ptr, std::min(size, old_size));

    SecDeallocate(ptr);
  }

  return new_addr;
}

void SecureMemoryAllocator::SecDeallocate(void* ptr) {
  // middle secure level
  if (secure_level_ < 2) {
    Deallocate(ptr);
    return;
  }

  if (ptr == nullptr) return;

  if (CRYPTO_secure_malloc_initialized() != 1) {
    FLOG_F("CRYPTO_secure_malloc_initialized failed");
  }

  {
    QMutexLocker locker(&mutex_);

    Q_ASSERT(allocated_.contains(ptr));
    if (!allocated_.contains(ptr)) {
      FLOG_W()
          << "this memory address was not allocated by SecureMemoryAllocator: "
          << ptr;
      return;
    }

    auto sz = OPENSSL_secure_actual_size(ptr);
    OPENSSL_secure_clear_free(ptr, sz);
    allocated_.remove(ptr);
  }
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