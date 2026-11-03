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

#include <optional>

#include "core/function/result_analyse/GpgOpResultInfo.h"
#include "core/function/result_analyse/GpgResultAnalyse.h"
#include "core/model/DataObject.h"
#include "core/model/GFBuffer.h"

/**
 * @file GpgOperaOutcome.h
 * @brief What an operation's result data object comes to, before it is
 * analysed.
 *
 * The rule every interface follows: an operation that stopped before it could
 * be analysed (cancelled, not supported, stopped by GpgFrontend, or with no
 * usable result) keeps its error as recorded.
 */

namespace GpgFrontend {

/// Whether @p err means the operation stopped before there was a result to
/// analyse: cancelled, not supported, or stopped by GpgFrontend.
inline auto IsStoppedBeforeAnalysis(const GFError& err) -> bool {
  return err.code == GFErrorCode::kNOT_SUPPORTED || err.Canceled() ||
         err.Aborted();
}

namespace detail {

/// Whether @p data holds @p Results, followed by the output when
/// @p kWithOutput.
template <bool kWithOutput, typename... Results>
auto HoldsOperaResults(const DataObjectPtr& data) -> bool {
  if (data == nullptr) return false;
  if constexpr (kWithOutput) {
    return data->Check<Results..., GFBuffer>();
  } else {
    return data->Check<Results...>();
  }
}

}  // namespace detail

/**
 * @brief The info of an operation that stopped before it could be analysed,
 * or nothing when it did not.
 *
 * One stage: data object [result, output], or [result] for an operation that
 * wrote its output to a file (@p kWithOutput false).
 */
template <typename Result, typename Analyse, bool kWithOutput = true>
auto StoppedOperaInfo(int channel, const GFError& err,
                      const DataObjectPtr& data)
    -> std::optional<GpgOpResultInfo> {
  const bool holds = detail::HoldsOperaResults<kWithOutput, Result>(data);
  if (!IsStoppedBeforeAnalysis(err) && holds) return std::nullopt;

  QList<QPair<GpgOperation, QString>> details;
  if (holds) {
    auto result = ExtractParams<Result>(data, 0);
    if (!result.ErrorDetail().isEmpty()) {
      details.append({Analyse::kOperation, result.ErrorDetail()});
    }
  }
  return MakeAbortedOpInfo(Analyse::kOperation, err,
                           DescribeOpenPGPEngine(channel), details);
}

/**
 * @brief The same, for two stages of one engine call: data object [result a,
 * result b, output], or [result a, result b] when @p kWithOutput is false.
 */
template <typename ResultA, typename AnalyseA, typename ResultB,
          typename AnalyseB, bool kWithOutput = true>
auto StoppedOperaInfo(int channel, const GFError& err,
                      const DataObjectPtr& data)
    -> std::optional<GpgOpResultInfo> {
  const bool holds =
      detail::HoldsOperaResults<kWithOutput, ResultA, ResultB>(data);
  if (!IsStoppedBeforeAnalysis(err) && holds) return std::nullopt;

  QList<QPair<GpgOperation, QString>> details;
  if (holds) {
    auto a = ExtractParams<ResultA>(data, 0);
    auto b = ExtractParams<ResultB>(data, 1);
    if (!a.ErrorDetail().isEmpty()) {
      details.append({AnalyseA::kOperation, a.ErrorDetail()});
    }
    if (!b.ErrorDetail().isEmpty()) {
      details.append({AnalyseB::kOperation, b.ErrorDetail()});
    }
  }
  return MakeAbortedOpInfo(
      CombineGpgOperations(AnalyseA::kOperation, AnalyseB::kOperation), err,
      DescribeOpenPGPEngine(channel), details);
}

}  // namespace GpgFrontend
