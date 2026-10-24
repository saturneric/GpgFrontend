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

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "core/utils/GpgUtils.h"
#include "ui/dialog/settings/KeyDatabaseSettingsModel.h"
#include "ui/function/KeyDatabaseDisplayNames.h"

namespace GpgFrontend::Test {

namespace {

using UI::BuiltInKeyDatabaseEngineDescription;
using UI::KeyDatabaseDisplayId;
using UI::KeyDatabaseDisplayPath;
using UI::KeyDatabaseKindDisplayName;
using UI::KeyDatabaseSettingsModel;
using UI::KeyDatabaseTravelsWithProfile;

constexpr auto kRoot = "/home/someone/.local/share/GpgFrontend";

auto Database(const QString& id, const QString& name, KeyDatabaseKind kind,
              const QString& path = QStringLiteral("/somewhere"))
    -> KeyDatabaseInfo {
  KeyDatabaseInfo info;
  info.id = id;
  info.name = name;
  info.kind = kind;
  info.path = path;
  info.origin_path = path;
  info.backend_type = QStringLiteral("gnupg");
  return info;
}

auto BuiltIn() -> KeyDatabaseInfo {
  return Database(kDefaultKeyDatabaseId, kDefaultKeyDatabaseName,
                  KeyDatabaseKind::kDEFAULT, QStringLiteral("/home/u/.gnupg"));
}

/// The built-in database, one of the profile's own and one of this
/// computer's, with nothing chosen.
auto ThreeDatabases() -> KeyDatabaseSettingsModel {
  KeyDatabaseSettingsModel model;
  model.Load({BuiltIn(), Database("id-work", "work", KeyDatabaseKind::kMANAGED),
              Database("id-usb", "usb", KeyDatabaseKind::kEXTERNAL)},
             {}, QStringLiteral("gnupg"));
  return model;
}

}  // namespace

TEST(KeyDatabaseDisplayNamesTest, EveryKindReadsDifferently) {
  const auto local_default =
      KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, false);
  const auto own_default =
      KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, true);
  const auto managed =
      KeyDatabaseKindDisplayName(KeyDatabaseKind::kMANAGED, true);
  const auto external =
      KeyDatabaseKindDisplayName(KeyDatabaseKind::kEXTERNAL, false);

  for (const auto& phrase : {local_default, own_default, managed, external}) {
    EXPECT_FALSE(phrase.isEmpty());
  }

  EXPECT_NE(local_default, own_default);
  EXPECT_NE(local_default, managed);
  EXPECT_NE(managed, external);
}

TEST(KeyDatabaseDisplayNamesTest, TheDefaultKindAloneCannotWordTheRow) {
  // ClassifyKeyDatabase() settles DEFAULT on the reserved name alone, because
  // its stored path is replaced on every read. But a self-contained profile's
  // default database is `@profile/db`: it travels, and calling it this
  // computer's would be exactly backwards.
  EXPECT_NE(KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, true),
            KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, false));
}

TEST(KeyDatabaseDisplayNamesTest, TheManagedDirectoriesTravel) {
  EXPECT_TRUE(KeyDatabaseTravelsWithProfile("@profile/db", kRoot));
  EXPECT_TRUE(
      KeyDatabaseTravelsWithProfile(QString("%1/dbs/Work").arg(kRoot), kRoot));
}

TEST(KeyDatabaseDisplayNamesTest, InsideTheProfileIsNotTheSameAsTravelling) {
  // The documented trap. A database under `workspace/` is inside the profile
  // root and still never travels, because the packer does not carry that
  // directory -- and a phrase promising a recipient a keyring that never
  // shipped is worse than no phrase.
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile(
      QString("%1/workspace/keys").arg(kRoot), kRoot));
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile("@profile/workspace/keys", kRoot));
}

TEST(KeyDatabaseDisplayNamesTest, BothSpellingsOfOnePathAgree) {
  // Which spelling reached the settings file is an accident of which build
  // wrote it; they name one directory and must answer identically.
  EXPECT_EQ(
      KeyDatabaseTravelsWithProfile("@profile/dbs/Work", kRoot),
      KeyDatabaseTravelsWithProfile(QString("%1/dbs/Work").arg(kRoot), kRoot));
}

TEST(KeyDatabaseDisplayNamesTest, AnythingOutsideTheProfileStaysBehind) {
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile("/home/someone/.gnupg", kRoot));
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile({}, kRoot));
  EXPECT_FALSE(KeyDatabaseTravelsWithProfile("@profile/db", {}));
}

