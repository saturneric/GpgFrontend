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

// When the staged output of a file or archive operation may be committed.
//
// A warning (status 0) must keep the output: decrypting a file that was also
// encrypted to recipients whose keys you lack used to delete the decrypted
// file, while the same message decrypted as text was shown. A failure (< 0)
// must still discard it.

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "ui/main_window/FileOperaOutcome.h"

namespace GpgFrontend::Test {

namespace {

using UI::GpgOperaResult;
using UI::IsFileOperaSuccessful;

auto Result(int status) -> GpgOperaResult {
  GpgOpResultInfo info;
  info.status = status;
  return GpgOperaResult(info);
}

}  // namespace

TEST(FileOperaOutcomeTest, NoResultsAreNotASuccess) {
  EXPECT_FALSE(IsFileOperaSuccessful({}, 0));
}

TEST(FileOperaOutcomeTest, SuccessIsCommitted) {
  EXPECT_TRUE(IsFileOperaSuccessful({Result(1)}, 1));
  EXPECT_TRUE(IsFileOperaSuccessful({Result(1), Result(1)}, 2));
}

TEST(FileOperaOutcomeTest, AWarningIsCommitted) {
  EXPECT_TRUE(IsFileOperaSuccessful({Result(0)}, 1));
  EXPECT_TRUE(IsFileOperaSuccessful({Result(1), Result(0)}, 2));
}

TEST(FileOperaOutcomeTest, AFailureIsDiscarded) {
  EXPECT_FALSE(IsFileOperaSuccessful({Result(-1)}, 1));
}

TEST(FileOperaOutcomeTest, AnyFailureAmongWarningsIsDiscarded) {
  EXPECT_FALSE(IsFileOperaSuccessful({Result(0), Result(-1)}, 2));
  EXPECT_FALSE(IsFileOperaSuccessful({Result(-1), Result(0)}, 2));
}

TEST(FileOperaOutcomeTest, AnyFailureAmongSuccessesIsDiscarded) {
  EXPECT_FALSE(IsFileOperaSuccessful({Result(1), Result(-1), Result(1)}, 3));
}

TEST(FileOperaOutcomeTest, AMissingResultIsAFailure) {
  // An operation that has not reported yet may still be writing into its
  // staging directory. Committing or cleaning up around it is not safe, so a
  // batch with fewer results than operations is never a success.
  EXPECT_FALSE(IsFileOperaSuccessful({Result(1)}, 2));
  EXPECT_FALSE(IsFileOperaSuccessful({Result(1), Result(0)}, 3));
  EXPECT_FALSE(IsFileOperaSuccessful({}, 1));
  EXPECT_TRUE(IsFileOperaSuccessful({Result(1), Result(0)}, 2));
}

TEST(FileOperaOutcomeTest, MoreResultsThanOperationsIsAFailure) {
  EXPECT_FALSE(IsFileOperaSuccessful({Result(1), Result(1)}, 1));
}

}  // namespace GpgFrontend::Test
