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

#include "shell/rules/FileOutputRules.h"

namespace GpgFrontend::Test {

using Shell::FileOutputFacts;
using Shell::KeepsFileOutput;

TEST(ShellFileOutputRulesTest, AFileWhoseSignatureFailsStillDecrypted) {
  // The operation as a whole failed, but making the output did not: it is
  // kept, as the desktop keeps it, and the failure is reported beside it.
  EXPECT_TRUE(KeepsFileOutput(
      {.releasable = true, .exists = true, .size = 12, .may_be_empty = true}));
}

TEST(ShellFileOutputRulesTest, AnOutputThatWasNotMadeIsNotKept) {
  EXPECT_FALSE(
      KeepsFileOutput({.releasable = false, .exists = true, .size = 12}));
  EXPECT_FALSE(KeepsFileOutput({.releasable = true, .exists = false}));
  EXPECT_FALSE(KeepsFileOutput(
      {.cancelled = true, .releasable = true, .exists = true, .size = 12}));
}

TEST(ShellFileOutputRulesTest, OnlyDecryptingMayWriteNothing) {
  // An empty file decrypts to nothing; encrypting or signing never does.
  EXPECT_TRUE(KeepsFileOutput(
      {.releasable = true, .exists = true, .size = 0, .may_be_empty = true}));
  EXPECT_FALSE(
      KeepsFileOutput({.releasable = true, .exists = true, .size = 0}));
}

}  // namespace GpgFrontend::Test