TEST(KeyDatabaseDisplayNamesTest, AFolderInTheProfileIsShownByItsToken) {
  // The absolute form is this computer's profile location: long, nothing the
  // user chose, and different once the profile moves.
  EXPECT_EQ(KeyDatabaseDisplayPath(QString("%1/dbs/Work").arg(kRoot), kRoot),
            QDir::toNativeSeparators("@profile/dbs/Work"));
  EXPECT_EQ(KeyDatabaseDisplayPath("@profile/dbs/Work", kRoot),
            QDir::toNativeSeparators("@profile/dbs/Work"));
}

TEST(KeyDatabaseDisplayNamesTest, AFolderOutsideTheProfileIsShownAsItIs) {
  EXPECT_EQ(KeyDatabaseDisplayPath("/home/someone/.gnupg", kRoot),
            QDir::toNativeSeparators("/home/someone/.gnupg"));
  EXPECT_TRUE(KeyDatabaseDisplayPath({}, kRoot).isEmpty());
}

TEST(KeyDatabaseDisplayNamesTest, AnIdIsShortenedToItsFirstGroup) {
  EXPECT_EQ(KeyDatabaseDisplayId("3f2a9c1e-5b7d-4e8f-9a0b-1c2d3e4f5a6b"),
            "3f2a9c1e");
  EXPECT_EQ(KeyDatabaseDisplayId("default"), "default");
  EXPECT_EQ(KeyDatabaseDisplayId({}), QStringLiteral("\u2013"));
}

// -----------------------------------------------------------------------------
// The built-in database is shown by a name of its own, told apart by its id.
// -----------------------------------------------------------------------------

TEST(KeyDatabaseDisplayNamesTest, TheBuiltInDatabaseIsShownByItsOwnName) {
  const auto shown =
      KeyDatabaseDisplayName(kDefaultKeyDatabaseId, kDefaultKeyDatabaseName);
  EXPECT_EQ(shown, QCoreApplication::tr("Built-in"));
  EXPECT_NE(shown, QString(kDefaultKeyDatabaseName));
}

TEST(KeyDatabaseDisplayNamesTest, AnyOtherDatabaseKeepsItsName) {
  EXPECT_EQ(KeyDatabaseDisplayName("id-work", "work"), "work");
}

TEST(KeyDatabaseDisplayNamesTest, TheIdDecidesNotTheName) {
  // A database that somehow carries the reserved name but not the reserved id
  // is not the built-in one: AssignKeyDatabaseIds gives "default" to the
  // built-in entry alone, so the id is the safer witness.
  EXPECT_EQ(KeyDatabaseDisplayName("id-other", "DEFAULT"), "DEFAULT");
  EXPECT_FALSE(IsBuiltInKeyDatabase("id-other", "DEFAULT"));
}

TEST(KeyDatabaseDisplayNamesTest, WithoutAnIdTheReservedNameDecides) {
  // A context built without an id, as tests and startup progress report one.
  EXPECT_TRUE(IsBuiltInKeyDatabase({}, "DEFAULT"));
  EXPECT_TRUE(IsBuiltInKeyDatabase({}, " default "));
  EXPECT_FALSE(IsBuiltInKeyDatabase({}, "work"));
}

TEST(KeyDatabaseDisplayNamesTest, AChoiceSaysDefaultOnceAndNoId) {
  const auto builtin = UI::KeyDatabaseChoiceLabel(
      kDefaultKeyDatabaseId, kDefaultKeyDatabaseName, "gnupg", true);
  EXPECT_TRUE(builtin.startsWith(QCoreApplication::tr("Built-in")));
  EXPECT_TRUE(builtin.contains("GNUPG"));
  // The role once; the built-in database's id ("default") not at all.
  EXPECT_EQ(builtin.count("default", Qt::CaseInsensitive), 1);

  const auto other =
      UI::KeyDatabaseChoiceLabel("3f2a9c1e-5b7d", "work", "rpgp", false);
  EXPECT_EQ(other, QStringLiteral("work  ·  RPGP"));
  EXPECT_FALSE(other.contains("3f2a9c1e"));

  EXPECT_EQ(UI::KeyDatabaseChoiceLabel("id", "work", {}, false), "work");
}

TEST(KeyDatabaseDisplayNamesTest, TheEngineDescriptionFollowsTheEngine) {
  const auto gnupg = BuiltInKeyDatabaseEngineDescription(OpenPGPEngine::kGNUPG);
  const auto rpgp = BuiltInKeyDatabaseEngineDescription(OpenPGPEngine::kRPGP);
  EXPECT_FALSE(gnupg.isEmpty());
  EXPECT_FALSE(rpgp.isEmpty());
  EXPECT_NE(gnupg, rpgp);
}

