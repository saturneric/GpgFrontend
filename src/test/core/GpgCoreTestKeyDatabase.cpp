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
#include "core/module/ModuleManager.h"
#include "core/profile/Profile.h"
#include "core/profile/ProfileRegistry.h"
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

// ------------------------------------------------- the reserved DEFAULT name

TEST_F(GFCoreTest, TheDefaultNameBelongsToTheApplication) {
  // DEFAULT is derived, not stored: its path comes from the engine and it is
  // the one database `basic/default_engine` speaks for. A second thing wearing
  // that name is found first by every lookup that goes by name, and takes the
  // default engine along with the identity.
  EXPECT_TRUE(IsReservedKeyDatabaseName("DEFAULT"));
}

TEST_F(GFCoreTest, TheReservedNameIsNotReservedOnlyInOneSpelling) {
  // The name becomes a folder, and "default" and "DEFAULT" are the same folder
  // on Windows and on macOS. Reserving the exact spelling alone would reserve
  // nothing on either.
  EXPECT_TRUE(IsReservedKeyDatabaseName("default"));
  EXPECT_TRUE(IsReservedKeyDatabaseName("Default"));
  EXPECT_TRUE(IsReservedKeyDatabaseName("  DEFAULT  "));
}

TEST_F(GFCoreTest, AnOrdinaryNameIsNotReserved) {
  EXPECT_FALSE(IsReservedKeyDatabaseName("Key DB 2"));
  EXPECT_FALSE(IsReservedKeyDatabaseName("DEFAULTS"));
  EXPECT_FALSE(IsReservedKeyDatabaseName("My DEFAULT"));
  EXPECT_FALSE(IsReservedKeyDatabaseName(""));
}

TEST_F(GFCoreTest, TheDerivedDefaultDatabaseNamesItselfReservedly) {
  // What the dialog's Default mode adds. The path depends on the engine this
  // machine has, so it is not asserted -- the identity is.
  const auto candidate = DefaultKeyDatabaseCandidate();

  EXPECT_TRUE(IsReservedKeyDatabaseName(candidate.name));
  EXPECT_EQ(candidate.channel, 0);
  EXPECT_FALSE(candidate.backend_type.isEmpty());
}

// ------------------------------------------------- one default, or none

TEST_F(GFCoreTest, ASecondDefaultIsDroppedAndTheFirstIsKept) {
  // A stored list is not only ever written by the dialog: it can arrive in a
  // package, be hand-edited, or come from a build older than this rule. Two
  // databases answering to one identity is not a state anything downstream can
  // make sense of -- every lookup by name finds whichever comes first, and the
  // default engine follows the name.
  QContainer<KeyDatabaseItemSO> databases;
  databases.push_back(MakeItem("DEFAULT", "/first", "gnupg", 0));
  databases.push_back(MakeItem("Work", "/work", "rpgp", 1));
  databases.push_back(MakeItem("Default", "/second", "rpgp", 2));
  databases.push_back(MakeItem("default", "/third", "rpgp", 3));

  const auto kept = DropDuplicateDefaultKeyDatabases(databases);

  ASSERT_EQ(kept.size(), 2);
  EXPECT_EQ(kept.at(0).name, "DEFAULT");
  EXPECT_EQ(kept.at(0).path, "/first") << "the first one is the one that stays";
  EXPECT_EQ(kept.at(1).name, "Work");
}

TEST_F(GFCoreTest, AListWithNoDefaultIsLeftAlone) {
  // Zero is as legitimate as one: a profile that arrived in a package has none,
  // because a package never carries a database that lived outside it.
  QContainer<KeyDatabaseItemSO> databases;
  databases.push_back(MakeItem("Key DB 2", "@profile/dbs/Key DB 2", "rpgp", 0));
  databases.push_back(
      MakeItem("Key DB 3", "@profile/dbs/Key DB 3", "gnupg", 1));

  const auto kept = DropDuplicateDefaultKeyDatabases(databases);

  ASSERT_EQ(kept.size(), 2);
  EXPECT_EQ(kept.at(0).name, "Key DB 2");
  EXPECT_EQ(kept.at(1).name, "Key DB 3");
}

