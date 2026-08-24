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
#include "ui/function/LanguageOptions.h"

namespace GpgFrontend::Test {

TEST(LanguageOptionsTest, EntriesComeBackSortedByNativeName) {
  // ListLanguages() hands back a QHash, whose iteration order is not stable,
  // so an unsorted box reads differently every time it is opened.
  const QHash<QString, QString> languages{
      {"zh_CN", "简体中文"},
      {"de_DE", "Deutsch"},
      {"fr_FR", "Français"},
      {"en_US", "English (US)"},
  };

  const auto entries = UI::SortedLanguageEntries(languages);
  ASSERT_EQ(entries.size(), 4);
  for (int i = 1; i < entries.size(); ++i) {
    EXPECT_LE(entries[i - 1].second.localeAwareCompare(entries[i].second), 0)
        << entries[i - 1].second.toStdString() << " should not follow "
        << entries[i].second.toStdString();
  }
}

TEST(LanguageOptionsTest, TheSystemDefaultEntryIsNotSortedInAmongTheRest) {
  // It carries the empty key and is pinned to index 0 separately, so letting
  // it through here would put it in the middle of the list.
  const QHash<QString, QString> languages{
      {"", "System Default"},
      {"de_DE", "Deutsch"},
      {"fr_FR", "Français"},
  };

  const auto entries = UI::SortedLanguageEntries(languages);
  ASSERT_EQ(entries.size(), 2);
  for (const auto& entry : entries) {
    EXPECT_FALSE(entry.first.isEmpty());
  }
}

TEST(LanguageOptionsTest, EveryEntryKeepsItsLocaleKey) {
  // The combo box reads the choice back with currentData(), never from the
  // display text, so the key has to survive the sort.
  const QHash<QString, QString> languages{{"de_DE", "Deutsch"},
                                          {"fr_FR", "Français"}};
  const auto entries = UI::SortedLanguageEntries(languages);
  ASSERT_EQ(entries.size(), 2);
  for (const auto& entry : entries) {
    EXPECT_EQ(languages.value(entry.first), entry.second);
  }
}

TEST(LanguageOptionsTest, AnEmptyListIsHandled) {
  EXPECT_TRUE(UI::SortedLanguageEntries({}).isEmpty());
  EXPECT_TRUE(UI::SortedLanguageEntries({{"", "System Default"}}).isEmpty());
}

}  // namespace GpgFrontend::Test
