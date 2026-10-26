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
#include "ui/dialog/KeyCategoryRules.h"

namespace GpgFrontend::Test {

using UI::KeyCategoryNameProblem;
using UI::KeyCategoryPresetColors;
using UI::SuggestKeyCategoryColor;
using UI::ValidateKeyCategoryName;

TEST(GFUiKeyCategoryRulesTest, BlankNameIsRejected) {
  EXPECT_EQ(ValidateKeyCategoryName({}, {}), KeyCategoryNameProblem::kEmpty);
  EXPECT_EQ(ValidateKeyCategoryName(u"   "_s, {}),
            KeyCategoryNameProblem::kEmpty);
}

TEST(GFUiKeyCategoryRulesTest, OverlongNameIsRejected) {
  const QString at_limit(UI::kMaxKeyCategoryNameLength, 'a');
  EXPECT_EQ(ValidateKeyCategoryName(at_limit, {}),
            KeyCategoryNameProblem::kNone);
  EXPECT_EQ(ValidateKeyCategoryName(at_limit + 'a', {}),
            KeyCategoryNameProblem::kTooLong);
  // Surrounding spaces do not count.
  EXPECT_EQ(ValidateKeyCategoryName(u"  "_s + at_limit + u"  "_s, {}),
            KeyCategoryNameProblem::kNone);
}

TEST(GFUiKeyCategoryRulesTest, DuplicateIgnoresCaseAndSpaces) {
  const QStringList taken{u"Work"_s, u"Family"_s};
  EXPECT_EQ(ValidateKeyCategoryName(u" work "_s, taken),
            KeyCategoryNameProblem::kDuplicate);
  EXPECT_EQ(ValidateKeyCategoryName(u"Work 2"_s, taken),
            KeyCategoryNameProblem::kNone);
}

TEST(GFUiKeyCategoryRulesTest, ProblemsHaveMessagesExceptEmpty) {
  EXPECT_TRUE(UI::DescribeKeyCategoryNameProblem(KeyCategoryNameProblem::kNone)
                  .isEmpty());
  // The dialog only disables its button for a blank name; nagging about it
  // would read as a complaint about opening the dialog.
  EXPECT_TRUE(UI::DescribeKeyCategoryNameProblem(KeyCategoryNameProblem::kEmpty)
                  .isEmpty());
  EXPECT_FALSE(
      UI::DescribeKeyCategoryNameProblem(KeyCategoryNameProblem::kTooLong)
          .isEmpty());
  EXPECT_FALSE(
      UI::DescribeKeyCategoryNameProblem(KeyCategoryNameProblem::kDuplicate)
          .isEmpty());
}

TEST(GFUiKeyCategoryRulesTest, PresetsAreValidDistinctAndNamed) {
  const auto presets = KeyCategoryPresetColors();
  QSet<QRgb> seen;
  for (const auto& preset : presets) {
    EXPECT_TRUE(preset.color.isValid());
    EXPECT_FALSE(preset.name.isEmpty());
    seen.insert(preset.color.rgb());
  }
  EXPECT_EQ(seen.size(), static_cast<qsizetype>(presets.size()));
}

TEST(GFUiKeyCategoryRulesTest, SuggestionSkipsUsedColors) {
  const auto presets = KeyCategoryPresetColors();
  EXPECT_EQ(SuggestKeyCategoryColor({}), presets.at(0).color);
  EXPECT_EQ(SuggestKeyCategoryColor({presets.at(0).color, QColor("#123456")}),
            presets.at(1).color);
}

TEST(GFUiKeyCategoryRulesTest, SuggestionAvoidsBuiltinTabColors) {
  // The built-in tabs' defaults (KeyList::resolve_category_color): blue,
  // emerald, violet, cyan, rose, red, orange and three greys.
  const QList<QColor> builtin{QColor("#3B82F6"), QColor("#10B981"),
                              QColor("#8B5CF6"), QColor("#06B6D4"),
                              QColor("#F43F5E"), QColor("#64748B"),
                              QColor("#9CA3AF"), QColor("#EF4444"),
                              QColor("#F97316"), QColor("#71717A")};
  const auto suggested = SuggestKeyCategoryColor(builtin);
  for (const auto& color : builtin) EXPECT_NE(suggested.rgb(), color.rgb());
  // Red and orange are taken; amber is the first preset clear of them all.
  EXPECT_EQ(suggested, QColor("#F59E0B"));
}

TEST(GFUiKeyCategoryRulesTest, SuggestionAvoidsNearlyTheSameHue) {
  const auto presets = KeyCategoryPresetColors();
  // A red the user picked that is not exactly the preset still takes it.
  EXPECT_EQ(SuggestKeyCategoryColor({QColor("#EE4545")}), presets.at(1).color);
  // A reddish grey has no hue to collide with.
  EXPECT_EQ(SuggestKeyCategoryColor({QColor("#8B7474")}), presets.at(0).color);
}

TEST(GFUiKeyCategoryRulesTest, SuggestionCyclesOnceAllAreUsed) {
  const auto presets = KeyCategoryPresetColors();
  QList<QColor> used;
  for (const auto& preset : presets) used.append(preset.color);
  EXPECT_EQ(SuggestKeyCategoryColor(used), presets.at(0).color);
  used.append(presets.at(0).color);
  EXPECT_EQ(SuggestKeyCategoryColor(used), presets.at(1).color);
}

}  // namespace GpgFrontend::Test
