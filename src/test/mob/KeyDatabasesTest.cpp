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
#include <QDir>
#include <QTemporaryDir>

#include "MobileTest.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/KeyDatabaseRuntime.h"
#include "core/model/SettingsObject.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/utils/GpgUtils.h"
#include "model/KeyDatabasesController.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::KeyDatabasesController;

namespace {

struct Outcome {
  bool succeeded = false;
  QString error;
};

auto CurrentDatabaseId() -> QString {
  return LookupChannelHandle(UsableDefaultChannelHandle()).database_id;
}

auto FindDatabase(const QVariantList& list, const QString& name)
    -> QVariantMap {
  for (const auto& entry : list) {
    const auto map = entry.toMap();
    if (map.value("name").toString() == name) return map;
  }
  return {};
}

}  // namespace

class KeyDatabasesTest : public ::testing::Test {
 protected:
  KeyDatabasesController* dbs_ = nullptr;
  Outcome last_;
  int reported_ = 0;
  QString original_;

  void SetUp() override {
    original_ = CurrentDatabaseId();
    RunOnMainThread([this]() {
      dbs_ = new KeyDatabasesController();
      const auto record = [this](bool ok, const QString& error) {
        last_ = {ok, error};
        ++reported_;
      };
      QObject::connect(dbs_, &KeyDatabasesController::SignalSwitched, record);
      QObject::connect(dbs_, &KeyDatabasesController::SignalCreated, record);
    });
  }

  void TearDown() override {
    RunOnMainThread([this]() { delete dbs_; });
  }

  /// Run @p op and wait for what it reports.
  auto Act(const std::function<void(KeyDatabasesController*)>& op) -> Outcome {
    int before = 0;
    RunOnMainThread([this, &before, &op]() {
      before = reported_;
      op(dbs_);
    });
    EXPECT_TRUE(WaitFor(
        [this, before]() {
          int now = 0;
          RunOnMainThread([this, &now]() { now = reported_; });
          return now > before;
        },
        30000));
    Outcome copy;
    RunOnMainThread([this, &copy]() { copy = last_; });
    return copy;
  }

  /// Take the database @p id out of the stored list again, with its folder,
  /// and work in the one the test began in.
  void Forget(const QString& id, const QString& path) {
    {
      SettingsObject so(QString::fromLatin1(kKeyDatabaseListObject));
      KeyDatabaseListSO list(so);
      list.key_databases.erase(
          std::remove_if(
              list.key_databases.begin(), list.key_databases.end(),
              [&](const KeyDatabaseItemSO& item) { return item.id == id; }),
          list.key_databases.end());
      list.default_database_id = original_;
      so.Store(list.ToJson());
    }
    ApplyKeyDatabaseRuntime();
    if (!path.isEmpty()) QDir(path).removeRecursively();
  }
};

TEST_F(KeyDatabasesTest, ListsTheDatabaseInUse) {
  QVariantList list;
  QString current_name;
  RunOnMainThread([&]() {
    list = dbs_->Databases();
    current_name = dbs_->CurrentName();
  });
  ASSERT_FALSE(list.isEmpty());
  int current = 0;
  for (const auto& entry : list) {
    const auto map = entry.toMap();
    if (!map.value("current").toBool()) continue;
    ++current;
    EXPECT_EQ(map.value("id").toString(), original_);
    EXPECT_EQ(map.value("name").toString(), current_name);
  }
  EXPECT_EQ(current, 1);
}

// A profile from a desktop may hold a GnuPG keyring: listed with its type,
// so it is clear the profile has it, never to be switched to here.
TEST_F(KeyDatabasesTest, AnotherEnginesDatabaseIsShownNotChosen) {
  const QString id = "mobile-test-gnupg-db";
  QTemporaryDir folder;
  {
    SettingsObject so(QString::fromLatin1(kKeyDatabaseListObject));
    KeyDatabaseListSO list(so);
    KeyDatabaseItemSO item;
    item.id = id;
    item.name = "Desktop GnuPG";
    item.path = folder.path();
    item.channel = 7;
    item.backend_type = "gnupg";
    item.kind = KeyDatabaseKind::kEXTERNAL;
    list.key_databases.append(item);
    so.Store(list.ToJson());
  }
  ApplyKeyDatabaseRuntime();

  QVariantMap entry;
  QVariantMap in_use;
  RunOnMainThread([&]() {
    const auto list = dbs_->Databases();
    entry = FindDatabase(list, "Desktop GnuPG");
    for (const auto& d : list) {
      if (d.toMap().value("current").toBool()) in_use = d.toMap();
    }
  });
  ASSERT_FALSE(entry.isEmpty());
  EXPECT_FALSE(entry.value("available").toBool());
  EXPECT_EQ(entry.value("backend").toString(), "GnuPG");
  EXPECT_TRUE(entry.value("reason").toString().contains("GnuPG"))
      << entry.value("reason").toString().toStdString();
  EXPECT_FALSE(entry.value("current").toBool());
  // The one in use says which engine it is.
  EXPECT_TRUE(in_use.value("available").toBool());
  EXPECT_EQ(in_use.value("backend").toString(), "rPGP");

  const auto refused = Act([&](auto* dbs) { dbs->switchTo(id); });
  EXPECT_FALSE(refused.succeeded);
  EXPECT_FALSE(refused.error.isEmpty());
  EXPECT_EQ(CurrentDatabaseId(), original_);

  Forget(id, {});
}

