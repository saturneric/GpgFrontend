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

// The Status Panel's report model: how every posting is normalized into one
// report, what it says about each object, and what it claims about the batch.
// All of it is pure, so it runs here off the GUI thread.

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "ui/function/GpgOperaHelper.h"
#include "ui/function/InfoBoardCardConverter.h"
#include "ui/function/InfoBoardReport.h"

namespace GpgFrontend::Test {

namespace {

using UI::BuildOpInfoCards;
using UI::BuildReportFromCards;
using UI::BuildReportFromResults;
using UI::BuildReportFromText;
using UI::GpgOperaResult;
using UI::InfoBoardBatchContext;
using UI::InfoBoardCard;
using UI::InfoBoardCardKind;
using UI::InfoBoardHeadline;
using UI::InfoBoardObject;
using UI::InfoBoardOutcome;
using UI::InfoBoardOutcomeCounts;
using UI::InfoBoardReport;
using UI::InfoBoardSource;
using UI::OutputAction;
using UI::OutputFate;
using UI::OutputStageError;

auto Field(const InfoBoardCard& card, const QString& key) -> QString {
  for (const auto& f : card.fields) {
    if (f.first == key) return f.second;
  }
  return {};
}

/// Every (value, note) of the fields labelled @p key, in order.
auto Rows(const InfoBoardCard& card, const QString& key)
    -> QList<QPair<QString, QString>> {
  QList<QPair<QString, QString>> rows;
  for (qsizetype i = 0; i < card.fields.size(); ++i) {
    if (card.fields[i].first == key) {
      rows.append({card.fields[i].second, card.MetaAt(i).note});
    }
  }
  return rows;
}

auto HasFieldStartingWith(const InfoBoardCard& card, const QString& prefix)
    -> bool {
  return std::any_of(card.fields.cbegin(), card.fields.cend(),
                     [&](const QPair<QString, QString>& f) {
                       return f.first.startsWith(prefix);
                     });
}

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

auto Result(int status, GpgOperation op = GpgOperation::kENCRYPT,
            QList<GpgOpStageError> errors = {}) -> GpgOpResultInfo {
  GpgOpResultInfo info;
  info.status = status;
  info.op = op;
  info.operation = GpgOperationName(op);
  info.stageErrors = std::move(errors);
  return info;
}

auto FileResult(const GpgOpResultInfo& info, int index, const QString& path)
    -> GpgOperaResult {
  GpgOperaResult r(info, QFileInfo(path).fileName());
  r.input_index = index;
  r.source_path = path;
  return r;
}

auto DocResult(const GpgOpResultInfo& info) -> GpgOperaResult {
  GpgOperaResult r(info);
  r.input_index = 0;
  return r;
}

auto OutcomeCard(const InfoBoardObject& object) -> const InfoBoardCard& {
  return object.cards.first();
}

auto Key(const char* text) -> QString {
  return QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport", text);
}

}  // namespace

// ---------------------------------------------------------------------------
// Synthetic results keep the real error.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, ACancelWithoutAResultKeepsTheCancelCode) {
  const auto info = UI::MakeAbortedOpInfo(GpgOperation::kENCRYPT,
                                          GPG_ERR_CANCELED, u"GPG v2"_s);
  EXPECT_EQ(info.status, -1);
  EXPECT_EQ(info.op, GpgOperation::kENCRYPT);
  EXPECT_EQ(info.engine, u"GPG v2"_s);
  ASSERT_EQ(info.stageErrors.size(), 1);
  EXPECT_EQ(info.stageErrors[0].origin, GpgErrorOrigin::kENGINE);
  EXPECT_EQ(info.stageErrors[0].code,
            static_cast<unsigned int>(GPG_ERR_CANCELED));
  EXPECT_TRUE(info.stageErrors[0].engineDetails.isEmpty());
  EXPECT_FALSE(info.stageErrors[0].codeText.isEmpty());
}

TEST(InfoBoardReportTest, NotSupportedWithoutAResultKeepsItsCode) {
  const auto info =
      UI::MakeAbortedOpInfo(GpgOperation::kSIGN, GPG_ERR_NOT_SUPPORTED, {});
  ASSERT_EQ(info.stageErrors.size(), 1);
  EXPECT_EQ(info.stageErrors[0].code,
            static_cast<unsigned int>(GPG_ERR_NOT_SUPPORTED));
  EXPECT_EQ(info.stageErrors[0].origin, GpgErrorOrigin::kENGINE);
}

TEST(InfoBoardReportTest, NoCodeAndNoResultIsTheApplicationsOwnFinding) {
  const auto info =
      UI::MakeAbortedOpInfo(GpgOperation::kDECRYPT, GPG_ERR_NO_ERROR, {});
  ASSERT_EQ(info.stageErrors.size(), 1);
  EXPECT_EQ(info.stageErrors[0].origin, GpgErrorOrigin::kAPPLICATION);
  EXPECT_EQ(info.stageErrors[0].code,
            static_cast<unsigned int>(GPG_ERR_GENERAL));

  // And it is never presented as the engine's.
  const auto report = BuildReportFromResults({DocResult(info)});
  const auto& card = OutcomeCard(report.objects.first());
  EXPECT_FALSE(HasFieldStartingWith(card, Key("Engine error")));
  EXPECT_FALSE(Field(card, Key("Application error")).isEmpty());
}

TEST(InfoBoardReportTest, AnInternalTaskFailureKeepsItsCodeAsTheApplications) {
  const auto info =
      UI::MakeAbortedOpInfo(GpgOperation::kDECRYPT, GPG_ERR_USER_1, {});
  ASSERT_EQ(info.stageErrors.size(), 1);
  EXPECT_EQ(info.stageErrors[0].origin, GpgErrorOrigin::kAPPLICATION);
  EXPECT_EQ(info.stageErrors[0].code,
            static_cast<unsigned int>(GPG_ERR_USER_1));
}

