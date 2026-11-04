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
#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "MobileTest.h"
#include "core/profile/ProfileRegistry.h"
#include "core/profile/ProfileSession.h"
#include "model/ProfilesController.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::ProfilesController;

namespace {

auto InstalledRoot() -> QString {
  return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

auto ProfilesRoot() -> QString {
  const auto root =
      QCoreApplication::instance()->property("GFProfilesRoot").toString();
  return root.isEmpty() ? InstalledRoot() + "/profiles" : root;
}

auto FindProfile(const QVariantList& list, const QString& key,
                 const QString& value) -> QVariantMap {
  for (const auto& entry : list) {
    const auto map = entry.toMap();
    if (map.value(key).toString() == value) return map;
  }
  return {};
}

}  // namespace

/// One controller, on the interface thread, as the page has it; what it
/// reported, in order.
class ProfilesTest : public ::testing::Test {
 protected:
  ProfilesController* profiles_ = nullptr;
  QStringList events_;  ///< "exported:1", "passphrase:<error>", ...
  QString imported_id_;
  QStringList made_;  ///< profiles to delete afterwards
  int restarts_ = 0;

  void SetUp() override {
    // Never the application's: that one shuts the test's core down.
    ProfilesController::SetRestarter([this]() {
      ++restarts_;
      return true;
    });
    RunOnMainThread([this]() {
      profiles_ = new ProfilesController();
      QObject::connect(profiles_, &ProfilesController::SignalExported,
                       [this](bool ok, const QString& error) {
                         events_
                             << QString("exported:%1:%2").arg(ok).arg(error);
                       });
      QObject::connect(
          profiles_, &ProfilesController::SignalPassphraseNeeded,
          [this](const QString& error) { events_ << "passphrase:" + error; });
      QObject::connect(profiles_, &ProfilesController::SignalImported,
                       [this](bool ok, const QString& error, const QString& id,
                              const QString&) {
                         events_
                             << QString("imported:%1:%2").arg(ok).arg(error);
                         if (ok) imported_id_ = id;
                       });
      QObject::connect(
          profiles_, &ProfilesController::SignalFailed,
          [this](const QString& error) { events_ << "failed:" + error; });
    });
  }

  void TearDown() override {
    RunOnMainThread([this]() { delete profiles_; });
    ProfilesController::SetRestarter({});
    QFile::remove(ProfilesController::ChosenProfileFile(InstalledRoot()));
    for (const auto& id : made_) DeleteProfile(ProfilesRoot(), id);
  }

  /// Run @p op and wait until it has reported @p count more events.
  auto Act(const std::function<void(ProfilesController*)>& op, int count = 1)
      -> QStringList {
    int before = 0;
    RunOnMainThread([this, &before, &op]() {
      before = static_cast<int>(events_.size());
      op(profiles_);
    });
    EXPECT_TRUE(WaitFor(
        [this, before, count]() {
          int now = 0;
          RunOnMainThread(
              [this, &now]() { now = static_cast<int>(events_.size()); });
          return now >= before + count;
        },
        60000));
    QStringList copy;
    RunOnMainThread([this, &copy, before]() { copy = events_.mid(before); });
    return copy;
  }
};

TEST_F(ProfilesTest, ListsTheOpenProfile) {
  QVariantList list;
  QString name;
  RunOnMainThread([&]() {
    list = profiles_->Profiles();
    name = profiles_->CurrentName();
  });
  const auto current =
      FindProfile(list, "id", ProfileSession::Instance().Profile().Id());
  ASSERT_FALSE(current.isEmpty());
  EXPECT_TRUE(current.value("current").toBool());
  EXPECT_EQ(current.value("name").toString(), name);
  EXPECT_FALSE(name.isEmpty());
}

TEST_F(ProfilesTest, CreatesOneByName) {
  QString name;
  RunOnMainThread([&]() { name = profiles_->CurrentName(); });
  RunOnMainThread([&]() {
    EXPECT_FALSE(profiles_->nameProblem(" ").isEmpty());
    EXPECT_FALSE(profiles_->nameProblem(name.toUpper()).isEmpty());
    EXPECT_FALSE(profiles_->nameProblem(QString(65, 'x')).isEmpty());
    EXPECT_TRUE(profiles_->nameProblem("Travel").isEmpty());
  });

  QString id;
  RunOnMainThread([&]() { id = profiles_->create(" Travel "); });
  ASSERT_FALSE(id.isEmpty());
  made_ << id;

  QVariantList list;
  RunOnMainThread([&]() { list = profiles_->Profiles(); });
  const auto made = FindProfile(list, "id", id);
  ASSERT_FALSE(made.isEmpty());
  EXPECT_EQ(made.value("name").toString(), "Travel");
  EXPECT_FALSE(made.value("current").toBool());

  // Twice is refused.
  QString again;
  RunOnMainThread([&]() { again = profiles_->create("travel"); });
  EXPECT_TRUE(again.isEmpty());
}

TEST_F(ProfilesTest, ExportsAndImportsAPackage) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto url = QUrl::fromLocalFile(dir.filePath("backup.gfp"));

