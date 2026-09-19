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

#include "GFCoreTest.h"
#include "core/function/CoreInitProgress.h"
#include "core/function/CoreSignalStation.h"

namespace GpgFrontend::Test {

namespace {

/**
 * @brief The aggregator is process-wide, so every test starts by clearing it.
 *
 * It is a singleton for the same reason ModuleLoadStats is -- it is written to
 * on either side of the lifetime SingletonStorage has -- which means the tests
 * share one instance and must not inherit each other's state.
 */
auto FreshProgress() -> CoreInitProgress& {
  auto& progress = CoreInitProgress::GetInstance();
  progress.Reset();
  return progress;
}

auto Weight(CoreInitStage stage) -> int {
  return CoreInitProgress::StageWeights()[static_cast<size_t>(stage)];
}

/**
 * @brief Records every SignalCoreInitProgress emission, synchronously.
 *
 * Direct rather than queued on purpose: gtest bodies run on a worker thread in
 * this project while the signal station lives on the main one, so an auto
 * connection would queue the emission and the test would finish before it was
 * ever delivered. QtTest is not linked here, which rules out QSignalSpy.
 */
class ProgressRecorder {
 public:
  ProgressRecorder() {
    connection_ = QObject::connect(
        CoreSignalStation::GetInstance(),
        &CoreSignalStation::SignalCoreInitProgress, &context_,
        [this](int percent, CoreInitStep step, QString subject) {
          percents_.append(percent);
          steps_.append(step);
          subjects_.append(subject);
        },
        Qt::DirectConnection);
  }

  ~ProgressRecorder() { QObject::disconnect(connection_); }

  ProgressRecorder(const ProgressRecorder&) = delete;
  auto operator=(const ProgressRecorder&) -> ProgressRecorder& = delete;

  [[nodiscard]] auto Count() const -> int {
    return static_cast<int>(percents_.size());
  }
  [[nodiscard]] auto PercentAt(int i) const -> int { return percents_.at(i); }
  [[nodiscard]] auto SubjectAt(int i) const -> QString {
    return subjects_.at(i);
  }