TEST(InfoBoardReportTest, ACombinedCallErrorIsNotSplitIntoStages) {
  const auto info =
      UI::MakeAbortedOpInfo(GpgOperation::kENCRYPT_SIGN, GPG_ERR_CANCELED, {},
                            {{GpgOperation::kSIGN, u"pinentry closed"_s}});
  ASSERT_EQ(info.stageErrors.size(), 1);
  EXPECT_EQ(info.stageErrors[0].scope, GpgOperation::kENCRYPT_SIGN);

  const auto report = BuildReportFromResults({DocResult(info)});
  const auto& card = OutcomeCard(report.objects.first());
  const auto call =
      QCoreApplication::translate("GpgFrontend::UI::InfoBoardReport", "%1 call")
          .arg(GpgOperationName(GpgOperation::kENCRYPT_SIGN));
  // Short labels; the call and the stage are in the line under each value.
  const auto errors = Rows(card, Key("Engine error"));
  ASSERT_EQ(errors.size(), 1);
  EXPECT_TRUE(errors[0].second.contains(call));
  const auto details = Rows(card, Key("Engine detail"));
  ASSERT_EQ(details.size(), 1);
  EXPECT_EQ(details[0].first, u"pinentry closed"_s);
  EXPECT_EQ(details[0].second, GpgOperationName(GpgOperation::kSIGN));
}

// ---------------------------------------------------------------------------
// Engine outcome and task outcome.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, EngineOutcomeFollowsStatusAndRecordedCodes) {
  using UI::NormalizeEngineOutcome;
  const auto cancel = EngineError(GpgOperation::kSIGN, GPG_ERR_CANCELED, {});
  const auto unsupported =
      EngineError(GpgOperation::kSIGN, GPG_ERR_NOT_SUPPORTED, {});
  const auto general = EngineError(GpgOperation::kSIGN, GPG_ERR_GENERAL, {});

  EXPECT_EQ(NormalizeEngineOutcome(-1, {}).outcome, InfoBoardOutcome::kFAILED);
  EXPECT_EQ(NormalizeEngineOutcome(-1, {cancel}).outcome,
            InfoBoardOutcome::kCANCELLED);
  EXPECT_EQ(NormalizeEngineOutcome(-1, {unsupported}).outcome,
            InfoBoardOutcome::kNOT_SUPPORTED);
  EXPECT_EQ(NormalizeEngineOutcome(-1, {cancel, unsupported}).outcome,
            InfoBoardOutcome::kNOT_SUPPORTED);
  // A cancelled stage never hides another stage's real failure.
  EXPECT_EQ(NormalizeEngineOutcome(-1, {cancel, general}).outcome,
            InfoBoardOutcome::kFAILED);
  EXPECT_EQ(NormalizeEngineOutcome(0, {general}).outcome,
            InfoBoardOutcome::kWARNING);
  EXPECT_EQ(NormalizeEngineOutcome(1, {}).outcome,
            InfoBoardOutcome::kSUCCEEDED);

  const auto conflict = NormalizeEngineOutcome(1, {general});
  EXPECT_EQ(conflict.outcome, InfoBoardOutcome::kWARNING);
  EXPECT_TRUE(conflict.conflict);
}

TEST(InfoBoardReportTest, ApplicationErrorsDoNotMakeACancel) {
  GpgOpStageError app;
  app.origin = GpgErrorOrigin::kAPPLICATION;
  app.code = GPG_ERR_CANCELED;
  EXPECT_EQ(UI::NormalizeEngineOutcome(-1, {app}).outcome,
            InfoBoardOutcome::kFAILED);
}

TEST(InfoBoardReportTest, TaskOutcomeAccountsForTheOutput) {
  using UI::ResolveTaskOutcome;
  for (const auto engine :
       {InfoBoardOutcome::kSUCCEEDED, InfoBoardOutcome::kWARNING}) {
    EXPECT_EQ(ResolveTaskOutcome(engine, OutputFate::kAPPLY_FAILED),
              InfoBoardOutcome::kFAILED);
    EXPECT_EQ(ResolveTaskOutcome(engine, OutputFate::kCOMMIT_FAILED),
              InfoBoardOutcome::kFAILED);
    EXPECT_EQ(ResolveTaskOutcome(engine, OutputFate::kDISCARD_FAILED),
              InfoBoardOutcome::kFAILED);
    EXPECT_EQ(ResolveTaskOutcome(engine, OutputFate::kSTAGED_DISCARDED),
              InfoBoardOutcome::kNOT_SAVED);
    EXPECT_EQ(ResolveTaskOutcome(engine, OutputFate::kNOT_COMMITTED),
              InfoBoardOutcome::kNOT_SAVED);
    for (const auto fate : {OutputFate::kUNKNOWN, OutputFate::kNOT_APPLICABLE,
                            OutputFate::kNOT_PRODUCED, OutputFate::kAPPLIED,
                            OutputFate::kCOMMITTED}) {
      EXPECT_EQ(ResolveTaskOutcome(engine, fate), engine);
    }
  }
  for (const auto engine :
       {InfoBoardOutcome::kFAILED, InfoBoardOutcome::kCANCELLED,
        InfoBoardOutcome::kNOT_SUPPORTED}) {
    EXPECT_EQ(ResolveTaskOutcome(engine, OutputFate::kSTAGED_DISCARDED),
              engine);
    EXPECT_EQ(ResolveTaskOutcome(engine, OutputFate::kCOMMIT_FAILED), engine);
  }
}

// ---------------------------------------------------------------------------
// Aggregation over every combination of outcomes.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, EveryOutcomeMixHasExactlyTheSpecifiedHeadline) {
  // The specification, spelled out independently of the implementation.
  const auto expected_headline = [](int s, int w, int f, int c, int u, int n,
                                    int d) -> InfoBoardHeadline {
    const int fu = f + u;
    const int sw = s + w;
    if (fu == 0 && c == 0 && n + d == 0 && w == 0) {
      return InfoBoardHeadline::kSUCCEEDED;
    }
    if (fu == 0 && c == 0 && n + d == 0) {
      return InfoBoardHeadline::kCOMPLETED_WITH_WARNINGS;
    }
    if (fu == 0 && c > 0 && sw == 0) return InfoBoardHeadline::kCANCELLED;
    if (fu == 0 && c > 0) return InfoBoardHeadline::kPARTIALLY_COMPLETED;
    if (fu == 0) return InfoBoardHeadline::kINCOMPLETE;
    if (f == 0 && sw + c + n + d == 0) return InfoBoardHeadline::kNOT_SUPPORTED;
    if (sw == 0) return InfoBoardHeadline::kFAILED;
    return InfoBoardHeadline::kPARTIALLY_FAILED;
  };
  const auto expected_severity = [](int w, int f, int c, int u, int n,
                                    int d) -> UI::InfoBoardStatus {
    if (f + u > 0) return UI::kINFO_ERROR_CRITICAL;
    if (w + c + n + d > 0) return UI::kINFO_ERROR_WARN;
    return UI::kINFO_ERROR_OK;
  };

  for (int mask = 1; mask < 128; ++mask) {
    InfoBoardOutcomeCounts counts;
    counts.succeeded = (mask >> 0) & 1;
    counts.warning = (mask >> 1) & 1;
    counts.failed = (mask >> 2) & 1;
    counts.cancelled = (mask >> 3) & 1;
    counts.not_supported = (mask >> 4) & 1;
    counts.not_reported = (mask >> 5) & 1;
    counts.not_saved = (mask >> 6) & 1;

    EXPECT_EQ(UI::AggregateHeadline(counts),
              expected_headline(counts.succeeded, counts.warning, counts.failed,
                                counts.cancelled, counts.not_supported,
                                counts.not_reported, counts.not_saved))
        << "mask " << mask;
    EXPECT_EQ(UI::AggregateSeverity(counts),
              expected_severity(counts.warning, counts.failed, counts.cancelled,
                                counts.not_supported, counts.not_reported,
                                counts.not_saved))
        << "mask " << mask;
  }
}