TEST_F(GFCoreTest, DroppingDuplicateDefaultsIsIdempotent) {
  QContainer<KeyDatabaseItemSO> databases;
  databases.push_back(MakeItem("DEFAULT", "/first", "gnupg", 0));
  databases.push_back(MakeItem("DEFAULT", "/second", "gnupg", 1));

  const auto once = DropDuplicateDefaultKeyDatabases(databases);
  const auto twice = DropDuplicateDefaultKeyDatabases(once);

  ASSERT_EQ(once.size(), 1);
  ASSERT_EQ(twice.size(), 1);
  EXPECT_EQ(twice.at(0).path, "/first");
}

TEST_F(GFCoreTest, ChannelsSharedByTwoEntriesAreBrokenInListOrder) {
  // Which database is channel 0 has to be a fact about the profile, not a
  // coin toss. Adding the DEFAULT database puts it at channel 0 while another
  // entry still holds 0, and an unstable sort would decide between them
  // differently from one run to the next.
  QContainer<KeyDatabaseItemSO> databases;
  databases.push_back(MakeItem("DEFAULT", "/default", "gnupg", 0));
  databases.push_back(MakeItem("Work", "/work", "rpgp", 0));
  databases.push_back(MakeItem("Spare", "/spare", "rpgp", 0));

  NormalizeKeyDatabaseChannels(databases);

  ASSERT_EQ(databases.size(), 3);
  EXPECT_EQ(databases.at(0).name, "DEFAULT");
  EXPECT_EQ(databases.at(0).channel, 0);
  EXPECT_EQ(databases.at(1).name, "Work");
  EXPECT_EQ(databases.at(1).channel, 1);
  EXPECT_EQ(databases.at(2).name, "Spare");
  EXPECT_EQ(databases.at(2).channel, 2);
}

// ------------------------------- the default database is this computer's

TEST_F(GFCoreTest, ADefaultPathFromAnotherSystemIsReplacedNotRepaired) {
  // The report this exists for: a package opened on Linux whose DEFAULT entry
  // still said "C:/Users/eric/AppData/Roaming/gnupg". Not a path Linux has, and
  // not even one Qt reads as absolute there. Repairing it is impossible anyway
  // -- the sender's keyring never travelled, because a package carries nothing
  // from outside the profile root -- and what the recipient wants under that
  // name is their own default keyring.
  QContainer<KeyDatabaseItemSO> databases;
  databases.push_back(
      MakeItem("DEFAULT", "C:/Users/eric/AppData/Roaming/gnupg", "gnupg", 0));
  databases.push_back(MakeItem("Key DB 2", "@profile/dbs/Key DB 2", "rpgp", 1));

  const auto adopted =
      AdoptLocalDefaultKeyDatabase(databases, "/home/eric/.gnupg", "gnupg");

  ASSERT_EQ(adopted.size(), 2);
  EXPECT_EQ(adopted.at(0).path, "/home/eric/.gnupg");
  EXPECT_EQ(adopted.at(1).path, "@profile/dbs/Key DB 2")
      << "only the derived database is derived";
}

TEST_F(GFCoreTest, TheBackendTravelsWithTheDefaultPath) {
  // Which engine derived the keyring is part of the same fact. A DEFAULT
  // recorded as gnupg by a machine that had it, opened on a build that does
  // not, has to become that build's default rather than name an engine that is
  // not there.
  QContainer<KeyDatabaseItemSO> databases;
  databases.push_back(MakeItem("Default", "C:/Users/eric/gnupg", "gnupg", 0));

  const auto adopted =
      AdoptLocalDefaultKeyDatabase(databases, "/home/eric/.rpgp_db", "rpgp");

  ASSERT_EQ(adopted.size(), 1);
  EXPECT_EQ(adopted.at(0).path, "/home/eric/.rpgp_db");
  EXPECT_EQ(adopted.at(0).backend_type, "rpgp");
}

