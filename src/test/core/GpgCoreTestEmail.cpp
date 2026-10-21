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

#include <gtest/gtest.h>

#include "core/utils/CommonUtils.h"

namespace GpgFrontend::Test {

TEST(CoreEmailTest, AcceptsPlainAddresses) {
  EXPECT_TRUE(IsEmailAddress("alice@example.com"));
  EXPECT_TRUE(IsEmailAddress("a.b+tag@sub.example.org"));
}

TEST(CoreEmailTest, AcceptsMixedCase) {
  EXPECT_TRUE(IsEmailAddress("alice@Example.com"));
  EXPECT_TRUE(IsEmailAddress("Alice.Smith@EXAMPLE.COM"));
}

TEST(CoreEmailTest, RejectsPartialMatches) {
  EXPECT_FALSE(IsEmailAddress("a b@c.com >x"));
  EXPECT_FALSE(IsEmailAddress("alice@example.com>"));
  EXPECT_FALSE(IsEmailAddress(" alice@example.com"));
  EXPECT_FALSE(IsEmailAddress(""));
  EXPECT_FALSE(IsEmailAddress("alice"));
}

}  // namespace GpgFrontend::Test