TEST_F(KeyDatabasesTest, NamesItRefuses) {
  // As stored: what is shown may be a translated name for the built-in one.
  QString existing;
  for (const auto& info : GetAllKeyDatabaseInfoBySettings()) {
    if (info.id == original_) existing = info.name;
  }
  ASSERT_FALSE(existing.isEmpty());
  RunOnMainThread([&]() {
    EXPECT_FALSE(dbs_->nameProblem("").isEmpty());
    EXPECT_FALSE(dbs_->nameProblem("   ").isEmpty());
    // The name becomes a folder.
    EXPECT_FALSE(dbs_->nameProblem("a/b").isEmpty());
    EXPECT_FALSE(dbs_->nameProblem("..").isEmpty());
    EXPECT_FALSE(dbs_->nameProblem("Schlüssel").isEmpty());
    EXPECT_FALSE(dbs_->nameProblem(QString::fromLatin1(kDefaultKeyDatabaseName))
                     .isEmpty());
    EXPECT_FALSE(dbs_->nameProblem(existing).isEmpty());
    EXPECT_FALSE(dbs_->nameProblem(existing.toLower()).isEmpty());
    EXPECT_FALSE(dbs_->nameProblem(dbs_->CurrentName()).isEmpty());
    EXPECT_TRUE(dbs_->nameProblem(" Work_keys-2 ").isEmpty());

    const auto suggested = dbs_->suggestName();
    EXPECT_FALSE(suggested.isEmpty());
    EXPECT_TRUE(dbs_->nameProblem(suggested).isEmpty());
  });
}

TEST_F(KeyDatabasesTest, CreatesOneByNameAndWorksInIt) {
  const QString name = "Mobile Test DB";
  const auto path = ManagedKeyDatabasePath(
      GlobalSettingStation::GetInstance().GetAppDataPath(), name);
  QDir(path).removeRecursively();  // left by an earlier run that crashed

  const auto made = Act([&](auto* dbs) { dbs->create(" " + name + " "); });
  EXPECT_TRUE(made.succeeded) << made.error.toStdString();

  QVariantMap entry;
  QString current_name;
  RunOnMainThread([&]() {
    entry = FindDatabase(dbs_->Databases(), name);
    current_name = dbs_->CurrentName();
  });
  ASSERT_FALSE(entry.isEmpty());
  const auto id = entry.value("id").toString();
  EXPECT_TRUE(entry.value("current").toBool());
  EXPECT_EQ(current_name, name);
  EXPECT_EQ(CurrentDatabaseId(), id);
  EXPECT_TRUE(QDir(path).exists());

  // Kept in the profile, as the desktop keeps one it adds there.
  bool managed = false;
  for (const auto& info : GetAllKeyDatabaseInfoBySettings()) {
    if (info.id == id) managed = info.kind == KeyDatabaseKind::kMANAGED;
  }
  EXPECT_TRUE(managed);

  // Twice is refused, before anything is touched.
  const auto again = Act([&](auto* dbs) { dbs->create(name.toUpper()); });
  EXPECT_FALSE(again.succeeded);
  EXPECT_FALSE(again.error.isEmpty());

  // And back.
  const auto back = Act([&](auto* dbs) { dbs->switchTo(original_); });
  EXPECT_TRUE(back.succeeded) << back.error.toStdString();
  EXPECT_EQ(CurrentDatabaseId(), original_);

  Forget(id, path);
  EXPECT_EQ(CurrentDatabaseId(), original_);
}

TEST_F(KeyDatabasesTest, AFolderAlreadyThereIsNeverTakenOver) {
  const QString name = "Mobile Leftover DB";
  const auto path = ManagedKeyDatabasePath(
      GlobalSettingStation::GetInstance().GetAppDataPath(), name);
  ASSERT_TRUE(QDir().mkpath(path));
  QFile marker(path + "/keep");
  ASSERT_TRUE(marker.open(QIODevice::WriteOnly));
  marker.close();

  const auto made = Act([&](auto* dbs) { dbs->create(name); });
  EXPECT_FALSE(made.succeeded);
  EXPECT_TRUE(QFile::exists(path + "/keep"));
  EXPECT_EQ(CurrentDatabaseId(), original_);

  QVariantMap entry;
  RunOnMainThread([&]() { entry = FindDatabase(dbs_->Databases(), name); });
  EXPECT_TRUE(entry.isEmpty());

  QDir(path).removeRecursively();
}

}  // namespace GpgFrontend::Test::Mobile
