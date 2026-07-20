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

#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

TEST(UserIdComponentTest, AcceptsOrdinaryAndShortNames) {
  EXPECT_TRUE(IsValidUserIdComponent("Saturneric"));
  // Short names are legal; only the UI warns about them.
  EXPECT_TRUE(IsValidUserIdComponent("Al"));
  EXPECT_TRUE(IsValidUserIdComponent("Ada Lovelace"));
}

TEST(UserIdComponentTest, RejectsStructuralDelimitersAndControlChars) {
  EXPECT_FALSE(IsValidUserIdComponent("Eric (work)"));
  EXPECT_FALSE(IsValidUserIdComponent("Eric <eric@bktus.com>"));
  EXPECT_FALSE(IsValidUserIdComponent(QString("Eric") + QChar(0x0A) + "Bad"));
}

TEST(AssembleUserIdTest, OmitsEmptyEmailAndComment) {
  // The email address is optional in a user id.
  EXPECT_EQ(AssembleUserId("Ada", "", ""), "Ada");
  EXPECT_EQ(AssembleUserId("Ada", "work", ""), "Ada (work)");
  EXPECT_EQ(AssembleUserId("Ada", "", "ada@bktus.com"), "Ada <ada@bktus.com>");
  EXPECT_EQ(AssembleUserId("Ada", "work", "ada@bktus.com"),
            "Ada (work) <ada@bktus.com>");
}

TEST(AssembleUserIdTest, TrimsEachComponent) {
  EXPECT_EQ(AssembleUserId("  Ada  ", "  work  ", "  ada@bktus.com  "),
            "Ada (work) <ada@bktus.com>");
  // Whitespace-only components collapse away entirely.
  EXPECT_EQ(AssembleUserId("Ada", "   ", "   "), "Ada");
}

TEST(EmailAddressTest, AcceptsValidAndRejectsMalformed) {
  EXPECT_TRUE(IsEmailAddress("ada@bktus.com"));
  EXPECT_TRUE(IsEmailAddress("ada.lovelace+pgp@sub.bktus.com"));
  EXPECT_FALSE(IsEmailAddress(""));
  EXPECT_FALSE(IsEmailAddress("not-an-email"));
  // The match is anchored: a valid address embedded in junk is not an address,
  // otherwise the trailing text would leak into the assembled user id.
  EXPECT_FALSE(IsEmailAddress("ada bad@bktus.com >evil"));
  EXPECT_FALSE(IsEmailAddress("<ada@bktus.com>"));
}

}  // namespace GpgFrontend::Test
