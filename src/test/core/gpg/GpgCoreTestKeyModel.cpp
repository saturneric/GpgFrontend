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

#include "GpgCoreTest.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GpgData.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreInitTest) {
  auto& ctx = OpenPGPContext::GetInstance(kGpgFrontendDefaultChannel);
  ASSERT_TRUE(ctx.Good());
}

TEST_F(GpgCoreTest, GpgDataTest) {
  auto data_buff = QString(
      "cqEh8fyKWtmiXrW2zzlszJVGJrpXDDpzgP7ZELGxhfZYFi8rMrSVKDwrpFZBSWMG");

  GpgData data(data_buff.data(), data_buff.size());

  auto out_buffer = data.Read2GFBuffer();
  ASSERT_EQ(out_buffer.Size(), 64);
}

TEST_F(GpgCoreTest, GpgKeyTest) {
  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key.IsGood());
  ASSERT_TRUE(key.IsPrivateKey());
  ASSERT_TRUE(key.IsHasMasterKey());

  ASSERT_FALSE(key.IsDisabled());
  ASSERT_FALSE(key.IsRevoked());

  ASSERT_EQ(key.Protocol(), "OpenPGP");

  ASSERT_EQ(key.SubKeys().size(), 2);
  ASSERT_EQ(key.UIDs().size(), 1);

  ASSERT_FALSE(key.IsHasCertCap());
  ASSERT_FALSE(key.IsHasEncrCap());
  ASSERT_FALSE(key.IsHasSignCap());
  ASSERT_FALSE(key.IsHasAuthCap());
  ASSERT_FALSE(key.IsHasActualCertCap());
  ASSERT_FALSE(key.IsHasActualEncrCap());
  ASSERT_FALSE(key.IsHasActualSignCap());
  ASSERT_FALSE(key.IsHasActualAuthCap());

  ASSERT_EQ(key.Name(), "GpgFrontendTest");
  ASSERT_TRUE(key.Comment().isEmpty());
  ASSERT_EQ(key.Email(), "gpgfrontend@gpgfrontend.pub");
  ASSERT_EQ(key.ID(), "81704859182661FB");
  ASSERT_EQ(key.Fingerprint(), "9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_EQ(key.ExpirationTime(),
            QDateTime::fromString("2023-09-05T04:00:00Z", Qt::ISODate));
  ASSERT_EQ(key.PublicKeyAlgo(), "RSA");
  ASSERT_EQ(key.Algo(), "RSA3072");
  ASSERT_EQ(key.PrimaryKeyLength(), 3072);
  ASSERT_EQ(key.LastUpdateTime(),
            QDateTime::fromString("1970-01-01T00:00:00Z", Qt::ISODate));
  ASSERT_EQ(key.CreationTime(),
            QDateTime::fromString("2021-09-05T06:01:53Z", Qt::ISODate));

  ASSERT_EQ(key.OwnerTrust(), "Unknown");
  ASSERT_EQ(key.IsExpired(),
            key.ExpirationTime() < QDateTime::currentDateTime());
}