TEST(InfoBoardReportTest, CompletedCountsSucceededAndWarnings) {
  InfoBoardOutcomeCounts counts;
  counts.succeeded = 2;
  counts.warning = 1;
  counts.cancelled = 2;
  EXPECT_EQ(counts.Completed(), 3);
  EXPECT_TRUE(UI::CountSummaryText(counts).startsWith(
      Key("%1 completed (%2 with warnings)").arg(3).arg(1)));
}

TEST(InfoBoardReportTest, ACancelledBatchCountsWhatFinishedBeforeIt) {
  InfoBoardBatchContext ctx;
  ctx.file_target = true;
  ctx.fates = {{0, OutputFate::kSTAGED_DISCARDED},
               {1, OutputFate::kSTAGED_DISCARDED},
               {2, OutputFate::kNOT_PRODUCED}};
  const auto report = BuildReportFromResults(
      {FileResult(Result(1), 0, u"/a/1.txt"_s),
       FileResult(Result(0), 1, u"/a/2.txt"_s),
       FileResult(
           UI::MakeAbortedOpInfo(GpgOperation::kENCRYPT, GPG_ERR_CANCELED, {}),
           2, u"/a/3.txt"_s)},
      ctx);

  // The two that finished were not saved: the batch was cancelled.
  EXPECT_EQ(report.headline, InfoBoardHeadline::kCANCELLED);
  EXPECT_EQ(report.severity, UI::kINFO_ERROR_WARN);
  EXPECT_TRUE(report.summary.contains(Key("No output files were written.")));
}

// ---------------------------------------------------------------------------
// What an object says: readable reason plus every recorded error.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, AFailureShowsAReasonAndTheEngineErrorVerbatim) {
  const auto report = BuildReportFromResults({DocResult(
      Result(-1, GpgOperation::kDECRYPT,
             {EngineError(GpgOperation::kDECRYPT, GPG_ERR_NO_SECKEY,
                          u"No secret key"_s, u"key 0xABCD not found"_s)}))});
  ASSERT_EQ(report.objects.size(), 1);
  const auto& card = OutcomeCard(report.objects.first());

  EXPECT_EQ(card.kind, InfoBoardCardKind::kOUTCOME);
  EXPECT_EQ(Field(card, Key("Result")),
            UI::OutcomeText(InfoBoardOutcome::kFAILED));
  EXPECT_FALSE(Field(card, Key("Reason")).isEmpty());
  const auto errors = Rows(card, Key("Engine error"));
  ASSERT_EQ(errors.size(), 1);
  EXPECT_EQ(errors[0].first, u"No secret key"_s) << "the message, verbatim";
  EXPECT_EQ(errors[0].second, Key("code %1").arg(GPG_ERR_NO_SECKEY));
  EXPECT_EQ(Field(card, Key("Engine detail")), u"key 0xABCD not found"_s);
  EXPECT_EQ(report.headline, InfoBoardHeadline::kFAILED);
  EXPECT_FALSE(report.summary.isEmpty());
}

TEST(InfoBoardReportTest, CancelAndUnsupportedStillShowTheEngineDetail) {
  for (const auto code : {GPG_ERR_CANCELED, GPG_ERR_NOT_SUPPORTED}) {
    const auto report = BuildReportFromResults({DocResult(
        UI::MakeAbortedOpInfo(GpgOperation::kSIGN, code, {},
                              {{GpgOperation::kSIGN, u"engine says why"_s}}))});
    const auto& card = OutcomeCard(report.objects.first());
    EXPECT_FALSE(Field(card, Key("Reason")).isEmpty());
    EXPECT_FALSE(Field(card, Key("Engine error")).isEmpty());
    EXPECT_EQ(Field(card, Key("Engine detail")), u"engine says why"_s);
    EXPECT_FALSE(Field(card, Key("Suggestion")).isEmpty());
  }
}

TEST(InfoBoardReportTest, TheScreenshotCaseExplainsItself) {
  // Text Encrypt, passphrase cancelled, nothing applied to the document.
  InfoBoardBatchContext ctx;
  ctx.document_target = true;
  ctx.fates = {{0, OutputFate::kNOT_PRODUCED}};
  const auto report = BuildReportFromResults(
      {DocResult(UI::MakeAbortedOpInfo(GpgOperation::kENCRYPT, GPG_ERR_CANCELED,
                                       u"GPG v2"_s))},
      ctx);

  EXPECT_EQ(report.headline, InfoBoardHeadline::kCANCELLED);
  EXPECT_EQ(report.severity, UI::kINFO_ERROR_WARN);
  // The headline says "Cancelled"; the summary says what that meant, from
  // the recorded facts, rather than saying "cancelled" again.
  EXPECT_FALSE(report.summary.contains(Key("The operation was cancelled.")));
  EXPECT_TRUE(
      report.summary.contains(Key("The current document was not changed.")));

  const auto& object = report.objects.first();
  EXPECT_EQ(object.source.kind, InfoBoardSource::kDOCUMENT);
  EXPECT_EQ(object.source.label, Key("Current document"));
  const auto& card = OutcomeCard(object);
  EXPECT_EQ(card.title, Key("Outcome"));
  EXPECT_EQ(Field(card, Key("Result")),
            UI::OutcomeText(InfoBoardOutcome::kCANCELLED));
  EXPECT_FALSE(Field(card, Key("Engine error")).isEmpty());

  // The reason is kept for the text outputs, marked as said elsewhere.
  bool reason_kept = false;
  for (qsizetype i = 0; i < card.fields.size(); ++i) {
    if (card.MetaAt(i).role == UI::InfoBoardFieldRole::kREASON) {
      reason_kept = true;
      EXPECT_TRUE(card.MetaAt(i).summarized);
      EXPECT_EQ(card.fields[i].second, Key("The operation was cancelled."));
    }
  }
  EXPECT_TRUE(reason_kept);
}

