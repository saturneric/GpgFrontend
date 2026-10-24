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
#include "ui/widgets/KeyList.h"

namespace GpgFrontend::Test {

using UI::CategoryFallbackColor;

TEST(GFUiCategoryColorTest, FallbackColorIsTheSameOnEveryLaunch) {
  // Pinned values: a hash seeded per process would give other hues in
  // another run, which is the bug this guards against.
  EXPECT_EQ(CategoryFallbackColor(u"cat:0123456789ABCD"_s).hue(), 86);
  EXPECT_EQ(CategoryFallbackColor(u"cat:FEDCBA98765432"_s).hue(), 298);
  EXPECT_EQ(CategoryFallbackColor(u"cat:0123456789ABCD"_s),
            CategoryFallbackColor(u"cat:0123456789ABCD"_s));
}

TEST(GFUiCategoryColorTest, FallbackColorKeepsItsTone) {
  const auto color = CategoryFallbackColor(u"cat:FEDCBA98765432"_s);
  EXPECT_EQ(color.saturation(), 160);
  EXPECT_EQ(color.value(), 205);
}

}  // namespace GpgFrontend::Test
