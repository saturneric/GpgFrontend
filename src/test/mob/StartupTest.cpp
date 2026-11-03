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

#include <QElapsedTimer>
#include <QProcess>
#include <QTemporaryDir>

#include "MobileTest.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"
#include "model/KeyFilterModel.h"
#include "model/KeyListController.h"
#include "startup/StartupController.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::KeyFilterModel;
using GpgFrontend::Mobile::KeyListController;
using GpgFrontend::Mobile::StartupController;

namespace {

struct ScenarioRun {
  int exit_code = -1;
  QString output;
  qint64 elapsed_ms = 0;
};

// The runner itself, acting out one startup in a fresh profile (main.cpp).
auto RunScenario(const QString& name, const QProcessEnvironment& extra = {})
    -> ScenarioRun {
  QTemporaryDir home;
  auto env = QProcessEnvironment::systemEnvironment();
  for (const auto& dir : {"HOME", "XDG_DATA_HOME", "XDG_CONFIG_HOME",
                          "XDG_CACHE_HOME", "XDG_STATE_HOME"}) {
    env.insert(dir, home.path() + "/" + dir);
  }
  env.insert("QT_QPA_PLATFORM", "offscreen");
  env.insert(extra);

  QProcess process;
  process.setProcessEnvironment(env);
  process.setProcessChannelMode(QProcess::MergedChannels);

  QElapsedTimer timer;
  timer.start();
  process.start(QCoreApplication::applicationFilePath(),
                {"--scenario=" + name});
  process.waitForFinished(60000);

  ScenarioRun run;
  run.elapsed_ms = timer.elapsed();
  run.output = QString::fromUtf8(process.readAll());
  run.exit_code =
      process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
  return run;
}

}  // namespace

TEST(StartupTest, TheCoreIsReadyWithoutBlockingTheInterface) {
  // main() started this process the way the application starts; by the time
  // the tests run the startup has reached the end.
  auto* startup = StartupController::create(nullptr, nullptr);
  ASSERT_NE(startup, nullptr);
  EXPECT_EQ(startup->GetState(), StartupController::State::kREADY);
  EXPECT_EQ(startup->Progress(), 100);
}

TEST(StartupTest, TheKeyListShowsANewKey) {
  auto* keys = KeyListController::create(nullptr, nullptr);
  ASSERT_NE(keys, nullptr);
  // Counted fresh: tests that ran before may have added keys without
  // refreshing the list.
  RunOnMainThread([keys]() { keys->refresh(); });
  ASSERT_TRUE(WaitFor([keys]() { return !keys->Refreshing(); }, 10000));
  const int before = keys->Keys()->rowCount();

  auto key = SecureCreateSharedObject<KeyGenerateInfo>();
  key->SetName("Mobile Test");
  key->SetEmail("mobile.test@example.invalid");
  key->SetAlgo(std::get<1>(KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519")));
  key->SetNonPassPhrase(true);
  key->SetKeyVersion(4);
  auto [err, data] =
      KeyGenerationOperation::GetInstance(UsableDefaultChannelHandle())
          .GenerateKeySync(key);
  ASSERT_TRUE(err.Ok());

  RunOnMainThread([keys]() { keys->refresh(); });
  ASSERT_TRUE(WaitFor([keys]() { return !keys->Refreshing(); }, 10000));
  EXPECT_EQ(keys->Keys()->rowCount(), before + 1);

  RunOnMainThread([keys]() {
    KeyFilterModel mine;
    mine.setSourceModel(keys->Keys());
    mine.SetSearchText("mobile.test@");
    mine.SetOnlySecret(true);
    EXPECT_EQ(mine.rowCount(), 1);
  });
}

TEST(StartupTest, AProfileHeldByALiveProcessIsNotForced) {
  const auto run = RunScenario("held-lock");
  EXPECT_EQ(run.exit_code, 0) << run.output.toStdString();
  EXPECT_TRUE(run.output.contains("asked: confirm_force_unlock"));
  EXPECT_TRUE(run.output.contains("state: kFAILED"));
}

TEST(StartupTest, QuittingWhileAQuestionWaitsEndsCleanly) {
  const auto run = RunScenario("pending-shutdown");
  EXPECT_EQ(run.exit_code, 0) << run.output.toStdString();
  EXPECT_TRUE(run.output.contains("shutdown: finished"));
}

TEST(StartupTest, QuittingWhileAnOperationWaitsAtAPromptEndsCleanly) {
  const auto run = RunScenario("passphrase-shutdown");
  EXPECT_EQ(run.exit_code, 0) << run.output.toStdString();
  EXPECT_TRUE(run.output.contains("asked: passphrase"));
  EXPECT_TRUE(run.output.contains("shutdown: finished"));
}

TEST(StartupTest, AWorkerThatWillNotFinishIsAbandoned) {
  QProcessEnvironment slow;
  slow.insert("GF_STARTUP_TEST_DELAY_MS", "20000");
  const auto run = RunScenario("stuck-worker", slow);
  EXPECT_EQ(run.exit_code, StartupController::kAbandonedExitCode)
      << run.output.toStdString();
  // Bounded by the shutdown timeout, not by the worker.
  EXPECT_LT(run.elapsed_ms, 15000);
}

}  // namespace GpgFrontend::Test::Mobile