TEST(InfoBoardReportTest, DifferentStageErrorsOfACombinedOperationBothShow) {
  const auto report = BuildReportFromResults({DocResult(
      Result(-1, GpgOperation::kENCRYPT_SIGN,
             {EngineError(GpgOperation::kENCRYPT, GPG_ERR_UNUSABLE_PUBKEY,
                          u"Unusable public key"_s),
              EngineError(GpgOperation::kSIGN, GPG_ERR_BAD_PASSPHRASE,
                          u"Bad passphrase"_s)}))});
  const auto& card = OutcomeCard(report.objects.first());
  const auto errors = Rows(card, Key("Engine error"));
  ASSERT_EQ(errors.size(), 2);
  EXPECT_EQ(errors[0].first, u"Unusable public key"_s);
  EXPECT_TRUE(
      errors[0].second.startsWith(GpgOperationName(GpgOperation::kENCRYPT)));
  EXPECT_EQ(errors[1].first, u"Bad passphrase"_s);
  EXPECT_TRUE(
      errors[1].second.startsWith(GpgOperationName(GpgOperation::kSIGN)));
}

TEST(InfoBoardReportTest, AMissingSignerKeyIsExplainedWithAWayOut) {
  auto info = Result(0, GpgOperation::kVERIFY);
  GpgVerifySigInfo sig;
  sig.validity = GpgSigValidity::kKEY_MISSING;
  info.signatures.append(sig);

  const auto report = BuildReportFromResults({DocResult(info)});
  const auto& card = OutcomeCard(report.objects.first());
  EXPECT_TRUE(Field(card, Key("Reason"))
                  .contains(Key("The signer's public key is not in this key "
                                "database, so the signature could not be "
                                "checked.")));
  EXPECT_EQ(Field(card, Key("Suggestion")),
            Key("Import the signer's public key and verify again."));
}

// ---------------------------------------------------------------------------
// Output stage.
// ---------------------------------------------------------------------------

namespace {

auto CommitError(const QString& path) -> OutputStageError {
  OutputStageError e;
  e.action = OutputAction::kCOMMIT_FILE;
  e.path = path;
  e.message = u"the output could not be moved into place"_s;
  return e;
}

}  // namespace

TEST(InfoBoardReportTest, ACompleteCommitFailureIsNeverASuccess) {
  InfoBoardBatchContext ctx;
  ctx.file_target = true;
  ctx.fates = {{0, OutputFate::kCOMMIT_FAILED},
               {1, OutputFate::kNOT_COMMITTED}};
  ctx.output_errors = {{0, {CommitError(u"/out/a.gpg"_s)}}};
  const auto report =
      BuildReportFromResults({FileResult(Result(1), 0, u"/in/a"_s),
                              FileResult(Result(1), 1, u"/in/b"_s)},
                             ctx);

  EXPECT_NE(report.headline, InfoBoardHeadline::kSUCCEEDED);
  EXPECT_EQ(report.severity, UI::kINFO_ERROR_CRITICAL);
  ASSERT_EQ(report.objects.size(), 2);
  EXPECT_EQ(report.objects[0].task_outcome, InfoBoardOutcome::kFAILED);
  EXPECT_EQ(report.objects[0].engine_outcome, InfoBoardOutcome::kSUCCEEDED);
  EXPECT_EQ(report.objects[1].task_outcome, InfoBoardOutcome::kNOT_SAVED);

  const auto& card = OutcomeCard(report.objects[0]);
  EXPECT_EQ(Field(card, Key("Engine result")),
            UI::OutcomeText(InfoBoardOutcome::kSUCCEEDED));
  const auto errors = Rows(card, Key("Output error"));
  ASSERT_EQ(errors.size(), 1);
  EXPECT_EQ(errors[0].first, u"the output could not be moved into place"_s);
  EXPECT_TRUE(errors[0].second.contains(Key("save file")));
  EXPECT_TRUE(errors[0].second.contains(u"/out/a.gpg"_s));
  EXPECT_TRUE(report.summary.contains(Key("No output files were written.")));
}

TEST(InfoBoardReportTest, APartialCommitFailureMakesNoBlanketClaim) {
  InfoBoardBatchContext ctx;
  ctx.file_target = true;
  ctx.fates = {{0, OutputFate::kCOMMITTED},
               {1, OutputFate::kCOMMIT_FAILED},
               {2, OutputFate::kNOT_COMMITTED}};
  ctx.output_paths = {{0, u"/out/a.gpg"_s}};
  ctx.output_errors = {{1, {CommitError(u"/out/b.gpg"_s)}}};
  const auto report =
      BuildReportFromResults({FileResult(Result(1), 0, u"/in/a"_s),
                              FileResult(Result(1), 1, u"/in/b"_s),
                              FileResult(Result(1), 2, u"/in/c"_s)},
                             ctx);

  EXPECT_EQ(report.headline, InfoBoardHeadline::kPARTIALLY_FAILED);
  EXPECT_FALSE(report.summary.contains(Key("No output files were written.")));
  EXPECT_FALSE(report.summary.contains(Key("All output files were written.")));
}

