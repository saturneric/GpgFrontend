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

#include "core/model/DataObject.h"
#include "core/model/GFBuffer.h"
#include "shell/result_analyse/GpgOpResultInfo.h"
#include "shell/result_analyse/GpgResultAnalyse.h"

/**
 * @file GpgOperaOutcome.h
 * @brief An OpenPGP operation's result data object, analysed.
 *
 * The rules every interface follows: an operation that stopped before it could
 * be analysed (cancelled, not supported, stopped by GpgFrontend, or with no
 * usable result) keeps its error as recorded; a combined operation's two
 * analyses are merged, and the one engine call's error counted once; and the
 * output is released only when producing it succeeded
 * (GpgOpResultInfo::OutputReleasable()).
 */

namespace GpgFrontend {

/// What an operation came to: its analysis and, where it has one and it may
/// be released, its output.
struct GpgOperaOutcome {
  GpgOpResultInfo info;
  std::optional<GFBuffer> output;
};

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

/// The output at @p index, or nothing when it may not be released.
template <bool kWithOutput>
auto ReleasedOutput(const GpgOpResultInfo& info, const DataObjectPtr& data,
                    int index) -> std::optional<GFBuffer> {
  if constexpr (kWithOutput) {
    if (info.OutputReleasable()) return ExtractParams<GFBuffer>(data, index);
  }
  return std::nullopt;
}

}  // namespace detail

/// Whether @p data holds @p Results, followed by the output when
/// @p kWithOutput: for a caller that picks which of an operation's results
/// to analyse (the phone's Decrypt & Verify of an unsigned message).
using detail::HoldsOperaResults;

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

/// One stage, analysed; see StoppedOperaInfo() for the data object's shape.
template <typename Result, typename Analyse, bool kWithOutput = true>
auto AnalyseOperaOutcome(int channel, const GFError& err,
                         const DataObjectPtr& data) -> GpgOperaOutcome {
  if (auto stopped =
          StoppedOperaInfo<Result, Analyse, kWithOutput>(channel, err, data)) {
    return {*stopped, std::nullopt};
  }

  Analyse analyse(channel, err, ExtractParams<Result>(data, 0));
  analyse.Analyse();
  auto info = analyse.GetOpInfo();
  auto output = detail::ReleasedOutput<kWithOutput>(info, data, 1);
  return {std::move(info), std::move(output)};
}

/// Two stages of one engine call, analysed and merged.
template <typename ResultA, typename AnalyseA, typename ResultB,
          typename AnalyseB, bool kWithOutput = true>
auto AnalyseOperaOutcome(int channel, const GFError& err,
                         const DataObjectPtr& data) -> GpgOperaOutcome {
  if (auto stopped =
          StoppedOperaInfo<ResultA, AnalyseA, ResultB, AnalyseB, kWithOutput>(
              channel, err, data)) {
    return {*stopped, std::nullopt};
  }

  AnalyseA analyse_a(channel, err, ExtractParams<ResultA>(data, 0));
  analyse_a.Analyse();
  AnalyseB analyse_b(channel, err, ExtractParams<ResultB>(data, 1));
  analyse_b.Analyse();

  auto info = analyse_a.GetOpInfo();
  info.Merge(analyse_b.GetOpInfo());
  // Both analysers saw the one engine call's error; it is one failure of the
  // combined call, not one per stage.
  info.CollapseSharedCallError(
      err, CombineGpgOperations(AnalyseA::kOperation, AnalyseB::kOperation));
  auto output = detail::ReleasedOutput<kWithOutput>(info, data, 2);
  return {std::move(info), std::move(output)};
}

}  // namespace GpgFrontend
