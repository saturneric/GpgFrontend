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
#include "ui/function/UIStyle.h"

namespace GpgFrontend::Test {

namespace {

auto LightPalette() -> QPalette {
  QPalette p;
  p.setColor(QPalette::Base, QColor(255, 255, 255));
  p.setColor(QPalette::Window, QColor(240, 240, 240));
  p.setColor(QPalette::WindowText, QColor(0, 0, 0));
  p.setColor(QPalette::Text, QColor(0, 0, 0));
  return p;
}

auto DarkPalette() -> QPalette {
  QPalette p;
  p.setColor(QPalette::Base, QColor(30, 30, 30));
  p.setColor(QPalette::Window, QColor(45, 45, 45));
  p.setColor(QPalette::WindowText, QColor(255, 255, 255));
  p.setColor(QPalette::Text, QColor(255, 255, 255));
  return p;
}

}  // namespace

TEST(UIStyleTest, ThePositiveAccentIsThemeSpecific) {
  // A green tuned for a white background loses its contrast on a dark one.
  EXPECT_NE(UI::AccentColor(LightPalette(), true),
            UI::AccentColor(DarkPalette(), true));
}

TEST(UIStyleTest, ANegativeStateIsDeEmphasisedRatherThanRed) {
  for (const auto& p : {LightPalette(), DarkPalette()}) {
    const auto negative = UI::AccentColor(p, false);
    // It is the ordinary text colour, merely quietened.
    EXPECT_EQ(negative.rgb(), p.color(QPalette::Text).rgb());
    EXPECT_LT(negative.alpha(), 255);
    EXPECT_NE(negative, UI::DangerColor(p));
  }
}

TEST(UIStyleTest, WarningAndDangerAreDistinctAndThemeSpecific) {
  // Amber says "this settled for less than it asked for"; red says "this
  // cannot be undone". Conflating them is what the split exists to prevent.
  for (const auto& p : {LightPalette(), DarkPalette()}) {
    EXPECT_NE(UI::WarningColor(p), UI::DangerColor(p));
  }
  EXPECT_NE(UI::WarningColor(LightPalette()), UI::WarningColor(DarkPalette()));
  EXPECT_NE(UI::DangerColor(LightPalette()), UI::DangerColor(DarkPalette()));
}

TEST(UIStyleTest, MixedColorsAreOpaque) {
  // A label drawing through rich text drops an alpha channel, so these have
  // to be mixed towards the background rather than made translucent.
  for (const auto& p : {LightPalette(), DarkPalette()}) {
    EXPECT_EQ(UI::MutedTextColor(p).alpha(), 255);
    EXPECT_EQ(UI::BorderColor(p).alpha(), 255);
  }
}

TEST(UIStyleTest, MutedTextStaysNearerTheTextThanTheBorderDoes) {
  // Both sit between the text and the window; the caption colour has to stay
  // readable, while the hairline should barely register.
  for (const auto& p : {LightPalette(), DarkPalette()}) {
    const auto text = p.color(QPalette::WindowText);
    const auto muted = UI::MutedTextColor(p);
    const auto border = UI::BorderColor(p);
    EXPECT_LT(std::abs(muted.lightness() - text.lightness()),
              std::abs(border.lightness() - text.lightness()));
  }
}

}  // namespace GpgFrontend::Test
