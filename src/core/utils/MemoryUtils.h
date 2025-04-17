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

#include "core/GpgFrontendCoreExport.h"
#include "core/function/SecureMemoryAllocator.h"

/* To avoid that a compiler optimizes certain memset calls away, these
   macros may be used instead. */
#define wipememory2(_ptr, _set, _len)               \
  do {                                              \
    volatile char *_vptr = (volatile char *)(_ptr); \
    size_t _vlen = (_len);                          \
    while (_vlen) {                                 \
      *_vptr = (_set);                              \
      _vptr++;                                      \
      _vlen--;                                      \
    }                                               \
  } while (0)
#define wipememory(_ptr, _len) wipememory2(_ptr, 0, _len)
#define wipe(_ptr, _len) wipememory2(_ptr, 0, _len)

#define xtoi_1(p)                    \
  (*(p) <= '9'   ? (*(p) - '0')      \
   : *(p) <= 'F' ? (*(p) - 'A' + 10) \
                 : (*(p) - 'a' + 10))
#define xtoi_2(p) ((xtoi_1(p) * 16) + xtoi_1((p) + 1))

namespace GpgFrontend {

template <typename T>
class PointerConverter {
 public:
  explicit PointerConverter(void *ptr) : ptr_(ptr) {}

  auto AsType() const -> T * { return static_cast<T *>(ptr_); }

 private:
  void *ptr_;
};

/**
 * @brief
 *
 * @return void*
 */
auto GPGFRONTEND_CORE_EXPORT SecureMalloc(std::size_t) -> void *;

/**
 * @brief
 *
 * @return void*
 */
auto GPGFRONTEND_CORE_EXPORT SecureRealloc(void *, std::size_t) -> void *;

/**
 * @brief
 *
 * @tparam T
 * @return T*
 */
template <typename T>
auto SecureMallocAsType(std::size_t size) -> T * {
  return PointerConverter<T>(SecureMemoryAllocator::Allocate(size)).AsType();
}

/**
 * @brief
 *
 * @return void*
 */
template <typename T>
auto SecureReallocAsType(T *ptr, std::size_t size) -> T * {
  return PointerConverter<T>(SecureMemoryAllocator::Reallocate(ptr, size))
      .AsType();
}

/**
 * @brief
 *
 */
void GPGFRONTEND_CORE_EXPORT SecureFree(void *);

template <typename T, typename... Args>
static auto SecureCreateObject(Args &&...args) -> T * {
  void *mem = SecureMemoryAllocator::Allocate(sizeof(T));
  if (!mem) return nullptr;

  try {
    return new (mem) T(std::forward<Args>(args)...);
  } catch (...) {
    SecureMemoryAllocator::Deallocate(mem);
    throw;
  }
}

template <typename T>
static void SecureDestroyObject(T *obj) {
  if (!obj) return;
  obj->~T();
  SecureMemoryAllocator::Deallocate(obj);
}

template <typename T, typename... Args>
static auto SecureCreateUniqueObject(Args &&...args)
    -> std::unique_ptr<T, SecureObjectDeleter<T>> {
  void *mem = SecureMemoryAllocator::Allocate(sizeof(T));
  if (!mem) throw std::bad_alloc();

  try {
    return std::unique_ptr<T, SecureObjectDeleter<T>>(
        new (mem) T(std::forward<Args>(args)...));
  } catch (...) {
    SecureMemoryAllocator::Deallocate(mem);
    throw;
  }
}

template <typename T, typename... Args>
auto SecureCreateSharedObject(Args &&...args) -> QSharedPointer<T> {
  void *mem = SecureMemoryAllocator::Allocate(sizeof(T));
  if (!mem) throw std::bad_alloc();

  try {
    T *obj = new (mem) T(std::forward<Args>(args)...);
    return QSharedPointer<T>(obj, [](T *ptr) {
      ptr->~T();
      SecureMemoryAllocator::Deallocate(ptr);
    });
  } catch (...) {
    SecureMemoryAllocator::Deallocate(mem);
    throw;
  }
}

};  // namespace GpgFrontend