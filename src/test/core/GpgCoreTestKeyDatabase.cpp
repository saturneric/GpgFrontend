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
#include "core/function/CoreSignalStation.h"
#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

auto MakeItem(const QString& name, const QString& path,
              const QString& backend_type = {}, int channel = 0)
    -> KeyDatabaseItemSO {
  KeyDatabaseItemSO item;
  item.name = name;
  item.path = path;
  item.backend_type = backend_type;
  item.channel = channel;
  return item;
}

auto FindByName(const QContainer<KeyDatabaseItemSO>& list, const QString& name)
    -> const KeyDatabaseItemSO* {
  for (const auto& item : list) {
    if (item.name == name) return &item;
  }
  return nullptr;
}

const KeyDatabaseItemSO kDefaultDb =
    MakeItem("DEFAULT", "/app-data/rpgp_db", "rpgp", 0);

const QSet<QString> kLiteBackends = {"rpgp"};           // macOS lite build
const QSet<QString> kFullBackends = {"gnupg", "rpgp"};  // Flathub build

auto MakeInfo(const QString& name, const QString& path, bool valid,
              int channel = 0) -> KeyDatabaseInfo {
  KeyDatabaseInfo info;
  info.name = name;
  info.path = path;
  info.origin_path = path;
  info.channel = channel;
  info.backend_type = "gnupg";
  info.valid = valid;
  return info;
}

const KeyDatabaseInfo kValidFallback =
    MakeInfo("DEFAULT", "/app-data/rpgp_db", true);

}  // namespace

// With nothing on disk and no stored settings, only the channel-0 DEFAULT
// database survives.
TEST_F(GFCoreTest, KeyDatabaseReconcileEmpty) {
  auto result =
      ReconcileSandboxKeyDatabaseList(kDefaultDb, {}, {}, kLiteBackends);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].name, QString("DEFAULT"));
  EXPECT_EQ(result[0].channel, 0);
  EXPECT_EQ(result[0].path, QString("/app-data/rpgp_db"));
  EXPECT_EQ(result[0].backend_type, QString("rpgp"));
}

// Databases discovered on disk but absent from settings get sequential channels
// and the default supported backend.
TEST_F(GFCoreTest, KeyDatabaseReconcileDiscoverNew) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("alpha", "/app-data/dbs/alpha"),
      MakeItem("beta", "/app-data/dbs/beta"),
  };

  auto result = ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, {},
                                                kLiteBackends);

  ASSERT_EQ(result.size(), 3);

  const auto* alpha = FindByName(result, "alpha");
  const auto* beta = FindByName(result, "beta");
  ASSERT_NE(alpha, nullptr);
  ASSERT_NE(beta, nullptr);

  EXPECT_EQ(alpha->path, QString("/app-data/dbs/alpha"));
  EXPECT_EQ(alpha->backend_type, QString("rpgp"));
  EXPECT_EQ(beta->backend_type, QString("rpgp"));

  // channels are unique and ascending, DEFAULT stays at 0
  EXPECT_EQ(FindByName(result, "DEFAULT")->channel, 0);
  EXPECT_NE(alpha->channel, beta->channel);
  EXPECT_NE(alpha->channel, 0);
  EXPECT_NE(beta->channel, 0);
}

// Metadata (backend type + channel) is recovered from settings by name for a
// database that still exists on disk.
TEST_F(GFCoreTest, KeyDatabaseReconcileRecoverMetadata) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("work", "/app-data/dbs/work"),
  };
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("work", "/stale/old/path", "rpgp", 5),
  };

  auto result = ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, stored,
                                                kLiteBackends);

  const auto* work = FindByName(result, "work");
  ASSERT_NE(work, nullptr);
  // path comes from the scan, not the stale stored value
  EXPECT_EQ(work->path, QString("/app-data/dbs/work"));
  EXPECT_EQ(work->backend_type, QString("rpgp"));
  EXPECT_EQ(work->channel, 5);
}

// A stored database whose directory no longer exists on disk is dropped.
TEST_F(GFCoreTest, KeyDatabaseReconcileDropMissing) {
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("ghost", "/app-data/dbs/ghost", "rpgp", 1),
  };

  auto result =
      ReconcileSandboxKeyDatabaseList(kDefaultDb, {}, stored, kLiteBackends);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(FindByName(result, "ghost"), nullptr);
  EXPECT_NE(FindByName(result, "DEFAULT"), nullptr);
}

// In the rpgp-only lite build a stale "gnupg" backend type carried over in
// settings must be replaced with a supported backend.
TEST_F(GFCoreTest, KeyDatabaseReconcileUnsupportedBackendLite) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("legacy", "/app-data/dbs/legacy"),
  };
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("legacy", "/app-data/dbs/legacy", "gnupg", 2),
  };

  auto result = ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, stored,
                                                kLiteBackends);

  const auto* legacy = FindByName(result, "legacy");
  ASSERT_NE(legacy, nullptr);
  EXPECT_EQ(legacy->backend_type, QString("rpgp"));
}

