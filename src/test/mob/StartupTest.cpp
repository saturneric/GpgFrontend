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
#include <functional>
#include <memory>

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
  // The profile outlives the run: earlier runs left keys of this name.
  const auto mine = [keys]() {
    int count = 0;
    RunOnMainThread([keys, &count]() {
      KeyFilterModel filter;
      filter.setSourceModel(keys->Keys());
      filter.SetSearchText("mobile.test@");
      filter.SetOnlySecret(true);
      count = filter.rowCount();
    });
    return count;
  };
  const int mine_before = mine();

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

  EXPECT_EQ(mine(), mine_before + 1);
}

TEST(StartupTest, ARefreshThatNeverReportsBackStillEnds) {
  // A controller of the test's own: the application's stays as it is.
  std::vector<std::function<void()>> held;
  KeyListController::SetRefresherForTesting(
      [&held](QObject*, std::function<void()> done) {
        held.push_back(std::move(done));
      });
  std::unique_ptr<KeyListController> keys;
  int refreshed = 0;
  RunOnMainThread([&]() {
    keys = std::make_unique<KeyListController>(nullptr);
    keys->SetRefreshTimeoutForTesting(50);
    QObject::connect(keys.get(), &KeyListController::SignalRefreshed,
                     keys.get(), [&refreshed]() { ++refreshed; });
    keys->refresh();
    EXPECT_TRUE(keys->Refreshing());
    EXPECT_FALSE(keys->Loaded());
  });

  // Ended by the wait, showing what the default database holds, and saying
  // it did not end as it should.
  ASSERT_TRUE(WaitFor(
      [&]() {
        bool refreshing = true;
        RunOnMainThread([&]() { refreshing = keys->Refreshing(); });
        return !refreshing;
      },
      10000));
  RunOnMainThread([&]() {
    EXPECT_EQ(refreshed, 1);
    EXPECT_NE(keys->Keys()->sourceModel(), nullptr);
    EXPECT_TRUE(keys->Loaded());
    EXPECT_FALSE(keys->Error().isEmpty());
    // Asked again after a run that may never report back: it starts now,
    // not after a report that may never come (the core runs the two in
    // turn on its one task thread).
    keys->refresh();
    EXPECT_TRUE(keys->Refreshing());
    EXPECT_EQ(held.size(), 2U);
    // The late report of the first changes nothing ...
    held[0]();
    EXPECT_EQ(refreshed, 1);
    EXPECT_TRUE(keys->Refreshing());
    // ... the end of the one asked for is shown, without the error.
    held[1]();
    EXPECT_EQ(refreshed, 2);
    EXPECT_FALSE(keys->Refreshing());
    EXPECT_TRUE(keys->Error().isEmpty());
    keys.reset();
  });
  KeyListController::SetRefresherForTesting({});
}

// A refresh asked for while one runs whose end is then lost: the timeout
// starts it, rather than leave the list refreshing for good.
TEST(StartupTest, ARefreshQueuedBehindALostOneStillRuns) {
  std::vector<std::function<void()>> held;
  KeyListController::SetRefresherForTesting(
      [&held](QObject*, std::function<void()> done) {
        held.push_back(std::move(done));
      });
  std::unique_ptr<KeyListController> keys;
  int refreshed = 0;
  RunOnMainThread([&]() {
    keys = std::make_unique<KeyListController>(nullptr);
    keys->SetRefreshTimeoutForTesting(50);
    QObject::connect(keys.get(), &KeyListController::SignalRefreshed,
                     keys.get(), [&refreshed]() { ++refreshed; });
    keys->refresh();
    keys->refresh();  // queued behind the first
    EXPECT_EQ(held.size(), 1U);
  });
  // The first never reports back: its timeout starts the queued one.
  ASSERT_TRUE(WaitFor(
      [&]() {
        size_t started = 0;
        RunOnMainThread([&]() { started = held.size(); });
        return started == 2;
      },
      10000));
  RunOnMainThread([&]() {
    EXPECT_TRUE(keys->Refreshing());
    held[1]();
    EXPECT_EQ(refreshed, 1);
    EXPECT_FALSE(keys->Refreshing());
    EXPECT_TRUE(keys->Error().isEmpty());
    // The lost one's late report changes nothing.
    held[0]();
    EXPECT_EQ(refreshed, 1);
    keys.reset();
  });
  KeyListController::SetRefresherForTesting({});
}

// The end of startup shows what the core has just read, without reading
// every key database a second time; a refresh under way is left to show it.
TEST(StartupTest, ShowingTheLoadedKeysReadsNothingAgain) {
  // The application's own list, shown when the startup ended.
  auto* app_keys = KeyListController::create(nullptr, nullptr);
  ASSERT_NE(app_keys, nullptr);
  RunOnMainThread([app_keys]() { EXPECT_TRUE(app_keys->Loaded()); });

  std::vector<std::function<void()>> held;
  KeyListController::SetRefresherForTesting(
      [&held](QObject*, std::function<void()> done) {
        held.push_back(std::move(done));
      });
  std::unique_ptr<KeyListController> keys;
  int refreshed = 0;
  RunOnMainThread([&]() {
    keys = std::make_unique<KeyListController>(nullptr);
    QObject::connect(keys.get(), &KeyListController::SignalRefreshed,
                     keys.get(), [&refreshed]() { ++refreshed; });
    keys->ShowLoaded();
    EXPECT_TRUE(held.empty());
    EXPECT_EQ(refreshed, 1);
    EXPECT_TRUE(keys->Loaded());
    EXPECT_FALSE(keys->Refreshing());
    EXPECT_NE(keys->Keys()->sourceModel(), nullptr);

    // While a refresh runs it changes nothing: the refresh shows its own.
    keys->refresh();
    keys->ShowLoaded();
    EXPECT_EQ(refreshed, 1);
    EXPECT_TRUE(keys->Refreshing());
    ASSERT_EQ(held.size(), 1U);
    held[0]();
    EXPECT_EQ(refreshed, 2);
    EXPECT_FALSE(keys->Refreshing());
    keys.reset();
  });
  KeyListController::SetRefresherForTesting({});
}

