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

#include "shell/rules/PassphraseEntryRules.h"

namespace GpgFrontend::Test {

using Shell::CheckPassphraseEntry;
using Shell::DescribePassphraseEntryProblem;
using Shell::PassphraseEntryProblem;

TEST(ShellPassphraseEntryRulesTest, AnOrdinaryPromptSendsWhatWasTyped) {
  // An empty answer included: whether it unlocks anything is the engine's.
  EXPECT_EQ(CheckPassphraseEntry({}, {}, false), PassphraseEntryProblem::kNone);
  EXPECT_EQ(CheckPassphraseEntry(u"a"_s, u"b"_s, false),
            PassphraseEntryProblem::kNone);
}

TEST(ShellPassphraseEntryRulesTest, ANewPassphraseIsTypedTwiceAlike) {
  EXPECT_EQ(CheckPassphraseEntry({}, {}, true), PassphraseEntryProblem::kEmpty);
  EXPECT_EQ(CheckPassphraseEntry(u"one"_s, u"two"_s, true),
            PassphraseEntryProblem::kMismatch);
  EXPECT_EQ(CheckPassphraseEntry(u"same"_s, u"same"_s, true),
            PassphraseEntryProblem::kNone);
}

TEST(ShellPassphraseEntryRulesTest, EveryProblemIsSaid) {
  EXPECT_TRUE(DescribePassphraseEntryProblem(PassphraseEntryProblem::kNone)
                  .text.isEmpty());
  for (const auto problem :
       {PassphraseEntryProblem::kEmpty, PassphraseEntryProblem::kMismatch}) {
    const auto words = DescribePassphraseEntryProblem(problem);
    EXPECT_FALSE(words.title.isEmpty());
    EXPECT_FALSE(words.text.isEmpty());
  }
}

}  // namespace GpgFrontend::Test