// When gnupg is available (full build), a stored gnupg type is honoured and the
// default database prefers gnupg.
TEST_F(GFCoreTest, KeyDatabaseReconcileSupportedBackendFull) {
  auto default_db = MakeItem("DEFAULT", "/app-data/db", "gnupg", 0);
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("legacy", "/app-data/dbs/legacy"),
  };
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("legacy", "/app-data/dbs/legacy", "gnupg", 2),
  };

  auto result = ReconcileSandboxKeyDatabaseList(default_db, discovered, stored,
                                                kFullBackends);

  EXPECT_EQ(FindByName(result, "DEFAULT")->backend_type, QString("gnupg"));
  EXPECT_EQ(FindByName(result, "legacy")->backend_type, QString("gnupg"));
}

// A directory accidentally named "DEFAULT" must not shadow the channel-0
// default database.
TEST_F(GFCoreTest, KeyDatabaseReconcileNoDefaultShadow) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("DEFAULT", "/app-data/dbs/DEFAULT"),
  };

  auto result = ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, {},
                                                kLiteBackends);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].path, QString("/app-data/rpgp_db"));
  EXPECT_EQ(result[0].channel, 0);
}

// Duplicate channels (DEFAULT at 0 plus a stored entry also at 0) are resolved
// to unique, ascending channels.
TEST_F(GFCoreTest, KeyDatabaseReconcileChannelCollision) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("dup", "/app-data/dbs/dup"),
  };
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("dup", "/app-data/dbs/dup", "rpgp", 0),
  };

  auto result = ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, stored,
                                                kLiteBackends);

  ASSERT_EQ(result.size(), 2);
  EXPECT_NE(result[0].channel, result[1].channel);
  EXPECT_EQ(FindByName(result, "DEFAULT")->channel, 0);
  EXPECT_GT(FindByName(result, "dup")->channel, 0);
}

// Valid entries are kept as-is and no fallback is injected alongside them.
TEST_F(GFCoreTest, KeyDatabaseSelectUsableKeepsValidOnly) {
  const QContainer<KeyDatabaseInfo> all{
      MakeInfo("gone", "/mnt/detached/db", false),
      MakeInfo("alive", "/home/u/.gnupg", true),
      MakeInfo("also-gone", "/mnt/other/db", false),
  };

  auto result = SelectUsableKeyDatabases(all, kValidFallback);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].name, QString("alive"));
}

// The Leg C regression test. A non-empty list whose every entry is invalid used
// to filter down to nothing and abort startup with "No valid Key Database".
TEST_F(GFCoreTest, KeyDatabaseSelectUsableReseedsWhenAllInvalid) {
  const QContainer<KeyDatabaseInfo> all{
      MakeInfo("gone", "/mnt/detached/db", false, 0),
      MakeInfo("also-gone", "/mnt/other/db", false, 1),
  };

  auto result = SelectUsableKeyDatabases(all, kValidFallback);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].name, QString("DEFAULT"));
  EXPECT_EQ(result[0].channel, 0);
  EXPECT_TRUE(result[0].valid);
}

TEST_F(GFCoreTest, KeyDatabaseSelectUsableEmptyInputUsesFallback) {
  auto result = SelectUsableKeyDatabases({}, kValidFallback);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].name, QString("DEFAULT"));
}

// A genuinely dead environment must still be reported as such, so startup can
// fail with an accurate message rather than pretending a database exists.
TEST_F(GFCoreTest, KeyDatabaseSelectUsableGivesUpWhenFallbackInvalid) {
  const QContainer<KeyDatabaseInfo> all{
      MakeInfo("gone", "/mnt/detached/db", false),
  };
  const auto bad_fallback = MakeInfo("DEFAULT", "", false);

  EXPECT_TRUE(SelectUsableKeyDatabases(all, bad_fallback).isEmpty());
}

// The front entry becomes channel 0, so ordering is load-bearing.
TEST_F(GFCoreTest, KeyDatabaseSelectUsablePreservesOrder) {
  const QContainer<KeyDatabaseInfo> all{
      MakeInfo("first", "/a", true, 0),
      MakeInfo("broken", "/mnt/detached/db", false, 1),
      MakeInfo("second", "/b", true, 2),
  };

  auto result = SelectUsableKeyDatabases(all, kValidFallback);

  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].name, QString("first"));
  EXPECT_EQ(result[1].name, QString("second"));
}

TEST_F(GFCoreTest, KeyDatabasePathActionUsesExistingDir) {
  EXPECT_EQ(DecideKeyDatabasePathAction(true, false, true, false),
            KeyDatabasePathAction::kUSE_AS_IS);
}

// A deleted database whose parent survives is recoverable -- this is the case
// the old code got wrong by creating the parent instead of the leaf.
TEST_F(GFCoreTest, KeyDatabasePathActionCreatesLeafWhenParentExists) {
  EXPECT_EQ(DecideKeyDatabasePathAction(false, false, true, false),
            KeyDatabasePathAction::kCREATE_LEAF);
}