TEST(InfoBoardReportTest, ADocumentThatCouldNotTakeTheOutputIsAFailure) {
  InfoBoardBatchContext ctx;
  ctx.document_target = true;
  ctx.fates = {{0, OutputFate::kAPPLY_FAILED}};
  OutputStageError e;
  e.action = OutputAction::kAPPLY_TO_DOCUMENT;
  e.message = u"no text document"_s;
  ctx.output_errors = {{0, {e}}};
  const auto report = BuildReportFromResults(
      {DocResult(Result(1, GpgOperation::kDECRYPT))}, ctx);

  EXPECT_EQ(report.headline, InfoBoardHeadline::kFAILED);
  const auto& card = OutcomeCard(report.objects.first());
  const auto errors = Rows(card, Key("Output error"));
  ASSERT_EQ(errors.size(), 1);
  EXPECT_EQ(errors[0].first, u"no text document"_s);
  EXPECT_TRUE(errors[0].second.contains(Key("apply to document")));
  EXPECT_TRUE(
      report.summary.contains(Key("The current document was not changed.")));
}

TEST(InfoBoardReportTest, AStagedFileThatCouldNotBeRemovedIsReported) {
  InfoBoardBatchContext ctx;
  ctx.file_target = true;
  ctx.fates = {{0, OutputFate::kDISCARD_FAILED}};
  OutputStageError e;
  e.action = OutputAction::kDISCARD_STAGED;
  e.path = u"/out/.work/a.gpg"_s;
  e.message = u"could not remove"_s;
  ctx.output_errors = {{0, {e}}};
  const auto report =
      BuildReportFromResults({FileResult(Result(1), 0, u"/in/a"_s)}, ctx);

  EXPECT_EQ(report.objects.first().task_outcome, InfoBoardOutcome::kFAILED);
  EXPECT_FALSE(report.summary.contains(Key("No output files were written.")));
  EXPECT_TRUE(UI::RenderReportText(report).contains(u"/out/.work/a.gpg"_s));
}

// ---------------------------------------------------------------------------
// Missing and late results.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, AMissingResultClaimsNothingAboutItsOutput) {
  InfoBoardBatchContext ctx;
  ctx.file_target = true;
  ctx.expected = {{0, {InfoBoardSource::kFILE, u"f0"_s, u"a"_s, u"/in/a"_s}},
                  {1, {InfoBoardSource::kFILE, u"f1"_s, u"b"_s, u"/in/b"_s}}};
  // Cleanup reported both staged outputs removed; one result never came.
  ctx.fates = {{0, OutputFate::kSTAGED_DISCARDED},
               {1, OutputFate::kSTAGED_DISCARDED}};
  const auto report =
      BuildReportFromResults({FileResult(Result(-1), 0, u"/in/a"_s)}, ctx);

  ASSERT_EQ(report.objects.size(), 2);
  const auto missing =
      std::find_if(report.objects.cbegin(), report.objects.cend(),
                   [](const InfoBoardObject& o) { return o.input_index == 1; });
  ASSERT_NE(missing, report.objects.cend());
  EXPECT_EQ(missing->task_outcome, InfoBoardOutcome::kNOT_REPORTED);
  EXPECT_EQ(missing->output, OutputFate::kUNKNOWN);
  EXPECT_EQ(missing->source.full, u"/in/b"_s);
  EXPECT_FALSE(report.summary.contains(Key("No output files were written.")));
}

TEST(InfoBoardReportTest, AResultArrivingLaterDoesNotChangeAPostedReport) {
  QList<GpgOperaResult> results{FileResult(Result(1), 0, u"/in/a"_s)};
  const auto report = BuildReportFromResults(results);
  const auto before = UI::CanonicalReportBytes(report);

  results.append(FileResult(Result(-1), 1, u"/in/b"_s));
  EXPECT_EQ(UI::CanonicalReportBytes(report), before);
  EXPECT_EQ(report.objects.size(), 1);
}

// ---------------------------------------------------------------------------
// Sources.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, SameNamedFilesStayApartAndAreLabelledApart) {
  const auto report =
      BuildReportFromResults({FileResult(Result(1), 0, u"/home/u/a/x.txt"_s),
                              FileResult(Result(1), 1, u"/home/u/b/x.txt"_s)});
  ASSERT_EQ(report.objects.size(), 2);
  EXPECT_NE(report.objects[0].source.id, report.objects[1].source.id);
  EXPECT_EQ(report.objects[0].source.label, u"a/x.txt"_s);
  EXPECT_EQ(report.objects[1].source.label, u"b/x.txt"_s);
  EXPECT_EQ(report.objects[0].source.full, u"/home/u/a/x.txt"_s);
}

TEST(InfoBoardReportTest, TheSamePathTwiceIsTwoObjects) {
  const auto report =
      BuildReportFromResults({FileResult(Result(1), 0, u"/a/x.txt"_s),
                              FileResult(Result(1), 1, u"/a/x.txt"_s)});
  ASSERT_EQ(report.objects.size(), 2);
  EXPECT_NE(report.objects[0].source.id, report.objects[1].source.id);
  EXPECT_EQ(report.objects[1].source.label,
            report.objects[0].source.label + u" (2)"_s);
}

TEST(InfoBoardReportTest, EveryCardCarriesItsObjectsSource) {
  auto info = Result(1, GpgOperation::kVERIFY);
  GpgVerifySigInfo sig;
  sig.validity = GpgSigValidity::kFULLY_VALID;
  info.signatures.append(sig);
  const auto report =
      BuildReportFromResults({FileResult(info, 0, u"/a/s.txt"_s)});
  for (const auto& card : report.objects.first().cards) {
    EXPECT_EQ(card.source.id, report.objects.first().source.id);
  }
}

// ---------------------------------------------------------------------------
// The cards entry point.
// ---------------------------------------------------------------------------

namespace {

auto Card(const QString& title, const QString& source_id = {},
          InfoBoardCardKind kind = InfoBoardCardKind::kGENERIC)
    -> InfoBoardCard {
  InfoBoardCard c;
  c.title = title;
  c.kind = kind;
  if (!source_id.isEmpty()) {
    c.source = {InfoBoardSource::kOPERATION, source_id, source_id, source_id};
  }
  c.fields.append({u"k"_s, title});
  return c;
}

auto StructuredOutcome(const QString& source_id, InfoBoardOutcome outcome)
    -> InfoBoardCard {
  auto c = Card(u"Outcome"_s, source_id, InfoBoardCardKind::kOUTCOME);
  c.outcome = outcome;
  c.fields = {{Key("Result"), UI::OutcomeText(outcome)},
              {Key("Reason"), u"structured reason"_s}};
  return c;
}

}  // namespace

