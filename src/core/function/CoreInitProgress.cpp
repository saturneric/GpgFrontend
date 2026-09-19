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

#include "core/function/CoreInitProgress.h"

#include <algorithm>

#include "core/function/CoreSignalStation.h"

namespace GpgFrontend {

namespace {

/**
 * @brief What each track is worth, out of 100.
 *
 * Measured rather than guessed, on a machine with two key databases and four
 * in-tree modules: the keyring flushes dominate, module hashing is the next
 * largest, and everything before an engine exists is nearly free. They must
 * sum to 100 -- a blend that cannot reach its own maximum is a bar that stops
 * short of the end.
 */
constexpr std::array<int, 4> kStageWeights{
    10,  // kCORE
    25,  // kENGINE
    40,  // kKEY_DATABASE
    25,  // kMODULES
};

static_assert(kStageWeights[0] + kStageWeights[1] + kStageWeights[2] +
                      kStageWeights[3] ==
                  100,
              "startup progress weights must sum to 100");

constexpr int kPermille = 1000;

}  // namespace

auto CoreInitProgress::GetInstance() -> CoreInitProgress& {
  static CoreInitProgress instance;
  return instance;
}

auto CoreInitProgress::StageWeights() -> const std::array<int, 4>& {
  return kStageWeights;
}

auto CoreInitProgress::index_of(CoreInitStage stage) -> size_t {
  return static_cast<size_t>(stage);
}

void CoreInitProgress::Report(CoreInitStage stage, double fraction,
                              CoreInitStep step, const QString& subject) {
  const auto clamped = std::clamp(fraction, 0.0, 1.0);
  auto permille = static_cast<int>(clamped * kPermille);

  // Raise only. Within one track the reports are ordered, but nothing stops a
  // later call from naming a smaller fraction (a loop recomputing a ratio
  // against a denominator that grew, say), and letting that through would
  // undo work already shown as done.
  auto& slot = stage_permille_[index_of(stage)];
  auto current = slot.load(std::memory_order_relaxed);
  while (permille > current &&
         !slot.compare_exchange_weak(current, permille,
                                     std::memory_order_relaxed)) {
  }

  publish(step, subject);
}

void CoreInitProgress::MarkStageDone(CoreInitStage stage, CoreInitStep step,
                                     const QString& subject) {
  Report(stage, 1.0, step, subject);
}

void CoreInitProgress::MarkAllDone() {
  for (auto& slot : stage_permille_) {
    slot.store(kPermille, std::memory_order_relaxed);
  }
  publish(CoreInitStep::kREADY, {});
}

void CoreInitProgress::publish(CoreInitStep step, const QString& subject) {
  auto total = 0;
  for (size_t i = 0; i < kStageWeights.size(); ++i) {
    total +=
        kStageWeights[i] * stage_permille_[i].load(std::memory_order_relaxed);
  }
  const auto percent = std::clamp(total / kPermille, 0, 100);

  // Same clamp again, now across tracks: three threads report independently,
  // so the sum one of them computes can be stale by the time it lands here.
  auto shown = shown_percent_.load(std::memory_order_relaxed);
  while (percent > shown && !shown_percent_.compare_exchange_weak(
                                shown, percent, std::memory_order_relaxed)) {
  }
  const auto effective = std::max(shown, percent);

  auto changed = false;
  {
    std::lock_guard<std::mutex> lock(subject_mutex_);
    const auto previous_step = step_.exchange(step, std::memory_order_relaxed);
    if (previous_step != step || subject_ != subject) {
      subject_ = subject;
      changed = true;
    }
  }

  // The module loop reports once per module, and several of those can land on
  // the same percentage. Emitting each one would queue a cross-thread signal
  // that changes nothing on screen.
  if (!changed && effective == shown) return;

  emit CoreSignalStation::GetInstance()
      -> SignalCoreInitProgress(effective, step, subject);
}

auto CoreInitProgress::Percent() const -> int {
  return shown_percent_.load(std::memory_order_relaxed);
}

auto CoreInitProgress::CurrentStep() const -> CoreInitStep {
  return step_.load(std::memory_order_relaxed);
}

auto CoreInitProgress::CurrentSubject() const -> QString {
  std::lock_guard<std::mutex> lock(subject_mutex_);
  return subject_;
}

void CoreInitProgress::Reset() {
  for (auto& slot : stage_permille_) {
    slot.store(0, std::memory_order_relaxed);
  }
  shown_percent_.store(0, std::memory_order_relaxed);
  step_.store(CoreInitStep::kSTARTING_UP, std::memory_order_relaxed);

  std::lock_guard<std::mutex> lock(subject_mutex_);
  subject_.clear();
}

}  // namespace GpgFrontend
