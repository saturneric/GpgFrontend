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

#include "MobileTest.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/utils/GpgUtils.h"
#include "crypto/WorkSelection.h"
#include "model/KeyFilterModel.h"
#include "model/KeyRoleModel.h"
#include "shell/models/GpgKeyTableModel.h"
#include "shell/present/KeyStatus.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::KeyFilterModel;
using GpgFrontend::Mobile::KeyRoleModel;
using GpgFrontend::Mobile::WorkSelection;

namespace {

/// A key that is only its answers.
struct FakeKey : GpgAbstractKey {
  QString id, fpr, name, email;
  GpgAbstractKeyType type = GpgAbstractKeyType::kGPG_KEY;
  bool secret = false, encr = true, sign = true, expired = false,
       revoked = false;

  [[nodiscard]] auto ID() const -> QString override { return id; }
  [[nodiscard]] auto Fingerprint() const -> QString override { return fpr; }
  [[nodiscard]] auto KeyType() const -> GpgAbstractKeyType override {
    return type;
  }
  [[nodiscard]] auto Name() const -> QString override { return name; }
  [[nodiscard]] auto Email() const -> QString override { return email; }
  [[nodiscard]] auto Comment() const -> QString override { return {}; }
  [[nodiscard]] auto IsPrivateKey() const -> bool override { return secret; }
  [[nodiscard]] auto IsHasEncrCap() const -> bool override { return encr; }
  [[nodiscard]] auto IsHasSignCap() const -> bool override { return sign; }
  [[nodiscard]] auto IsHasCertCap() const -> bool override { return true; }
  [[nodiscard]] auto IsHasAuthCap() const -> bool override { return false; }
  [[nodiscard]] auto IsRevoked() const -> bool override { return revoked; }
  [[nodiscard]] auto IsDisabled() const -> bool override { return false; }
  [[nodiscard]] auto IsExpired() const -> bool override { return expired; }
  [[nodiscard]] auto IsGood() const -> bool override { return true; }
  [[nodiscard]] auto PublicKeyAlgo() const -> QString override {
    return "EdDSA";
  }
  [[nodiscard]] auto Algo() const -> QString override { return "ED25519"; }
  [[nodiscard]] auto CreationTime() const -> QDateTime override {
    return QDateTime::fromSecsSinceEpoch(1700000000);
  }
  [[nodiscard]] auto ExpirationTime() const -> QDateTime override { return {}; }
};

auto Key(QString name, QString email, QString id,
         const std::function<void(FakeKey&)>& tweak = {}) -> GpgAbstractKeyPtr {
  auto key = QSharedPointer<FakeKey>::create();
  key->name = std::move(name);
  key->email = std::move(email);
  key->id = id;
  key->fpr = id.repeated(4);
  if (tweak) tweak(*key);
  return key;
}

auto Names(const QAbstractItemModel& model) -> QStringList {
  QStringList names;
  for (int row = 0; row < model.rowCount(); ++row) {
    names << model.index(row, 0).data(KeyRoleModel::kNameRole).toString();
  }
  return names;
}

struct Fixture {
  GpgKeyTableModel table{
      0,
      {
          Key("Carol", "carol@example.org", "CCCC0003"),
          Key("alice", "alice@example.org", "AAAA0001",
              [](FakeKey& k) { k.secret = true; }),
          Key("Bob", "bob@example.net", "BBBB0002",
              [](FakeKey& k) { k.expired = true; }),
          Key("Dave", "dave@example.org", "DDDD0004",
              [](FakeKey& k) { k.encr = false; }),
          Key("Team", "", "GROUP001",
              [](FakeKey& k) { k.type = GpgAbstractKeyType::kGPG_KEYGROUP; }),
      }};
  KeyRoleModel roles;
  KeyFilterModel filtered;

  Fixture() {
    roles.setSourceModel(&table);
    filtered.setSourceModel(&roles);
  }
};

}  // namespace