TEST_F(GFCoreTest, WithNoLocalDefaultNothingIsTakenAway) {
  // An engine reporting no database is not a reason to blank the entry: the
  // stored path is still the best thing known about it.
  QContainer<KeyDatabaseItemSO> databases;
  databases.push_back(MakeItem("DEFAULT", "/home/eric/.gnupg", "gnupg", 0));

  const auto adopted = AdoptLocalDefaultKeyDatabase(databases, {}, {});

  ASSERT_EQ(adopted.size(), 1);
  EXPECT_EQ(adopted.at(0).path, "/home/eric/.gnupg");
}

TEST_F(GFCoreTest, AdoptingTheLocalDefaultIsIdempotent) {
  QContainer<KeyDatabaseItemSO> databases;
  databases.push_back(MakeItem("DEFAULT", "C:/Users/eric/gnupg", "gnupg", 0));

  const auto once =
      AdoptLocalDefaultKeyDatabase(databases, "/home/eric/.gnupg", "gnupg");
  const auto twice =
      AdoptLocalDefaultKeyDatabase(once, "/home/eric/.gnupg", "gnupg");

  ASSERT_EQ(twice.size(), 1);
  EXPECT_EQ(twice.at(0).path, "/home/eric/.gnupg");
}

// ------------------------------------- the engine belongs to the database

TEST_F(GFCoreTest, AKeyDatabaseIsOpenedWithItsOwnBackend) {
  // The bug this exists for. Channel 0 took its engine from
  // `basic/default_engine` while every other channel took it from the database,
  // so the two agreed only while the first database was the DEFAULT one. An
  // imported profile whose DEFAULT still names a path on the machine it came
  // from is dropped as unusable, the next database moves up into channel 0, and
  // an rPGP keyring was opened with GnuPG.
  const auto choice = ChooseKeyDatabaseEngine("rpgp", "GNUPG", true, true);

  EXPECT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kRPGP)
      << "the setting outvoted the database it was not about";
}

TEST_F(GFCoreTest, TheDefaultEngineOnlyAnswersForADatabaseThatNamesNone) {
  // What the setting is actually for: it preselects the backend for a database
  // the user is creating, and stands in for a stored entry old enough not to
  // record one.
  EXPECT_EQ(ChooseKeyDatabaseEngine("", "rpgp", true, true).engine,
            OpenPGPEngine::kRPGP);
  EXPECT_EQ(ChooseKeyDatabaseEngine("   ", "rpgp", true, true).engine,
            OpenPGPEngine::kRPGP);
  EXPECT_EQ(ChooseKeyDatabaseEngine("gnupg", "rpgp", true, true).engine,
            OpenPGPEngine::kGNUPG);
}

TEST_F(GFCoreTest, ADatabaseBackendThisBuildLacksStillFallsBack) {
  // The fallback is the one in ChooseOpenPGPEngine(), unchanged: a database
  // stored as "rpgp" by a build that had it must still open somewhere.
  const auto choice = ChooseKeyDatabaseEngine("rpgp", "rpgp", true, false);

  EXPECT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kGNUPG);

  EXPECT_FALSE(ChooseKeyDatabaseEngine("rpgp", "gnupg", false, false).ok);
}

