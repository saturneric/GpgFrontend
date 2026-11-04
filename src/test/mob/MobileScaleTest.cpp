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
/**
 * @file MobileScaleTest.cpp
 * @brief The phone application with a thousand keys and a thousand files:
 * how long what a person waits for takes.
 *
 * Each test says what it measured; a limit is generous, there to catch
 * what grows with the count (a loop inside a loop), not to time a device.
 * The keys come from `sq` (GF_ALLOW_MISSING_INTEROP_TOOLS=1 skips them
 * without it) and go into a key database of the test's own, removed after.
 */

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "MobileCryptoTest.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/openpgp/KeyDatabaseRuntime.h"
#include "core/model/SettingsObject.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "crypto/WorkSelection.h"
#include "files/FileFilterModel.h"
#include "files/FileWorkspace.h"
#include "lifecycle/AppLifecycle.h"
#include "model/KeyCategoriesController.h"
#include "model/KeyDatabasesController.h"
#include "model/KeyFilterModel.h"
#include "model/KeyImportController.h"
#include "model/KeyListController.h"
#include "model/KeyRoleModel.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::FileFilterModel;
using GpgFrontend::Mobile::FileWorkspace;
using GpgFrontend::Mobile::KeyCategoriesController;
using GpgFrontend::Mobile::KeyDatabasesController;
using GpgFrontend::Mobile::KeyFilterModel;
using GpgFrontend::Mobile::KeyImportController;
using GpgFrontend::Mobile::KeyListController;
using GpgFrontend::Mobile::KeyRoleModel;
using GpgFrontend::Mobile::OperationQueue;
using GpgFrontend::Mobile::WorkSelection;

namespace {

constexpr int kKeys = 1000;
constexpr int kSecretKeys = 10;
constexpr int kFiles = 1000;

void Report(const char* what, qint64 ms) {
  std::printf("[ scale    ] %-48s %6lld ms\n", what,
              static_cast<long long>(ms));
  std::fflush(stdout);
}

/// @p op on the interface thread, timed.
auto Timed(const char* what, const std::function<void()>& op) -> qint64 {
  qint64 ms = 0;
  RunOnMainThread([&]() {
    QElapsedTimer timer;
    timer.start();
    op();
    ms = timer.elapsed();
  });
  Report(what, ms);
  return ms;
}

/// Generated once per run with `sq`: the public keys, then the secret ones.
struct Fixture {
  QString pub;
  QString sec;
};

auto MakeFixture(const QString& dir) -> std::optional<Fixture> {
  const auto sq = QStandardPaths::findExecutable("sq");
  if (sq.isEmpty()) return std::nullopt;
  QStringList keys;
  for (int i = 1; i <= kKeys; ++i) {
    const auto key = QString("%1/k%2.key").arg(dir).arg(i);
    QProcess gen;
    gen.start(sq, {"key", "generate", "--own-key", "--userid",
                   QString("Scale User %1 <scale%1@example.org>").arg(i),
                   "--without-password", "--cipher-suite", "cv25519",
                   "--output", key, "--rev-cert", key + ".rev", "--overwrite"});
    if (!gen.waitForFinished(30000) || gen.exitCode() != 0) return std::nullopt;
    keys << key;
  }
  Fixture fixture{dir + "/pub.asc", dir + "/sec.asc"};
  QProcess filter;
  filter.start(sq,
               QStringList{"keyring", "filter", "--experimental", "--to-cert",
                           "--output", fixture.pub, "--overwrite"} +
                   keys.mid(kSecretKeys));
  if (!filter.waitForFinished(120000) || filter.exitCode() != 0) {
    return std::nullopt;
  }
  QFile sec(fixture.sec);
  if (!sec.open(QIODevice::WriteOnly)) return std::nullopt;
  for (const auto& key : keys.mid(0, kSecretKeys)) {
    QFile in(key);
    if (!in.open(QIODevice::ReadOnly)) return std::nullopt;
    sec.write(in.readAll());
  }
  return fixture;
}

auto ReadAll(const QString& path) -> QString {
  QFile file(path);
  return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll())
                                        : QString();
}

auto CurrentDatabaseId() -> QString {
  return LookupChannelHandle(UsableDefaultChannelHandle()).database_id;
}

