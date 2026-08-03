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
#include "ui/model/SortKeyCompare.h"

namespace GpgFrontend::Test {

// These rules live away from the proxy model precisely so they can be checked
// without a model, a view or a gpg context.

TEST(SortKeyCompareTest, NumbersCompareNumericallyNotAsText) {
  // The regression this exists for: the Subkeys column used to be compared as
  // display text, which put "10" before "2".
  EXPECT_LT(UI::CompareSortKeys(QVariant(2), QVariant(10)), 0);
  EXPECT_GT(UI::CompareSortKeys(QVariant(10), QVariant(2)), 0);
  EXPECT_EQ(UI::CompareSortKeys(QVariant(7), QVariant(7)), 0);
}

TEST(SortKeyCompareTest, MixedNumericTypesStillCompareNumerically) {
  EXPECT_LT(UI::CompareSortKeys(QVariant(2), QVariant(2.5)), 0);
  EXPECT_LT(UI::CompareSortKeys(QVariant(-1), QVariant(0U)), 0);
}

TEST(SortKeyCompareTest, DateTimesCompareChronologically) {
  const QDateTime early(QDate(2020, 1, 1), QTime(0, 0));
  const QDateTime late(QDate(2030, 6, 15), QTime(0, 0));

  EXPECT_LT(UI::CompareSortKeys(early, late), 0);
  EXPECT_GT(UI::CompareSortKeys(late, early), 0);
  EXPECT_EQ(UI::CompareSortKeys(early, early), 0);
}

TEST(SortKeyCompareTest, NeverExpiresSentinelSortsAfterEveryRealDate) {
  // How the model reports "Never": a date beyond any expiry a real key can
  // carry, so those keys gather at the end of an ascending sort instead of
  // landing wherever the alphabet puts the word.
  const QDateTime never(QDate(9999, 1, 1), QTime(0, 0));
  const QDateTime far_future(QDate(2200, 1, 1), QTime(0, 0));

  EXPECT_GT(UI::CompareSortKeys(never, far_future), 0);
}

TEST(SortKeyCompareTest, StringsCompareLocaleAware) {
  EXPECT_LT(UI::CompareSortKeys(QString("alice"), QString("bob")), 0);
  EXPECT_GT(UI::CompareSortKeys(QString("bob"), QString("alice")), 0);
  EXPECT_EQ(UI::CompareSortKeys(QString("same"), QString("same")), 0);
}

TEST(SortKeyCompareTest, InvalidSortsAfterEveryValidValue) {
  // An invalid value means the column had nothing to offer for that row, which
  // is not the same as an empty string and must not sort with the empties.
  EXPECT_LT(UI::CompareSortKeys(QVariant(1), QVariant()), 0);
  EXPECT_GT(UI::CompareSortKeys(QVariant(), QVariant(1)), 0);
  EXPECT_LT(UI::CompareSortKeys(QString(""), QVariant()), 0);
}

TEST(SortKeyCompareTest, TwoInvalidsAreEquivalent) {
  EXPECT_EQ(UI::CompareSortKeys(QVariant(), QVariant()), 0);
}

TEST(SortKeyCompareTest, ComparisonIsAntisymmetric) {
  const QVector<QVariant> values = {QVariant(),
                                    QVariant(0),
                                    QVariant(42),
                                    QVariant(QString("alice")),
                                    QVariant(QString("zoe")),
                                    QVariant(QDateTime(QDate(2020, 1, 1), {}))};

  // A comparator that is not antisymmetric makes std::sort undefined, and the
  // failure shows up as a crash far from here.
  for (const auto& a : values) {
    for (const auto& b : values) {
      const auto forward = UI::CompareSortKeys(a, b);
      const auto backward = UI::CompareSortKeys(b, a);
      if (forward == 0) {
        EXPECT_EQ(backward, 0);
      } else {
        EXPECT_EQ(forward < 0, backward > 0);
      }
    }
  }
}

}  // namespace GpgFrontend::Test
