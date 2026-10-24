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

#include "ui/main_window/FileOperaOutcome.h"

namespace GpgFrontend::UI {

auto IsFileOperaSuccessful(const QList<GpgOperaResult>& results,
                           qsizetype operation_count) -> bool {
  // A missing result is an operation that may still be writing its output.
  if (results.empty() || results.size() != operation_count) return false;

  return std::all_of(results.cbegin(), results.cend(),
                     [](const GpgOperaResult& result) -> bool {
                       return result.op_info.status >= 0;
                     });
}

namespace {

auto CommitStepMessage(SafeOutputCommitStep step) -> QString {
  switch (step) {
    case SafeOutputCommitStep::kSTAGED_OUTPUT_MISSING:
      return QCoreApplication::translate(
          "GpgFrontend::UI::FileOperaOutcome",
          "The staged output to save was missing.");
    case SafeOutputCommitStep::kMOVE_EXISTING_ASIDE:
      return QCoreApplication::translate(
          "GpgFrontend::UI::FileOperaOutcome",
          "The existing file could not be moved aside to be replaced.");
    case SafeOutputCommitStep::kMOVE_INTO_PLACE:
    default:
      return QCoreApplication::translate(
          "GpgFrontend::UI::FileOperaOutcome",
          "The output could not be moved into place.");
  }
}

auto DiscardError(const QString& staged_path) -> OutputStageError {
  return {OutputAction::kDISCARD_STAGED, staged_path, std::nullopt,
          QCoreApplication::translate("GpgFrontend::UI::FileOperaOutcome",
                                      "The staged output could not be "
                                      "removed."),
          OutputErrorOrigin::kAPPLICATION};
}

}  // namespace

auto AssignFileBatchIdentity(QList<GpgOperaResult>& results,
                             const QStringList& staged_paths,
                             const QStringList& input_paths)
    -> InfoBoardBatchContext {
  InfoBoardBatchContext context;
  context.file_target = true;

  for (int i = 0; i < staged_paths.size(); ++i) {
    const auto input = input_paths.value(i);
    context.expected.insert(
        i, {InfoBoardSource::kFILE, u"file:%1:%2"_s.arg(i).arg(input),
            QFileInfo(input).fileName(), input});
  }

  for (auto& result : results) {
    result.input_index =
        static_cast<int>(staged_paths.indexOf(result.staged_output_path));
  }
  return context;
}

void RecordDiscardedOutputs(InfoBoardBatchContext& context,
                            const QList<GpgOperaResult>& results,
                            const QStringList& staged_paths,
                            const QList<bool>& removed) {
  for (const auto& result : results) {
    const auto i = result.input_index;
    if (i < 0 || i >= removed.size()) continue;

    if (!removed.at(i)) {
      context.fates.insert(i, OutputFate::kDISCARD_FAILED);
      context.output_errors[i].append(DiscardError(staged_paths.value(i)));
    } else if (result.op_info.status < 0) {
      context.fates.insert(i, OutputFate::kNOT_PRODUCED);
    } else {
      context.fates.insert(i, OutputFate::kSTAGED_DISCARDED);
    }
  }
}

void RecordCommittedOutputs(InfoBoardBatchContext& context,
                            const QStringList& final_paths,
                            qsizetype failed_index,
                            SafeOutputCommitStep failed_step,
                            const QStringList& staged_paths,
                            const QList<bool>& removed) {
  for (int i = 0; i < final_paths.size(); ++i) {
    if (failed_index < 0 || i < failed_index) {
      context.fates.insert(i, OutputFate::kCOMMITTED);
      context.output_paths.insert(i, final_paths.at(i));
      continue;
    }
    if (i == failed_index) {
      context.fates.insert(i, OutputFate::kCOMMIT_FAILED);
      context.output_errors[i].append(
          {OutputAction::kCOMMIT_FILE, final_paths.at(i), std::nullopt,
           CommitStepMessage(failed_step), OutputErrorOrigin::kAPPLICATION});
    } else {
      context.fates.insert(i, OutputFate::kNOT_COMMITTED);
    }
    if (i < removed.size() && !removed.at(i)) {
      if (i != failed_index)
        context.fates.insert(i, OutputFate::kDISCARD_FAILED);
      context.output_errors[i].append(DiscardError(staged_paths.value(i)));
    }
  }
}

}  // namespace GpgFrontend::UI