TEST(InfoBoardReportTest, CardsFromSeveralSourcesBecomeSeveralObjects) {
  const auto report = BuildReportFromCards(
      u"text"_s, UI::kINFO_ERROR_OK,
      {Card(u"one"_s, u"s1"_s), Card(u"two"_s, u"s2"_s),
       Card(u"three"_s, u"s3"_s), Card(u"one-b"_s, u"s1"_s)},
      u"Op"_s, {}, {}, {});
  ASSERT_EQ(report.objects.size(), 3);
  for (const auto& object : report.objects) {
    ASSERT_FALSE(object.cards.isEmpty());
    EXPECT_EQ(object.cards.first().kind, InfoBoardCardKind::kOUTCOME);
    const auto outcomes = std::count_if(
        object.cards.cbegin(), object.cards.cend(), [](const InfoBoardCard& c) {
          return c.kind == InfoBoardCardKind::kOUTCOME;
        });
    EXPECT_EQ(outcomes, 1);
  }
  // Generic cards keep the order they came in.
  const auto& s1 = report.objects[0];
  EXPECT_EQ(s1.cards[1].title, u"one"_s);
  EXPECT_EQ(s1.cards[2].title, u"one-b"_s);
}

TEST(InfoBoardReportTest, AnEmptyCardSourceIsThePostingsOwn) {
  const auto report = BuildReportFromCards(
      {}, UI::kINFO_ERROR_OK, {Card(u"c"_s)}, u"Encode"_s, {}, {}, {});
  ASSERT_EQ(report.objects.size(), 1);
  EXPECT_EQ(report.objects[0].source.kind, InfoBoardSource::kOPERATION);
  EXPECT_EQ(report.objects[0].source.label, u"Encode"_s);
}

TEST(InfoBoardReportTest, StructuredOutcomesWinOverThePostingStatus) {
  auto styled = Card(u"styled red"_s, u"legacy"_s);
  styled.status = UI::kINFO_ERROR_CRITICAL;  // colour only

  const auto report = BuildReportFromCards(
      {}, UI::kINFO_ERROR_OK,
      {StructuredOutcome(u"mail"_s, InfoBoardOutcome::kFAILED), styled},
      u"Decrypt E-Mail"_s, {}, {}, {});

  ASSERT_EQ(report.objects.size(), 2);
  const auto mail = std::find_if(
      report.objects.cbegin(), report.objects.cend(),
      [](const InfoBoardObject& o) { return o.source.id == u"mail"_s; });
  const auto legacy = std::find_if(
      report.objects.cbegin(), report.objects.cend(),
      [](const InfoBoardObject& o) { return o.source.id == u"legacy"_s; });
  ASSERT_NE(mail, report.objects.cend());
  ASSERT_NE(legacy, report.objects.cend());
  EXPECT_EQ(mail->task_outcome, InfoBoardOutcome::kFAILED);
  EXPECT_EQ(mail->reason, u"structured reason"_s);
  // The red card was styling; its group follows the posting status.
  EXPECT_EQ(legacy->task_outcome, InfoBoardOutcome::kSUCCEEDED);
  EXPECT_EQ(report.severity, UI::kINFO_ERROR_CRITICAL);
  EXPECT_FALSE(report.notes.isEmpty());
}

TEST(InfoBoardReportTest, ExtraOrMalformedOutcomeCardsBecomeOrdinaryCards) {
  auto malformed = Card(u"bad"_s, u"x"_s, InfoBoardCardKind::kOUTCOME);
  const auto report = BuildReportFromCards(
      {}, UI::kINFO_ERROR_WARN,
      {StructuredOutcome(u"s"_s, InfoBoardOutcome::kSUCCEEDED),
       StructuredOutcome(u"s"_s, InfoBoardOutcome::kFAILED), malformed},
      {}, {}, {}, {});

  for (const auto& object : report.objects) {
    const auto outcomes = std::count_if(
        object.cards.cbegin(), object.cards.cend(), [](const InfoBoardCard& c) {
          return c.kind == InfoBoardCardKind::kOUTCOME;
        });
    EXPECT_EQ(outcomes, 1) << object.source.id.toStdString();
  }
  const auto s = std::find_if(
      report.objects.cbegin(), report.objects.cend(),
      [](const InfoBoardObject& o) { return o.source.id == u"s"_s; });
  ASSERT_NE(s, report.objects.cend());
  EXPECT_EQ(s->task_outcome, InfoBoardOutcome::kSUCCEEDED);
  // The malformed one's group got its own Outcome, from the posting status.
  const auto x = std::find_if(
      report.objects.cbegin(), report.objects.cend(),
      [](const InfoBoardObject& o) { return o.source.id == u"x"_s; });
  ASSERT_NE(x, report.objects.cend());
  EXPECT_EQ(x->task_outcome, InfoBoardOutcome::kWARNING);
  EXPECT_GE(report.notes.size(), 2);
}

// ---------------------------------------------------------------------------
// Ordering.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, ProblemsComeFirstThenInputOrder) {
  const auto report =
      BuildReportFromResults({FileResult(Result(1), 0, u"/a/0"_s),
                              FileResult(Result(-1), 1, u"/a/1"_s),
                              FileResult(Result(0), 2, u"/a/2"_s),
                              FileResult(Result(-1), 3, u"/a/3"_s),
                              FileResult(Result(1), 4, u"/a/4"_s)});
  QList<int> order;
  for (const auto& o : report.objects) order << o.input_index;
  EXPECT_EQ(order, (QList<int>{1, 3, 2, 0, 4}));
}

TEST(InfoBoardReportTest, OutcomeFirstThenCardsByKind) {
  auto info = Result(1, GpgOperation::kDECRYPT);
  GpgRecipientInfo reci;
  reci.keyId = u"AB"_s;
  reci.keyFound = true;
  info.recipients.append(reci);
  info.details.append(u"someone"_s);
  const auto report = BuildReportFromResults({DocResult(info)});
  const auto& cards = report.objects.first().cards;
  ASSERT_GE(cards.size(), 3);
  EXPECT_EQ(cards[0].kind, InfoBoardCardKind::kOUTCOME);
  for (qsizetype i = 1; i < cards.size(); ++i) {
    EXPECT_LE(static_cast<int>(cards[i - 1].kind),
              static_cast<int>(cards[i].kind));
  }
}

