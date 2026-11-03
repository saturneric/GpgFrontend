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
#include "core/model/GpgKeyTableModel.h"
#include "core/utils/GpgUtils.h"
#include "model/KeyFilterModel.h"
#include "model/KeyRoleModel.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::KeyFilterModel;
using GpgFrontend::Mobile::KeyRoleModel;

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

}  // namespace GpgFrontend::Test::Mobile
