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

#include <array>
#include <atomic>
#include <mutex>

#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

/**
 * @brief How far startup has got, as one number between 0 and 100.
 *
 * The four startup tracks do not run in sequence. Modules are discovered,
 * hashed and registered on the Module task runner while the core is still
 * asking gpgconf where GnuPG lives on the Default one, and the key databases
 * past the first are flushed on yet another task afterwards. A progress bar
 * that followed any single one of them would sit still for most of a start.
 *
 * So each track reports its own completion fraction, and the percentage shown
 * is the weighted sum of all four. The weights are a claim about relative
 * cost, not about order.
 *
 * Two properties matter to anyone reading the bar, and both are enforced here
 * rather than hoped for at the call sites:
 *
 *  - It never goes backwards. Reports arrive from three threads with no
 *    ordering between them, and a bar that jumps back is read as a fault.
 *  - It only speaks when something changed. The module loop would otherwise
 *    push a queued cross-thread signal per module for no visible difference.
 *
 * Process-wide and deliberately not a SingletonStorage member: it is written
 * to during the earliest and latest moments of a run, on either side of the
 * lifetime that storage has -- the same reason ModuleLoadStats stands alone.
 */
class GF_CORE_EXPORT CoreInitProgress {
 public:
  static auto GetInstance() -> CoreInitProgress&;

  /**
   * @brief Publish one track's completion fraction and what it is doing.
   *
   * @param stage which track
   * @param fraction how much of it is done, clamped to [0, 1]
   * @param step what is happening right now, for the status line
   * @param subject the key database or module name, where the step names one
   */
  void Report(CoreInitStage stage, double fraction, CoreInitStep step,
              const QString& subject = {});

  /// Report(stage, 1.0, ...): the track is finished.
  void MarkStageDone(CoreInitStage stage, CoreInitStep step,
                     const QString& subject = {});

  /// Everything is done. 100% and kREADY, whatever the tracks last said.
  void MarkAllDone();

  /// The percentage to show. Monotonically non-decreasing within a run.
  [[nodiscard]] auto Percent() const -> int;

  [[nodiscard]] auto CurrentStep() const -> CoreInitStep;

  [[nodiscard]] auto CurrentSubject() const -> QString;

  /**
   * @brief Back to zero, for the start of a run.
   *
   * A deep restart re-enters initialization in the same process, and a test
   * that asserts on a blend needs to start from a known state.
   */
  void Reset();

  /// The weight each track carries out of 100. Sums to 100.
  static auto StageWeights() -> const std::array<int, 4>&;

 private:
  CoreInitProgress() = default;

  /// Recompute, clamp monotonically, and emit if anything visibly changed.
  void publish(CoreInitStep step, const QString& subject);

  static auto index_of(CoreInitStage stage) -> size_t;

  /// Per-track completion in permille, so the blend stays integral.
  std::array<std::atomic<int>, 4> stage_permille_{};

  /// The last percentage handed out. Only ever raised.
  std::atomic<int> shown_percent_{0};

  std::atomic<CoreInitStep> step_{CoreInitStep::kSTARTING_UP};

  mutable std::mutex subject_mutex_;
  QString subject_;
};

}  // namespace GpgFrontend
