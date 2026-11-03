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

#include <QTemporaryDir>

#include "MobileCryptoTest.h"
#include "model/KeyActionsController.h"
#include "model/KeyImportController.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::KeyActionsController;
using GpgFrontend::Mobile::KeyImportController;

namespace {

struct Finished {
  QString action;
  bool succeeded = false;
  QVariantMap detail;
};

auto HasKey(const QString& fingerprint) -> bool {
  const auto channel = UsableDefaultChannelHandle();
  GpgKeyRepository::GetInstance(channel).FlushKeyCache();
  return GpgKeyRepository::GetInstance(channel).GetKeyPtr(
             fingerprint.toUpper()) != nullptr;
}

}  // namespace

/// One controller of each, on the interface thread, as the pages have them.
class KeyActionsTest : public ::testing::Test {
 protected:
  KeyActionsController* actions_ = nullptr;
  KeyImportController* importer_ = nullptr;
  Finished last_;
  int finished_ = 0;
  int answered_ = 0;  ///< prompts Act() answered

  void SetUp() override {
    RunOnMainThread([this]() {
      actions_ = new KeyActionsController();
      importer_ = new KeyImportController();
      QObject::connect(actions_, &KeyActionsController::SignalFinished,
                       [this](const QString& action, bool ok,
                              const QVariantMap& detail) {
                         last_ = {action, ok, detail};
                         ++finished_;
                       });
    });
  }

  void TearDown() override {
    RunOnMainThread([]() { Broker()->CancelAll(); });
    RunOnMainThread([this]() {
      delete actions_;
      delete importer_;
    });
  }

  /// Run @p op and wait for its SignalFinished, answering a passphrase
  /// prompt with @p passphrase if one comes.
  auto Act(const std::function<void(KeyActionsController*)>& op,
           const QString& passphrase = {}) -> Finished {
    const int before = finished_;
    RunOnMainThread([this, &op]() { op(actions_); });
    EXPECT_TRUE(WaitFor(
        [this, before, &passphrase]() {
          if (!passphrase.isEmpty() && !CurrentRequest().isEmpty()) {
            Answer(passphrase);
            ++answered_;
          }
          int now = 0;
          RunOnMainThread([this, &now]() { now = finished_; });
          return now > before;
        },
        30000));
    Finished copy;
    RunOnMainThread([this, &copy]() { copy = last_; });
    return copy;
  }

  auto Import(const std::function<void(KeyImportController*)>& op)
      -> QVariantMap {
    RunOnMainThread([this, &op]() { op(importer_); });
    bool busy = true;
    EXPECT_TRUE(WaitFor(
        [this, &busy]() {
          RunOnMainThread([this, &busy]() { busy = importer_->Busy(); });
          return !busy;
        },
        30000));
    QVariantMap result;
    RunOnMainThread([this, &result]() { result = importer_->Result(); });
    return result;
  }
};

TEST_F(KeyActionsTest, TheRecommendedProfileComesFirst) {
  QVariantList profiles;
  RunOnMainThread([this, &profiles]() { profiles = actions_->Profiles(); });
  ASSERT_FALSE(profiles.isEmpty());
  EXPECT_TRUE(profiles.front().toMap().value("name").toString().contains(
      "Ed25519", Qt::CaseInsensitive));
}

TEST_F(KeyActionsTest, AKeyIsGeneratedWithItsPassphrase) {
  const auto done = Act(
      [](auto* a) {
        a->generate("Carol Phone", "carol@mobile.example.invalid", 0, true);
      },
      "carol's passphrase");
  ASSERT_TRUE(done.succeeded) << done.detail.value("error").toString()
                                     .toStdString();
  EXPECT_EQ(done.action, "generate");
  const auto fpr = done.detail.value("fingerprint").toString();
  ASSERT_FALSE(fpr.isEmpty());
  ASSERT_TRUE(HasKey(fpr));
  // Asked for, so the key really is protected by it.
  EXPECT_GT(answered_, 0);

  const auto key = GpgKeyRepository::GetInstance(UsableDefaultChannelHandle())
                       .GetKeyPtr(fpr.toUpper());
  EXPECT_TRUE(key->IsHasEncrCap());
  EXPECT_TRUE(key->IsHasSignCap());
}

TEST_F(KeyActionsTest, AnUnknownProfileIsRefused) {
  const auto done = Act([](auto* a) {
    a->generate("Nobody", "nobody@mobile.example.invalid", 999, false);
  });
  EXPECT_FALSE(done.succeeded);
  EXPECT_FALSE(done.detail.value("error").toString().isEmpty());
}

TEST_F(KeyActionsTest, ExportDeleteAndImportFromAFile) {
  const auto made = Act([](auto* a) {
    a->generate("Dave Phone", "dave@mobile.example.invalid", 0, false);
  });
  ASSERT_TRUE(made.succeeded);
  const auto fpr = made.detail.value("fingerprint").toString();

  // Exported: the public key only.
  const auto exported = Act([&fpr](auto* a) { a->exportPublic(fpr); });
  ASSERT_TRUE(exported.succeeded);
  const auto armored = exported.detail.value("armored").toString();
  EXPECT_TRUE(armored.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK-----"));
  EXPECT_FALSE(armored.contains("PRIVATE KEY"));

  // Deleted: gone, secret part and all.
  const auto deleted = Act([&fpr](auto* a) { a->remove(fpr); });
  ASSERT_TRUE(deleted.succeeded);
  EXPECT_FALSE(HasKey(fpr));

  // Back from a file, as a picker would hand it over.
  QTemporaryDir dir;
  const auto path = dir.filePath("dave.asc");
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write(armored.toUtf8());
  file.close();

  const auto imported = Import(
      [&path](auto* i) { i->importFile(QUrl::fromLocalFile(path)); });
  EXPECT_EQ(imported.value("imported").toInt(), 1);
  EXPECT_EQ(imported.value("secretImported").toInt(), 0);
  EXPECT_TRUE(HasKey(fpr));
}

TEST_F(KeyActionsTest, AFileThatIsNotAKeyImportsNothing) {
  QTemporaryDir dir;
  const auto path = dir.filePath("notes.txt");
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("just some notes");
  file.close();

  const auto result = Import(
      [&path](auto* i) { i->importFile(QUrl::fromLocalFile(path)); });
  EXPECT_EQ(result.value("imported").toInt(), 0);

  const auto missing = Import([&dir](auto* i) {
    i->importFile(QUrl::fromLocalFile(dir.filePath("no-such-file.asc")));
  });
  EXPECT_TRUE(missing.isEmpty());
}

TEST_F(KeyActionsTest, DeletingAnUnknownKeyFails) {
  const auto done = Act([](auto* a) {
    a->remove("0000000000000000000000000000000000000000");
  });
  EXPECT_FALSE(done.succeeded);
}

}  // namespace GpgFrontend::Test::Mobile