TEST(KeyDatabaseDisplayNamesTest, TheBuiltInKindIsNotCalledTheDefault) {
  // "default" is a role another database can hold; the kind says built-in.
  for (const auto packed : {true, false}) {
    EXPECT_FALSE(KeyDatabaseKindDisplayName(KeyDatabaseKind::kDEFAULT, packed)
                     .contains("default", Qt::CaseInsensitive));
  }
}

// -----------------------------------------------------------------------------
// The Key Databases page's pending state: which database is the default, and
// what is written back.
// -----------------------------------------------------------------------------

TEST(KeyDatabaseSettingsModelTest, WithNoChoiceTheBuiltInDatabaseIsDefault) {
  const auto model = ThreeDatabases();
  EXPECT_EQ(model.EffectiveDefaultId(), kDefaultKeyDatabaseId);
}

TEST(KeyDatabaseSettingsModelTest, AChoiceMakesAnotherDatabaseTheDefault) {
  auto model = ThreeDatabases();
  ASSERT_TRUE(model.UseAsDefault("id-usb"));
  EXPECT_EQ(model.EffectiveDefaultId(), "id-usb");
  EXPECT_EQ(model.choice_id, "id-usb");
}

TEST(KeyDatabaseSettingsModelTest, WithTheBuiltInOffTheFirstIsDefault) {
  auto model = ThreeDatabases();
  model.SetBuiltIn(std::nullopt);
  EXPECT_EQ(model.EffectiveDefaultId(), "id-work");
}

TEST(KeyDatabaseSettingsModelTest, RemovingTheChosenDefaultWithdrawsTheChoice) {
  auto model = ThreeDatabases();
  ASSERT_TRUE(model.UseAsDefault("id-usb"));

  model.RemoveAt(model.external, 0);
  EXPECT_TRUE(model.choice_id.isEmpty());
  EXPECT_EQ(model.EffectiveDefaultId(), kDefaultKeyDatabaseId);
}

TEST(KeyDatabaseSettingsModelTest, RemovingAnotherDatabaseKeepsTheChoice) {
  auto model = ThreeDatabases();
  ASSERT_TRUE(model.UseAsDefault("id-usb"));
  model.RemoveAt(model.managed, 0);
  EXPECT_EQ(model.choice_id, "id-usb");
}

TEST(KeyDatabaseSettingsModelTest, DisablingTheChosenBuiltInClearsTheChoice) {
  auto model = ThreeDatabases();
  ASSERT_TRUE(model.UseAsDefault("id-usb"));
  ASSERT_TRUE(model.UseAsDefault(kDefaultKeyDatabaseId));

  // Disable, save, reopen, re-enable: the explicit choice is gone at every
  // step, and the default is whatever the fallback says.
  model.SetBuiltIn(std::nullopt);
  EXPECT_TRUE(model.choice_id.isEmpty());
  EXPECT_EQ(model.EffectiveDefaultId(), "id-work");

  KeyDatabaseListSO saved;
  model.ApplyTo(saved);
  EXPECT_TRUE(saved.default_database_id.isEmpty());

  const auto reread = KeyDatabaseListSO(saved.ToJson());
  EXPECT_TRUE(reread.default_database_id.isEmpty());

  KeyDatabaseSettingsModel reopened;
  reopened.Load({Database("id-work", "work", KeyDatabaseKind::kMANAGED),
                 Database("id-usb", "usb", KeyDatabaseKind::kEXTERNAL)},
                reread.default_database_id, model.engine);
  EXPECT_EQ(reopened.EffectiveDefaultId(), "id-work");

  reopened.SetBuiltIn(BuiltIn());
  EXPECT_TRUE(reopened.choice_id.isEmpty())
      << "re-enabling restored the withdrawn choice";
  // Default again, but only by the fallback.
  EXPECT_EQ(reopened.EffectiveDefaultId(), kDefaultKeyDatabaseId);
}

TEST(KeyDatabaseSettingsModelTest, DisablingTheBuiltInKeepsAnotherChoice) {
  auto model = ThreeDatabases();
  ASSERT_TRUE(model.UseAsDefault("id-usb"));
  model.SetBuiltIn(std::nullopt);
  EXPECT_EQ(model.choice_id, "id-usb");
}

