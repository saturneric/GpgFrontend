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
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

// A key is routinely several of these at once — a revoked key is usually also
// expired — so the interesting part is not "does it detect revoked" but "which
// one wins". These tests pin the precedence down, because the row tint and the
// Status column both read it and a change here would silently make them
// disagree with what users have seen for years.

TEST(KeyStatusTest, DisabledOutranksEverythingElse) {
  for (const bool revoked : {false, true}) {
    for (const bool expired : {false, true}) {
      for (const bool soon : {false, true}) {
        EXPECT_EQ(ClassifyKeyStatus(revoked, /*disabled=*/true, expired, soon),
                  GpgKeyStatus::kDisabled)
            << "revoked=" << revoked << " expired=" << expired
            << " soon=" << soon;
      }
    }
  }
}

TEST(KeyStatusTest, RevokedOutranksExpired) {
  EXPECT_EQ(ClassifyKeyStatus(/*revoked=*/true, /*disabled=*/false,
                              /*expired=*/true, /*expiring_soon=*/false),
            GpgKeyStatus::kRevoked);
}

TEST(KeyStatusTest, ExpiredOutranksExpiringSoon) {
  // Not a combination gpg produces — IsKeyExpiringSoon() already excludes
  // expired keys — but the classifier must not depend on its caller for that.
  EXPECT_EQ(ClassifyKeyStatus(/*revoked=*/false, /*disabled=*/false,
                              /*expired=*/true, /*expiring_soon=*/true),
            GpgKeyStatus::kExpired);
}

TEST(KeyStatusTest, EachConditionAloneClassifiesAsItself) {
  EXPECT_EQ(ClassifyKeyStatus(false, false, false, false), GpgKeyStatus::kOk);
  EXPECT_EQ(ClassifyKeyStatus(false, false, false, true),
            GpgKeyStatus::kExpiringSoon);
  EXPECT_EQ(ClassifyKeyStatus(false, false, true, false),
            GpgKeyStatus::kExpired);
  EXPECT_EQ(ClassifyKeyStatus(true, false, false, false),
            GpgKeyStatus::kRevoked);
  EXPECT_EQ(ClassifyKeyStatus(false, true, false, false),
            GpgKeyStatus::kDisabled);
}

TEST(KeyStatusTest, SortRankIsStrictlyOrderedBySeverity) {
  const QVector<GpgKeyStatus> ascending = {
      GpgKeyStatus::kOk, GpgKeyStatus::kExpiringSoon, GpgKeyStatus::kExpired,
      GpgKeyStatus::kRevoked, GpgKeyStatus::kDisabled};

  for (int i = 1; i < ascending.size(); ++i) {
    EXPECT_LT(KeyStatusSortRank(ascending[i - 1]),
              KeyStatusSortRank(ascending[i]))
        << "at index " << i;
  }
  EXPECT_EQ(KeyStatusSortRank(GpgKeyStatus::kOk), 0);
}

TEST(KeyStatusTest, EveryStatusHasALabel) {
  // A missing case would return an empty string and leave the Status column
  // blank for exactly the keys that need it most.
  for (const auto status :
       {GpgKeyStatus::kOk, GpgKeyStatus::kExpiringSoon, GpgKeyStatus::kExpired,
        GpgKeyStatus::kRevoked, GpgKeyStatus::kDisabled}) {
    EXPECT_FALSE(DescribeKeyStatus(status).isEmpty())
        << "status rank " << KeyStatusSortRank(status);
  }
}

TEST(KeyStatusTest, GetKeyStatusOfNullKeyIsOk) {
  EXPECT_EQ(GetKeyStatus(nullptr), GpgKeyStatus::kOk);
}

// A key group shows one trust value for all its members, "*" when they
// disagree. Sorting needs the same reduction as a number.

TEST(OwnerTrustRankTest, UnanimousMembersRankAtTheirSharedLevel) {
  EXPECT_EQ(AggregateOwnerTrustRank({4, 4, 4}), 4);
  EXPECT_EQ(AggregateOwnerTrustRank({0}), 0);
}

TEST(OwnerTrustRankTest, DisagreeingMembersRankBelowEveryRealLevel) {
  // "*" in the column: the group as a whole tells you nothing, so it must not
  // sort as though it were trusted at its highest member's level.
  EXPECT_EQ(AggregateOwnerTrustRank({5, 2}), -1);
  EXPECT_LT(AggregateOwnerTrustRank({5, 2}), AggregateOwnerTrustRank({0}));
}

TEST(OwnerTrustRankTest, EmptyGroupRanksBelowEveryRealLevel) {
  EXPECT_EQ(AggregateOwnerTrustRank({}), -1);
}

}  // namespace GpgFrontend::Test