// ---------------------------------------------------------------------------
// The text entry point and setter equivalence.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, PlainTextIsTheCallersMessage) {
  const auto report =
      BuildReportFromText(u"could not finalize"_s, UI::kINFO_ERROR_CRITICAL);
  ASSERT_EQ(report.objects.size(), 1);
  const auto& card = OutcomeCard(report.objects.first());
  EXPECT_EQ(Field(card, Key("Message")), u"could not finalize"_s);
  EXPECT_EQ(Field(card, Key("Result")),
            UI::OutcomeText(InfoBoardOutcome::kFAILED));
  EXPECT_FALSE(Field(card, Key("Reason")).isEmpty());
  EXPECT_FALSE(HasFieldStartingWith(card, Key("Engine error")));
  EXPECT_EQ(report.severity, UI::kINFO_ERROR_CRITICAL);
}

TEST(InfoBoardReportTest, NeutralTextIsInformation) {
  const auto report = BuildReportFromText(u"fyi"_s, UI::kINFO_ERROR_NEUTRAL);
  EXPECT_EQ(report.headline, InfoBoardHeadline::kINFORMATION);
  EXPECT_TRUE(report.objects.isEmpty());
  EXPECT_EQ(report.summary, u"fyi"_s);
}

TEST(InfoBoardReportTest, EqualInformationReadsTheSameThroughEveryEntry) {
  for (const auto status : {1, -1}) {
    auto info = Result(status, GpgOperation::kSIGN);
    info.description = u"D"_s;
    const auto by_results = BuildReportFromResults({DocResult(info)});
    const auto by_cards = BuildReportFromCards(
        {}, status > 0 ? UI::kINFO_ERROR_OK : UI::kINFO_ERROR_CRITICAL, {},
        GpgOperationName(GpgOperation::kSIGN), u"D"_s, {}, {});

    EXPECT_EQ(by_results.headline, by_cards.headline);
    EXPECT_EQ(by_results.severity, by_cards.severity);
    EXPECT_EQ(by_results.summary, by_cards.summary);
    const auto& a = OutcomeCard(by_results.objects.first());
    const auto& b = OutcomeCard(by_cards.objects.first());
    EXPECT_EQ(Field(a, Key("Result")), Field(b, Key("Result")));
    EXPECT_EQ(Field(a, Key("Reason")), Field(b, Key("Reason")));
  }
}

// ---------------------------------------------------------------------------
// Text outputs and the hash.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, CopyTextHoldsEveryObjectAndEveryError) {
  QList<GpgOperaResult> results;
  for (int i = 0; i < 9; ++i) {
    results.append(FileResult(
        Result(-1, GpgOperation::kENCRYPT,
               {EngineError(GpgOperation::kENCRYPT, GPG_ERR_GENERAL,
                            u"err-%1"_s.arg(i), u"detail-%1"_s.arg(i))}),
        i, u"/very/long/path/file-%1.txt"_s.arg(i)));
  }
  auto report = BuildReportFromResults(results);
  UI::FinalizeReport(
      report, QDateTime(QDate(2026, 1, 2), QTime(3, 4, 5), QTimeZone::utc()));
  const auto text = UI::RenderReportText(report);
  for (int i = 0; i < 9; ++i) {
    EXPECT_TRUE(text.contains(u"/very/long/path/file-%1.txt"_s.arg(i)));
    EXPECT_TRUE(text.contains(u"err-%1"_s.arg(i)));
    EXPECT_TRUE(text.contains(u"detail-%1"_s.arg(i)));
  }
  EXPECT_TRUE(text.contains(report.ref_id));

  const auto details = UI::RenderRawDetails(report);
  EXPECT_FALSE(details.contains(u"Object:"_s));
  EXPECT_TRUE(details.contains(u"detail-8"_s));
  // Each item headed by its full source, numbered, with a rule under it.
  EXPECT_TRUE(details.contains(u"9/9  /very/long/path/file-8.txt\n"_s));
  EXPECT_TRUE(details.startsWith(u"GPGFRONTEND"_s));
}

TEST(InfoBoardReportTest, TheReportHashIsStableAndCoversNotItself) {
  auto info = Result(1);
  info.inputHash = u"abc123"_s;
  const auto issued =
      QDateTime(QDate(2026, 10, 24), QTime(12, 56, 0), QTimeZone::utc());

  auto a = BuildReportFromResults({DocResult(info)});
  auto b = BuildReportFromResults({DocResult(info)});
  UI::FinalizeReport(a, issued);
  b.report_hash = u"something else"_s;
  b.ref_id = u"REF-X"_s;
  UI::FinalizeReport(b, issued.addMSecs(400));  // sub-second noise
  EXPECT_EQ(a.report_hash, b.report_hash);
  EXPECT_EQ(a.ref_id, b.ref_id);
  EXPECT_NE(a.report_hash.toLower(), info.inputHash);
  EXPECT_TRUE(a.ref_id.endsWith(a.report_hash.left(8)));
  EXPECT_FALSE(UI::CanonicalReportBytes(a).contains(a.report_hash.toUtf8()));

  auto c = BuildReportFromResults({DocResult(Result(0))});
  UI::FinalizeReport(c, issued);
  EXPECT_NE(a.report_hash, c.report_hash);

  auto d = a;
  UI::FinalizeReport(d, issued.addSecs(1));
  EXPECT_NE(a.report_hash, d.report_hash);
}

// ---------------------------------------------------------------------------
// Card JSON.
// ---------------------------------------------------------------------------

TEST(InfoBoardReportTest, CardJsonRoundTripsKindSourceAndOutcome) {
  auto card = StructuredOutcome(u"s"_s, InfoBoardOutcome::kCANCELLED);
  card.source = {InfoBoardSource::kFILE, u"file:0:/a/b"_s, u"b"_s, u"/a/b"_s};

  const auto json = UI::encode_info_board_cards({card});
  QByteArray wrapped = "{\"cards\":" + json + "}";
  const auto payload = UI::decode_info_board_cards(wrapped);
  ASSERT_TRUE(payload.valid);
  ASSERT_EQ(payload.cards.size(), 1);
  const auto& back = payload.cards.first();
  EXPECT_EQ(back.kind, InfoBoardCardKind::kOUTCOME);
  EXPECT_EQ(back.outcome, InfoBoardOutcome::kCANCELLED);
  EXPECT_EQ(back.source.kind, InfoBoardSource::kFILE);
  EXPECT_EQ(back.source.id, u"file:0:/a/b"_s);
  EXPECT_EQ(back.source.label, u"b"_s);
  EXPECT_EQ(back.source.full, u"/a/b"_s);
}

