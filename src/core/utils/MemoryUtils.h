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

// Volatile memory wipe — zero @p _len bytes at @p _ptr without being
// optimised away by the compiler.
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

// Zero @p _len bytes at @p _ptr without compiler optimisation.
#define wipememory(_ptr, _len) wipememory2(_ptr, 0, _len)

// Alias for wipememory — zero @p _len bytes at @p _ptr.
#define wipe(_ptr, _len) wipememory2(_ptr, 0, _len)

// Convert a hex digit character to its 4-bit integer value.
#define xtoi_1(p)                    \
  (*(p) <= '9'   ? (*(p) - '0')      \
   : *(p) <= 'F' ? (*(p) - 'A' + 10) \
                 : (*(p) - 'a' + 10))

// Convert two consecutive hex digit characters to an 8-bit integer value.
#define xtoi_2(p) ((xtoi_1(p) * 16) + xtoi_1((p) + 1))

namespace GpgFrontend {

/**
 * @brief Type-safe wrapper for casting a void pointer to a typed pointer.
 *
 * @tparam T target type
 */
template <typename T>
class PointerConverter {
 public:
  /**
   * @brief Wrap a raw void pointer.
   * @param ptr pointer to wrap
   */
  explicit PointerConverter(void *ptr) : ptr_(ptr) {}

  /**
   * @brief Cast the wrapped pointer to T*.
   * @return typed pointer
   */
  auto AsType() const -> T * { return static_cast<T *>(ptr_); }

 private:
  void *ptr_;
};

/**
 * @brief Allocate @p size bytes via SMAMalloc and cast the result to T*.
 *
 * @tparam T target type
 * @param size number of bytes to allocate
 * @return typed pointer to the allocated memory
 */
template <typename T>
auto SecureMallocAsType(std::size_t size) -> T * {
  return PointerConverter<T>(SMAMalloc(size)).AsType();
}

/**
 * @brief Reallocate @p ptr to @p size bytes via SMARealloc and cast to T*.
 *
 * @tparam T target type
 * @param ptr existing pointer to reallocate
 * @param size new size in bytes
 * @return typed pointer to the reallocated memory
 */
template <typename T>
auto SecureReallocAsType(T *ptr, std::size_t size) -> T * {
  return PointerConverter<T>(SMARealloc(ptr, size)).AsType();
}

/**
 * @brief Construct a T in-place on SMAMalloc-allocated memory.
 *
 * Throws the constructor exception (and frees the memory) if construction
 * fails.
 *
 * @tparam T type to construct
 * @tparam Args constructor argument types
 * @param args arguments forwarded to the T constructor
 * @return raw pointer to the constructed object, or nullptr if allocation fails
 */
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

/**
 * @brief Call the destructor of @p obj and free its memory with SMAFree.
 *
 * @tparam T type of the object
 * @param obj pointer to the object to destroy; no-op if nullptr
 */
template <typename T>
static void SecureDestroyObject(T *obj) {
  if (!obj) return;
  obj->~T();
  SMAFree(obj);
}

/**
 * @brief Deleter for SecureUniquePtr — calls the destructor then SMAFree.
 *
 * @tparam T type of the managed object
 */
template <typename T>
struct SecureObjectDeleter {
  void operator()(T *ptr) {
    if (ptr) {
      ptr->~T();
      SMAFree(ptr);
    }
  }
};

// unique_ptr whose deleter calls the destructor then SMAFree.
template <typename T>
using SecureUniquePtr = std::unique_ptr<T, SecureObjectDeleter<T>>;

/**
 * @brief Construct a SecureUniquePtr<T> via placement-new on SMAMalloc memory.
 *
 * Throws std::bad_alloc if allocation fails, or re-throws any constructor
 * exception.
 *
 * @tparam T type to construct
 * @tparam Args constructor argument types
 * @param args arguments forwarded to the T constructor
 * @return SecureUniquePtr owning the new object
 */
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

/**
 * @brief Construct a QSharedPointer<T> via placement-new on SMAMalloc memory.
 *
 * The shared pointer's deleter calls the destructor and SMAFree.
 *
 * @tparam T type to construct
 * @tparam Args constructor argument types
 * @param args arguments forwarded to the T constructor
 * @return QSharedPointer owning the new object
 */
template <typename T, typename... Args>
auto SecureCreateSharedObject(Args &&...args) -> QSharedPointer<T> {
  void *mem = SMAMalloc(sizeof(T));
  if (!mem) throw std::bad_alloc();

  try {
    T *obj = new (mem) T(std::forward<Args>(args)...);
    return QSharedPointer<T>(obj, [](T *ptr) noexcept {
      if (ptr) {
        ptr->~T();
        SMAFree(ptr);
      }
    });
  } catch (...) {
    SMAFree(mem);
    throw;
  }
}

/**
 * @brief STL-compatible allocator backed by SMAMalloc and SMAFree.
 *
 * Suitable for use with standard containers (std::vector, std::map, etc.)
 * when allocations must go through the SMA tracking layer.
 *
 * @tparam T element type
 */
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

/**
 * @brief STL-compatible allocator backed by SMASecMalloc and SMASecFree.
 *
 * Use this for containers holding sensitive data that should be stored in
 * libsodium guarded memory when the application secure level is >= 2.
 *
 * @tparam T element type
 */
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
