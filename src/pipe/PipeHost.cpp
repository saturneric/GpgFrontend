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

// The two functions the Rust engine imports from its host. In GpgFrontend they
// free what the C++ side allocated with its secure allocator. gfpipe hands the
// engine nothing it allocated itself: every buffer that reaches these was
// allocated by the engine, so each goes back to the engine's own free, which
// wipes it.

#include <cstddef>
#include <cstdint>

#include "core/GFCoreRust.h"

extern "C" {

// NOLINTNEXTLINE
void gfc_secure_free_cstr(char* cstr) {
  GpgFrontend::Rust::gfr_crypto_free_string(cstr);
}

// NOLINTNEXTLINE
void gfc_secure_free_buffer(uint8_t* ptr, size_t len) {
  GpgFrontend::Rust::gfr_crypto_free_buffer(ptr, len);
}
}