 private:
  QObject context_;
  QMetaObject::Connection connection_;
  QList<int> percents_;
  QList<CoreInitStep> steps_;
  QList<QString> subjects_;
};

}  // namespace

TEST(CoreInitProgressTest, WeightsSumToOneHundred) {
  auto total = 0;
  for (const auto weight : CoreInitProgress::StageWeights()) total += weight;
  ASSERT_EQ(total, 100);
}

TEST(CoreInitProgressTest, StartsAtZero) {
  auto& progress = FreshProgress();

  ASSERT_EQ(progress.Percent(), 0);
  ASSERT_EQ(progress.CurrentStep(), CoreInitStep::kSTARTING_UP);
  ASSERT_TRUE(progress.CurrentSubject().isEmpty());
}

TEST(CoreInitProgressTest, EveryStageCompleteIsExactlyOneHundred) {
  auto& progress = FreshProgress();

  progress.MarkStageDone(CoreInitStage::kCORE, CoreInitStep::kRESOLVING_PATHS);
  progress.MarkStageDone(CoreInitStage::kENGINE,
                         CoreInitStep::kBUILDING_DEFAULT_CONTEXT);
  progress.MarkStageDone(CoreInitStage::kKEY_DATABASE,
                         CoreInitStep::kLOADING_KEY_DATABASE);
  progress.MarkStageDone(CoreInitStage::kMODULES,
                         CoreInitStep::kLOADING_MODULE);

  ASSERT_EQ(progress.Percent(), 100);
}

TEST(CoreInitProgressTest, BlendsConcurrentStages) {
  auto& progress = FreshProgress();

  // The case the weighting exists for: modules are half done on their own task
  // runner while the core track has finished on another.
  progress.MarkStageDone(CoreInitStage::kCORE, CoreInitStep::kRESOLVING_PATHS);
  progress.Report(CoreInitStage::kMODULES, 0.5, CoreInitStep::kLOADING_MODULE);

  ASSERT_EQ(progress.Percent(),
            Weight(CoreInitStage::kCORE) + Weight(CoreInitStage::kMODULES) / 2);
}

TEST(CoreInitProgressTest, FractionIsClamped) {
  auto& progress = FreshProgress();

  progress.Report(CoreInitStage::kENGINE, -1.0,
                  CoreInitStep::kREFRESHING_BACKEND_ENGINE);
  ASSERT_EQ(progress.Percent(), 0);

  progress.Report(CoreInitStage::kENGINE, 7.0,
                  CoreInitStep::kBUILDING_DEFAULT_CONTEXT);
  ASSERT_EQ(progress.Percent(), Weight(CoreInitStage::kENGINE));
}

TEST(CoreInitProgressTest, NeverGoesBackwards) {
  auto& progress = FreshProgress();

  progress.Report(CoreInitStage::kKEY_DATABASE, 0.8,
                  CoreInitStep::kLOADING_KEY_DATABASE, "big");
  const auto high = progress.Percent();
  ASSERT_GT(high, 0);

  // A later report naming a smaller fraction must not undo what is shown: a
  // bar that jumps back reads as a fault, and the reports arrive from three
  // threads with no ordering between them.
  progress.Report(CoreInitStage::kKEY_DATABASE, 0.2,
                  CoreInitStep::kLOADING_KEY_DATABASE, "small");
  ASSERT_EQ(progress.Percent(), high);

  // The step still follows the latest report, though -- only the number is
  // held.
  ASSERT_EQ(progress.CurrentSubject(), QString("small"));
}

TEST(CoreInitProgressTest, ReportsCurrentStepAndSubject) {
  auto& progress = FreshProgress();

  progress.Report(CoreInitStage::kMODULES, 0.7, CoreInitStep::kLOADING_MODULE,
                  "mod_email");

  ASSERT_EQ(progress.CurrentStep(), CoreInitStep::kLOADING_MODULE);
  ASSERT_EQ(progress.CurrentSubject(), QString("mod_email"));
}

TEST(CoreInitProgressTest, MarkAllDoneIsHundredAndReady) {
  auto& progress = FreshProgress();

  progress.Report(CoreInitStage::kCORE, 0.3, CoreInitStep::kSTARTING_UP);
  progress.MarkAllDone();

  ASSERT_EQ(progress.Percent(), 100);
  ASSERT_EQ(progress.CurrentStep(), CoreInitStep::kREADY);
  ASSERT_TRUE(progress.CurrentSubject().isEmpty());
}

TEST(CoreInitProgressTest, ResetClearsEverything) {
  auto& progress = FreshProgress();

  progress.MarkAllDone();
  ASSERT_EQ(progress.Percent(), 100);

  progress.Reset();
  ASSERT_EQ(progress.Percent(), 0);
  ASSERT_EQ(progress.CurrentStep(), CoreInitStep::kSTARTING_UP);
}

TEST(CoreInitProgressTest, EmitsOnlyWhenSomethingChanged) {
  auto& progress = FreshProgress();

  ProgressRecorder recorder;

  progress.Report(CoreInitStage::kMODULES, 0.5, CoreInitStep::kLOADING_MODULE,
                  "mod_a");
  const auto after_first = recorder.Count();
  ASSERT_EQ(after_first, 1);

  // Identical report: same percent, same step, same subject. The module loop
  // makes several of these in a row, and each one would otherwise queue a
  // cross-thread signal that changes nothing on screen.
  progress.Report(CoreInitStage::kMODULES, 0.5, CoreInitStep::kLOADING_MODULE,
                  "mod_a");
  ASSERT_EQ(recorder.Count(), after_first);

  // Same percent, different subject: still worth saying, because the status
  // line is what moves even when the number does not.
  progress.Report(CoreInitStage::kMODULES, 0.5, CoreInitStep::kLOADING_MODULE,
                  "mod_b");
  ASSERT_EQ(recorder.Count(), after_first + 1);
}

TEST(CoreInitProgressTest, EmittedPercentMatchesTheGetter) {
  auto& progress = FreshProgress();

  ProgressRecorder recorder;

  progress.MarkStageDone(CoreInitStage::kKEY_DATABASE,
                         CoreInitStep::kLOADING_KEY_DATABASE, "default");

  ASSERT_EQ(recorder.Count(), 1);
  ASSERT_EQ(recorder.PercentAt(0), progress.Percent());
  ASSERT_EQ(recorder.SubjectAt(0), QString("default"));
}

}  // namespace GpgFrontend::Test