TEST(KeyModelsTest, RolesReadTheKeyItself) {
  Fixture f;
  const auto names = f.roles.roleNames();
  EXPECT_EQ(names.value(KeyRoleModel::kFingerprintRole), "fingerprint");
  EXPECT_EQ(names.value(KeyRoleModel::kHasSecretRole), "hasSecret");

  const auto alice = f.roles.index(1, 0);
  EXPECT_EQ(alice.data(KeyRoleModel::kNameRole).toString(), "alice");
  EXPECT_EQ(alice.data(KeyRoleModel::kKeyIdRole).toString(), "AAAA0001");
  EXPECT_EQ(alice.data(KeyRoleModel::kFingerprintRole).toString(),
            QString("AAAA0001").repeated(4));
  EXPECT_TRUE(alice.data(KeyRoleModel::kHasSecretRole).toBool());
  EXPECT_TRUE(alice.data(KeyRoleModel::kUsableRole).toBool());
  EXPECT_FALSE(f.roles.index(2, 0).data(KeyRoleModel::kUsableRole).toBool());
  EXPECT_FALSE(f.roles.index(4, 0).data(KeyRoleModel::kIsKeyRole).toBool());
  EXPECT_FALSE(alice.data(KeyRoleModel::kExpiresRole).toDateTime().isValid());
}

// What a key can do, in GnuPG's letters and order, from the key itself; a
// key that cannot be used and a key group say nothing.
TEST(KeyModelsTest, CapabilitiesAreGnuPGsLettersInOrder) {
  Fixture f;
  EXPECT_EQ(f.roles.roleNames().value(KeyRoleModel::kCapabilitiesRole),
            "capabilities");
  const auto at = [&](int row) {
    return f.roles.index(row, 0)
        .data(KeyRoleModel::kCapabilitiesRole)
        .toString();
  };
  EXPECT_EQ(at(0), "CES");  // Carol: certify, encrypt, sign
  EXPECT_EQ(at(2), "");     // Bob: expired
  EXPECT_EQ(at(3), "CS");   // Dave: cannot encrypt
  EXPECT_EQ(at(4), "");     // a key group
}

TEST(KeyModelsTest, CheckingAKeyGoesToTheTable) {
  Fixture f;
  const auto bob = f.roles.index(2, 0);
  EXPECT_FALSE(bob.data(KeyRoleModel::kCheckedRole).toBool());
  EXPECT_TRUE(f.roles.setData(bob, true, KeyRoleModel::kCheckedRole));
  EXPECT_TRUE(bob.data(KeyRoleModel::kCheckedRole).toBool());
  EXPECT_EQ(f.table.GetCheckedKeyIds(), QStringList{"BBBB0002"});
}

TEST(KeyModelsTest, KeysAreSortedByNameAndGroupsLeftOut) {
  Fixture f;
  EXPECT_EQ(Names(f.filtered), QStringList({"alice", "Bob", "Carol", "Dave"}));
  EXPECT_EQ(f.filtered.rowCount(), 4);
}

TEST(KeyModelsTest, LongListsSortNumbersAsNumbers) {
  using GpgFrontend::Mobile::NaturalCompare;
  QStringList names{"User 100", "user 2", "User 1000", "User 10",
                    "User 1",   "User",   "Anna 7"};
  std::sort(names.begin(), names.end(), [](const QString& a, const QString& b) {
    return NaturalCompare(a, b) < 0;
  });
  EXPECT_EQ(names, (QStringList{"Anna 7", "User", "User 1", "user 2", "User 10",
                                "User 100", "User 1000"}));
  // Leading zeros do not count: the fingerprint then decides.
  EXPECT_EQ(NaturalCompare("User 02", "user 2"), 0);
  EXPECT_EQ(NaturalCompare("Key 9", "Key 10"), -1);
  EXPECT_EQ(NaturalCompare("key 10", "KEY 10"), 0);
}