TEST_F(GpgCoreTest, GpgSubKeyTest) {
  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto s_keys = key.SubKeys();
  ASSERT_EQ(s_keys.size(), 2);

  auto& p_key = s_keys.front();

  ASSERT_EQ(p_key.ID(), "81704859182661FB");
  ASSERT_EQ(p_key.Fingerprint(), "9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_EQ(p_key.ExpirationTime(),
            QDateTime::fromString("2023-09-05T04:00:00Z", Qt::ISODate));
  ASSERT_EQ(p_key.PublicKeyAlgo(), "RSA");
  ASSERT_EQ(p_key.Algo(), "RSA3072");
  ASSERT_EQ(p_key.KeyLength(), 3072);
  ASSERT_EQ(p_key.CreationTime(),
            QDateTime::fromString("2021-09-05T06:01:53Z", Qt::ISODate));
  ASSERT_FALSE(p_key.IsCardKey());

  auto& s_key = s_keys.back();

  ASSERT_FALSE(s_key.IsRevoked());
  ASSERT_FALSE(s_key.IsDisabled());
  ASSERT_EQ(s_key.CreationTime(),
            QDateTime::fromString("2021-09-05T06:01:53Z", Qt::ISODate));

  ASSERT_FALSE(s_key.IsCardKey());
  ASSERT_TRUE(s_key.IsPrivateKey());
  ASSERT_EQ(s_key.ID(), "2B36803235B5E25B");
  ASSERT_EQ(s_key.Fingerprint(), "50D37E8F8EE7340A6794E0592B36803235B5E25B");
  ASSERT_EQ(s_key.KeyLength(), 3072);
  ASSERT_EQ(s_key.Algo(), "RSA3072");
  ASSERT_EQ(s_key.PublicKeyAlgo(), "RSA");
  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_FALSE(s_key.IsHasSignCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_EQ(s_key.ExpirationTime(),
            QDateTime::fromString("2023-09-05T04:00:00Z", Qt::ISODate));

  ASSERT_EQ(s_key.IsExpired(),
            s_key.ExpirationTime() < QDateTime::currentDateTime());
}

// The keygrip is how a single subkey is addressed in gpg-agent, which is the
// only way GnuPG can change one subkey's passphrase on its own. It only arrives
// if the context sets GPGME_KEYLIST_MODE_WITH_KEYGRIP.
TEST_F(GpgCoreTest, GpgSubKeyKeygripTest) {
  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto s_keys = key.SubKeys();
  ASSERT_EQ(s_keys.size(), 2);

  QSet<QString> keygrips;
  for (const auto& s_key : s_keys) {
    auto keygrip = s_key.Keygrip();
    EXPECT_FALSE(keygrip.isEmpty())
        << "no keygrip for subkey " << s_key.Fingerprint().toStdString();
    keygrips.insert(keygrip);
  }

  // Each key has its own agent-side secret, so the keygrips must differ --
  // otherwise a per-subkey PASSWD would hit the wrong key.
  EXPECT_EQ(keygrips.size(), s_keys.size());
}

TEST_F(GpgCoreTest, GpgUIDTest) {
  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto uids = key.UIDs();
  ASSERT_EQ(uids.size(), 1);
  auto& uid = uids.front();

  ASSERT_EQ(uid.GetName(), "GpgFrontendTest");
  ASSERT_TRUE(uid.GetComment().isEmpty());
  ASSERT_EQ(uid.GetEmail(), "gpgfrontend@gpgfrontend.pub");
  ASSERT_EQ(uid.GetUID(), "GpgFrontendTest <gpgfrontend@gpgfrontend.pub>");
  ASSERT_FALSE(uid.GetInvalid());
  ASSERT_FALSE(uid.GetRevoked());
}

TEST_F(GpgCoreTest, GpgKeySignatureTest) {
  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto uids = key.UIDs();
  ASSERT_EQ(uids.size(), 1);
  auto& uid = uids.front();

  auto signatures = uid.GetSignatures();
  ASSERT_EQ(signatures->size(), 1);
  auto& signature = signatures->front();

  ASSERT_EQ(signature.GetName(), "GpgFrontendTest");
  ASSERT_TRUE(signature.GetComment().isEmpty());
  ASSERT_EQ(signature.GetEmail(), "gpgfrontend@gpgfrontend.pub");
  ASSERT_EQ(signature.GetKeyID(), "81704859182661FB");
  ASSERT_EQ(signature.GetPubkeyAlgo(), "RSA");

  ASSERT_FALSE(signature.IsRevoked());
  ASSERT_FALSE(signature.IsInvalid());
  ASSERT_EQ(CheckGpgError(signature.GetStatus()), GPG_ERR_NO_ERROR);
  ASSERT_EQ(signature.GetUID(),
            "GpgFrontendTest <gpgfrontend@gpgfrontend.pub>");
}

TEST_F(GpgCoreTest, GpgKeyGetterTest) {
  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr("9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key != nullptr);
  auto keys = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel).Fetch();

  EXPECT_GT(keys.size(), 0);
  ASSERT_TRUE(std::find(keys.begin(), keys.end(), key) != keys.end());
}

// GPGME exposes no key version, so it is derived: a 40-hex (SHA-1)
// fingerprint can only belong to a v4 key, which settles it without exporting
// or parsing anything. Every fixture key is v4.
TEST_F(GpgCoreTest, GpgKeyVersionIsDerivedFromTheFingerprint) {
  auto& repo = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel);

  auto key = repo.GetKeyPtr("9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key != nullptr);
  ASSERT_EQ(key->Fingerprint().size(), 40);

  // the model itself still cannot answer -- this is the gap being filled
  EXPECT_EQ(key->KeyVersion(), 0);
  EXPECT_EQ(repo.GetKeyVersion(key->Fingerprint()), 4);

  // second call comes from the memo and must agree
  EXPECT_EQ(repo.GetKeyVersion(key->Fingerprint()), 4);
}

TEST_F(GpgCoreTest, GpgKeyVersionOfEveryKeyInTheKeyringIsKnown) {
  auto& repo = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel);

  auto keys = repo.Fetch();
  ASSERT_GT(keys.size(), 0);

  for (const auto& key : keys) {
    EXPECT_EQ(repo.GetKeyVersion(key->Fingerprint()), 4)
        << "key " << key->ID().toStdString() << " reported no version";
  }
}