// Switching A, then B, then back to A while the key list is read: the core
// never refreshes twice at once, each run reads the database in use when it
// starts, and only the last one asked for changes what is shown.
TEST(StartupTest, SwitchingBackAndForthShowsTheLastDatabaseOnly) {
  QString database = "A";
  struct Run {
    QString database;
    std::function<void()> done;
  };
  std::vector<Run> runs;
  int outstanding = 0;
  int most = 0;
  KeyListController::SetDatabaseIdForTesting(
      [&database]() { return database; });
  KeyListController::SetRefresherForTesting(
      [&](QObject*, std::function<void()> done) {
        most = std::max(most, ++outstanding);
        runs.push_back({database, [&outstanding, done = std::move(done)]() {
                          --outstanding;
                          done();
                        }});
      });
  std::unique_ptr<KeyListController> keys;
  int refreshed = 0;
  RunOnMainThread([&]() {
    keys = std::make_unique<KeyListController>(nullptr);
    QObject::connect(keys.get(), &KeyListController::SignalRefreshed,
                     keys.get(), [&refreshed]() { ++refreshed; });
    keys->refresh();  // A
    database = "B";
    keys->refresh();  // B, while A's runs
    database = "A";
    keys->refresh();  // A again
    ASSERT_EQ(runs.size(), 1U);
    EXPECT_EQ(runs[0].database, "A");

    // The first run ends: not shown, it is not the latest. The latest runs.
    runs[0].done();
    EXPECT_EQ(refreshed, 0);
    EXPECT_TRUE(keys->Refreshing());
    EXPECT_FALSE(keys->Loaded());
    ASSERT_EQ(runs.size(), 2U);
    EXPECT_EQ(runs[1].database, "A");

    runs[1].done();
    EXPECT_EQ(refreshed, 1);
    EXPECT_FALSE(keys->Refreshing());
    EXPECT_TRUE(keys->Loaded());
    EXPECT_EQ(keys->LoadedDatabase(), "A");
    EXPECT_EQ(most, 1);

    // A database that came into use while a run went on, without anyone
    // asking: what the run read may be the old one's, so it runs again.
    keys->refresh();
    database = "B";
    runs[2].done();
    EXPECT_TRUE(keys->Refreshing());
    ASSERT_EQ(runs.size(), 4U);
    EXPECT_EQ(runs[3].database, "B");
    runs[3].done();
    EXPECT_EQ(keys->LoadedDatabase(), "B");
    EXPECT_FALSE(keys->Refreshing());
    EXPECT_EQ(most, 1);
    keys.reset();
  });
  KeyListController::SetRefresherForTesting({});
  KeyListController::SetDatabaseIdForTesting({});
}

// A run that timed out and reports back late, after a refresh of another
// database was asked for, does not show its own result: the one asked for
// runs then, and its end alone is shown.
TEST(StartupTest, ALateReportOfAnEarlierRefreshShowsNothingOfItsOwn) {
  QString database = "A";
  std::vector<std::pair<QString, std::function<void()>>> runs;
  KeyListController::SetDatabaseIdForTesting(
      [&database]() { return database; });
  KeyListController::SetRefresherForTesting(
      [&](QObject*, std::function<void()> done) {
        runs.emplace_back(database, std::move(done));
      });
  std::unique_ptr<KeyListController> keys;
  int refreshed = 0;
  RunOnMainThread([&]() {
    keys = std::make_unique<KeyListController>(nullptr);
    keys->SetRefreshTimeoutForTesting(50);
    QObject::connect(keys.get(), &KeyListController::SignalRefreshed,
                     keys.get(), [&refreshed]() { ++refreshed; });
    keys->refresh();
  });
  ASSERT_TRUE(WaitFor(
      [&]() {
        bool refreshing = true;
        RunOnMainThread([&]() { refreshing = keys->Refreshing(); });
        return !refreshing;
      },
      10000));
  RunOnMainThread([&]() {
    EXPECT_EQ(keys->LoadedDatabase(), "A");
    database = "B";
    keys->refresh();
    EXPECT_FALSE(keys->Loaded());
    const int before = refreshed;
    // B's run starts at once; A's late report shows nothing of A's.
    ASSERT_EQ(runs.size(), 2U);
    EXPECT_EQ(runs[1].first, "B");
    runs[0].second();
    EXPECT_EQ(refreshed, before);
    EXPECT_TRUE(keys->Refreshing());
    EXPECT_FALSE(keys->Loaded());
    runs[1].second();
    EXPECT_EQ(keys->LoadedDatabase(), "B");
    EXPECT_TRUE(keys->Loaded());
    EXPECT_TRUE(keys->Error().isEmpty());
    keys.reset();
  });
  KeyListController::SetRefresherForTesting({});
  KeyListController::SetDatabaseIdForTesting({});
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