TEST(KeyModelsTest, SearchMatchesNameEmailIdAndFingerprint) {
  Fixture f;
  f.filtered.SetSearchText("EXAMPLE.NET");
  EXPECT_EQ(Names(f.filtered), QStringList({"Bob"}));
  f.filtered.SetSearchText("cccc0003cccc");
  EXPECT_EQ(Names(f.filtered), QStringList({"Carol"}));
  f.filtered.SetSearchText("  ");
  EXPECT_EQ(f.filtered.rowCount(), 4);
}

TEST(KeyModelsTest, NarrowingToOwnOrEncryptableKeys) {
  Fixture f;
  f.filtered.SetOnlySecret(true);
  EXPECT_EQ(Names(f.filtered), QStringList({"alice"}));

  f.filtered.SetOnlySecret(false);
  f.filtered.SetOnlyEncryptCapable(true);
  // Bob has expired, Dave cannot encrypt.
  EXPECT_EQ(Names(f.filtered), QStringList({"alice", "Carol"}));
}

TEST(KeyModelsTest, AKeysStatusIsTheDesktops) {
  Fixture f;
  const auto carol = f.roles.index(0, 0);
  const auto bob = f.roles.index(2, 0);
  EXPECT_EQ(carol.data(KeyRoleModel::kStatusRole).toString(), "ok");
  EXPECT_TRUE(carol.data(KeyRoleModel::kUsableRole).toBool());

  EXPECT_EQ(bob.data(KeyRoleModel::kStatusRole).toString(), "expired");
  EXPECT_EQ(bob.data(KeyRoleModel::kStatusTextRole).toString(),
            DescribeKeyStatus(GpgKeyStatus::kExpired));
  EXPECT_FALSE(bob.data(KeyRoleModel::kUsableRole).toBool());
}

TEST(KeyModelsTest, APastedFingerprintMatchesInAnyGrouping) {
  Fixture f;
  f.filtered.SetSearchText("aaaa 0001 AAAA:0001");
  ASSERT_EQ(f.filtered.rowCount(), 1);
  EXPECT_EQ(f.filtered.index(0, 0).data(KeyRoleModel::kNameRole).toString(),
            "alice");
}

namespace {

auto Fpr(const QString& id) -> QString { return id.repeated(4); }

auto Issues(const WorkSelection& selection) -> QStringList {
  QStringList issues;
  for (const auto& key : selection.RecipientKeys()) {
    issues << key.toMap().value("issue").toString();
  }
  return issues;
}

/// Leaves the profile's preferred signer as it found it: none.
struct SignerReset {
  ~SignerReset() {
    Fixture f;
    WorkSelection selection(&f.roles);
    selection.SetKeysReady();
    selection.setPreferredSigner({});
  }
};

}  // namespace

TEST(WorkSelectionTest, RecipientsStayUntilReset) {
  Fixture f;
  WorkSelection selection(&f.roles);
  selection.SetKeysReady();
  selection.addRecipient(Fpr("AAAA0001"));
  selection.addRecipient(Fpr("CCCC0003"));
  selection.addRecipient(Fpr("aaaa0001"));  // the same key, however written
  EXPECT_EQ(selection.Recipients(),
            (QStringList{Fpr("AAAA0001"), Fpr("CCCC0003")}));
  EXPECT_TRUE(selection.isRecipient(Fpr("cccc0003")));
  selection.toggleRecipient(Fpr("CCCC0003"));
  EXPECT_FALSE(selection.isRecipient(Fpr("CCCC0003")));
  EXPECT_TRUE(selection.EncryptBlock().isEmpty());

  selection.SetPassphraseMode(true);
  selection.SetSigningEnabled(true);
  selection.reset();
  EXPECT_TRUE(selection.Recipients().isEmpty());
  EXPECT_FALSE(selection.PassphraseMode());
  EXPECT_FALSE(selection.SigningEnabled());
  EXPECT_EQ(selection.EncryptBlock(), "no_recipients");
}

