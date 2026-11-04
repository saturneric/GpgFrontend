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
#include "core/function/openpgp/KeyGroupRepository.h"

namespace GpgFrontend::Test {

TEST(KeyGroupMetadataRulesTest, NameShorterThanFiveCharactersIsRejected) {
  EXPECT_EQ(ValidateKeyGroupMetadata("", {}),
            KeyGroupMetadataProblem::kNameTooShort);
  EXPECT_EQ(ValidateKeyGroupMetadata("abcd", {}),
            KeyGroupMetadataProblem::kNameTooShort);
  EXPECT_EQ(ValidateKeyGroupMetadata("abcde", {}),
            KeyGroupMetadataProblem::kNone);
}

TEST(KeyGroupMetadataRulesTest, WhitespaceDoesNotCountTowardsTheMinimum) {
  // Otherwise "     " would name a group, and nothing downstream would show it.
  EXPECT_EQ(ValidateKeyGroupMetadata("  a   ", {}),
            KeyGroupMetadataProblem::kNameTooShort);
  EXPECT_EQ(ValidateKeyGroupMetadata("  abcde  ", {}),
            KeyGroupMetadataProblem::kNone);
}

TEST(KeyGroupMetadataRulesTest, EmailIsOptionalButMustLookLikeOneWhenGiven) {
  EXPECT_EQ(ValidateKeyGroupMetadata("Team Alpha", ""),
            KeyGroupMetadataProblem::kNone);
  EXPECT_EQ(ValidateKeyGroupMetadata("Team Alpha", "   "),
            KeyGroupMetadataProblem::kNone);
  EXPECT_EQ(ValidateKeyGroupMetadata("Team Alpha", "team@example.com"),
            KeyGroupMetadataProblem::kNone);

  for (const auto* bad : {"no-at", "a@", "@b", "a@b@c", "a b@c.d"}) {
    EXPECT_EQ(ValidateKeyGroupMetadata("Team Alpha", bad),
              KeyGroupMetadataProblem::kEmailMalformed)
        << "accepted: " << bad;
  }
}

TEST(KeyGroupMetadataRulesTest, TheNameIsCheckedBeforeTheEmail) {
  // A user fixing one problem at a time should be told about the field they
  // are most likely still filling in.
  EXPECT_EQ(ValidateKeyGroupMetadata("ab", "no-at"),
            KeyGroupMetadataProblem::kNameTooShort);
}

TEST(KeyGroupMetadataRulesTest, EveryProblemHasSomethingToSay) {
  EXPECT_TRUE(DescribeKeyGroupMetadataProblem(KeyGroupMetadataProblem::kNone)
                  .isEmpty());

  for (const auto problem : {KeyGroupMetadataProblem::kNameTooShort,
                             KeyGroupMetadataProblem::kEmailMalformed}) {
    EXPECT_FALSE(DescribeKeyGroupMetadataProblem(problem).isEmpty());
  }
}

}  // namespace GpgFrontend::Test