/// @p name under @p root, by the item tree too.
auto FindItem(QObject* root, const QString& name) -> QObject* {
  if (auto* found = root->findChild<QObject*>(name)) return found;
  auto* item = qobject_cast<QQuickItem*>(root);
  if (item == nullptr) return nullptr;
  for (auto* child : item->childItems()) {
    if (child->objectName() == name) return child;
    if (auto* found = FindItem(child, name)) return found;
  }
  return nullptr;
}

}  // namespace

/// A thousand keys in a database of the suite's own, for every test.
class MobileScaleTest : public ::testing::Test {
 protected:
  static inline QTemporaryDir* fixture_dir_ = nullptr;
  static inline std::optional<Fixture> fixture_;
  static inline QString original_db_;
  static inline QString scale_db_;
  static inline QString scale_path_;
  static inline bool imported_ = false;

  static void SetUpTestSuite() {
    fixture_dir_ = new QTemporaryDir();
    QElapsedTimer timer;
    timer.start();
    fixture_ = MakeFixture(fixture_dir_->path());
    if (!fixture_) return;
    Report("sq: generate the keys (not the app)", timer.elapsed());

    original_db_ = CurrentDatabaseId();
    const QString name = "Mobile Scale DB";
    scale_path_ = ManagedKeyDatabasePath(
        GlobalSettingStation::GetInstance().GetAppDataPath(), name);
    QDir(scale_path_).removeRecursively();
    KeyDatabasesController* dbs = nullptr;
    bool done = false;
    RunOnMainThread([&]() {
      dbs = new KeyDatabasesController();
      QObject::connect(dbs, &KeyDatabasesController::SignalCreated,
                       [&done](bool, const QString&) { done = true; });
      dbs->create(name);
    });
    WaitFor(
        [&]() {
          bool now = false;
          RunOnMainThread([&]() { now = done; });
          return now;
        },
        30000);
    RunOnMainThread([&]() { delete dbs; });
    scale_db_ = CurrentDatabaseId();
    if (scale_db_ == original_db_) return;

    // Imported the way the Keys page imports pasted text.
    KeyImportController* importer = nullptr;
    RunOnMainThread([&]() { importer = new KeyImportController(); });
    for (const auto& path : {fixture_->pub, fixture_->sec}) {
      const auto text = ReadAll(path);
      QElapsedTimer import_timer;
      import_timer.start();
      RunOnMainThread([&]() { importer->importText(text); });
      WaitFor(
          [&]() {
            bool busy = true;
            RunOnMainThread([&]() { busy = importer->Busy(); });
            return !busy;
          },
          300000);
      Report(path == fixture_->pub ? "import 990 public keys (pasted text)"
                                   : "import 10 secret keys (pasted text)",
             import_timer.elapsed());
    }
    RunOnMainThread([&]() { delete importer; });
    imported_ = true;
  }

  static void TearDownTestSuite() {
    if (!scale_db_.isEmpty() && scale_db_ != original_db_) {
      {
        SettingsObject so(QString::fromLatin1(kKeyDatabaseListObject));
        KeyDatabaseListSO list(so);
        list.key_databases.erase(
            std::remove_if(list.key_databases.begin(), list.key_databases.end(),
                           [](const KeyDatabaseItemSO& item) {
                             return item.id == scale_db_;
                           }),
            list.key_databases.end());
        list.default_database_id = original_db_;
        so.Store(list.ToJson());
      }
      ApplyKeyDatabaseRuntime();
      QDir(scale_path_).removeRecursively();
    }
    delete fixture_dir_;
    fixture_dir_ = nullptr;
  }

  void SetUp() override {
    if (!fixture_) {
      if (qEnvironmentVariableIntValue("GF_ALLOW_MISSING_INTEROP_TOOLS") != 0) {
        GTEST_SKIP() << "sq is not installed";
      }
      FAIL() << "sq is needed to make the keys";
    }
    ASSERT_TRUE(imported_);
  }