TEST(WorkSelectionTest, AKeyThatCannotBeUsedIsReportedNotDropped) {
  Fixture f;
  WorkSelection selection(&f.roles);
  const auto gone = QStringLiteral("0123456789ABCDEF0123456789ABCDEF01234567");
  selection.addRecipient(Fpr("BBBB0002"));  // expired
  selection.addRecipient(Fpr("DDDD0004"));  // cannot encrypt
  selection.addRecipient(gone);
  selection.addRecipient(Fpr("CCCC0003"));

  // Nothing is known of the keys before they are listed: nothing to report.
  EXPECT_EQ(selection.IssueCount(), 0);
  selection.SetKeysReady();
  EXPECT_EQ(Issues(selection),
            (QStringList{"expired", "cannot_encrypt", "missing", ""}));
  EXPECT_EQ(selection.IssueCount(), 3);
  EXPECT_EQ(selection.EncryptBlock(), "recipient_issue");
  EXPECT_EQ(selection.Recipients().size(), 4);
  const auto first = selection.RecipientKeys().first().toMap();
  EXPECT_EQ(first.value("name").toString(), "Bob");
  EXPECT_EQ(selection.RecipientKeys().at(2).toMap().value("fingerprint"), gone);
  // Each says why in the words both interfaces use (Shell::EncryptBlockNote).
  QStringList notes;
  for (const auto& key : selection.RecipientKeys()) {
    notes << key.toMap().value("note").toString();
  }
  EXPECT_EQ(notes, (QStringList{"Expired", "Cannot be encrypted to",
                                "No longer in your keys", ""}));

  // Removing them is the person's to do.
  selection.removeRecipient(Fpr("BBBB0002"));
  selection.removeRecipient(Fpr("DDDD0004"));
  selection.removeRecipient(gone);
  EXPECT_TRUE(selection.EncryptBlock().isEmpty());
}

// What a summary shows of many recipients: those with a problem always, then
// the others up to a few; the count is of all of them.
TEST(WorkSelectionTest, ManyRecipientsAreSummedUp) {
  Fixture f;
  WorkSelection selection(&f.roles);
  selection.SetKeysReady();
  const auto missing = [](int i) {
    return QString("%1").arg(i, 40, 16, QLatin1Char('0')).toUpper();
  };
  selection.addRecipient(Fpr("AAAA0001"));
  selection.addRecipient(Fpr("CCCC0003"));
  selection.addRecipient(missing(1));
  const auto names = [&selection]() {
    QStringList fprs;
    for (const auto& key : selection.ShownRecipientKeys()) {
      fprs << key.toMap().value("fingerprint").toString();
    }
    return fprs;
  };
  EXPECT_EQ(selection.RecipientCount(), 3);
  // The one with a problem first.
  EXPECT_EQ(names(),
            (QStringList{missing(1), Fpr("AAAA0001"), Fpr("CCCC0003")}));

  for (int i = 2; i <= 7; ++i) selection.addRecipient(missing(i));
  EXPECT_EQ(selection.RecipientCount(), 9);
  // Seven with problems: all of them, and no room left for the others.
  EXPECT_EQ(names().size(), 7);
  EXPECT_FALSE(names().contains(Fpr("AAAA0001")));

  // What it says follows each change.
  for (int i = 1; i <= 7; ++i) selection.removeRecipient(missing(i));
  EXPECT_EQ(selection.RecipientCount(), 2);
  EXPECT_EQ(names(), (QStringList{Fpr("AAAA0001"), Fpr("CCCC0003")}));
  EXPECT_EQ(selection.IssueCount(), 0);
}

