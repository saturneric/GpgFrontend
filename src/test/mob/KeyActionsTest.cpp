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

#include <QClipboard>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "MobileCryptoTest.h"
#include "core/function/rpgp/PasswordFetcher.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/RustUtils.h"
#include "crypto/KeySelection.h"
#include "crypto/OutputActions.h"
#include "model/KeyActionsController.h"
#include "model/KeyCategoriesController.h"
#include "model/KeyFilterModel.h"
#include "model/KeyGenProfilesController.h"
#include "model/KeyGroupsController.h"
#include "model/KeyImportController.h"
#include "model/KeyListController.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::KeyActionsController;
using GpgFrontend::Mobile::KeyCategoriesController;
using GpgFrontend::Mobile::KeyGenProfilesController;
using GpgFrontend::Mobile::KeyImportController;
using GpgFrontend::Mobile::OutputActions;

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

// As the desktop's wizard: a name, an email only if wanted, and a comment.
TEST_F(KeyActionsTest, AKeyIsMadeWithACommentAndNoEmail) {
  const auto done =
      Act([](auto* a) { a->generate("Kim Comment", "", 0, false, "phone"); });
  ASSERT_TRUE(done.succeeded)
      << done.detail.value("error").toString().toStdString();
  RunOnMainThread([&]() {
    KeyActionsController a;
    const auto d = a.describe(done.detail.value("fingerprint").toString());
    EXPECT_EQ(d.value("name").toString(), "Kim Comment");
    EXPECT_EQ(d.value("comment").toString(), "phone");
    EXPECT_TRUE(d.value("email").toString().isEmpty());
  });
}