// SignalBadOpenPGPEnv is emitted from a worker thread and received on the main
// thread, so its custom enum argument has to survive a queued connection. If
// the metatype registration and the spelling moc records ever drift apart, Qt
// drops the emission silently -- and because this signal is what releases the
// startup event loop, the app would hang on the loading dialog forever. That
// failure is invisible without a test like this one.
//
// The station here is deliberately a local instance, not the singleton: the
// production UI connects the singleton's signal to a modal message box that
// ends in std::exit(0), so emitting on it would kill the test process. A local
// instance is the same class, hence the same moc signature and metatype.
TEST_F(GFCoreTest, BadOpenPGPEnvReasonSurvivesQueuedConnection) {
  // the metatype is registered here, exactly as in production
  ASSERT_NE(CoreSignalStation::GetInstance(), nullptr);

  // catch a spelling drift directly instead of waiting for the timeout: moc
  // records the parameter type as written in the header, and parameterType()
  // resolves that spelling against the runtime registry, yielding
  // QMetaType::UnknownType when it no longer matches the registration.
  const auto& mo = CoreSignalStation::staticMetaObject;
  const auto idx = mo.indexOfSignal(
      "SignalBadOpenPGPEnv(GpgFrontend::BadOpenPGPEnvReason,QString)");
  ASSERT_NE(idx, -1);
  EXPECT_EQ(mo.method(idx).parameterType(0),
            qMetaTypeId<BadOpenPGPEnvReason>());

  CoreSignalStation station;

  std::atomic<bool> received = false;
  auto got_reason = BadOpenPGPEnvReason::kUNKNOWN;
  QString got_detail;

  QEventLoop loop;
  QObject context;

  QObject::connect(
      &station, &CoreSignalStation::SignalBadOpenPGPEnv, &context,
      [&](BadOpenPGPEnvReason reason, const QString& detail) {
        got_reason = reason;
        got_detail = detail;
        received = true;
        loop.quit();
      },
      Qt::QueuedConnection);

  QThread* worker = QThread::create([&station]() {
    emit station.SignalBadOpenPGPEnv(
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

// gpg-agent could not create its sockets because the key database path was
// longer than a unix socket address can hold, and nothing checked. These pin
// the arithmetic that now catches it.
TEST_F(GFCoreTest, GnuPGHomePathAcceptsAPathExactlyAtTheBudget) {
  const auto budget = GnuPGHomePathByteBudget();
  if (budget < 0) GTEST_SKIP() << "no socket path limit on this platform";

  const QString path = "/" + QString(budget - 1, QLatin1Char('a'));
  ASSERT_EQ(path.toUtf8().size(), budget);

  EXPECT_TRUE(GnuPGHomePathFitsSocketBudget(path));
}

TEST_F(GFCoreTest, GnuPGHomePathRejectsAPathOneByteOverTheBudget) {
  const auto budget = GnuPGHomePathByteBudget();
  if (budget < 0) GTEST_SKIP() << "no socket path limit on this platform";

  const QString path = "/" + QString(budget, QLatin1Char('a'));
  ASSERT_EQ(path.toUtf8().size(), budget + 1);

  EXPECT_FALSE(GnuPGHomePathFitsSocketBudget(path));
}

// sun_path is a byte buffer, so a non-ASCII user name costs more room than its
// character count suggests. Measuring QChars would let exactly these paths
// through and reproduce the original failure.
TEST_F(GFCoreTest, GnuPGHomePathIsMeasuredInBytesNotCharacters) {
  const auto budget = GnuPGHomePathByteBudget();
  if (budget < 0) GTEST_SKIP() << "no socket path limit on this platform";

  // Each of these is one QChar but two UTF-8 bytes, so a path of budget
  // characters is twice the budget in bytes.
  const QString path = "/" + QString(budget, QChar{0x00E4});
  ASSERT_LT(path.size(), path.toUtf8().size());

  EXPECT_FALSE(GnuPGHomePathFitsSocketBudget(path));
}

TEST_F(GFCoreTest, GnuPGHomePathUnusableReasonRoundTrips) {
  const auto previous = GnuPGHomePathUnusableReason();

  RegisterGnuPGHomePathUnusable("homedir too long: 110 bytes, max 85");
  EXPECT_EQ(GnuPGHomePathUnusableReason(),
            "homedir too long: 110 bytes, max 85");

  // An empty reason clears it: the condition can be resolved, and a stale
  // explanation on screen is worse than none.
  RegisterGnuPGHomePathUnusable({});
  EXPECT_TRUE(GnuPGHomePathUnusableReason().isEmpty());

  RegisterGnuPGHomePathUnusable(previous);
}

namespace {

// A directory whose path is comfortably past the budget, so the resolver has to
// fall back to a link. Built under `base` so the test owns everything it makes.
auto MakeOverBudgetDir(const QString& base) -> QString {
  auto path = base;
  while (GnuPGHomePathFitsSocketBudget(path)) path += "/0123456789";

  return QDir().mkpath(path) ? path : QString{};
}

}  // namespace

// A path that already fits must be handed to GnuPG untouched: the link only
// exists to rescue paths that cannot work, and creating one anyway would leave
// a trail of them on every installation that never had the problem.
TEST_F(GFCoreTest, GnuPGEngineHomePathIsUnchangedWhenItAlreadyFits) {
  QTemporaryDir root;
  ASSERT_TRUE(root.isValid());
  ASSERT_TRUE(GnuPGHomePathFitsSocketBudget(root.path()));

  EXPECT_EQ(ResolveGnuPGEngineHomePath(root.path(), root.path() + "/links"),
            root.path());
}

TEST_F(GFCoreTest, GnuPGEngineHomePathRedirectsAnOverBudgetPath) {
  QTemporaryDir base;
  ASSERT_TRUE(base.isValid());
  if (GnuPGHomePathByteBudget() < 0) {
    GTEST_SKIP() << "no socket path limit on this platform";
  }

  const auto deep = MakeOverBudgetDir(base.path());
  ASSERT_FALSE(deep.isEmpty());

  const auto links = base.path() + "/l";
  const auto alias = ResolveGnuPGEngineHomePath(deep, links);

  ASSERT_FALSE(alias.isEmpty());
  EXPECT_NE(alias, deep);

  // The whole point: short enough for the sockets, and still the same
  // directory.
  EXPECT_TRUE(GnuPGHomePathFitsSocketBudget(alias));
  EXPECT_EQ(QFileInfo(alias).symLinkTarget(),
            QFileInfo(deep).canonicalFilePath());

  // Deterministic, so restarts and sibling channels converge on one link rather
  // than littering a new one per call.
  EXPECT_EQ(ResolveGnuPGEngineHomePath(deep, links), alias);
}

TEST_F(GFCoreTest, GnuPGEngineHomePathReplacesALinkPointingElsewhere) {
  QTemporaryDir base;
  ASSERT_TRUE(base.isValid());
  if (GnuPGHomePathByteBudget() < 0) {
    GTEST_SKIP() << "no socket path limit on this platform";
  }

  const auto deep = MakeOverBudgetDir(base.path());
  ASSERT_FALSE(deep.isEmpty());

  const auto links = base.path() + "/l";
  const auto alias = ResolveGnuPGEngineHomePath(deep, links);
  ASSERT_FALSE(alias.isEmpty());

  // Point it somewhere else behind the resolver's back: a stale link left by a
  // profile that moved, or an eight-hex collision. Either way the answer must
  // be corrected rather than trusted.
  const auto decoy = base.path() + "/decoy";
  ASSERT_TRUE(QDir().mkpath(decoy));
  ASSERT_TRUE(QFile::remove(alias));
  ASSERT_TRUE(QFile::link(decoy, alias));

  EXPECT_EQ(ResolveGnuPGEngineHomePath(deep, links), alias);
  EXPECT_EQ(QFileInfo(alias).symLinkTarget(),
            QFileInfo(deep).canonicalFilePath());
}

// A real directory sitting on the short name is somebody's data. Refuse rather
// than clear it out of the way.
TEST_F(GFCoreTest, GnuPGEngineHomePathRefusesToDisplaceARealDirectory) {
  QTemporaryDir base;
  ASSERT_TRUE(base.isValid());
  if (GnuPGHomePathByteBudget() < 0) {
    GTEST_SKIP() << "no socket path limit on this platform";
  }

  const auto deep = MakeOverBudgetDir(base.path());
  ASSERT_FALSE(deep.isEmpty());

  const auto links = base.path() + "/l";
  const auto alias = ResolveGnuPGEngineHomePath(deep, links);
  ASSERT_FALSE(alias.isEmpty());

  ASSERT_TRUE(QFile::remove(alias));
  ASSERT_TRUE(QDir().mkpath(alias));
  ASSERT_TRUE(QFile::exists(alias + "/../"));

  EXPECT_TRUE(ResolveGnuPGEngineHomePath(deep, links).isEmpty());
  EXPECT_FALSE(GnuPGHomePathUnusableReason().isEmpty());
  EXPECT_TRUE(QFileInfo(alias).isDir());

  RegisterGnuPGHomePathUnusable({});
}

// The profile directory name is part of the GnuPG home path, which is why it is
// short now. A regression here silently pushes macOS profiles back over the
// socket limit.
TEST_F(GFCoreTest, MintedProfileDirectoryIdIsShortAndValid) {
  QTemporaryDir profiles_root;
  ASSERT_TRUE(profiles_root.isValid());

  const auto id = MintProfileDirectoryId(profiles_root.path());

  EXPECT_EQ(id.size(), 6);
  EXPECT_TRUE(IsValidProfileId(id));
}

TEST_F(GFCoreTest, MintedProfileDirectoryIdSkipsNamesAlreadyTaken) {
  QTemporaryDir profiles_root;
  ASSERT_TRUE(profiles_root.isValid());

  // Mint one, occupy it, then mint again: the second must not collide with the
  // directory the first one named.
  const auto first = MintProfileDirectoryId(profiles_root.path());
  ASSERT_FALSE(first.isEmpty());
  ASSERT_TRUE(QDir(profiles_root.path()).mkpath(first));

  const auto second = MintProfileDirectoryId(profiles_root.path());

  ASSERT_FALSE(second.isEmpty());
  EXPECT_NE(second, first);
}

// ------------------------------------------- key database paths from elsewhere

// The tail is what survives a profile changing machines. A stored path was
// written against a root that no longer exists -- another computer's, or this
// one's before the folder moved -- and on a different platform it is not even
// recognisable as absolute: Qt reads "C:/Users/..." as relative on Unix and
// "/Users/..." as absolute on Windows, so both name a directory that is not
// there. The keys themselves never moved out of the profile.

TEST(ForeignKeyDatabasePathTest, AWindowsPathIsReadOnAPlatformWithoutDrives) {
  EXPECT_EQ(
      ForeignKeyDatabasePathTail(
          R"(C:\Users\erich\AppData\Roaming\BKTUS\GpgFrontend\profiles\5be270\dbs\Key DB 2)"),
      "dbs/Key DB 2");
}

TEST(ForeignKeyDatabasePathTest, AMacOSPathIsReadOnAPlatformWithDrives) {
  EXPECT_EQ(
      ForeignKeyDatabasePathTail("/Users/erich/Library/Application Support/"
                                 "BKTUS/GpgFrontend/profiles/5be270/dbs/"
                                 "Key DB 2"),
      "dbs/Key DB 2");
}

TEST(ForeignKeyDatabasePathTest, EveryManagedDirectoryIsRecognised) {
  // "db" and "rpgp_db" are key databases in their own right -- the DEFAULT one
  // is exactly that -- so the final component has to count as a match.
  EXPECT_EQ(ForeignKeyDatabasePathTail("/somewhere/profiles/ab/db"), "db");
  EXPECT_EQ(ForeignKeyDatabasePathTail("/somewhere/profiles/ab/rpgp_db"),
            "rpgp_db");
  EXPECT_EQ(ForeignKeyDatabasePathTail("/somewhere/profiles/ab/dbs/Work"),
            "dbs/Work");
}

TEST(ForeignKeyDatabasePathTest, TheInnermostMatchWins) {
  // A profiles folder the user happened to put inside a folder called "db"
  // would otherwise hand back the whole rest of the path.
  EXPECT_EQ(ForeignKeyDatabasePathTail("/backup/db/profiles/ab/dbs/Work"),
            "dbs/Work");
}

TEST(ForeignKeyDatabasePathTest, APathNamingNoManagedDirectoryIsNotRecovered) {
  // A key database the user pointed at by hand is genuinely somewhere else,
  // and inventing a profile-local location for it would hand them an empty
  // keyring wearing the name of a real one.
  EXPECT_TRUE(ForeignKeyDatabasePathTail("/home/eric/.gnupg").isEmpty());
  EXPECT_TRUE(ForeignKeyDatabasePathTail("/srv/keys/work").isEmpty());
  EXPECT_TRUE(ForeignKeyDatabasePathTail({}).isEmpty());
}

TEST(ForeignKeyDatabasePathTest, AlreadyProfileRelativeIsLeftAlone) {
  // It is not foreign: it resolves against whatever root it is opened under,
  // which is the whole point of the token.
  EXPECT_TRUE(ForeignKeyDatabasePathTail("@profile/dbs/Work").isEmpty());
  EXPECT_TRUE(ForeignKeyDatabasePathTail("@profile").isEmpty());
}

TEST(ForeignKeyDatabasePathTest, APathThatClimbsIsRefused) {
  // The stored value is not necessarily one we wrote, and the tail is joined
  // onto a profile root by the caller.
  EXPECT_TRUE(
      ForeignKeyDatabasePathTail("/somewhere/dbs/../../../etc").isEmpty());
  EXPECT_TRUE(
      ForeignKeyDatabasePathTail(R"(C:\p\dbs\..\..\Windows)").isEmpty());
}

// ------------------------------------------------ re-anchoring what travelled

namespace {

/// A profile root with the two key database directories a package carries.
auto MakeProfileWithDatabases(const QString& root) -> bool {
  return QDir().mkpath(root + "/db") && QDir().mkpath(root + "/dbs/Key DB 2");
}

}  // namespace

TEST(ReanchorKeyDatabasePathTest, APathFromAnotherMachineFindsTheLocalCopy) {
  // The report this exists for: a profile exported on macOS and opened on
  // Windows, whose key database still named a folder under /Users. The keys
  // were in the profile the whole time.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto root = dir.path() + "/5be270";
  ASSERT_TRUE(MakeProfileWithDatabases(root));

  EXPECT_EQ(
      ReanchorKeyDatabasePath("/Users/erich/Library/Application Support/BKTUS/"
                              "GpgFrontendTesting/profiles/5be270/dbs/Key DB 2",
                              root),
      "@profile/dbs/Key DB 2");

  EXPECT_EQ(
      ReanchorKeyDatabasePath(
          R"(C:\Users\erich\AppData\Roaming\BKTUS\GpgFrontend\profiles\ab\db)",
          root),
      "@profile/db");
}

TEST(ReanchorKeyDatabasePathTest, APathInsideThisProfileIsSpelledPortably) {
  // The same normalisation an export used to do to the live list behind the
  // user's back, in the one place that owns the stored list.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto root = dir.path() + "/5be270";
  ASSERT_TRUE(MakeProfileWithDatabases(root));

  EXPECT_EQ(ReanchorKeyDatabasePath(root + "/dbs/Key DB 2", root),
            "@profile/dbs/Key DB 2");
}

TEST(ReanchorKeyDatabasePathTest, APathThatIsThereIsNeverRepointed) {
  // A key database really kept outside the profile stays where it is, even
  // when the profile happens to hold a directory of the same name. Guessing
  // here would silently swap one keyring for another.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto root = dir.path() + "/5be270";
  ASSERT_TRUE(MakeProfileWithDatabases(root));

  const auto elsewhere = dir.path() + "/elsewhere/dbs/Key DB 2";
  ASSERT_TRUE(QDir().mkpath(elsewhere));

  EXPECT_EQ(ReanchorKeyDatabasePath(elsewhere, root), elsewhere);
}

TEST(ReanchorKeyDatabasePathTest, AMissingPathThisProfileCannotSupplyIsKept) {
  // An unmounted volume, or a folder the user moved themselves. Reporting it
  // as unavailable is the honest answer; inventing a location is not.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto root = dir.path() + "/5be270";
  ASSERT_TRUE(MakeProfileWithDatabases(root));

  EXPECT_EQ(ReanchorKeyDatabasePath("/Volumes/stick/dbs/Not Here", root),
            "/Volumes/stick/dbs/Not Here");
  EXPECT_EQ(ReanchorKeyDatabasePath("/home/eric/.gnupg", root),
            "/home/eric/.gnupg");
}

TEST(ReanchorKeyDatabasePathTest, WithoutARootNothingIsTouched) {
  EXPECT_EQ(ReanchorKeyDatabasePath("/anything/dbs/Work", {}),
            "/anything/dbs/Work");
  EXPECT_EQ(ReanchorKeyDatabasePath("@profile/dbs/Work", "/srv/p/work"),
            "@profile/dbs/Work");
}

// The UI turns a fatal environment signal into a modal dialog that ends in
// std::exit(0). This flag is what suppresses it here, so if it stops being
// published the suppression rots silently.
TEST_F(GFCoreTest, UnitTestModeFlagIsPublishedToRuntime) {
  EXPECT_EQ(Module::RetrieveRTValueTypedOrDefault<>(
                "core", "env.state.unit_test_mode", 0),
            1);
}

}  // namespace GpgFrontend::Test