  /// The application's key list, refreshed; how many rows it has.
  static auto Refresh() -> int {
    auto* keys = KeyListController::create(nullptr, nullptr);
    QElapsedTimer timer;
    timer.start();
    RunOnMainThread([&]() { keys->refresh(); });
    WaitFor(
        [&]() {
          bool busy = true;
          RunOnMainThread([&]() { busy = keys->Refreshing(); });
          return !busy;
        },
        60000);
    Report("key list: refresh", timer.elapsed());
    int rows = 0;
    RunOnMainThread([&]() { rows = keys->Keys()->rowCount(); });
    return rows;
  }
};

TEST_F(MobileScaleTest, TheKeyListHoldsAThousandKeys) {
  EXPECT_GE(Refresh(), kKeys);

  auto* keys = KeyListController::create(nullptr, nullptr);
  KeyFilterModel* filter = nullptr;
  RunOnMainThread([&]() {
    filter = new KeyFilterModel();
    filter->setSourceModel(keys->Keys());
  });
  // Read as a person reads a long list: 2 before 10, not 1, 10, 100.
  RunOnMainThread([&]() {
    QStringList first;
    for (int row = 0; row < 3; ++row) {
      first << filter->index(row, 0).data(KeyRoleModel::kNameRole).toString();
    }
    EXPECT_EQ(first,
              (QStringList{"Scale User 1", "Scale User 2", "Scale User 3"}));
  });
  EXPECT_LT(Timed("key filter: search one name",
                  [&]() { filter->SetSearchText("scale user 777 "); }),
            500);
  RunOnMainThread([&]() { EXPECT_EQ(filter->rowCount(), 1); });
  EXPECT_LT(Timed("key filter: search that matches all",
                  [&]() { filter->SetSearchText("example.org"); }),
            500);
  Timed("key filter: clear search", [&]() { filter->SetSearchText({}); });
  EXPECT_LT(Timed("key filter: my keys (builtin category)",
                  [&]() { filter->SetCategory("has_private_key"); }),
            500);
  RunOnMainThread([&]() { EXPECT_EQ(filter->rowCount(), kSecretKeys); });
  EXPECT_LT(Timed("key filter: expiring soon (builtin)",
                  [&]() { filter->SetCategory("expiring_soon"); }),
            500);
  Timed("key filter: all", [&]() { filter->SetCategory({}); });
  RunOnMainThread([&]() { delete filter; });
}

TEST_F(MobileScaleTest, ACategoryOfFiveHundredKeys) {
  Refresh();
  auto* keys = KeyListController::create(nullptr, nullptr);
  KeyCategoriesController* categories = nullptr;
  QString id;
  QStringList key_ids;
  RunOnMainThread([&]() {
    categories = new KeyCategoriesController();
    // add() says what is wrong, or nothing; the id is in the list.
    EXPECT_EQ(categories->add("Scale half", categories->suggestColor()), "");
    for (const auto& c : categories->Categories()) {
      if (c.toMap().value("name") == "Scale half") {
        id = c.toMap().value("id").toString();
      }
    }
    for (int row = 0; row < keys->Keys()->rowCount() && key_ids.size() < 500;
         ++row) {
      key_ids << keys->Keys()
                     ->data(keys->Keys()->index(row, 0),
                            KeyRoleModel::kKeyIdRole)
                     .toString();
    }
  });
  ASSERT_FALSE(id.isEmpty());
  ASSERT_EQ(key_ids.size(), 500);
  EXPECT_LT(Timed("categories: put 500 keys in, one at a time",
                  [&]() {
                    for (const auto& key : key_ids) {
                      categories->setMember(id, key, true);
                    }
                  }),
            20000);
  QVariantList list;
  EXPECT_LT(Timed("categories: list them (with counts)",
                  [&]() { list = categories->Categories(); }),
            500);
  KeyFilterModel* filter = nullptr;
  RunOnMainThread([&]() {
    filter = new KeyFilterModel();
    filter->setSourceModel(keys->Keys());
  });
  EXPECT_LT(Timed("key filter: the 500-key category",
                  [&]() { filter->SetCategory(id); }),
            1000);
  RunOnMainThread([&]() { EXPECT_EQ(filter->rowCount(), 500); });
  EXPECT_LT(Timed("categories: which ones one key is in",
                  [&]() { categories->categoriesOf(key_ids.first()); }),
            200);
  RunOnMainThread([&]() {
    delete filter;
    categories->remove(id);
    delete categories;
  });
}