// A whole missing chain outside app data usually means an unmounted volume.
// Materialising it there would hand GnuPG an empty keyring in the wrong place.
TEST_F(GFCoreTest, KeyDatabasePathActionRefusesFullChainOutsideAppData) {
  EXPECT_EQ(DecideKeyDatabasePathAction(false, false, false, false),
            KeyDatabasePathAction::kREJECT);
}

TEST_F(GFCoreTest, KeyDatabasePathActionCreatesFullChainInsideAppData) {
  EXPECT_EQ(DecideKeyDatabasePathAction(false, false, false, true),
            KeyDatabasePathAction::kCREATE_FULL);
}

TEST_F(GFCoreTest, KeyDatabasePathActionRejectsExistingFile) {
  EXPECT_EQ(DecideKeyDatabasePathAction(false, true, true, true),
            KeyDatabasePathAction::kREJECT);
}

// A stored entry without a "channel" field must not surface an indeterminate
// value into channel normalization.
TEST_F(GFCoreTest, KeyDatabaseItemChannelDefaultsToZero) {
  const KeyDatabaseItemSO item(QJsonObject{{"name", "x"}, {"path", "/x"}});

  EXPECT_EQ(item.channel, 0);
}

TEST_F(GFCoreTest, ChooseEngineHonoursGnupgPreferenceWhenSupported) {
  const auto choice = ChooseOpenPGPEngine("GNUPG", true, true);

  EXPECT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kGNUPG);
}

TEST_F(GFCoreTest, ChooseEngineHonoursRpgpPreferenceWhenSupported) {
  const auto choice = ChooseOpenPGPEngine("rpgp", true, true);

  EXPECT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kRPGP);
}

TEST_F(GFCoreTest, ChooseEngineFallsBackToRpgpWhenGnupgUnsupported) {
  const auto choice = ChooseOpenPGPEngine("GNUPG", false, true);

  EXPECT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kRPGP);
}

// The asymmetry this check exists to fix: a database stored as "rpgp" by a
// build that had it, opened by a build that does not.
TEST_F(GFCoreTest, ChooseEngineFallsBackToGnupgWhenRpgpUnsupported) {
  const auto choice = ChooseOpenPGPEngine("rpgp", true, false);

  EXPECT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kGNUPG);
}

TEST_F(GFCoreTest, ChooseEngineFailsWhenNeitherSupported) {
  EXPECT_FALSE(ChooseOpenPGPEngine("rpgp", false, false).ok);
  EXPECT_FALSE(ChooseOpenPGPEngine("", false, false).ok);
}

TEST_F(GFCoreTest, ChooseEngineIsCaseAndWhitespaceInsensitive) {
  EXPECT_EQ(ChooseOpenPGPEngine(" GnuPG ", true, true).engine,
            OpenPGPEngine::kGNUPG);
  EXPECT_EQ(ChooseOpenPGPEngine("RPGP", true, true).engine,
            OpenPGPEngine::kRPGP);
}

TEST_F(GFCoreTest, ChooseEngineDefaultsToGnupgOnEmptyPreference) {
  const auto choice = ChooseOpenPGPEngine("", true, true);

  EXPECT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kGNUPG);
}

// SignalBadOpenPGPEnv is emitted from a worker thread and received on the main
// thread, so its custom enum argument has to survive a queued connection. If
// the metatype registration and the spelling moc records ever drift apart, Qt
// drops the emission silently -- and because this signal is what releases the
// startup event loop, the app would hang on the loading dialog forever. That
// failure is invisible without a test like this one.
TEST_F(GFCoreTest, BadOpenPGPEnvReasonSurvivesQueuedConnection) {
  auto* station = CoreSignalStation::GetInstance();

  std::atomic<bool> received = false;
  auto got_reason = BadOpenPGPEnvReason::kUNKNOWN;
  QString got_detail;

  QEventLoop loop;
  QObject context;

  QObject::connect(
      station, &CoreSignalStation::SignalBadOpenPGPEnv, &context,
      [&](BadOpenPGPEnvReason reason, const QString& detail) {
        got_reason = reason;
        got_detail = detail;
        received = true;
        loop.quit();
      },
      Qt::QueuedConnection);

  QThread* worker = QThread::create([station]() {
    emit station->SignalBadOpenPGPEnv(
        BadOpenPGPEnvReason::kNO_VALID_KEY_DATABASE, "detail-payload");
  });

  QTimer::singleShot(5000, &loop, &QEventLoop::quit);
  worker->start();
  loop.exec();

  worker->wait();
  worker->deleteLater();

  ASSERT_TRUE(received.load());
  EXPECT_EQ(got_reason, BadOpenPGPEnvReason::kNO_VALID_KEY_DATABASE);
  EXPECT_EQ(got_detail, QString("detail-payload"));
}

}  // namespace GpgFrontend::Test
