/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#if !defined(MACOS) && defined(DEBUG)
#include <mimalloc.h>
#endif

namespace GpgFrontend {

auto SecureMemoryAllocator::Allocate(std::size_t size) -> void* {
#if !defined(MACOS) && defined(DEBUG)
  auto* addr = mi_malloc(size);
#else
  auto* addr = malloc(size);
#endif
  return addr;
}

auto SecureMemoryAllocator::Reallocate(void* ptr, std::size_t size) -> void* {
#if !defined(MACOS) && defined(DEBUG)
  auto* addr = mi_realloc(ptr, size);
#else
  auto* addr = realloc(ptr, size);
#endif
  return addr;
}

void SecureMemoryAllocator::Deallocate(void* p) {
#if !defined(MACOS) && defined(DEBUG)
  mi_free(p);
#else
  free(p);
#endif
}

}  // namespace GpgFrontend