  QString offered;
  RunOnMainThread([&]() { offered = profiles_->exportFileName(); });
  EXPECT_TRUE(offered.endsWith(".gfp"));

  // Too short a passphrase is refused before anything is written.
  auto events = Act([&](auto* p) { p->exportTo(url, "short"); });
  ASSERT_EQ(events.size(), 1);
  EXPECT_TRUE(events.front().startsWith("exported:0:"));
  EXPECT_FALSE(QFile::exists(url.toLocalFile()));

  events = Act([&](auto* p) { p->exportTo(url, "a good passphrase"); });
  ASSERT_EQ(events.size(), 1);
  EXPECT_TRUE(events.front().startsWith("exported:1"))
      << events.front().toStdString();
  ASSERT_TRUE(QFile::exists(url.toLocalFile()));
  EXPECT_GT(QFileInfo(url.toLocalFile()).size(), 0);

  // Sealed: asked for, then asked again after a wrong one.
  events = Act([&](auto* p) { p->importFrom(url); });
  ASSERT_EQ(events.size(), 1);
  EXPECT_EQ(events.front(), "passphrase:");
  events = Act([&](auto* p) { p->importWith("not the passphrase"); });
  ASSERT_EQ(events.size(), 1);
  EXPECT_TRUE(events.front().startsWith("passphrase:"));
  EXPECT_GT(events.front().size(), QString("passphrase:").size());

  events = Act([&](auto* p) { p->importWith("a good passphrase"); });
  ASSERT_EQ(events.size(), 1);
  EXPECT_TRUE(events.front().startsWith("imported:1"))
      << events.front().toStdString();
  ASSERT_FALSE(imported_id_.isEmpty());
  made_ << imported_id_;

  // A copy beside the original, named so the two can be told apart.
  QVariantList list;
  QString name;
  RunOnMainThread([&]() {
    list = profiles_->Profiles();
    name = profiles_->CurrentName();
  });
  const auto copy = FindProfile(list, "id", imported_id_);
  ASSERT_FALSE(copy.isEmpty());
  EXPECT_EQ(copy.value("name").toString(), name + " (2)");
  EXPECT_FALSE(copy.value("current").toBool());

  // Nothing of the packages is left in the cache.
  const QDir cache(
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
      "/profile-packages");
  EXPECT_TRUE(cache.entryList(QDir::Files).isEmpty());
}

TEST_F(ProfilesTest, ImportRefusesWhatIsNotAPackage) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  QFile file(dir.filePath("not.gfp"));
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("not a profile package");
  file.close();

  const auto events = Act(
      [&](auto* p) { p->importFrom(QUrl::fromLocalFile(file.fileName())); });
  ASSERT_EQ(events.size(), 1);
  EXPECT_TRUE(events.front().startsWith("imported:0:"));
  EXPECT_GT(events.front().size(), QString("imported:0:").size());
}

TEST_F(ProfilesTest, SwitchingRemembersTheChoiceAndRestarts) {
  QString id;
  RunOnMainThread([&]() { id = profiles_->create("Elsewhere"); });
  ASSERT_FALSE(id.isEmpty());
  made_ << id;

  // The open one: nothing to do.
  bool switched = false;
  RunOnMainThread([&]() {
    switched = profiles_->switchTo(ProfileSession::Instance().Profile().Id());
  });
  EXPECT_TRUE(switched);
  EXPECT_EQ(restarts_, 0);

  RunOnMainThread([&]() { switched = profiles_->switchTo("no-such-one"); });
  EXPECT_FALSE(switched);
  EXPECT_EQ(restarts_, 0);

  RunOnMainThread([&]() { switched = profiles_->switchTo(id); });
  EXPECT_TRUE(switched);
  EXPECT_EQ(restarts_, 1);

  // The next start names it, while it is there.
  const auto saved = qgetenv("GF_PROFILE");
  ProfilesController::ApplyChosenProfile(InstalledRoot());
  EXPECT_EQ(qEnvironmentVariable("GF_PROFILE"), id);

  DeleteProfile(ProfilesRoot(), id);
  made_.clear();
  qunsetenv("GF_PROFILE");
  ProfilesController::ApplyChosenProfile(InstalledRoot());
  EXPECT_TRUE(qEnvironmentVariable("GF_PROFILE").isEmpty());
  EXPECT_FALSE(
      QFile::exists(ProfilesController::ChosenProfileFile(InstalledRoot())));

  if (saved.isEmpty()) {
    qunsetenv("GF_PROFILE");
  } else {
    qputenv("GF_PROFILE", saved);
  }
}

}  // namespace GpgFrontend::Test::Mobile