// Each part of a user ID is judged on its own, as the desktop judges it.
TEST_F(KeyActionsTest, AUserIdIsCheckedPartByPart) {
  RunOnMainThread([]() {
    KeyActionsController a;
    auto c = a.checkIdentity("Lee", "", "");
    EXPECT_TRUE(c.value("ok").toBool());
    c = a.checkIdentity("", "lee@example.org", "");
    EXPECT_FALSE(c.value("ok").toBool());
    EXPECT_FALSE(c.value("name").toString().isEmpty());
    c = a.checkIdentity("Lee", "lee@", "x<y");
    EXPECT_FALSE(c.value("ok").toBool());
    EXPECT_TRUE(c.value("name").toString().isEmpty());
    EXPECT_FALSE(c.value("email").toString().isEmpty());
    EXPECT_FALSE(c.value("comment").toString().isEmpty());
  });
  const auto done = Act(
      [](auto* a) { a->generate("Lee", "lee@example.org", 0, false, "(x)"); });
  EXPECT_FALSE(done.succeeded);
  EXPECT_FALSE(done.detail.value("error").toString().isEmpty());
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
  QVariantMap own;
  RunOnMainThread([&]() { own = actions_->describe(fpr); });
  ASSERT_FALSE(own.value("capabilities").toString().isEmpty());

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

  // Someone else's key, as it now is here: it still certifies and signs
  // (what a signature is checked against), not only encrypts.
  QVariantMap theirs;
  RunOnMainThread([&]() { theirs = actions_->describe(fpr); });
  EXPECT_FALSE(theirs.value("hasSecret").toBool());
  EXPECT_EQ(theirs.value("capabilities").toString(),
            own.value("capabilities").toString());
  EXPECT_TRUE(theirs.value("capabilities").toString().contains("S"));
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

// The clipboard is read only when asked, and what it says decides: nothing
// to import, text that is no key, or a key.
TEST_F(KeyActionsTest, KeysAreImportedFromTheClipboard) {
  const auto made = Act([](auto* a) {
    a->generate("Clip Board", "clip@mobile.example.invalid", 0, false);
  });
  ASSERT_TRUE(made.succeeded);
  const auto fpr = made.detail.value("fingerprint").toString();
  const auto exported = Act([&fpr](auto* a) { a->exportPublic(fpr); });
  ASSERT_TRUE(exported.succeeded);
  const auto armored = exported.detail.value("armored").toString();
  ASSERT_TRUE(Act([&fpr](auto* a) { a->remove(fpr); }).succeeded);

  // Empty, or only white space: nothing is started.
  bool started = true;
  RunOnMainThread([&]() {
    QGuiApplication::clipboard()->setText(QStringLiteral("  \n "));
    started = importer_->importClipboard();
  });
  EXPECT_FALSE(started);

  // Text that is no key: read, and nothing imported.
  const auto notes = Import([](auto* i) {
    QGuiApplication::clipboard()->setText(QStringLiteral("just some notes"));
    EXPECT_TRUE(i->importClipboard());
  });
  EXPECT_EQ(notes.value("imported").toInt(), 0);

  // A key block: imported.
  const auto imported = Import([&armored](auto* i) {
    QGuiApplication::clipboard()->setText(armored);
    EXPECT_TRUE(i->importClipboard());
  });
  EXPECT_EQ(imported.value("imported").toInt(), 1);
  EXPECT_TRUE(HasKey(fpr));
  RunOnMainThread([]() { QGuiApplication::clipboard()->clear(); });
}

TEST_F(KeyActionsTest, AKeyIsDescribedAsTheDesktopDescribesIt) {
  const auto made = Act([](auto* a) {
    a->generate("Erin Details", "erin.details@mobile.example.invalid", 0,
                false);
  });
  ASSERT_TRUE(made.succeeded);
  const auto fpr = made.detail.value("fingerprint").toString();

  QVariantMap key;
  RunOnMainThread([&]() { key = actions_->describe(fpr); });
  ASSERT_FALSE(key.isEmpty());
  EXPECT_EQ(key.value("name").toString(), "Erin Details");
  EXPECT_EQ(key.value("fingerprint").toString().toUpper(), fpr.toUpper());
  EXPECT_TRUE(key.value("hasSecret").toBool());
  EXPECT_TRUE(key.value("primarySecret").toBool());
  // The mark the key lists show, from the same rule.
  EXPECT_EQ(key.value("secretState").toString(), "full");
  EXPECT_GT(key.value("version").toInt(), 0);
  EXPECT_FALSE(key.value("algorithm").toString().isEmpty());
  EXPECT_TRUE(key.value("usage").toMap().value("sign").toBool());

  const auto uids = key.value("uids").toList();
  ASSERT_EQ(uids.size(), 1);
  EXPECT_TRUE(uids.first().toMap().value("primary").toBool());
  EXPECT_EQ(uids.first().toMap().value("email").toString(),
            "erin.details@mobile.example.invalid");

  // The primary key first, then its subkeys.
  const auto subkeys = key.value("subkeys").toList();
  ASSERT_GE(subkeys.size(), 2);
  EXPECT_TRUE(subkeys.at(0).toMap().value("primary").toBool());
  EXPECT_TRUE(
      subkeys.at(0).toMap().value("usage").toMap().value("certify").toBool());
  EXPECT_FALSE(subkeys.at(1).toMap().value("primary").toBool());

  // What the key can do is what its parts can do together, in GnuPG's
  // order; each part says what it does itself (the primary certifies, the
  // encryption subkey encrypts), never what another part does.
  QString together;
  for (const auto& sub : subkeys)
    together += sub.toMap()["capabilities"].toString();
  QString expected;
  for (const auto letter : QStringLiteral("CESA")) {
    if (together.contains(letter)) expected += letter;
  }
  EXPECT_EQ(key.value("capabilities").toString(), expected);
  EXPECT_TRUE(expected.startsWith("C"));
  EXPECT_TRUE(subkeys.at(0).toMap()["capabilities"].toString().contains("C"));
  EXPECT_FALSE(subkeys.at(1).toMap()["capabilities"].toString().contains("C"));

  QVariantList validities;
  RunOnMainThread([&]() { validities = actions_->Validities(); });
  ASSERT_FALSE(validities.isEmpty());
  EXPECT_EQ(validities.last().toMap().value("id").toString(), "forever");

  const auto can = key.value("can").toMap();
  EXPECT_TRUE(can.value("addSubkey").toBool());
  EXPECT_TRUE(can.value("setExpiration").toBool());

  RunOnMainThread([&]() {
    EXPECT_TRUE(actions_->describe("0000000000000000000000000000000000000000")
                    .isEmpty());
  });
}

TEST_F(KeyActionsTest, SubkeysAreAddedReDatedRevokedAndDeleted) {
  const auto made = Act([](auto* a) {
    a->generate("Fay Subkeys", "fay.subkeys@mobile.example.invalid", 0, false);
  });
  ASSERT_TRUE(made.succeeded);
  const auto fpr = made.detail.value("fingerprint").toString();
  const auto subkeys = [&]() {
    QVariantList list;
    RunOnMainThread(
        [&]() { list = actions_->describe(fpr).value("subkeys").toList(); });
    return list;
  };
  const auto before = subkeys();
  // The subkey with @p fingerprint, as the key now lists it. The new one is
  // found by its fingerprint: where the engine puts it among the others is
  // its own matter (made in the same second, it went before the key's
  // encryption subkey).
  const auto find = [&](const QString& fingerprint) {
    for (const auto& s : subkeys()) {
      if (s.toMap().value("fingerprint").toString() == fingerprint)
        return s.toMap();
    }
    return QVariantMap{};
  };

  // A signing subkey, of the primary key's own family, for a year.
  const auto added =
      Act([&](auto* a) { a->addSubkey(fpr, "sign", "1y", false); });
  ASSERT_TRUE(added.succeeded)
      << added.detail.value("error").toString().toStdString();
  const auto list = subkeys();
  ASSERT_EQ(list.size(), before.size() + 1);
  // The one whose fingerprint was not there before (the others' indexes may
  // have moved).
  QStringList known;
  for (const auto& s : before)
    known << s.toMap().value("fingerprint").toString();
  QVariantMap newest;
  for (const auto& s : list) {
    if (!known.contains(s.toMap().value("fingerprint").toString()))
      newest = s.toMap();
  }
  ASSERT_FALSE(newest.isEmpty());
  EXPECT_TRUE(newest.value("usage").toMap().value("sign").toBool());
  EXPECT_FALSE(newest.value("usage").toMap().value("encrypt").toBool());
  const auto expires = newest.value("expires").toDateTime();
  EXPECT_TRUE(expires.isValid());
  EXPECT_LT(qAbs(expires.daysTo(QDateTime::currentDateTime().addYears(1))), 2);
  const auto sub = newest.value("fingerprint").toString();

  // Made never to expire.
  const auto dated =
      Act([&](auto* a) { a->setExpiration(fpr, sub, "forever"); });
  ASSERT_TRUE(dated.succeeded)
      << dated.detail.value("error").toString().toStdString();
  EXPECT_FALSE(find(sub).value("expires").toDateTime().isValid());

  // Revoked: still listed, and marked so.
  const auto revoked =
      Act([&](auto* a) { a->revokeSubkey(fpr, sub, 3, "no longer used"); });
  ASSERT_TRUE(revoked.succeeded)
      << revoked.detail.value("error").toString().toStdString();
  EXPECT_TRUE(find(sub).value("revoked").toBool());

  // Deleted: gone.
  const auto deleted = Act([&](auto* a) { a->deleteSubkey(fpr, sub); });
  ASSERT_TRUE(deleted.succeeded);
  EXPECT_EQ(subkeys().size(), before.size());
  EXPECT_TRUE(find(sub).isEmpty());

  // The primary key is no subkey to delete or revoke.
  const auto primary = Act([&](auto* a) { a->deleteSubkey(fpr, fpr); });
  EXPECT_FALSE(primary.succeeded);
  const auto unknown =
      Act([&](auto* a) { a->addSubkey(fpr, "launch", "forever", false); });
  EXPECT_FALSE(unknown.succeeded);
}

TEST_F(KeyActionsTest, ProfilesAreEditedAsTheDesktopEditsThem) {
  // The test profile's own settings, put back at the end.
  const auto before = LoadKeyGenSettings();
  RunOnMainThread([&]() {
    KeyGenProfilesController profiles;
    KeyGenProfilesController other;
    auto list = profiles.Profiles();
    ASSERT_EQ(other.Profiles().size(), list.size());
    ASSERT_FALSE(list.isEmpty());
    const auto first = list.first().toMap();
    EXPECT_TRUE(first.value("available").toBool());
    // Exactly one is what Create a key starts from, chosen or not.
    int defaults = 0;
    for (const auto& p : list)
      defaults += p.toMap().value("isDefault").toBool();
    EXPECT_EQ(defaults, 1);
    EXPECT_FALSE(first.value("algorithms").toString().isEmpty());

    // A copy under a new name: added, and offered for generating.
    auto copy = first;
    copy["storedName"] = "Phone Test Profile";
    copy["description"] = "Made on the phone.";
    EXPECT_EQ(profiles.save(-1, copy), "");
    list = profiles.Profiles();
    EXPECT_EQ(list.last().toMap().value("name").toString(),
              "Phone Test Profile");
    const auto added = list.size() - 1;

    // A name already taken is refused, with the reason.
    EXPECT_FALSE(profiles.save(-1, copy).isEmpty());
    // So is a profile that cannot be made here.
    auto broken = copy;
    broken["storedName"] = "Phone Broken Profile";
    broken["primary"] = QVariantMap{{"id", "nothing"}, {"type", ""}};
    EXPECT_FALSE(profiles.save(-1, broken).isEmpty());

    // The default follows a rename.
    profiles.setDefault(added);
    copy["storedName"] = "Phone Renamed Profile";
    EXPECT_EQ(profiles.save(added, copy), "");
    EXPECT_TRUE(
        profiles.Profiles().at(added).toMap().value("isDefault").toBool());
    EXPECT_EQ(LoadStoredKeyGenProfiles().default_profile,
              "Phone Renamed Profile");

    // Another page's controller, which read them before the change, does not
    // write its older copy back over it.
    other.setDefault(0);
    bool kept = false;
    for (const auto& p : LoadStoredKeyGenProfiles().profiles) {
      kept |= p.name == "Phone Renamed Profile";
    }
    EXPECT_TRUE(kept);
    profiles.setDefault(added);

    // Shared with KeyActionsController, which offers it once told.
    KeyActionsController actions;
    bool named = false;
    for (const auto& p : actions.Profiles()) {
      named |= p.toMap().value("name").toString() == "Phone Renamed Profile";
    }
    EXPECT_TRUE(named);

    // Moved: the order Create a key lists them in.
    const auto second = profiles.Profiles().at(1).toMap().value("storedName");
    profiles.move(1, 0);
    EXPECT_EQ(profiles.Profiles().first().toMap().value("storedName"), second);
    profiles.move(0, 99);  // nowhere: nothing happens
    EXPECT_EQ(profiles.Profiles().first().toMap().value("storedName"), second);

    // Expert Mode is remembered, in the desktop's setting.
    const auto expert = profiles.ExpertMode();
    profiles.SetExpertMode(!expert);
    EXPECT_EQ(LoadKeyGenSettings().expert_mode, !expert);
    EXPECT_EQ(KeyGenProfilesController().ExpertMode(), !expert);
    profiles.SetExpertMode(expert);

    // The default can be neither hidden nor removed, as on the desktop:
    // Create a key would start from nothing. The list says why.
    const auto renamed = profiles.Profiles().at(added).toMap();
    EXPECT_FALSE(renamed.value("deleteRefusal").toString().isEmpty());
    EXPECT_FALSE(renamed.value("hideRefusal").toString().isEmpty());
    auto hidden = renamed;
    hidden["hidden"] = true;
    EXPECT_FALSE(profiles.save(added, hidden).isEmpty());
    EXPECT_FALSE(
        profiles.Profiles().at(added).toMap().value("hidden").toBool());
    EXPECT_FALSE(profiles.remove(added));
    EXPECT_EQ(profiles.Profiles().size(), list.size());

    // Once another is the default, it goes, and the built-ins come back when
    // asked.
    profiles.setDefault(0);
    EXPECT_TRUE(profiles.Profiles()
                    .at(added)
                    .toMap()
                    .value("deleteRefusal")
                    .toString()
                    .isEmpty());
    EXPECT_TRUE(profiles.remove(added));
    EXPECT_EQ(profiles.Profiles().size(), list.size() - 1);
    profiles.setDefault(1);
    profiles.remove(0);
    profiles.restoreDefaults();
    bool back = false;
    for (const auto& p : profiles.Profiles()) {
      back |= p.toMap().value("storedName") == first.value("storedName");
    }
    EXPECT_TRUE(back);
  });
  StoreKeyGenSettings(before);
}

TEST_F(KeyActionsTest, AnAlgorithmDecidesWhatItsKeyMayBeUsedFor) {
  RunOnMainThread([&]() {
    KeyGenProfilesController profiles;
    const QVariantMap ed25519{{"id", "ed25519"}, {"type", ""}};
    const auto usage = profiles.usage(ed25519, false);
    ASSERT_FALSE(usage.isEmpty());
    // A signing algorithm: it never encrypts, and signing may be turned off.
    EXPECT_FALSE(usage.value("encrypt").toBool());
    EXPECT_FALSE(usage.value("changeEncrypt").toBool());
    EXPECT_TRUE(usage.value("sign").toBool());
    EXPECT_TRUE(usage.value("changeSign").toBool());

    EXPECT_EQ(profiles.suggestSubkey(ed25519, "encrypt").value("id"),
              "cv25519");
    EXPECT_FALSE(profiles.PrimaryAlgos().isEmpty());
    EXPECT_FALSE(profiles.SubkeyAlgos().isEmpty());
    // Not a hybrid: nothing to pair with.
    EXPECT_TRUE(profiles.pairings(ed25519).isEmpty());
  });
}

TEST_F(KeyActionsTest, AnExpertKeyIsMadeOfWhatWasChosen) {
  const QVariantMap primary{
      {"algo", QVariantMap{{"id", "ed25519"}, {"type", ""}}},
      {"usage", QVariantMap{{"sign", true}, {"authenticate", false}}},
      {"validity", "1y"}};
  const QVariantMap subkey{
      {"algo", QVariantMap{{"id", "cv25519"}, {"type", ""}}},
      {"validity", "forever"}};
  const auto made = Act([&](auto* a) {
    a->generateCustom("Gil Expert", "gil.expert@mobile.example.invalid",
                      primary, subkey, false);
  });
  ASSERT_TRUE(made.succeeded)
      << made.detail.value("error").toString().toStdString();
  const auto fpr = made.detail.value("fingerprint").toString();

  QVariantMap key;
  RunOnMainThread([&]() { key = actions_->describe(fpr); });
  const auto subkeys = key.value("subkeys").toList();
  ASSERT_EQ(subkeys.size(), 2);
  const auto first = subkeys.at(0).toMap();
  EXPECT_FALSE(first.value("usage").toMap().value("authenticate").toBool());
  EXPECT_TRUE(first.value("expires").toDateTime().isValid());
  const auto second = subkeys.at(1).toMap();
  EXPECT_TRUE(second.value("usage").toMap().value("encrypt").toBool());
  EXPECT_FALSE(second.value("expires").toDateTime().isValid());

  // A subkey for logging in, as chosen.
  const auto added = Act([&](auto* a) {
    a->addCustomSubkey(
        fpr,
        {{"algo", QVariantMap{{"id", "ed25519"}, {"type", ""}}},
         {"usage", QVariantMap{{"sign", false}, {"authenticate", true}}},
         {"validity", "2y"}},
        false);
  });
  ASSERT_TRUE(added.succeeded)
      << added.detail.value("error").toString().toStdString();
  RunOnMainThread([&]() { key = actions_->describe(fpr); });
  // The new one by its fingerprint: where the engine lists it is its own
  // matter (made in the same second, it can go before the others).
  QStringList known;
  for (const auto& s : subkeys)
    known << s.toMap().value("fingerprint").toString();
  QVariantMap newest;
  for (const auto& s : key.value("subkeys").toList()) {
    if (!known.contains(s.toMap().value("fingerprint").toString()))
      newest = s.toMap();
  }
  ASSERT_FALSE(newest.isEmpty());
  EXPECT_TRUE(newest.value("usage").toMap().value("authenticate").toBool());
  EXPECT_FALSE(newest.value("usage").toMap().value("sign").toBool());

  // An algorithm that is not on offer is refused before anything runs.
  const auto refused = Act([&](auto* a) {
    a->generateCustom("Gil Expert", "gil@mobile.example.invalid",
                      {{"algo", QVariantMap{{"id", "nothing"}}}}, {}, false);
  });
  EXPECT_FALSE(refused.succeeded);
}

TEST_F(KeyActionsTest, UserIdsAreAddedMadePrimaryRevokedAndDeleted) {
  const auto made = Act([](auto* a) {
    a->generate("Hal Uids", "hal@mobile.example.invalid", 0, false);
  });
  ASSERT_TRUE(made.succeeded);
  const auto fpr = made.detail.value("fingerprint").toString();
  const auto uids = [&]() {
    QVariantList list;
    RunOnMainThread(
        [&]() { list = actions_->describe(fpr).value("uids").toList(); });
    return list;
  };

  const auto added = Act([&](auto* a) {
    a->addUid(fpr, "Hal Work", "hal.work@mobile.example.invalid", "office");
  });
  ASSERT_TRUE(added.succeeded)
      << added.detail.value("error").toString().toStdString();
  auto list = uids();
  ASSERT_EQ(list.size(), 2);
  QString work;
  for (const auto& u : list) {
    if (u.toMap().value("email") == "hal.work@mobile.example.invalid") {
      work = u.toMap().value("uid").toString();
      EXPECT_EQ(u.toMap().value("comment").toString(), "office");
    }
  }
  ASSERT_FALSE(work.isEmpty());

  // Made primary: listed first.
  const auto primary = Act([&](auto* a) { a->setPrimaryUid(fpr, work); });
  ASSERT_TRUE(primary.succeeded);
  EXPECT_EQ(uids().first().toMap().value("uid").toString(), work);

  // The other one, revoked, then deleted; the primary cannot be.
  const auto home = uids().last().toMap().value("uid").toString();
  EXPECT_FALSE(Act([&](auto* a) { a->deleteUid(fpr, work); }).succeeded);
  EXPECT_FALSE(Act([&](auto* a) { a->revokeUid(fpr, work, 0, {}); }).succeeded);
  // A user ID takes only "no reason" (0) or "no longer valid" (4), as the
  // desktop offers them: a subkey's "no longer used" (3) is refused.
  EXPECT_FALSE(Act([&](auto* a) {
                 a->revokeUid(fpr, home, 3, "moved away");
               }).succeeded);
  const auto revoked =
      Act([&](auto* a) { a->revokeUid(fpr, home, 4, "moved away"); });
  ASSERT_TRUE(revoked.succeeded)
      << revoked.detail.value("error").toString().toStdString();
  EXPECT_TRUE(uids().last().toMap().value("revoked").toBool());
  const auto deleted = Act([&](auto* a) { a->deleteUid(fpr, home); });
  ASSERT_TRUE(deleted.succeeded);
  EXPECT_EQ(uids().size(), 1);

  // A name that would break the address is refused before anything runs.
  EXPECT_FALSE(Act([&](auto* a) {
                 a->addUid(fpr, "Hal <x>", "hal@mobile.example.invalid", "");
               }).succeeded);
}

// A wrong passphrase is said as one -- not as a general error -- and what
// was asked can be asked again; a declined prompt is a cancellation.
TEST_F(KeyActionsTest, AWrongPassphraseIsSaidAndTheUserIdIsAddedOnRetry) {
  const auto made = Act(
      [](auto* a) {
        a->generate("Ivy Secret", "ivy@mobile.example.invalid", 0, true);
      },
      "ivy's passphrase");
  ASSERT_TRUE(made.succeeded);
  const auto fpr = made.detail.value("fingerprint").toString();
  const auto count = [&]() {
    int n = 0;
    RunOnMainThread(
        [&]() { n = actions_->describe(fpr).value("uids").toList().size(); });
    return n;
  };
  ASSERT_EQ(count(), 1);
  const auto add = [&](auto* a) {
    a->addUid(fpr, "Ivy Work", "ivy.work@mobile.example.invalid", "");
  };

  ClearRpgpPasswordCache();
  const auto wrong = Act(add, "not ivy's passphrase");
  EXPECT_FALSE(wrong.succeeded);
  EXPECT_EQ(wrong.detail.value("reason").toString(), "bad_passphrase");
  EXPECT_EQ(wrong.detail.value("error").toString(),
            "Incorrect passphrase. Please try again.");
  EXPECT_EQ(count(), 1);

  // Declined: stopped by the person, not a failure to explain.
  ClearRpgpPasswordCache();
  const int before = finished_;
  RunOnMainThread([&]() { add(actions_); });
  ASSERT_TRUE(WaitFor([]() { return !CurrentRequest().isEmpty(); }, 30000));
  Decline();
  ASSERT_TRUE(WaitFor(
      [&]() {
        int now = 0;
        RunOnMainThread([&]() { now = finished_; });
        return now > before;
      },
      30000));
  Finished declined;
  RunOnMainThread([&]() { declined = last_; });
  EXPECT_FALSE(declined.succeeded);
  EXPECT_EQ(declined.detail.value("reason").toString(), "cancelled")
      << declined.detail.value("error").toString().toStdString();
  EXPECT_EQ(count(), 1);

  ClearRpgpPasswordCache();
  const auto right = Act(add, "ivy's passphrase");
  ASSERT_TRUE(right.succeeded)
      << right.detail.value("error").toString().toStdString();
  EXPECT_EQ(count(), 2);
}

// Sharing a public key: only public packets, read by the engine's packet
// parser (not by the armor header), in a file named after the key, each
// share in a place of its own so a second one never replaces the first.
TEST_F(KeyActionsTest,
       ASharedPublicKeyHoldsNothingSecretAndIsNeverOverwritten) {
  const auto made = Act([](auto* a) {
    a->generate("Jo Share", "jo@mobile.example.invalid", 0, false);
  });
  ASSERT_TRUE(made.succeeded);
  const auto fpr = made.detail.value("fingerprint").toString();

  const auto exported = Act([&](auto* a) { a->exportPublic(fpr); });
  ASSERT_TRUE(exported.succeeded);
  const auto armored = exported.detail.value("armored").toString();
  const auto name = exported.detail.value("fileName").toString();
  EXPECT_TRUE(name.startsWith("Jo_Share")) << name.toStdString();
  EXPECT_TRUE(name.endsWith("_pub.asc")) << name.toStdString();

  const auto document =
      QJsonDocument::fromJson(InspectOpenPGPData(GFBuffer(armored))).object();
  QList<int> tags;
  for (const auto& block : document.value("blocks").toArray()) {
    for (const auto& packet : block.toObject().value("packets").toArray()) {
      tags << packet.toObject().value("tag").toInt();
    }
  }
  ASSERT_FALSE(tags.isEmpty());
  EXPECT_TRUE(tags.contains(6));   // public key
  EXPECT_FALSE(tags.contains(5));  // secret key
  EXPECT_FALSE(tags.contains(7));  // secret subkey

  QString first;
  QString second;
  RunOnMainThread([&]() {
    first = OutputActions::KeepForSharing(armored.toUtf8(), name);
    second = OutputActions::KeepForSharing(armored.toUtf8(), name);
  });
  ASSERT_FALSE(first.isEmpty());
  ASSERT_FALSE(second.isEmpty());
  EXPECT_NE(first, second);
  EXPECT_TRUE(QFile::exists(first));
  EXPECT_TRUE(QFile::exists(second));
  EXPECT_EQ(QFileInfo(first).fileName(), name);
  QFile kept(first);
  ASSERT_TRUE(kept.open(QIODevice::ReadOnly));
  EXPECT_EQ(kept.readAll(), armored.toUtf8());
}

// "Fill in anonymously" and Quick Generate, as on the desktop: a name and an
// address from the pattern in the key generation settings, under a domain
// that reaches nobody; another each time.
TEST_F(KeyActionsTest, AnAnonymousUserIdReachesNobody) {
  RunOnMainThread([]() {
    KeyGenProfilesController profiles;
    const auto first = profiles.anonymousUid();
    const auto second = profiles.anonymousUid();
    EXPECT_TRUE(first.value("name").toString().startsWith("anon-"));
    const auto email = first.value("email").toString();
    EXPECT_TRUE(email.endsWith(".test") || email.endsWith(".example") ||
                email.endsWith(".invalid"))
        << email.toStdString();
    EXPECT_NE(first.value("name"), second.value("name"));
    // What generate() would accept.
    EXPECT_TRUE(
        CheckKeyGenIdentity(first.value("name").toString(), email, {}).Ok());
  });
}

TEST_F(KeyActionsTest, KeysAreSortedIntoTheDesktopsCategories) {
  RunOnMainThread([&]() {
    KeyCategoriesController categories;
    KeyCategoriesController other;  // another page's
    int heard = 0;
    QObject::connect(&other, &KeyCategoriesController::SignalChanged,
                     [&heard]() { ++heard; });
    const auto before = categories.Categories().size();

    EXPECT_EQ(categories.add("Phone Team A", "#EF4444"), "");
    EXPECT_EQ(categories.add("Phone Team B", categories.suggestColor()), "");
    // A name taken, whatever its case, and an empty one are refused.
    EXPECT_FALSE(categories.add("phone team a", "#22C55E").isEmpty());
    EXPECT_FALSE(categories.add("   ", "#22C55E").isEmpty());
    EXPECT_GT(heard, 0);

    auto list = categories.Categories();
    ASSERT_EQ(list.size(), before + 2);
    const auto a = list.at(before).toMap();
    const auto b = list.at(before + 1).toMap();
    EXPECT_EQ(a.value("name").toString(), "Phone Team A");
    EXPECT_EQ(a.value("color").toString().toUpper(), "#EF4444");
    // Offered a colour unlike the one in use.
    EXPECT_NE(b.value("color").toString().toUpper(), "#EF4444");
    const auto a_id = a.value("id").toString();
    const auto b_id = b.value("id").toString();

    // A key in both, then in one.
    const QString key = "0123456789ABCDEF";
    categories.setMember(a_id, key, true);
    categories.setMember(b_id, key, true);
    EXPECT_EQ(other.categoriesOf(key), (QStringList{a_id, b_id}));
    EXPECT_EQ(other.keyIdsOf(a_id), QStringList{key});
    categories.setMember(b_id, key, false);
    EXPECT_EQ(other.categoriesOf(key), QStringList{a_id});
    EXPECT_EQ(other.Categories().at(before).toMap().value("count").toInt(), 1);

    // Many at once, as the category page adds them: one change, and a key
    // already in is not counted again.
    const auto changes_before = heard;
    QStringList many{key};
    for (int i = 0; i < 50; ++i) {
      many << QString("FEDCBA98765432%1").arg(i, 2, 10, QLatin1Char('0'));
    }
    EXPECT_EQ(categories.setMembers(a_id, many, true), 50);
    EXPECT_EQ(heard, changes_before + 1);
    EXPECT_EQ(other.keyIdsOf(a_id).size(), 51);
    EXPECT_EQ(categories.setMembers(a_id, many.mid(1), false), 50);
    EXPECT_EQ(other.keyIdsOf(a_id), QStringList{key});

    // Reordered, renamed, recoloured: the same for every page.
    categories.move(b_id, -1);
    EXPECT_EQ(other.Categories().at(before).toMap().value("id"), b_id);
    EXPECT_FALSE(categories.edit(b_id, "Phone Team A", "#000000").isEmpty());
    EXPECT_EQ(categories.edit(b_id, "Phone Team C", "#22C55E"), "");
    EXPECT_EQ(other.Categories().at(before).toMap().value("name"),
              "Phone Team C");

    // The key list narrowed to a category shows its keys only.
    auto* keys =
        GpgFrontend::Mobile::KeyListController::create(nullptr, nullptr);
    GpgFrontend::Mobile::KeyFilterModel all;
    all.setSourceModel(keys->Keys());
    if (all.rowCount() > 0) {
      const auto real = all.index(0, 0)
                            .data(GpgFrontend::Mobile::KeyRoleModel::kKeyIdRole)
                            .toString();
      GpgFrontend::Mobile::KeyFilterModel narrowed;
      narrowed.setSourceModel(keys->Keys());
      narrowed.SetCategory(b_id);
      EXPECT_EQ(narrowed.rowCount(), 0);
      categories.setMember(b_id, real, true);
      narrowed.refreshCategory();
      EXPECT_EQ(narrowed.rowCount(), 1);
      narrowed.SetCategory({});
      EXPECT_EQ(narrowed.rowCount(), all.rowCount());
    }

    // The built-in ones go by what keys are, as the desktop's tabs.
    const auto builtins = categories.Builtins();
    ASSERT_FALSE(builtins.isEmpty());
    EXPECT_EQ(builtins.first().toMap().value("id"), "has_private_key");
    int mine = 0;
    for (int row = 0; row < all.rowCount(); ++row) {
      const auto at = all.index(row, 0);
      mine +=
          at.data(GpgFrontend::Mobile::KeyRoleModel::kHasSecretRole).toBool() &&
          !at.data(GpgFrontend::Mobile::KeyRoleModel::kExpiredRole).toBool() &&
          !at.data(GpgFrontend::Mobile::KeyRoleModel::kRevokedRole).toBool();
    }
    GpgFrontend::Mobile::KeyFilterModel own;
    own.setSourceModel(keys->Keys());
    own.SetCategory("has_private_key");
    EXPECT_EQ(own.rowCount(), mine);
    own.SetCategory("only_public_key");
    EXPECT_LE(own.rowCount(), all.rowCount() - mine);

    categories.remove(a_id);
    categories.remove(b_id);
    EXPECT_EQ(other.Categories().size(), before);
    EXPECT_TRUE(other.categoriesOf(key).isEmpty());
    EXPECT_EQ(categories.PresetColors().size(), 20);
  });
}

TEST_F(KeyActionsTest, KeyGroupsAreMadeFilledAndEncryptedTo) {
  // A key of its own to put in the group.
  const auto own = GenerateKey("Group Member", "pw");
  ASSERT_FALSE(own.isEmpty());
  GpgKeyRepository::GetInstance(UsableDefaultChannelHandle()).FlushKeyCache();
  // A refresh an earlier test began shows what it read when it ends; a
  // group's change is shown at once only outside one.
  ASSERT_TRUE(WaitFor(
      []() {
        bool busy = true;
        RunOnMainThread([&busy]() {
          busy =
              GpgFrontend::Mobile::KeyListController::create(nullptr, nullptr)
                  ->Refreshing();
        });
        return !busy;
      },
      30000));
  RunOnMainThread([&]() {
    GpgFrontend::Mobile::KeyListController::Reload();
    using GpgFrontend::Mobile::KeyFilterModel;
    using GpgFrontend::Mobile::KeyRoleModel;
    GpgFrontend::Mobile::KeyGroupsController groups;
    auto* keys =
        GpgFrontend::Mobile::KeyListController::create(nullptr, nullptr);
    ASSERT_NE(keys, nullptr);

    // The desktop's rule: a name of five characters at least.
    EXPECT_FALSE(groups.nameProblem("Team").isEmpty());
    EXPECT_EQ(groups.create("Team"), "");
    const auto id = groups.create("  Phone Group  ");
    ASSERT_TRUE(IsKeyGroupID(id)) << id.toStdString();
    EXPECT_EQ(groups.group(id).value("name"), "Phone Group");
    EXPECT_EQ(groups.group(id).value("memberCount").toInt(), 0);

    // In the list at once: in its own category and where groups are asked
    // for, never among keys or in a key category.
    const auto rows = [&](const QString& category, bool with_groups) {
      KeyFilterModel model;
      model.setSourceModel(keys->Keys());
      model.SetCategory(category);
      model.SetWithGroups(with_groups);
      QStringList found;
      for (int row = 0; row < model.rowCount(); ++row) {
        found << model.index(row, 0)
                     .data(KeyRoleModel::kFingerprintRole)
                     .toString();
      }
      return found;
    };
    EXPECT_TRUE(
        rows(GpgFrontend::Mobile::kKeyGroupCategory, false).contains(id));
    EXPECT_TRUE(rows({}, true).contains(id));
    EXPECT_FALSE(rows({}, false).contains(id));
    EXPECT_FALSE(rows("only_public_key", true).contains(id));

    // Filled with a key of the list, which the picker gives by key ID.
    KeyFilterModel all;
    all.setSourceModel(keys->Keys());
    all.SetSearchText(own);
    ASSERT_EQ(all.rowCount(), 1);
    const auto member_id =
        all.index(0, 0).data(KeyRoleModel::kKeyIdRole).toString();
    const auto member_fpr = own;
    // A group is not put into a group here, and a key goes in once.
    EXPECT_EQ(groups.addMembers(id, {member_id, member_id, id}), 1);
    EXPECT_EQ(groups.memberIds(id), QStringList{member_id});
    const auto members = groups.members(id);
    ASSERT_EQ(members.size(), 1);
    EXPECT_EQ(members.first().toMap().value("fingerprint").toString().toUpper(),
              member_fpr.toUpper());
    EXPECT_FALSE(members.first().toMap().value("missing").toBool());

    // Chosen as a recipient by its id, it stands for the key in it; the
    // same key chosen on its own as well goes in once.
    const auto channel = UsableDefaultChannelHandle();
    const auto chosen =
        GpgFrontend::Mobile::SelectKeys(channel, {id, member_fpr});
    EXPECT_TRUE(chosen.missing.isEmpty());
    ASSERT_EQ(chosen.keys.size(), 1);
    EXPECT_EQ(chosen.keys.first()->Fingerprint().toUpper(),
              member_fpr.toUpper());

    // Renamed by the same rule, emptied, deleted; its key stays.
    EXPECT_FALSE(groups.rename(id, "Grp").isEmpty());
    EXPECT_EQ(groups.rename(id, "Phone Group B"), "");
    EXPECT_EQ(groups.group(id).value("name"), "Phone Group B");
    EXPECT_TRUE(groups.removeMember(id, member_id));
    EXPECT_TRUE(groups.memberIds(id).isEmpty());
    // Empty, it reaches nobody: as good as gone.
    EXPECT_EQ(GpgFrontend::Mobile::SelectKeys(channel, {id}).missing,
              QStringList{id});
    EXPECT_TRUE(groups.remove(id));
    EXPECT_TRUE(groups.group(id).isEmpty());
    EXPECT_FALSE(rows({}, true).contains(id));
    EXPECT_TRUE(HasKey(member_fpr));
  });
}

TEST_F(KeyActionsTest, DeletingAnUnknownKeyFails) {
  const auto done = Act([](auto* a) {
    a->remove("0000000000000000000000000000000000000000");
  });
  EXPECT_FALSE(done.succeeded);
}

}  // namespace GpgFrontend::Test::Mobile
