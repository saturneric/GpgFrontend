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

#include "shell/present/TextDirection.h"

namespace GpgFrontend::Test {

using Shell::FirstStrongDirection;

TEST(ShellTextDirectionTest, TheFirstStrongCharacterDecides) {
  EXPECT_EQ(FirstStrongDirection(u"123 abc"), Qt::LeftToRight);
  EXPECT_EQ(FirstStrongDirection(u"123 \u05e9\u05dc\u05d5\u05dd abc"),
            Qt::RightToLeft);
  EXPECT_EQ(FirstStrongDirection(u"123 ..."), std::nullopt);
  EXPECT_EQ(FirstStrongDirection(u""), std::nullopt);
}

TEST(ShellTextDirectionTest, AScriptAboveTheBmpIsReadWhole) {
  // Adlam (U+1E900..), right-to-left, written as a surrogate pair: its
  // halves alone would read as left-to-right.
  const auto adlam = QString::fromUcs4(U"\U0001E900\U0001E901");
  EXPECT_EQ(FirstStrongDirection(adlam), Qt::RightToLeft);
}

}  // namespace GpgFrontend::Test
