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
#include "ui/function/WindowGeometry.h"

namespace GpgFrontend::Test {

namespace {
const QRect kAvailable(0, 0, 1000, 800);  // 95% is 950 x 760
}

TEST(WindowGeometryTest, AFittingRectIsLeftAlone) {
  const QRect rect(100, 100, 400, 300);
  EXPECT_EQ(UI::ClampRectToAvailableGeometry(rect, kAvailable), rect);
}

TEST(WindowGeometryTest, OversizeIsClampedTo95Percent) {
  // Not 100%: a window filling the whole work area reads as broken rather
  // than as maximised, because none of its edges are visible.
  const auto out =
      UI::ClampRectToAvailableGeometry(QRect(0, 0, 5000, 5000), kAvailable);
  EXPECT_EQ(out.width(), 950);
  EXPECT_EQ(out.height(), 760);
}

TEST(WindowGeometryTest, ARectOffAnyEdgeIsMovedBackInside) {
  // The case that matters in practice: a geometry saved on a monitor that is
  // no longer attached restores wholly or partly off screen.
  for (const auto& rect :
       {QRect(-500, 100, 200, 200), QRect(100, -500, 200, 200),
        QRect(2000, 100, 200, 200), QRect(100, 2000, 200, 200)}) {
    const auto out = UI::ClampRectToAvailableGeometry(rect, kAvailable);
    EXPECT_TRUE(kAvailable.contains(out))
        << rect.x() << "," << rect.y() << " -> " << out.x() << "," << out.y();
    EXPECT_EQ(out.size(), rect.size());
  }
}

TEST(WindowGeometryTest, AnOffsetAvailableAreaIsHonoured) {
  // A second monitor does not start at the origin.
  const QRect available(1920, 0, 1000, 800);
  const auto out =
      UI::ClampRectToAvailableGeometry(QRect(0, 0, 200, 200), available);
  EXPECT_EQ(out.left(), 1920);
  EXPECT_TRUE(available.contains(out));
}

}  // namespace GpgFrontend::Test