TEST_F(MobileScaleTest, FiveHundredRecipients) {
  Refresh();
  auto* keys = KeyListController::create(nullptr, nullptr);
  auto* selection = WorkSelection::Instance();
  QStringList fprs;
  RunOnMainThread([&]() {
    for (int row = 0; row < keys->Keys()->rowCount() && fprs.size() < 500;
         ++row) {
      fprs << keys->Keys()
                  ->data(keys->Keys()->index(row, 0),
                         KeyRoleModel::kFingerprintRole)
                  .toString();
    }
  });
  EXPECT_LT(Timed("selection: add 500 recipients, one at a time",
                  [&]() {
                    for (const auto& fpr : fprs) selection->addRecipient(fpr);
                  }),
            5000);
  QVariantList shown;
  EXPECT_LT(Timed("selection: the recipients' keys",
                  [&]() { shown = selection->RecipientKeys(); }),
            500);
  EXPECT_EQ(shown.size(), 500);
  Timed("selection: clear", [&]() { selection->clearRecipients(); });
}

TEST_F(MobileScaleTest, TheKeysPageAndToolboxWithAThousandKeys) {
  Refresh();
  RunOnMainThread([]() {
    QQmlEngine engine;
    QElapsedTimer timer;
    timer.start();
    QQmlComponent page_type(&engine, "GpgFrontend.Mobile", "KeysPage");
    std::unique_ptr<QObject> page(page_type.create());
    ASSERT_NE(page, nullptr) << page_type.errorString().toStdString();
    Report("QML: make the Keys page", timer.elapsed());
    auto* list = FindItem(page.get(), "keyList");
    if (list != nullptr) {
      EXPECT_GE(list->property("count").toInt(), kKeys);
    }

    timer.restart();
    QQmlComponent box_type(&engine, "GpgFrontend.Mobile", "KeyToolbox");
    std::unique_ptr<QObject> box(box_type.create());
    ASSERT_NE(box, nullptr) << box_type.errorString().toStdString();
    QMetaObject::invokeMethod(box.get(), "begin", Q_ARG(QVariant, "encrypt"),
                              Q_ARG(QVariant, QVariant()));
    Report("QML: make and open the key toolbox", timer.elapsed());
    EXPECT_LT(timer.elapsed(), 1000);

    // Ticking keys with the toolbox open: every row and line that shows the
    // choice follows each tick.
    auto* keys = KeyListController::create(nullptr, nullptr);
    QStringList fprs;
    for (int row = 0; row < keys->Keys()->rowCount() && fprs.size() < 500;
         ++row) {
      fprs << keys->Keys()
                  ->data(keys->Keys()->index(row, 0),
                         KeyRoleModel::kFingerprintRole)
                  .toString();
    }
    auto* selection = WorkSelection::Instance();
    // Each tick a tap: the events in between are handled, as they are
    // between two taps (rows made again are deleted then, not piled up).
    timer.restart();
    for (const auto& fpr : fprs) {
      selection->toggleRecipient(fpr);
      QCoreApplication::processEvents();
      QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    Report("QML: tick 500 keys in the open toolbox", timer.elapsed());
    EXPECT_LT(timer.elapsed(), 5000);
    timer.restart();
    selection->toggleRecipient(fprs.last());
    Report("QML: one more tick with 499 chosen", timer.elapsed());
    // Was 90 ms: every tick read every chosen key's roles again.
    EXPECT_LT(timer.elapsed(), 30);
    selection->clearRecipients();
  });
}

/// A thousand small files, in the workspace the Files page has.
class MobileScaleFilesTest : public ::testing::Test {
 protected:
  QTemporaryDir dir_;
  FileWorkspace* files_ = nullptr;
  QList<QUrl> urls_;

  void SetUp() override {
    for (int i = 0; i < kFiles; ++i) {
      const auto path = dir_.filePath(QString("note-%1.txt").arg(i));
      QFile file(path);
      ASSERT_TRUE(file.open(QIODevice::WriteOnly));
      file.write(QByteArray("note ") + QByteArray::number(i));
      urls_ << QUrl::fromLocalFile(path);
    }
    RunOnMainThread([this]() {
      files_ = new FileWorkspace(
          OperationQueue::Instance(),
          GpgFrontend::Mobile::AppLifecycle::create(nullptr, nullptr));
    });
  }

  void TearDown() override {
    RunOnMainThread([this]() { delete files_; });
  }

  void Finish(int timeout_ms) {
    EXPECT_TRUE(WaitFor(
        [this]() {
          bool busy = true;
          RunOnMainThread([this, &busy]() { busy = files_->Busy(); });
          return !busy;
        },
        timeout_ms));
  }
};

TEST_F(MobileScaleFilesTest, AThousandFilesAreAddedChosenAndRemoved) {
  EXPECT_LT(
      Timed("files: add 1000 at once", [&]() { files_->addFiles(urls_); }),
      5000);
  RunOnMainThread([&]() { EXPECT_EQ(files_->Count(), kFiles); });
  EXPECT_LT(Timed("files: select all", [&]() { files_->selectAll(); }), 1000);
  RunOnMainThread([&]() { EXPECT_EQ(files_->SelectedCount(), kFiles); });
  EXPECT_LT(Timed("files: the chosen ids", [&]() { files_->selectedIds(); }),
            200);
  Timed("files: clear selection", [&]() { files_->clearSelection(); });
  EXPECT_LT(Timed("files: tick 1000 boxes, one at a time",
                  [&]() {
                    for (int row = 0; row < kFiles; ++row) {
                      files_->setSelected(files_->idAt(row), true);
                    }
                  }),
            3000);
  FileFilterModel* filter = nullptr;
  RunOnMainThread([&]() {
    filter = new FileFilterModel();
    filter->setSourceModel(files_);
  });
  EXPECT_LT(Timed("file filter: search", [&]() { filter->SetSearchText("9"); }),
            500);
  EXPECT_LT(Timed("file filter: the shown ids", [&]() { filter->ids(); }), 500);
  RunOnMainThread([&]() { delete filter; });
  QVariantList all;
  RunOnMainThread([&]() { all = files_->selectedIds(); });
  EXPECT_LT(Timed("files: remove 1000", [&]() { files_->remove(all); }), 3000);
  RunOnMainThread([&]() { EXPECT_EQ(files_->Count(), 0); });
}

TEST_F(MobileScaleFilesTest, AHundredFilesAreEncryptedInOneBatch) {
  const auto fred = GenerateKey("ScaleFred", {});
  ASSERT_FALSE(fred.isEmpty());
  QVariantList ids;
  RunOnMainThread([&]() { ids = files_->addFiles(urls_.mid(0, 100)); });
  ASSERT_EQ(ids.size(), 100);
  QElapsedTimer timer;
  timer.start();
  RunOnMainThread([&]() { files_->encrypt(ids, {fred}, {}, false); });
  Finish(300000);
  Report("files: encrypt 100 in one batch", timer.elapsed());
  int outputs = 0;
  RunOnMainThread([&]() {
    for (int row = 0; row < files_->Count(); ++row) {
      outputs +=
          files_->entry(files_->idAt(row)).value("origin") == "made" ? 1 : 0;
    }
  });
  EXPECT_EQ(outputs, 100);
}

TEST_F(MobileScaleFilesTest, TheFilesPageWithAThousandFiles) {
  RunOnMainThread([&]() {
    QQmlEngine engine;
    QQmlComponent page_type(&engine, "GpgFrontend.Mobile", "FilesPage");
    std::unique_ptr<QObject> page(page_type.create());
    ASSERT_NE(page, nullptr) << page_type.errorString().toStdString();
    auto* workspace = page->property("workspace").value<FileWorkspace*>();
    ASSERT_NE(workspace, nullptr);
    QElapsedTimer timer;
    timer.start();
    workspace->addFiles(urls_);
    Report("QML: Files page takes 1000 files", timer.elapsed());
    timer.restart();
    auto* all = FindItem(page.get(), "selectAll");
    ASSERT_NE(all, nullptr);
    QMetaObject::invokeMethod(all, "clicked");
    Report("QML: Files page, Select all", timer.elapsed());
    EXPECT_EQ(workspace->SelectedCount(), kFiles);
    EXPECT_LT(timer.elapsed(), 2000);
    workspace->clear();
  });
}

}  // namespace GpgFrontend::Test::Mobile
