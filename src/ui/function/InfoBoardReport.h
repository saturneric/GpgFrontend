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

#pragma once

// The Status Panel's one report model. Every way a result reaches the panel is
// normalized into an InfoBoardReport here, and the panel, its copy text, its
// raw details and its PNG export are all produced from that model. Nothing in
// this file touches a widget, so all of it runs off the GUI thread.

#include "core/function/result_analyse/GpgOpResultInfo.h"
#include "ui/struct/GpgOperaResult.h"
#include "ui/struct/InfoBoardTypes.h"

namespace GpgFrontend::UI {

// ---------------------------------------------------------------------------
// Output stage: what happened to an engine's output after the engine ran.
// ---------------------------------------------------------------------------

enum class OutputAction : std::uint8_t {
  kAPPLY_TO_DOCUMENT,
  kCOMMIT_FILE,
  kDISCARD_STAGED,
};

enum class OutputErrorOrigin : std::uint8_t {
  kSYSTEM,       ///< the operating system's or Qt's own message, verbatim
  kAPPLICATION,  ///< GpgFrontend's description of what failed
};

/// One failure after the engine ran: saving, applying or cleaning up.
struct OutputStageError {
  OutputAction action = OutputAction::kCOMMIT_FILE;
  QString path;             ///< affected path; empty for the document
  std::optional<int> code;  ///< only when the platform reported one
  QString message;
  OutputErrorOrigin origin = OutputErrorOrigin::kAPPLICATION;
};

/// The established fate of an object's output. kUNKNOWN claims nothing.
enum class OutputFate : std::uint8_t {
  kUNKNOWN,
  kNOT_APPLICABLE,    ///< no output by design (Verify)
  kNOT_PRODUCED,      ///< the engine produced none
  kWITHHELD,          ///< a failed result's output, deliberately not used
  kAPPLIED,           ///< written into the current document
  kAPPLY_FAILED,      ///< usable output that could not be applied
  kCOMMITTED,         ///< moved to its final path
  kCOMMIT_FAILED,     ///< the commit was attempted and failed
  kNOT_COMMITTED,     ///< the commit stopped before reaching this output
  kSTAGED_DISCARDED,  ///< the staged output was removed, confirmed
  kDISCARD_FAILED,    ///< removing the staged output failed
};

// ---------------------------------------------------------------------------
// The model.
// ---------------------------------------------------------------------------

enum class InfoBoardHeadline : std::uint8_t {
  kSUCCEEDED,
  kCOMPLETED_WITH_WARNINGS,
  kCANCELLED,
  kPARTIALLY_COMPLETED,
  kINCOMPLETE,
  kNOT_SUPPORTED,
  kFAILED,
  kPARTIALLY_FAILED,
  kINFORMATION,
};

/// One object of a report (a file, the current document, a module's item),
/// with everything shown about it.
struct InfoBoardObject {
  InfoBoardSource source;
  int input_index = -1;
  InfoBoardOutcome engine_outcome = InfoBoardOutcome::kSUCCEEDED;
  InfoBoardOutcome task_outcome = InfoBoardOutcome::kSUCCEEDED;
  OutputFate output = OutputFate::kUNKNOWN;
  QString output_path;
  QList<GpgOpStageError> stage_errors;
  QList<OutputStageError> output_errors;
  QString reason;
  /// The reason only restates the outcome (e.g. "The operation was
  /// cancelled."); the headline says as much.
  bool reason_restates_outcome = false;
  QString suggestion;
  QString message;  ///< text a caller posted, labelled as such
  /// The engine that ran it, as recorded; empty when none was.
  QString engine;
  QString input_hash;
  QString raw_report;
  /// In display order; the first is always this object's Outcome card.
  QList<InfoBoardCard> cards;
};

struct InfoBoardReport {
  InfoBoardStatus severity = kINFO_ERROR_NEUTRAL;
  InfoBoardHeadline headline = InfoBoardHeadline::kINFORMATION;
  QString operation_label;
  std::optional<GpgOperation> op;
  QString engine;
  QString count_summary;
  QString summary;
  /// The summary only restates the headline (a plain success); the text
  /// outputs keep it, the screen may leave it out.
  bool summary_restates_headline = false;
  QString chips_title;
  QStringList chips;
  /// Every object, in display order; never truncated.
  QList<InfoBoardObject> objects;
  QDateTime issued_at;
  QString report_hash;
  QString ref_id;
  /// The posting's own text when it belongs to no single object; shown in
  /// the Details view before the objects.
  QString raw_text;
  /// Conflicts found while normalizing; logged once when posted.
  QStringList notes;