// A fingerprint that belongs to no key must not be guessed at from its length
// alone -- but a 40-hex one still can be, because only v4 uses SHA-1.
TEST_F(GpgCoreTest, GpgKeyVersionOfAMalformedFingerprintIsUnknown) {
  auto& repo = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel);

  EXPECT_EQ(repo.GetKeyVersion(""), 0);
  EXPECT_EQ(repo.GetKeyVersion("81704859182661FB"), 0);  // key id, not a fpr
  EXPECT_EQ(repo.GetKeyVersion(QString(64, 'A')), 0);    // v5/v6 length, no key
}

TEST_F(GpgCoreTest, GpgKeyTableModelCheckedKeyIdsRoundTrip) {
  auto model = AbstractKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetGpgKeyTableModel();
  ASSERT_TRUE(model != nullptr);

  const auto rows = model->rowCount({});
  ASSERT_GT(rows, 0);
  ASSERT_TRUE(model->GetCheckedKeyIds().isEmpty());

  const auto id = model->GetAllKeys().front()->ID();
  model->SetCheckedKeyIds({id});

  ASSERT_EQ(model->GetCheckedKeyIds(), QStringList{id});
  ASSERT_EQ(model->data(model->index(0, 0, {}), Qt::CheckStateRole).toInt(),
            Qt::Checked);
}

TEST_F(GpgCoreTest, GpgKeyTableModelCheckedKeyIdsReplacesAndIgnoresUnknown) {
  auto model = AbstractKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetGpgKeyTableModel();
  ASSERT_TRUE(model != nullptr);
  ASSERT_GT(model->rowCount({}), 0);

  const auto id = model->GetAllKeys().front()->ID();
  model->SetCheckedKeyIds({id});
  ASSERT_EQ(model->GetCheckedKeyIds().size(), 1);

  // A later call defines the whole checked set, and ids that are not in the
  // model (a key deleted between sessions) are ignored rather than reported.
  model->SetCheckedKeyIds({"NOTAREALKEYID000"});
  ASSERT_TRUE(model->GetCheckedKeyIds().isEmpty());
  ASSERT_EQ(model->data(model->index(0, 0, {}), Qt::CheckStateRole).toInt(),
            Qt::Unchecked);
}

// The Expire Date column was inserted between Create Date and Algorithm. This
// guards both the header order and the renumbering of every column after it.
TEST_F(GpgCoreTest, GpgKeyTableModelExpireColumnLayout) {
  auto model = AbstractKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetGpgKeyTableModel();
  ASSERT_TRUE(model != nullptr);

  ASSERT_EQ(model->columnCount({}), 12);

  auto header = [&](int col) {
    return model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
  };
  EXPECT_EQ(header(7), QObject::tr("Create Date"));
  EXPECT_EQ(header(8), QObject::tr("Expire Date"));
  EXPECT_EQ(header(9), QObject::tr("Algorithm"));
  EXPECT_EQ(header(10), QObject::tr("Subkey(s)"));
  EXPECT_EQ(header(11), QObject::tr("Comment"));
}

// A key that carries an actual expiry renders the formatted date, and the
// columns after the insertion point still return the right values.
TEST_F(GpgCoreTest, GpgKeyTableModelExpireColumnData) {
  auto model = AbstractKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetGpgKeyTableModel();
  ASSERT_TRUE(model != nullptr);

  const auto rows = model->rowCount({});
  int target_row = -1;
  for (int r = 0; r < rows; ++r) {
    if (model->data(model->index(r, 6, {}), Qt::DisplayRole).toString() ==
        "81704859182661FB") {
      target_row = r;
      break;
    }
  }
  ASSERT_GE(target_row, 0);

  auto cell = [&](int col) {
    return model->data(model->index(target_row, col, {}), Qt::DisplayRole)
        .toString();
  };

  const auto expected = QLocale().toString(
      QDateTime::fromString("2023-09-05T04:00:00Z", Qt::ISODate), "yyyy-MM-dd");
  EXPECT_EQ(cell(8), expected);
  EXPECT_NE(cell(8), QObject::tr("Never"));
  // Renumbering guard: Algorithm and Comment must still land where expected.
  EXPECT_EQ(cell(9), "RSA3072");
  EXPECT_TRUE(cell(11).isEmpty());
}

}  // namespace GpgFrontend::Test