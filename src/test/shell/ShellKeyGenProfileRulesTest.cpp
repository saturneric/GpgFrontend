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

#include "shell/rules/KeyGenProfileRules.h"

namespace GpgFrontend::Test {

using Shell::CheckKeyGenProfileAction;
using Shell::DescribeKeyGenProfileRefusal;
using Shell::KeyGenProfileAction;
using Shell::KeyGenProfileRefusal;
using Shell::KeyGenProfileState;

TEST(ShellKeyGenProfileRulesTest, TheDefaultCanBeNeitherDeletedNorHidden) {
  const KeyGenProfileState state{.is_default = true};
  EXPECT_EQ(CheckKeyGenProfileAction(state, KeyGenProfileAction::kDelete),
            KeyGenProfileRefusal::kIsDefault);
  EXPECT_EQ(CheckKeyGenProfileAction(state, KeyGenProfileAction::kToggleHidden),
            KeyGenProfileRefusal::kIsDefault);
  EXPECT_EQ(CheckKeyGenProfileAction(state, KeyGenProfileAction::kMakeDefault),
            KeyGenProfileRefusal::kAlreadyDefault);
  EXPECT_EQ(CheckKeyGenProfileAction(state, KeyGenProfileAction::kEdit),
            KeyGenProfileRefusal::kNone);
}

TEST(ShellKeyGenProfileRulesTest, AnotherProfileCanBeDeletedAndHidden) {
  const KeyGenProfileState state{};
  for (const auto action :
       {KeyGenProfileAction::kEdit, KeyGenProfileAction::kDelete,
        KeyGenProfileAction::kToggleHidden,
        KeyGenProfileAction::kMakeDefault}) {
    EXPECT_EQ(CheckKeyGenProfileAction(state, action),
              KeyGenProfileRefusal::kNone);
  }
}

TEST(ShellKeyGenProfileRulesTest, AHiddenDefaultCanBeShownAgain) {
  // A stored default the user hid is shown again on request; only hiding
  // the default is refused.
  const KeyGenProfileState state{.is_default = true, .hidden = true};
  EXPECT_EQ(CheckKeyGenProfileAction(state, KeyGenProfileAction::kToggleHidden),
            KeyGenProfileRefusal::kNone);
}

TEST(ShellKeyGenProfileRulesTest, OnlyAnOfferedProfileCanBeTheDefault) {
  EXPECT_EQ(CheckKeyGenProfileAction({.hidden = true},
                                     KeyGenProfileAction::kMakeDefault),
            KeyGenProfileRefusal::kNotOffered);
  EXPECT_EQ(CheckKeyGenProfileAction({.available = false},
                                     KeyGenProfileAction::kMakeDefault),
            KeyGenProfileRefusal::kNotOffered);
}

TEST(ShellKeyGenProfileRulesTest, AProfileThatCannotBeMadeCannotBeEdited) {
  const KeyGenProfileState state{.available = false};
  EXPECT_EQ(CheckKeyGenProfileAction(state, KeyGenProfileAction::kEdit),
            KeyGenProfileRefusal::kUnavailable);
  // Everything else still works on it.
  EXPECT_EQ(CheckKeyGenProfileAction(state, KeyGenProfileAction::kDelete),
            KeyGenProfileRefusal::kNone);
  EXPECT_EQ(CheckKeyGenProfileAction(state, KeyGenProfileAction::kToggleHidden),
            KeyGenProfileRefusal::kNone);
}

TEST(ShellKeyGenProfileRulesTest, EveryRefusalIsSaidAndAllowingIsNot) {
  EXPECT_TRUE(
      DescribeKeyGenProfileRefusal(KeyGenProfileRefusal::kNone).isEmpty());
  for (const auto refusal :
       {KeyGenProfileRefusal::kIsDefault, KeyGenProfileRefusal::kAlreadyDefault,
        KeyGenProfileRefusal::kNotOffered,
        KeyGenProfileRefusal::kUnavailable}) {
    EXPECT_FALSE(DescribeKeyGenProfileRefusal(refusal).isEmpty());
  }
}

}  // namespace GpgFrontend::Test