  [[nodiscard]] auto IsEmpty() const -> bool {
    return objects.isEmpty() && summary.trimmed().isEmpty();
  }
};

/// What a batch's call site knows beyond the results themselves.
struct InfoBoardBatchContext {
  /// Every input, by input index; empty when the batch is not known.
  QMap<int, InfoBoardSource> expected;
  QMap<int, OutputFate> fates;
  QMap<int, QList<OutputStageError>> output_errors;
  QMap<int, QString> output_paths;
  /// Cards a stage around the engine contributed for an input, e.g. a
  /// codec's section; shown in that input's group.
  QMap<int, QList<InfoBoardCard>> extra_cards;
  /// The batch's output goes into the current document.
  bool document_target = false;
  /// The batch writes files.
  bool file_target = false;
};

// ---------------------------------------------------------------------------
// Semantics.
// ---------------------------------------------------------------------------

struct NormalizedEngineOutcome {
  InfoBoardOutcome outcome = InfoBoardOutcome::kSUCCEEDED;
  /// status > 0 although the engine reported an error.
  bool conflict = false;
};

/**
 * @brief What the engine did, from what was recorded.
 *
 * @p status alone decides whether the output is usable; the recorded engine
 * codes name why it is not. Cancelled and not supported never hide another
 * error: any other engine code makes the result a failure.
 */
auto GF_UI_EXPORT NormalizeEngineOutcome(int status,
                                         const QList<GpgOpStageError>& errors)
    -> NormalizedEngineOutcome;

/// What the user's task achieved: the engine's outcome after its output was
/// applied, saved or discarded.
auto GF_UI_EXPORT ResolveTaskOutcome(InfoBoardOutcome engine, OutputFate fate)
    -> InfoBoardOutcome;

/// Per-outcome counts of a set of objects.
struct InfoBoardOutcomeCounts {
  int succeeded = 0;
  int warning = 0;
  int failed = 0;
  int cancelled = 0;
  int not_supported = 0;
  int not_reported = 0;
  int not_saved = 0;

  void Add(InfoBoardOutcome outcome);
  [[nodiscard]] auto Total() const -> int;
  /// Items whose task kept its output: succeeded plus warnings.
  [[nodiscard]] auto Completed() const -> int { return succeeded + warning; }
};

auto GF_UI_EXPORT AggregateSeverity(const InfoBoardOutcomeCounts& counts)
    -> InfoBoardStatus;

auto GF_UI_EXPORT AggregateHeadline(const InfoBoardOutcomeCounts& counts)
    -> InfoBoardHeadline;

// ---------------------------------------------------------------------------
// Presentation words.
// ---------------------------------------------------------------------------

auto GF_UI_EXPORT OutcomeText(InfoBoardOutcome outcome) -> QString;
auto GF_UI_EXPORT HeadlineText(InfoBoardHeadline headline) -> QString;
/// The non-zero counts with fixed labels, e.g. "2 completed · 1 failed".
auto GF_UI_EXPORT CountSummaryText(const InfoBoardOutcomeCounts& counts)
    -> QString;

/// Stable ids for the card JSON and the report hash; never translated.
auto GF_UI_EXPORT OutcomeId(InfoBoardOutcome outcome) -> QString;
auto GF_UI_EXPORT OutcomeFromId(const QString& id)
    -> std::optional<InfoBoardOutcome>;
auto GF_UI_EXPORT CardKindId(InfoBoardCardKind kind) -> QString;
auto GF_UI_EXPORT CardKindFromId(const QString& id) -> InfoBoardCardKind;
auto GF_UI_EXPORT SeverityId(InfoBoardStatus status) -> QString;
auto GF_UI_EXPORT SourceKindId(InfoBoardSource::Kind kind) -> QString;

// ---------------------------------------------------------------------------
// Builders: one per way a result reaches the panel.
// ---------------------------------------------------------------------------

/// A plain message: one object, outcome from @p status, the text as a
/// caller's Message.
auto GF_UI_EXPORT BuildReportFromText(const QString& text,
                                      InfoBoardStatus status)
    -> InfoBoardReport;

/**
 * @brief Cards a module or codec composed.
 *
 * Grouped by source id. A group's valid structured Outcome card decides its
 * outcome; @p status is the fallback for groups without one. Severity and
 * headline come from the groups, never from @p status directly.
 */
auto GF_UI_EXPORT BuildReportFromCards(
    const QString& text, InfoBoardStatus status,
    const QList<InfoBoardCard>& cards, const QString& operation,
    const QString& description, const QString& details_title,
    const QStringList& details_items) -> InfoBoardReport;

/// Analysed results, one object each, plus what the call site knows.
auto GF_UI_EXPORT BuildReportFromResults(
    const QList<GpgOperaResult>& results,
    const InfoBoardBatchContext& context = {}) -> InfoBoardReport;

/// The source of a buffer operation: the document in the current tab.
auto GF_UI_EXPORT CurrentDocumentSource(int input_index = 0) -> InfoBoardSource;

/**
 * @brief Outcome and domain cards for one analysed result.
 *
 * What the SDK hands a module, so the module can forward a complete object
 * back through the cards entry point.
 */
auto GF_UI_EXPORT BuildOpInfoCards(const GpgOpResultInfo& info,
                                   const InfoBoardSource& source)
    -> QList<InfoBoardCard>;

/**
 * @brief Stamp the report with its issue time, hash and reference id.
 *
 * The hash covers the canonical content (CanonicalReportBytes) and so never
 * itself, the reference id or how much of the report is on screen.
 */
void GF_UI_EXPORT FinalizeReport(InfoBoardReport& report,
                                 const QDateTime& issued_at);

/// The exact bytes the report hash is computed over.
auto GF_UI_EXPORT CanonicalReportBytes(const InfoBoardReport& report)
    -> QByteArray;

// ---------------------------------------------------------------------------
// Text outputs. Always the full report, whatever the panel shows.
// ---------------------------------------------------------------------------

/// What Copy puts on the clipboard and the PNG carries as its comment.
auto GF_UI_EXPORT RenderReportText(const InfoBoardReport& report) -> QString;

/// The Details view: each object's engine report and every error.
auto GF_UI_EXPORT RenderRawDetails(const InfoBoardReport& report) -> QString;

/// Append an object's error rows (engine, application, output) to @p card,
/// in display order: short labels, the stage and code as each row's note.
void GF_UI_EXPORT AppendObjectErrorFields(InfoBoardCard& card,
                                          const InfoBoardObject& object);

}  // namespace GpgFrontend::UI
