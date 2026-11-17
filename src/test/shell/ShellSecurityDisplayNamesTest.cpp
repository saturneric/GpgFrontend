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

#include "shell/present/SecurityDisplayNames.h"

namespace GpgFrontend::Test {

TEST(ShellSecurityDisplayNamesTest, EverySecureLevelHasItsOwnName) {
  // The Advanced tab and the About dialog both read these; when they were
  // written out separately the two drifted apart.
  QSet<QString> seen;
  for (int level = 0; level <= 3; ++level) {
    const auto name = SecureLevelDisplayName(level);
    EXPECT_FALSE(name.isEmpty()) << level;
    EXPECT_FALSE(seen.contains(name))
        << level << " duplicates " << name.toStdString();
    seen.insert(name);
  }
}

TEST(ShellSecurityDisplayNamesTest, AnUnrecognisedLevelIsReportedAsUnknown) {
  const auto unknown = SecureLevelDisplayName(4);
  EXPECT_FALSE(unknown.isEmpty());
  for (int level = 0; level <= 3; ++level) {
    EXPECT_NE(SecureLevelDisplayName(level), unknown);
  }
  EXPECT_EQ(SecureLevelDisplayName(-1), unknown);
  EXPECT_EQ(SecureLevelDisplayName(99), unknown);
}

TEST(ShellSecurityDisplayNamesTest, EveryKeyProtectionModeHasItsOwnName) {
  const auto keychain =
      AppKeyProtectionDisplayName(AppKeyProtection::kKEYCHAIN);
  const auto pin = AppKeyProtectionDisplayName(AppKeyProtection::kPIN);
  const auto none = AppKeyProtectionDisplayName(AppKeyProtection::kNONE);

  EXPECT_FALSE(keychain.isEmpty());
  EXPECT_NE(keychain, pin);
  EXPECT_NE(pin, none);
  EXPECT_NE(keychain, none);
}

}  // namespace GpgFrontend::Test
