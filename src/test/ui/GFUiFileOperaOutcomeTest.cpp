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

using UI::AssignFileBatchIdentity;
using UI::GpgOperaResult;
using UI::IsFileOperaSuccessful;
using UI::OutputAction;
using UI::OutputFate;
using UI::RecordCommittedOutputs;
using UI::RecordDiscardedOutputs;

auto Result(int status) -> GpgOperaResult {
  GpgOpResultInfo info;
  info.status = status;
  return GpgOperaResult(info);
}

auto Staged(int status, const QString& staged) -> GpgOperaResult {
  auto r = Result(status);
  r.staged_output_path = staged;
  // Per-category indices repeat across categories; the batch rewrites them.
  r.input_index = 0;
  return r;
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

TEST(FileOperaOutcomeTest, BatchIdentityComesFromTheStagedOutput) {
  // A file and a directory are both index 0 of their own category.
  QList<GpgOperaResult> results{Staged(1, u"/w/dir.tar.gpg"_s),
                                Staged(1, u"/w/a.txt.gpg"_s)};
  const auto ctx = AssignFileBatchIdentity(
      results, {u"/w/a.txt.gpg"_s, u"/w/dir.tar.gpg"_s, u"/w/b.txt.gpg"_s},
      {u"/in/a.txt"_s, u"/in/dir"_s, u"/in/b.txt"_s});

  EXPECT_EQ(results[0].input_index, 1);
  EXPECT_EQ(results[1].input_index, 0);
  EXPECT_TRUE(ctx.file_target);
  ASSERT_EQ(ctx.expected.size(), 3);
  EXPECT_EQ(ctx.expected.value(2).full, u"/in/b.txt"_s);
  EXPECT_NE(ctx.expected.value(0).id, ctx.expected.value(1).id);
}

TEST(FileOperaOutcomeTest, OnlyAConfirmedRemovalCountsAsDiscarded) {
  QList<GpgOperaResult> results{Staged(1, u"/w/a"_s), Staged(-1, u"/w/b"_s),
                                Staged(1, u"/w/c"_s)};
  const QStringList staged{u"/w/a"_s, u"/w/b"_s, u"/w/c"_s};
  auto ctx = AssignFileBatchIdentity(results, staged,
                                     {u"/in/a"_s, u"/in/b"_s, u"/in/c"_s});
  RecordDiscardedOutputs(ctx, results, staged, {true, true, false});

  EXPECT_EQ(ctx.fates.value(0), OutputFate::kSTAGED_DISCARDED);
  EXPECT_EQ(ctx.fates.value(1), OutputFate::kNOT_PRODUCED);
  EXPECT_EQ(ctx.fates.value(2), OutputFate::kDISCARD_FAILED);
  ASSERT_EQ(ctx.output_errors.value(2).size(), 1);
  EXPECT_EQ(ctx.output_errors.value(2).first().action,
            OutputAction::kDISCARD_STAGED);
  EXPECT_EQ(ctx.output_errors.value(2).first().path, u"/w/c"_s);
}

TEST(FileOperaOutcomeTest, AMissingResultGetsNoFateFromTheCleanup) {
  // Cleanup ran, but the item never reported: it may still be writing.
  QList<GpgOperaResult> results{Staged(-1, u"/w/a"_s)};
  const QStringList staged{u"/w/a"_s, u"/w/b"_s};
  auto ctx = AssignFileBatchIdentity(results, staged, {u"/in/a"_s, u"/in/b"_s});
  RecordDiscardedOutputs(ctx, results, staged, {true, true});
  EXPECT_FALSE(ctx.fates.contains(1));

  const auto report = UI::BuildReportFromResults(results, ctx);
  EXPECT_FALSE(report.summary.contains(QCoreApplication::translate(
      "GpgFrontend::UI::InfoBoardReport", "No output files were written.")));
}

TEST(FileOperaOutcomeTest, ACommitFailureMarksWhatWasAndWasNotWritten) {
  const QStringList finals{u"/o/a"_s, u"/o/b"_s, u"/o/c"_s};
  const QStringList staged{u"/w/a"_s, u"/w/b"_s, u"/w/c"_s};
  UI::InfoBoardBatchContext ctx;
  RecordCommittedOutputs(ctx, finals, 1,
                         SafeOutputCommitStep::kMOVE_EXISTING_ASIDE, staged,
                         {true, true, false});

  EXPECT_EQ(ctx.fates.value(0), OutputFate::kCOMMITTED);
  EXPECT_EQ(ctx.output_paths.value(0), u"/o/a"_s);
  EXPECT_EQ(ctx.fates.value(1), OutputFate::kCOMMIT_FAILED);
  ASSERT_FALSE(ctx.output_errors.value(1).isEmpty());
  EXPECT_EQ(ctx.output_errors.value(1).first().action,
            OutputAction::kCOMMIT_FILE);
  EXPECT_EQ(ctx.output_errors.value(1).first().path, u"/o/b"_s);
  EXPECT_FALSE(ctx.output_errors.value(1).first().message.isEmpty());
  // Never reached, and its staged copy could not be removed either.
  EXPECT_EQ(ctx.fates.value(2), OutputFate::kDISCARD_FAILED);
  EXPECT_EQ(ctx.output_errors.value(2).first().action,
            OutputAction::kDISCARD_STAGED);
}

TEST(FileOperaOutcomeTest, ASuccessfulCommitMarksEveryOutputWritten) {
  UI::InfoBoardBatchContext ctx;
  RecordCommittedOutputs(ctx, {u"/o/a"_s, u"/o/b"_s}, -1,
                         SafeOutputCommitStep::kMOVE_INTO_PLACE,
                         {u"/w/a"_s, u"/w/b"_s}, {});
  EXPECT_EQ(ctx.fates.value(0), OutputFate::kCOMMITTED);
  EXPECT_EQ(ctx.fates.value(1), OutputFate::kCOMMITTED);
  EXPECT_TRUE(ctx.output_errors.isEmpty());
}

}  // namespace GpgFrontend::Test
