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

#include <array>
#include <new>

#include "GFCoreTest.h"
#include "boot/BootContext.h"
#include "core/GFCoreInit.h"

namespace GpgFrontend::Test {

// A program that does not set a flag gets it off. The phone application never
// sets unit_test_mode; left uninitialized it read true on a real device, and
// the core skipped opening every key database as if under test.
TEST(BootContextTest, FlagsAreOffUnlessSet) {
  // Built on memory already full of ones, so a member without an initializer
  // reads true rather than happening to read zero.
  alignas(BootContext) std::array<unsigned char, sizeof(BootContext)> storage;
  storage.fill(0xFF);
  std::array<char*, 1> argv{nullptr};
  auto* ctx = new (storage.data()) BootContext(0, argv.data());

  EXPECT_FALSE(ctx->unit_test_mode);
  EXPECT_FALSE(ctx->gather_external_gnupg_info);

  ctx->~BootContext();
}

TEST(BootContextTest, CoreInitArgsAreOffUnlessSet) {
  alignas(CoreInitArgs) std::array<unsigned char, sizeof(CoreInitArgs)> storage;
  storage.fill(0xFF);
  auto* args = new (storage.data()) CoreInitArgs;

  EXPECT_FALSE(args->unit_test_mode);
  EXPECT_FALSE(args->gather_external_gnupg_info);
}

}  // namespace GpgFrontend::Test
