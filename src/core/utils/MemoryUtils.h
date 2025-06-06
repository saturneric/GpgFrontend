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
 * @tparam T
 * @return T*
 */
template <typename T>
auto SecureMallocAsType(std::size_t size) -> T * {
  return PointerConverter<T>(SMAMalloc(size)).AsType();
}

/**
 * @brief
 *
 * @return void*
 */
template <typename T>
auto SecureReallocAsType(T *ptr, std::size_t size) -> T * {
  return PointerConverter<T>(SMARealloc(ptr, size)).AsType();
}

template <typename T, typename... Args>
static auto SecureCreateObject(Args &&...args) -> T * {
  void *mem = SMAMalloc(sizeof(T));
  if (!mem) return nullptr;

  try {
    return new (mem) T(std::forward<Args>(args)...);
  } catch (...) {
    SMAFree(mem);
    throw;
  }
}

template <typename T>
static void SecureDestroyObject(T *obj) {
  if (!obj) return;
  obj->~T();
  SMAFree(obj);
}

template <typename T, typename... Args>
static auto SecureCreateUniqueObject(Args &&...args)
    -> std::unique_ptr<T, SecureObjectDeleter<T>> {
  void *mem = SMAMalloc(sizeof(T));
  if (!mem) throw std::bad_alloc();

  try {
    return std::unique_ptr<T, SecureObjectDeleter<T>>(
        new (mem) T(std::forward<Args>(args)...));
  } catch (...) {
    SMAFree(mem);
    throw;
  }
}

template <typename T, typename... Args>
auto SecureCreateSharedObject(Args &&...args) -> QSharedPointer<T> {
  void *mem = SMAMalloc(sizeof(T));
  if (!mem) throw std::bad_alloc();

  try {
    T *obj = new (mem) T(std::forward<Args>(args)...);
    return QSharedPointer<T>(obj, [](T *ptr) {
      ptr->~T();
      SMAFree(ptr);
    });
  } catch (...) {
    SMAFree(mem);
    throw;
  }
}

template <typename T>
class SMAAllocator {
 public:
  using value_type = T;

  SMAAllocator() noexcept = default;
  template <class U>
  explicit SMAAllocator(const SMAAllocator<U> &) noexcept {}

  auto allocate(std::size_t n) -> T * {
    void *p = SMAMalloc(n * sizeof(T));
    if (p == nullptr) throw std::bad_alloc();
    return static_cast<T *>(p);
  }

  void deallocate(T *p, std::size_t /*n*/) noexcept {
    SMAFree(static_cast<void *>(p));
  }

  // C++17: allocator must be equality comparable
  auto operator==(const SMAAllocator &) const noexcept -> bool { return true; }

  auto operator!=(const SMAAllocator &) const noexcept -> bool { return false; }
};

template <typename T>
class SMASecAllocator {
 public:
  using value_type = T;

  SMASecAllocator() noexcept = default;
  template <class U>
  explicit SMASecAllocator(const SMASecAllocator<U> &) noexcept {}

  auto allocate(std::size_t n) -> T * {
    void *p = SMASecMalloc(n * sizeof(T));
    if (p == nullptr) throw std::bad_alloc();
    return static_cast<T *>(p);
  }

  void deallocate(T *p, std::size_t /*n*/) noexcept {
    SMASecFree(static_cast<void *>(p));
  }

  // C++17: allocator must be equality comparable
  auto operator==(const SMASecAllocator &) const noexcept -> bool {
    return true;
  }

  auto operator!=(const SMASecAllocator &) const noexcept -> bool {
    return false;
  }
};

};  // namespace GpgFrontend