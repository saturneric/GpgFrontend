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

namespace GpgFrontend {

class GF_CORE_EXPORT SecureMemoryAllocator {
 public:
  static auto GetInstance() -> SecureMemoryAllocator *;

  SecureMemoryAllocator(const SecureMemoryAllocator &) = delete;

  auto operator=(const SecureMemoryAllocator &)
      -> SecureMemoryAllocator & = delete;

  auto Allocate(size_t) -> void *;

  auto Reallocate(void *, size_t) -> void *;

  void Deallocate(void *);

  auto SecAllocate(size_t) -> void *;

  auto SecReallocate(void *, size_t) -> void *;

  void SecDeallocate(void *);

 private:
  QHash<void *, size_t> allocated_;

  SecureMemoryAllocator();

  ~SecureMemoryAllocator();
};

auto GF_CORE_EXPORT SMAMalloc(size_t size) -> void *;

auto GF_CORE_EXPORT SMARealloc(void *ptr, size_t size) -> void *;

void GF_CORE_EXPORT SMAFree(void *ptr);

auto GF_CORE_EXPORT SMASecMalloc(size_t size) -> void *;

auto GF_CORE_EXPORT SMASecRealloc(void *ptr, size_t size) -> void *;

void GF_CORE_EXPORT SMASecFree(void *ptr);

template <typename T>
struct SecureObjectDeleter {
  void operator()(T *ptr) {
    if (ptr) {
      ptr->~T();
      SMAFree(ptr);
    }
  }
};

template <typename T>
using SecureUniquePtr = std::unique_ptr<T, SecureObjectDeleter<T>>;

}  // namespace GpgFrontend