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
#include "ui/widgets/CopyableLabel.h"

namespace GpgFrontend::Test {

namespace {

constexpr auto kV4Fingerprint = "A5AAEDDC3C785231263D61E662C66B5141580C08";
constexpr auto kV6Fingerprint =
    "CB186C4F0609A697E4D52DFA6C722B0C1F1E27C18A56708F6525EC27BAD9ACC9";

}  // namespace

// Only the formatting is tested here: the label is a QWidget, and test bodies
// run off the GUI thread.

TEST(CopyableLabelTest, AV4FingerprintStaysOnOneLine) {
  const auto text = UI::FingerprintDisplayText(kV4Fingerprint);

  EXPECT_FALSE(text.contains('\n'));
  EXPECT_EQ(text.split(' ').size(), 8);
}

TEST(CopyableLabelTest, AV6FingerprintBreaksIntoTwoBalancedLines) {
  const auto lines = UI::FingerprintDisplayText(kV6Fingerprint).split('\n');

  ASSERT_EQ(lines.size(), 2);
  EXPECT_EQ(lines[0].split(' ').size(), 7);
  EXPECT_EQ(lines[1].split(' ').size(), 6);
  for (const auto& line : lines) {
    EXPECT_EQ(line, line.trimmed());
  }
}

TEST(CopyableLabelTest, TheDisplayTextKeepsEveryDigit) {
  for (const QString fpr : {kV4Fingerprint, kV6Fingerprint}) {
    auto text = UI::FingerprintDisplayText(fpr);
    text.remove(QRegularExpression(u"\\s"_s));
    EXPECT_EQ(text, fpr);
  }
}

}  // namespace GpgFrontend::Test