TEST(InfoBoardReportTest, CardJsonRoundTripsFieldPresentation) {
  InfoBoardCard card;
  card.title = u"T"_s;
  card.AddField(
      u"Engine error"_s, u"Bad passphrase"_s,
      {u"Sign · code 11"_s, false, UI::InfoBoardFieldGroup::kDIAGNOSTIC});
  card.AddField(u"Key ID"_s, u"ABCD"_s,
                {{}, true, UI::InfoBoardFieldGroup::kDETAIL});

  const auto payload = UI::decode_info_board_cards(
      "{\"cards\":" + UI::encode_info_board_cards({card}) + "}");
  ASSERT_EQ(payload.cards.size(), 1);
  const auto& back = payload.cards.first();
  EXPECT_EQ(back.MetaAt(0).note, u"Sign · code 11"_s);
  EXPECT_EQ(back.MetaAt(0).group, UI::InfoBoardFieldGroup::kDIAGNOSTIC);
  EXPECT_FALSE(back.MetaAt(0).technical);
  EXPECT_TRUE(back.MetaAt(1).technical);
}

TEST(InfoBoardReportTest, OutcomeFieldsComeInTheirRuns) {
  InfoBoardBatchContext ctx;
  ctx.document_target = true;
  ctx.fates = {{0, OutputFate::kNOT_PRODUCED}};
  auto info = UI::MakeAbortedOpInfo(GpgOperation::kENCRYPT, GPG_ERR_CANCELED,
                                    {}, {{GpgOperation::kENCRYPT, u"d"_s}});
  info.inputHash = u"abc"_s;
  const auto report = BuildReportFromResults({DocResult(info)}, ctx);
  const auto& card = OutcomeCard(report.objects.first());

  QList<UI::InfoBoardFieldGroup> groups;
  for (qsizetype i = 0; i < card.fields.size(); ++i) {
    groups << card.MetaAt(i).group;
  }
  // Runs never interleave: what happened, why, what next, then details.
  for (qsizetype i = 1; i < groups.size(); ++i) {
    EXPECT_LE(static_cast<int>(groups[i - 1]), static_cast<int>(groups[i]));
  }
  EXPECT_TRUE(card.MetaAt(card.fields.size() - 1).technical)
      << "the input hash is an identifier";
  const auto text = UI::RenderReportText(report);
  EXPECT_TRUE(
      text.contains(u"[%1]"_s.arg(Key("code %1").arg(GPG_ERR_CANCELED))))
      << "notes are in the text outputs";
}

TEST(InfoBoardReportTest, OnlyAPlainSuccessSummaryIsMarkedRedundant) {
  // Decided from structure: the standard sentence, nothing else to say.
  EXPECT_TRUE(
      BuildReportFromResults({DocResult(Result(1))}).summary_restates_headline);

  auto described = Result(1);
  described.description = u"D"_s;
  EXPECT_FALSE(
      BuildReportFromResults({DocResult(described)}).summary_restates_headline);

  EXPECT_FALSE(
      BuildReportFromResults({DocResult(Result(0))}).summary_restates_headline);

  // An output fact is never redundant.
  InfoBoardBatchContext ctx;
  ctx.file_target = true;
  ctx.fates = {{0, OutputFate::kCOMMITTED}};
  const auto written =
      BuildReportFromResults({FileResult(Result(1), 0, u"/a"_s)}, ctx);
  EXPECT_FALSE(written.summary_restates_headline);
  EXPECT_EQ(written.summary, Key("All output files were written."));
}

TEST(InfoBoardReportTest, LegacyCardJsonDecodesAsBefore) {
  const auto payload = UI::decode_info_board_cards(
      R"({"operation":"Op","cards":[{"title":"T","status":"ok",
          "fields":[["k","v"]]}]})");
  ASSERT_TRUE(payload.valid);
  ASSERT_EQ(payload.cards.size(), 1);
  const auto& c = payload.cards.first();
  EXPECT_EQ(c.kind, InfoBoardCardKind::kGENERIC);
  EXPECT_TRUE(c.source.IsEmpty());
  EXPECT_FALSE(c.outcome.has_value());
  EXPECT_EQ(c.fields.first(), qMakePair(u"k"_s, u"v"_s));
}

TEST(InfoBoardReportTest, AStringSourceAndAnUnknownOutcomeDecodeSafely) {
  const auto payload = UI::decode_info_board_cards(
      R"({"cards":[{"title":"T","kind":"outcome","outcome":"exploded",
          "source":"part-1","fields":[]}]})");
  ASSERT_EQ(payload.cards.size(), 1);
  const auto& c = payload.cards.first();
  EXPECT_FALSE(c.outcome.has_value());
  EXPECT_EQ(c.source.id, u"part-1"_s);
  EXPECT_EQ(c.source.label, u"part-1"_s);
}

TEST(InfoBoardReportTest, OpInfoCardsCarryTheOutcomeForModules) {
  auto info = Result(-1, GpgOperation::kDECRYPT,
                     {EngineError(GpgOperation::kDECRYPT, GPG_ERR_NO_SECKEY,
                                  u"No secret key"_s)});
  info.inputHash = u"feed"_s;
  const InfoBoardSource source{InfoBoardSource::kOPERATION, u"op:x"_s, u"x"_s,
                               u"x"_s};
  const auto cards = BuildOpInfoCards(info, source);
  ASSERT_FALSE(cards.isEmpty());
  EXPECT_EQ(cards.first().kind, InfoBoardCardKind::kOUTCOME);
  EXPECT_EQ(cards.first().outcome, InfoBoardOutcome::kFAILED);
  EXPECT_EQ(Field(cards.first(), Key("Input SHA-256")), u"feed"_s);
  EXPECT_FALSE(Field(cards.first(), Key("Engine error")).isEmpty());

  // Forwarded back through the cards entry, it means the same thing.
  const auto report =
      BuildReportFromCards({}, UI::kINFO_ERROR_OK, cards, u"x"_s, {}, {}, {});
  EXPECT_EQ(report.objects.first().task_outcome, InfoBoardOutcome::kFAILED);
  EXPECT_EQ(report.severity, UI::kINFO_ERROR_CRITICAL);
}

}  // namespace GpgFrontend::Test
