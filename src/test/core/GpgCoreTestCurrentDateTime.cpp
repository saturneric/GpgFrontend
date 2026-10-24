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

// CurrentDateTime() is the system clock unless the screenshot mode pins it.
// Normal runs never publish the pin, so the first case is the one that
// matters for them.

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "core/module/ModuleManager.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend::Test {

TEST(GpgCoreTestCurrentDateTime, FollowsTheSystemClockByDefault) {
  Module::RemoveRTValue("core", kFixedClockRTKey);
  const auto before = QDateTime::currentDateTime();
  const auto now = CurrentDateTime();
  const auto after = QDateTime::currentDateTime();
  EXPECT_LE(before, now);
  EXPECT_LE(now, after);
}

TEST(GpgCoreTestCurrentDateTime, ReturnsThePinnedClock) {
  constexpr qint64 kPinned = 1768471200;  // 2026-01-15T10:00:00Z
  Module::UpsertRTValue("core", kFixedClockRTKey, kPinned);
  EXPECT_EQ(CurrentDateTime().toSecsSinceEpoch(), kPinned);
  EXPECT_EQ(CurrentDateTime().timeSpec(),
            QDateTime::currentDateTime().timeSpec());

  Module::RemoveRTValue("core", kFixedClockRTKey);
  EXPECT_NE(CurrentDateTime().toSecsSinceEpoch(), kPinned);
}

TEST(GpgCoreTestCurrentDateTime, IgnoresAPinOfTheWrongType) {
  Module::UpsertRTValue("core", kFixedClockRTKey, QString("1768471200"));
  EXPECT_NE(CurrentDateTime().toSecsSinceEpoch(), 1768471200);
  Module::RemoveRTValue("core", kFixedClockRTKey);
}

}  // namespace GpgFrontend::Test
