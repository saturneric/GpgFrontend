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

#include "core/typedef/CoreTypedef.h"
#include "core/utils/IOUtils.h"
#include "ui/function/InfoBoardReport.h"
#include "ui/struct/GpgOperaResult.h"

namespace GpgFrontend::UI {

/**
 * @brief Whether the staged output of a file or archive operation may be
 * committed to its final path.
 *
 * Status contract (see GpgResultAnalyse::GetStatus): > 0 is success, 0 is
 * valid output with non-fatal warnings (e.g. an unknown recipient or a missing
 * signer key) and is kept, < 0 is a failure and the output is discarded. A
 * successful staging and finalization step is still required on top of this.
 *
 * Exposed primarily for unit testing.
 *
 * @param results the results of every operation that wrote staged output
 * @param operation_count how many operations the batch started; an operation
 *        without a result may still be writing, so it counts as a failure
 * @return true when every operation reported exactly one result and none
 *         failed
 */
auto GF_UI_EXPORT IsFileOperaSuccessful(const QList<GpgOperaResult>& results,
                                        qsizetype operation_count) -> bool;

/**
 * @brief Give a file batch's results their batch identity.
 *
 * Each input is the output staged for it: a result's index in the batch is
 * the position of its staged path in @p staged_paths, which is unique even
 * where the per-category operation indices are not. Every input becomes an
 * expected source, so one that never reports is still listed.
 *
 * @param results the snapshot of results; their input_index is rewritten
 * @param staged_paths each input's staged output path, in batch order
 * @param input_paths each input's own path, in the same order
 */
auto GF_UI_EXPORT AssignFileBatchIdentity(QList<GpgOperaResult>& results,
                                          const QStringList& staged_paths,
                                          const QStringList& input_paths)
    -> InfoBoardBatchContext;

/**
 * @brief Record what the cleanup of a batch that is not committed did.
 *
 * @param removed DiscardSafeOutput()'s answer per staged path, in batch order;
 *        only a confirmed removal is reported as one
 */
void GF_UI_EXPORT RecordDiscardedOutputs(InfoBoardBatchContext& context,
                                         const QList<GpgOperaResult>& results,
                                         const QStringList& staged_paths,
                                         const QList<bool>& removed);

/**
 * @brief Record what committing a batch did.
 *
 * @param final_paths each input's final output path, in batch order
 * @param failed_index the output whose commit failed; -1 when all were
 * @param failed_step which step of that commit failed
 * @param staged_paths each input's staged path, for the cleanup's errors
 * @param removed DiscardSafeOutput()'s answer per staged path after a failed
 *        commit; empty when nothing was cleaned up
 */
void GF_UI_EXPORT RecordCommittedOutputs(InfoBoardBatchContext& context,
                                         const QStringList& final_paths,
                                         qsizetype failed_index,
                                         SafeOutputCommitStep failed_step,
                                         const QStringList& staged_paths,
                                         const QList<bool>& removed);

}  // namespace GpgFrontend::UI
