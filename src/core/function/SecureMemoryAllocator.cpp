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

namespace GpgFrontend {

SecureMemoryAllocator::SecureMemoryAllocator() = default;

SecureMemoryAllocator::~SecureMemoryAllocator() = default;

auto SecureMemoryAllocator::Allocate(size_t size) -> void* {
  auto* addr = OPENSSL_zalloc(size);
  if (addr == nullptr) FLOG_F("OPENSSL_zalloc failed");
  allocated_[addr] = size;
  return addr;
}

auto SecureMemoryAllocator::Reallocate(void* ptr, size_t size) -> void* {
  if (ptr == nullptr) return Allocate(size);

  Q_ASSERT(allocated_.contains(ptr));
  if (!allocated_.contains(ptr)) {
    FLOG_W()
        << "this memory address was not allocated by SecureMemoryAllocator: "
        << ptr;
    return nullptr;
  }

  const auto ptr_size = allocated_.value(ptr);
  auto* addr = OPENSSL_clear_realloc(ptr, ptr_size, size);
  if (addr == nullptr) FLOG_F("OPENSSL_clear_realloc failed");

  allocated_[addr] = size;
  allocated_.remove(ptr);
  return addr;
}

void SecureMemoryAllocator::Deallocate(void* ptr) {
  if (ptr == nullptr) return;

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

auto SecureMemoryAllocator::GetInstance() -> SecureMemoryAllocator* {
  static SecureMemoryAllocator instance;
  return &instance;
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
  if (CRYPTO_secure_malloc_initialized() != 1) {
    FLOG_F("CRYPTO_secure_malloc_initialized failed");
  }

  auto* addr = OPENSSL_secure_zalloc(size);
  if (addr == nullptr) FLOG_F("OPENSSL_secure_malloc failed");
  allocated_[addr] = size;
  return addr;
}

auto SecureMemoryAllocator::SecReallocate(void* ptr, size_t size) -> void* {
  void* new_addr = SecAllocate(size);

  if (ptr != nullptr) {
    auto old_size = OPENSSL_secure_actual_size(ptr);
    std::memcpy(new_addr, ptr, std::min(size, old_size));
    SecDeallocate(ptr);
  }

  return new_addr;
}

void SecureMemoryAllocator::SecDeallocate(void* ptr) {
  if (ptr == nullptr) return;

  if (CRYPTO_secure_malloc_initialized() != 1) {
    FLOG_F("CRYPTO_secure_malloc_initialized failed");
  }

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

auto SMASecMalloc(size_t size) -> void* {
  return SecureMemoryAllocator::GetInstance()->SecAllocate(size);
}

void SMASecFree(void* ptr) {
  SecureMemoryAllocator::GetInstance()->SecDeallocate(ptr);
}

auto GF_CORE_EXPORT SMASecRealloc(void* ptr, size_t size) -> void* {
  return SecureMemoryAllocator::GetInstance()->SecReallocate(ptr, size);
}
}  // namespace GpgFrontend