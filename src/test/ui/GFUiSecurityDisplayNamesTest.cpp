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

#include "GpgFrontendTest.h"
#include "ui/function/SecurityDisplayNames.h"

namespace GpgFrontend::Test {

TEST(SecurityDisplayNamesTest, EverySecureLevelHasItsOwnName) {
  // The Advanced tab and the About dialog both read these; when they were
  // written out separately the two drifted apart.
  QSet<QString> seen;
  for (int level = 0; level <= 3; ++level) {
    const auto name = UI::SecureLevelDisplayName(level);
    EXPECT_FALSE(name.isEmpty()) << level;
    EXPECT_FALSE(seen.contains(name))
        << level << " duplicates " << name.toStdString();
    seen.insert(name);
  }
}

TEST(SecurityDisplayNamesTest, AnUnrecognisedLevelIsReportedAsUnknown) {
  const auto unknown = UI::SecureLevelDisplayName(4);
  EXPECT_FALSE(unknown.isEmpty());
  for (int level = 0; level <= 3; ++level) {
    EXPECT_NE(UI::SecureLevelDisplayName(level), unknown);
  }
  EXPECT_EQ(UI::SecureLevelDisplayName(-1), unknown);
  EXPECT_EQ(UI::SecureLevelDisplayName(99), unknown);
}

TEST(SecurityDisplayNamesTest, EveryKeyProtectionModeHasItsOwnName) {
  const auto keychain =
      UI::AppKeyProtectionDisplayName(AppKeyProtection::kKEYCHAIN);
  const auto pin = UI::AppKeyProtectionDisplayName(AppKeyProtection::kPIN);
  const auto none = UI::AppKeyProtectionDisplayName(AppKeyProtection::kNONE);

  EXPECT_FALSE(keychain.isEmpty());
  EXPECT_NE(keychain, pin);
  EXPECT_NE(pin, none);
  EXPECT_NE(keychain, none);
}

}  // namespace GpgFrontend::Test
