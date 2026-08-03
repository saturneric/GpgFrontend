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
#include "ui/widgets/KeyTableEmptyState.h"

namespace GpgFrontend::Test {

namespace {

using UI::ClassifyKeyTableEmptyState;
using UI::DescribeKeyTableEmptyState;
using UI::KeyTableEmptyReason;

}  // namespace

TEST(KeyTableEmptyStateTest, RowsOnScreenMeanNoMessageAtAll) {
  // Whatever else is true, if the user can see keys there is nothing to
  // explain.
  for (const bool keyword : {false, true}) {
    for (const bool category : {false, true}) {
      for (const int source_rows : {0, 10}) {
        EXPECT_EQ(ClassifyKeyTableEmptyState(3, source_rows, keyword, category),
                  KeyTableEmptyReason::kNotEmpty);
      }
    }
  }
}

TEST(KeyTableEmptyStateTest, AnEmptyKeyringOutranksEveryOtherExplanation) {
  // Telling someone their search found nothing is useless when they have no
  // keys to search; generating one is the only thing that helps.
  for (const bool keyword : {false, true}) {
    for (const bool category : {false, true}) {
      EXPECT_EQ(ClassifyKeyTableEmptyState(0, 0, keyword, category),
                KeyTableEmptyReason::kKeyringEmpty);
    }
  }
}

TEST(KeyTableEmptyStateTest, SearchBeatsCategoryWhenBothAreActive) {
  // The search is what the user just did; the category is where they already
  // were.
  EXPECT_EQ(ClassifyKeyTableEmptyState(0, 10, /*keyword=*/true,
                                       /*category=*/true),
            KeyTableEmptyReason::kNoSearchMatch);
}

TEST(KeyTableEmptyStateTest, EmptyCategoryIsDistinguishedFromABuiltinTab) {
  EXPECT_EQ(ClassifyKeyTableEmptyState(0, 10, /*keyword=*/false,
                                       /*category=*/true),
            KeyTableEmptyReason::kCategoryEmpty);

  // A healthy keyring showing nothing under Revoked / Expired / Disabled.
  EXPECT_EQ(ClassifyKeyTableEmptyState(0, 10, /*keyword=*/false,
                                       /*category=*/false),
            KeyTableEmptyReason::kFilteredOut);
}

TEST(KeyTableEmptyStateTest, NegativeCountsAreTreatedAsEmpty) {
  EXPECT_EQ(ClassifyKeyTableEmptyState(-1, -1, false, false),
            KeyTableEmptyReason::kKeyringEmpty);
}

TEST(KeyTableEmptyStateTest, OnlyTheNoMatchMessageQuotesTheSearchTerm) {
  EXPECT_TRUE(
      DescribeKeyTableEmptyState(KeyTableEmptyReason::kNoSearchMatch, "alice")
          .contains("alice"));

  for (const auto reason :
       {KeyTableEmptyReason::kKeyringEmpty, KeyTableEmptyReason::kCategoryEmpty,
        KeyTableEmptyReason::kFilteredOut}) {
    EXPECT_FALSE(DescribeKeyTableEmptyState(reason, "alice").contains("alice"));
  }
}

TEST(KeyTableEmptyStateTest, EveryReasonWorthShowingHasAMessage) {
  // A missing case would paint nothing, leaving the blank table the message
  // exists to explain.
  for (const auto reason :
       {KeyTableEmptyReason::kKeyringEmpty, KeyTableEmptyReason::kNoSearchMatch,
        KeyTableEmptyReason::kCategoryEmpty,
        KeyTableEmptyReason::kFilteredOut}) {
    EXPECT_FALSE(DescribeKeyTableEmptyState(reason, "x").isEmpty());
  }
}

TEST(KeyTableEmptyStateTest, NotEmptyHasNoMessage) {
  EXPECT_TRUE(DescribeKeyTableEmptyState(KeyTableEmptyReason::kNotEmpty, "x")
                  .isEmpty());
}

}  // namespace GpgFrontend::Test
