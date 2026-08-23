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
#include "ui/dialog/KeyGroupMetadataRules.h"

namespace GpgFrontend::Test {

namespace {

using UI::DescribeKeyGroupCreation;
using UI::DescribeKeyGroupDeletion;
using UI::DescribeKeyGroupMembership;
using UI::DescribeKeyGroupMetadataProblem;
using UI::KeyGroupMetadataProblem;
using UI::ValidateKeyGroupMetadata;

}  // namespace

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

TEST(KeyGroupMetadataRulesTest, MembershipSummaryCountsWhatIsThere) {
  EXPECT_FALSE(DescribeKeyGroupMembership(0, 0, 0).isEmpty());

  const auto many = DescribeKeyGroupMembership(8, 2, 1);
  EXPECT_TRUE(many.contains("8"));
  EXPECT_TRUE(many.contains("2"));
  EXPECT_TRUE(many.contains("1"));
}

TEST(KeyGroupMetadataRulesTest, MembershipSummarySpellsOutSingularAndPlural) {
  // With no translator loaded Qt's "%n key(s)" renders the literal "(s)", so
  // each number is spelled out instead.
  for (const auto& text : {DescribeKeyGroupMembership(1, 0, 0),
                           DescribeKeyGroupMembership(2, 1, 0),
                           DescribeKeyGroupMembership(0, 0, 1)}) {
    EXPECT_FALSE(text.contains("(s)")) << text.toStdString();
    EXPECT_FALSE(text.contains("%n")) << text.toStdString();
  }

  EXPECT_TRUE(DescribeKeyGroupMembership(1, 0, 0).contains("1 key"));
  EXPECT_TRUE(DescribeKeyGroupMembership(3, 0, 0).contains("3 keys"));
  EXPECT_TRUE(DescribeKeyGroupMembership(2, 1, 0).contains("1 nested group"));
  EXPECT_TRUE(DescribeKeyGroupMembership(2, 4, 0).contains("4 nested groups"));
}

TEST(KeyGroupMetadataRulesTest, MembershipSummaryStaysQuietAboutAbsentParts) {
  const auto no_nesting = DescribeKeyGroupMembership(3, 0, 0);
  EXPECT_FALSE(no_nesting.contains("nested"));
  EXPECT_FALSE(no_nesting.contains("keyring"));
}

TEST(KeyGroupMetadataRulesTest, CreationSummarySpellsOutTheCount) {
  EXPECT_TRUE(DescribeKeyGroupCreation(1).contains("1 checked key "));
  EXPECT_TRUE(DescribeKeyGroupCreation(4).contains("4 checked keys"));

  for (const auto n : {0, 1, 2}) {
    const auto text = DescribeKeyGroupCreation(n);
    EXPECT_FALSE(text.isEmpty());
    EXPECT_FALSE(text.contains("(s)")) << text.toStdString();
  }
}

TEST(KeyGroupMetadataRulesTest, DeletionPromptNamesTheGroup) {
  const auto text = DescribeKeyGroupDeletion("Team Alpha", {});
  EXPECT_TRUE(text.contains("Team Alpha"));
  // Nothing to warn about when no other group holds it.
  EXPECT_FALSE(text.contains("member of"));
}

TEST(KeyGroupMetadataRulesTest, DeletionPromptListsEveryParentGroup) {
  const auto text = DescribeKeyGroupDeletion("Team Alpha", {"Ops", "Release"});
  EXPECT_TRUE(text.contains("Ops"));
  EXPECT_TRUE(text.contains("Release"));
}

TEST(KeyGroupMetadataRulesTest, DeletionPromptDoesNotEscapeTheName) {
  // The caller shows this as plain text; escaping here would double up.
  const auto text = DescribeKeyGroupDeletion("a<b>c", {});
  EXPECT_TRUE(text.contains("a<b>c"));
}

}  // namespace GpgFrontend::Test
