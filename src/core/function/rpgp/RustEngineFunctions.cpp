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

#include "RustEngineFunctions.h"

#include <sodium.h>

#include <cstring>

#include "core/function/SecureMemoryAllocator.h"

// NOLINTNEXTLINE
void *gfc_secure_free_cstr(char *cstr) {
  if (cstr != nullptr) {
    // Use secure zeroing before freeing the memory. NOTE: this derives the wipe
    // length from strlen(), so it is ONLY safe for genuine NUL-terminated C
    // strings. For length-delimited binary buffers (e.g. passphrase bytes) use
    // gfc_secure_free_buffer() instead — strlen() here would read past the end.
    std::memset(cstr, 0, std::strlen(cstr));
    GpgFrontend::SMAFree(cstr);
  }
  return nullptr;
}

// NOLINTNEXTLINE
void gfc_secure_free_buffer(uint8_t *ptr, size_t len) {
  if (ptr == nullptr) return;
  // Wipe exactly the bytes we own. sodium_memzero is not elided by the
  // optimizer and we never scan for a terminator, so this cannot over-read the
  // allocation.
  if (len > 0) sodium_memzero(ptr, len);
  GpgFrontend::SMAFree(ptr);
}

// NOLINTNEXTLINE
void gfc_secure_free(void *ptr, void *) {
  if (ptr != nullptr) {
    GpgFrontend::SMAFree(ptr);
  }
}