TEST(WorkSelectionTest, SigningNeedsAUsableKeyOfYourOwn) {
  SignerReset reset;
  Fixture f;
  WorkSelection selection(&f.roles);
  selection.SetKeysReady();
  EXPECT_EQ(selection.SignBlock(), "no_signer");
  // alice is the one usable signing key of your own: offered, not used.
  EXPECT_EQ(selection.SuggestedSigner(), Fpr("AAAA0001"));
  EXPECT_TRUE(selection.PreferredSigner().isEmpty());

  selection.addRecipient(Fpr("CCCC0003"));
  selection.SetSigningEnabled(true);
  EXPECT_EQ(selection.EncryptBlock(), "no_signer");
  selection.setPreferredSigner(Fpr("CCCC0003"));  // not your own
  EXPECT_EQ(selection.SignerKey().value("issue").toString(), "no_secret");
  EXPECT_EQ(selection.EncryptBlock(), "signer_issue");
  EXPECT_EQ(selection.SignBlock(), "signer_issue");
  EXPECT_EQ(selection.IssueCount(), 1);

  selection.setPreferredSigner(Fpr("AAAA0001"));
  EXPECT_TRUE(selection.SignBlock().isEmpty());
  EXPECT_TRUE(selection.EncryptBlock().isEmpty());
  EXPECT_TRUE(selection.SignsEncryption());
  EXPECT_TRUE(selection.SuggestedSigner().isEmpty());

  // A passphrase encrypts alone: not signed, and not held up by the signer.
  selection.SetPassphraseMode(true);
  EXPECT_FALSE(selection.SignsEncryption());
  selection.setPreferredSigner(Fpr("CCCC0003"));
  EXPECT_TRUE(selection.EncryptBlock().isEmpty());
}

TEST(WorkSelectionTest, ARememberedSignerNeverTurnsSigningOn) {
  SignerReset reset;
  Fixture f;
  {
    WorkSelection selection(&f.roles);
    selection.SetKeysReady();
    selection.setPreferredSigner(Fpr("AAAA0001"));
    selection.SetSigningEnabled(true);
  }
  // The next session: the key is remembered, signing is not.
  WorkSelection next(&f.roles);
  next.SetKeysReady();
  EXPECT_EQ(next.PreferredSigner(), Fpr("AAAA0001"));
  EXPECT_FALSE(next.SigningEnabled());
  EXPECT_FALSE(next.SignsEncryption());
  EXPECT_TRUE(next.SignBlock().isEmpty());
  // Reset keys keeps it too.
  next.reset();
  EXPECT_EQ(next.PreferredSigner(), Fpr("AAAA0001"));
}

// The quick path: the last operation and its keys, kept between sessions,
// done again with the same keys; forgotten once the keys are not the same.
TEST(WorkSelectionTest, TheLastOperationIsKeptUntilTheKeysChange) {
  SignerReset reset;
  Fixture f;
  {
    WorkSelection selection(&f.roles);
    selection.SetKeysReady();
    selection.forgetLast();
    EXPECT_TRUE(selection.LastOperation().isEmpty());
    selection.addRecipient(Fpr("CCCC0003"));
    selection.SetSigningEnabled(true);
    selection.setPreferredSigner(Fpr("AAAA0001"));
    selection.rememberOperation("encrypt");

    const auto last = selection.LastOperation();
    EXPECT_EQ(last.value("op").toString(), "encrypt");
    EXPECT_EQ(last.value("recipientCount").toInt(), 1);
    EXPECT_TRUE(last.value("signs").toBool());
    EXPECT_EQ(last.value("signerKey").toMap().value("name").toString(),
              "alice");

    // Chosen again after the keys were reset.
    selection.reset();
    EXPECT_EQ(selection.restoreLast(), "encrypt");
    EXPECT_EQ(selection.Recipients(), QStringList{Fpr("CCCC0003")});
    EXPECT_TRUE(selection.SigningEnabled());
    EXPECT_FALSE(selection.PassphraseMode());
    EXPECT_TRUE(selection.EncryptBlock().isEmpty());
  }

  // The next session finds it, with the same keys.
  {
    WorkSelection next(&f.roles);
    next.SetKeysReady();
    EXPECT_EQ(next.LastOperation().value("op").toString(), "encrypt");
    // Read again, unchanged: kept.
    next.KeysRefreshed();
    EXPECT_FALSE(next.LastOperation().isEmpty());
  }

  // A key database that holds other keys (one imported, deleted, edited, or
  // another database): forgotten, here and in the settings.
  {
    GpgKeyTableModel other{0,
                           {Key("Carol", "carol@example.org", "CCCC0003"),
                            Key("alice", "alice@example.org", "AAAA0001",
                                [](FakeKey& k) { k.secret = true; })}};
    KeyRoleModel roles;
    roles.setSourceModel(&other);
    WorkSelection changed(&roles);
    changed.SetKeysReady();
    EXPECT_TRUE(changed.LastOperation().isEmpty());
    EXPECT_TRUE(changed.restoreLast().isEmpty());
  }
  WorkSelection after(&f.roles);
  after.SetKeysReady();
  EXPECT_TRUE(after.LastOperation().isEmpty());
}