TEST(KeyDatabaseSettingsModelTest, TheChoiceIsWrittenOnEveryStorePath) {
  auto model = ThreeDatabases();
  ASSERT_TRUE(model.UseAsDefault("id-work"));

  // The ordinary path writes into the list as it was read...
  KeyDatabaseListSO loaded;
  loaded.default_database_id = "id-stale";
  model.ApplyTo(loaded);
  EXPECT_EQ(loaded.default_database_id, "id-work");

  // ...and the one that overrides an unreadable list starts from nothing.
  KeyDatabaseListSO fresh;
  model.ApplyTo(fresh);
  EXPECT_EQ(fresh.default_database_id, "id-work");
  ASSERT_EQ(fresh.key_databases.size(), 3);
  EXPECT_EQ(fresh.key_databases.front().id, kDefaultKeyDatabaseId);
}

TEST(KeyDatabaseSettingsModelTest, SavingAndReopeningRestoresChoiceAndEngine) {
  auto model = ThreeDatabases();
  model.engine = QStringLiteral("RPGP");
  ASSERT_TRUE(model.UseAsDefault("id-usb"));

  KeyDatabaseListSO saved;
  model.ApplyTo(saved);
  const auto reread = KeyDatabaseListSO(saved.ToJson());

  QList<KeyDatabaseInfo> infos;
  for (const auto& item : reread.key_databases) {
    auto info = Database(item.id, item.name,
                         item.kind.value_or(KeyDatabaseKind::kEXTERNAL));
    infos.append(info);
  }
  KeyDatabaseSettingsModel reopened;
  reopened.Load(infos, reread.default_database_id, model.engine);
  EXPECT_EQ(reopened.EffectiveDefaultId(), "id-usb");
  EXPECT_EQ(reopened.engine, "RPGP");
}

TEST(KeyDatabaseSettingsModelTest, NothingIsWrittenUntilApplied) {
  // Cancel is simply not applying: the stored list stays as it was read.
  KeyDatabaseListSO stored;
  stored.default_database_id = "id-work";

  auto model = ThreeDatabases();
  model.choice_id = stored.default_database_id;
  model.engine = QStringLiteral("RPGP");
  ASSERT_TRUE(model.UseAsDefault("id-usb"));
  model.SetBuiltIn(std::nullopt);

  EXPECT_EQ(stored.default_database_id, "id-work");
  EXPECT_TRUE(stored.key_databases.isEmpty());
}

TEST(KeyDatabaseSettingsModelTest, UseAsDefaultIsOfferedOnlyWhereItChanges) {
  auto model = ThreeDatabases();
  EXPECT_FALSE(model.CanUseAsDefault(kDefaultKeyDatabaseId))
      << "already the default";
  EXPECT_TRUE(model.CanUseAsDefault("id-work"));
  EXPECT_TRUE(model.CanUseAsDefault("id-usb"));
  EXPECT_FALSE(model.CanUseAsDefault({}));
  EXPECT_FALSE(model.CanUseAsDefault("id-gone"));

  model.SetBuiltIn(std::nullopt);
  EXPECT_FALSE(model.CanUseAsDefault(kDefaultKeyDatabaseId))
      << "offered while turned off";
  EXPECT_FALSE(model.CanUseAsDefault("id-work")) << "it is the fallback now";
  EXPECT_FALSE(model.UseAsDefault(kDefaultKeyDatabaseId));
}

TEST(KeyDatabaseSettingsModelTest, AnUnresolvedChoiceFallsToTheFirstUsable) {
  // The core skips a database whose folder did not resolve, so the page does.
  KeyDatabaseSettingsModel model;
  model.Load(
      {BuiltIn(), Database("id-gone", "gone", KeyDatabaseKind::kEXTERNAL, {})},
      "id-gone", QStringLiteral("GNUPG"));
  EXPECT_EQ(model.EffectiveDefaultId(), kDefaultKeyDatabaseId);
}

TEST(KeyDatabaseSettingsModelTest, TheBuiltInEngineIsTheCoresDecision) {
  auto model = ThreeDatabases();

  model.engine = QStringLiteral("RPGP");
  auto choice = model.BuiltInEngine(true, true);
  ASSERT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kRPGP);

  model.engine = QStringLiteral("GNUPG");
  choice = model.BuiltInEngine(true, true);
  ASSERT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kGNUPG);

  // Asked for what this build lacks: shown as what it falls back to.
  model.engine = QStringLiteral("GNUPG");
  choice = model.BuiltInEngine(false, true);
  ASSERT_TRUE(choice.ok);
  EXPECT_EQ(choice.engine, OpenPGPEngine::kRPGP);
}

}  // namespace GpgFrontend::Test
