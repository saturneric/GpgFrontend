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
#include "ui/widgets/SecretEntryState.h"

namespace GpgFrontend::Test {

namespace {

using UI::EvaluateSecretEntry;
using UI::SecretEntryInput;
using UI::SecretEntryProblem;

/// Entering a secret that already exists: one field, no floor.
auto Unlocking() -> SecretEntryInput {
  SecretEntryInput in;
  in.ask_current = true;
  in.ask_new = false;
  return in;
}

/// Choosing a new secret: a field, a confirmation, and the floor.
auto Choosing(int length, bool matches) -> SecretEntryInput {
  SecretEntryInput in;
  in.ask_new = true;
  in.min_length = 8;
  in.secret_length = length;
  in.confirm_length = matches ? length : length + 1;
  in.confirm_matches = matches;
  return in;
}

}  // namespace

TEST(SecretEntryStateTest, AnUntouchedFormIsNotYetAMistake) {
  // The message row carries a dimmed hint while it has nothing to correct.
  // Reporting a problem against an empty form would replace that guidance with
  // a complaint before the user has done anything at all.
  const auto unlock = EvaluateSecretEntry(Unlocking());
  EXPECT_FALSE(unlock.acceptable);
  EXPECT_EQ(unlock.problem, SecretEntryProblem::kEmpty);

  SecretEntryInput setting;
  setting.ask_new = true;
  const auto set = EvaluateSecretEntry(setting);
  EXPECT_FALSE(set.acceptable);
  EXPECT_EQ(set.problem, SecretEntryProblem::kEmpty);
}

TEST(SecretEntryStateTest, UnlockAcceptsAnySingleCharacter) {
  // The floor governs choosing a secret, never entering one that exists. A
  // profile file sealed with four characters must stay openable, and the
  // application key's own PIN was accepted under whatever rule applied then.
  auto in = Unlocking();
  in.current_length = 1;

  const auto state = EvaluateSecretEntry(in);
  EXPECT_TRUE(state.acceptable);
  EXPECT_EQ(state.problem, SecretEntryProblem::kNone);
}

TEST(SecretEntryStateTest, ChoosingASecretRequiresTheFloor) {
  for (int length = 1; length < 8; ++length) {
    const auto state = EvaluateSecretEntry(Choosing(length, true));
    EXPECT_FALSE(state.acceptable) << "length " << length;
    EXPECT_EQ(state.problem, SecretEntryProblem::kTooShort)
        << "length " << length;
  }

  const auto at_floor = EvaluateSecretEntry(Choosing(8, true));
  EXPECT_TRUE(at_floor.acceptable);
  EXPECT_EQ(at_floor.problem, SecretEntryProblem::kNone);
}

TEST(SecretEntryStateTest, ACallerMayLowerTheFloor) {
  // The write-back prompt re-seals a file somebody already sealed, and the
  // process asking does not hold the old passphrase. Enforcing eight there
  // would make a file sealed with four impossible to write back without
  // silently giving it a new passphrase.
  auto in = Choosing(4, true);
  in.min_length = 1;

  const auto state = EvaluateSecretEntry(in);
  EXPECT_TRUE(state.acceptable);
  EXPECT_EQ(state.problem, SecretEntryProblem::kNone);
}

TEST(SecretEntryStateTest, MismatchWaitsForSomethingToCompareAgainst) {
  // Telling someone their two entries differ before they have finished typing
  // the second one is noise, not help.
  auto typing = Choosing(10, false);
  typing.confirm_length = 0;

  const auto quiet = EvaluateSecretEntry(typing);
  EXPECT_FALSE(quiet.acceptable);
  EXPECT_NE(quiet.problem, SecretEntryProblem::kMismatch);

  typing.confirm_length = 3;
  const auto loud = EvaluateSecretEntry(typing);
  EXPECT_FALSE(loud.acceptable);
  EXPECT_EQ(loud.problem, SecretEntryProblem::kMismatch);
}

TEST(SecretEntryStateTest, TooShortOutranksMismatch) {
  // Naming the deeper problem first stops the user fixing the wrong one:
  // making the two entries agree at three characters achieves nothing.
  const auto state = EvaluateSecretEntry(Choosing(3, false));
  EXPECT_FALSE(state.acceptable);
  EXPECT_EQ(state.problem, SecretEntryProblem::kTooShort);
}

TEST(SecretEntryStateTest, ChangingAlsoNeedsTheCurrentSecret) {
  auto in = Choosing(10, true);
  in.ask_current = true;

  const auto without = EvaluateSecretEntry(in);
  EXPECT_FALSE(without.acceptable);
  EXPECT_EQ(without.problem, SecretEntryProblem::kCurrentMissing);

  in.current_length = 1;
  const auto with = EvaluateSecretEntry(in);
  EXPECT_TRUE(with.acceptable);
  EXPECT_EQ(with.problem, SecretEntryProblem::kNone);
}

TEST(SecretEntryStateTest, NegativeLengthsAreTreatedAsEmpty) {
  // Nothing should ever hand this a negative length, but a rule that answers
  // an impossible input with an impossible verdict is harder to reason about
  // than one that simply treats it as the empty field it stands for.
  SecretEntryInput in;
  in.ask_new = true;
  in.secret_length = -1;
  in.confirm_length = -4;

  const auto state = EvaluateSecretEntry(in);
  EXPECT_FALSE(state.acceptable);
  EXPECT_EQ(state.problem, SecretEntryProblem::kEmpty);
}

}  // namespace GpgFrontend::Test