TEST(WorkSelectionTest, APassphraseOrASignatureIsKeptToo) {
  SignerReset reset;
  Fixture f;
  WorkSelection selection(&f.roles);
  selection.SetKeysReady();
  selection.SetPassphraseMode(true);
  selection.rememberOperation("symmetric");
  selection.SetPassphraseMode(false);
  EXPECT_EQ(selection.restoreLast(), "symmetric");
  EXPECT_TRUE(selection.PassphraseMode());

  selection.setPreferredSigner(Fpr("AAAA0001"));
  selection.rememberOperation("sign");
  selection.setPreferredSigner({});
  // Recipients chosen since are not what signing is about: they stay.
  selection.addRecipient(Fpr("CCCC0003"));
  EXPECT_EQ(selection.restoreLast(), "sign");
  EXPECT_EQ(selection.PreferredSigner(), Fpr("AAAA0001"));
  EXPECT_EQ(selection.Recipients(), QStringList{Fpr("CCCC0003")});
  selection.forgetLast();
}

TEST(WorkSelectionTest, ASignerThatIsGoneIsReported) {
  SignerReset reset;
  Fixture f;
  WorkSelection selection(&f.roles);
  selection.SetKeysReady();
  selection.setPreferredSigner(QStringLiteral("FFFF").repeated(10));
  EXPECT_EQ(selection.SignerKey().value("issue").toString(), "missing");
  EXPECT_EQ(selection.SignBlock(), "signer_issue");
}

TEST(WorkSelectionTest, SeveralRecipientsAreConfirmedAsOnTheDesktop) {
  Fixture f;
  WorkSelection selection(&f.roles);
  selection.SetKeysReady();
  selection.addRecipient(Fpr("AAAA0001"));
  selection.addRecipient(Fpr("CCCC0003"));

  const auto set = [](bool on) {
    auto settings = GetSettings();
    SettingItems::kConfirmMultipleRecipients.Write(settings, on);
  };
  const auto before =
      SettingItems::kConfirmMultipleRecipients.Read(GetSettings());

  set(false);
  EXPECT_TRUE(selection.encryptQuestions().isEmpty());

  // The desktop's question, in its words (Shell::EncryptQuestions).
  set(true);
  const auto questions = selection.encryptQuestions();
  ASSERT_EQ(questions.size(), 1);
  const auto question = questions.first().toMap();
  EXPECT_EQ(question.value("title").toString(), "Confirm Recipients");
  EXPECT_EQ(question.value("accept").toString(), "Encrypt");

  // One recipient, or a passphrase, is never asked about.
  selection.removeRecipient(Fpr("CCCC0003"));
  EXPECT_TRUE(selection.encryptQuestions().isEmpty());
  selection.addRecipient(Fpr("CCCC0003"));
  selection.SetPassphraseMode(true);
  EXPECT_TRUE(selection.encryptQuestions().isEmpty());

  set(before);
}

}  // namespace GpgFrontend::Test::Mobile
