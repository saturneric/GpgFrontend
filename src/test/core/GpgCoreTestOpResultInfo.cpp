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

// How an operation's recorded facts combine: the operation identity, the
// worst status, and every error, without inventing or losing any.

#include <gtest/gtest.h>

#include "core/function/result_analyse/GpgOpResultInfo.h"

namespace GpgFrontend::Test {

namespace {

auto EngineError(GpgOperation scope, unsigned int code, const QString& text,
                 const QString& detail = {}) -> GpgOpStageError {
  GpgOpStageError e;
  e.scope = scope;
  e.origin = GpgErrorOrigin::kENGINE;
  e.code = code;
  e.codeText = text;
  if (!detail.isEmpty()) e.engineDetails.append({scope, detail});
  return e;
}

auto Info(GpgOperation op, int status, QList<GpgOpStageError> errors = {})
    -> GpgOpResultInfo {
  GpgOpResultInfo info;
  info.op = op;
  info.operation = GpgOperationName(op);
  info.status = status;
  info.stageErrors = std::move(errors);
  return info;
}

}  // namespace

TEST(GpgOpResultInfoTest, OperationsCombineOnlyAsOneCall) {
  using Op = GpgOperation;
  EXPECT_EQ(CombineGpgOperations(Op::kENCRYPT, Op::kSIGN), Op::kENCRYPT_SIGN);
  EXPECT_EQ(CombineGpgOperations(Op::kSIGN, Op::kENCRYPT), Op::kENCRYPT_SIGN);
  EXPECT_EQ(CombineGpgOperations(Op::kDECRYPT, Op::kVERIFY),
            Op::kDECRYPT_VERIFY);
  EXPECT_EQ(CombineGpgOperations(Op::kVERIFY, Op::kDECRYPT),
            Op::kDECRYPT_VERIFY);
  EXPECT_EQ(CombineGpgOperations(Op::kSIGN, Op::kSIGN), Op::kSIGN);
  EXPECT_EQ(CombineGpgOperations(Op::kNONE, Op::kVERIFY), Op::kVERIFY);
  EXPECT_EQ(CombineGpgOperations(Op::kDECRYPT, Op::kNONE), Op::kDECRYPT);
  EXPECT_EQ(CombineGpgOperations(Op::kENCRYPT, Op::kDECRYPT), Op::kNONE);
  EXPECT_EQ(CombineGpgOperations(Op::kSIGN, Op::kVERIFY), Op::kNONE);
}

TEST(GpgOpResultInfoTest, CombinedNamesAreBuiltFromTheStageNames) {
  EXPECT_EQ(GpgOperationName(GpgOperation::kENCRYPT_SIGN),
            GpgOperationName(GpgOperation::kENCRYPT) + u" + "_s +
                GpgOperationName(GpgOperation::kSIGN));
  EXPECT_TRUE(GpgOperationName(GpgOperation::kNONE).isEmpty());
}

TEST(GpgOpResultInfoTest, MergeKeepsTheWorstStatusAndEveryError) {
  auto info = Info(GpgOperation::kENCRYPT, 1,
                   {EngineError(GpgOperation::kENCRYPT, 1, u"a"_s)});
  info.Merge(Info(GpgOperation::kSIGN, -1,
                  {EngineError(GpgOperation::kSIGN, 2, u"b"_s)}));

  EXPECT_EQ(info.status, -1);
  EXPECT_EQ(info.op, GpgOperation::kENCRYPT_SIGN);
  EXPECT_EQ(info.operation, GpgOperationName(GpgOperation::kENCRYPT_SIGN));
  ASSERT_EQ(info.stageErrors.size(), 2);
  EXPECT_EQ(info.stageErrors[0].scope, GpgOperation::kENCRYPT);
  EXPECT_EQ(info.stageErrors[1].scope, GpgOperation::kSIGN);
}

TEST(GpgOpResultInfoTest, MergingUnrelatedOperationsHasNoIdentity) {
  auto info = Info(GpgOperation::kENCRYPT, 1);
  info.Merge(Info(GpgOperation::kVERIFY, 1));
  EXPECT_EQ(info.op, GpgOperation::kNONE);
}

TEST(GpgOpResultInfoTest, ASharedCallErrorBecomesOneErrorOfTheCall) {
  // Both analysers of one Decrypt + Verify call were handed the call's
  // error: that is one failure, and each stage's own detail stays.
  auto info = Info(GpgOperation::kDECRYPT, -1,
                   {EngineError(GpgOperation::kDECRYPT, 11, u"Bad passphrase"_s,
                                u"decrypt detail"_s)});
  info.Merge(Info(GpgOperation::kVERIFY, -1,
                  {EngineError(GpgOperation::kVERIFY, 11, u"Bad passphrase"_s,
                               u"verify detail"_s)}));

  info.CollapseSharedCallError(11, GpgOperation::kDECRYPT_VERIFY);

  ASSERT_EQ(info.stageErrors.size(), 1);
  const auto& e = info.stageErrors.first();
  EXPECT_EQ(e.scope, GpgOperation::kDECRYPT_VERIFY);
  EXPECT_EQ(e.code, 11U);
  EXPECT_EQ(e.codeText, u"Bad passphrase"_s);
  ASSERT_EQ(e.engineDetails.size(), 2);
  EXPECT_EQ(e.engineDetails[0],
            qMakePair(GpgOperation::kDECRYPT, u"decrypt detail"_s));
  EXPECT_EQ(e.engineDetails[1],
            qMakePair(GpgOperation::kVERIFY, u"verify detail"_s));
}

TEST(GpgOpResultInfoTest, CollapsingLeavesOtherErrorsWithTheirStage) {
  auto info = Info(GpgOperation::kENCRYPT_SIGN, -1,
                   {EngineError(GpgOperation::kENCRYPT, 11, u"call"_s),
                    EngineError(GpgOperation::kSIGN, 11, u"call"_s),
                    EngineError(GpgOperation::kSIGN, 54, u"own"_s)});
  GpgOpStageError app;
  app.origin = GpgErrorOrigin::kAPPLICATION;
  app.code = 11;
  info.stageErrors.append(app);

  info.CollapseSharedCallError(11, GpgOperation::kENCRYPT_SIGN);

  ASSERT_EQ(info.stageErrors.size(), 3);
  EXPECT_EQ(info.stageErrors[0].scope, GpgOperation::kENCRYPT_SIGN);
  EXPECT_EQ(info.stageErrors[1].scope, GpgOperation::kSIGN);
  EXPECT_EQ(info.stageErrors[1].code, 54U);
  // GpgFrontend's own finding is not an engine error and is not merged in.
  EXPECT_EQ(info.stageErrors[2].origin, GpgErrorOrigin::kAPPLICATION);
}

TEST(GpgOpResultInfoTest, NoCallErrorCollapsesNothing) {
  auto info = Info(GpgOperation::kSIGN, -1,
                   {EngineError(GpgOperation::kSIGN, 54, u"own"_s)});
  info.CollapseSharedCallError(0, GpgOperation::kENCRYPT_SIGN);
  ASSERT_EQ(info.stageErrors.size(), 1);
  EXPECT_EQ(info.stageErrors[0].scope, GpgOperation::kSIGN);
}

}  // namespace GpgFrontend::Test
