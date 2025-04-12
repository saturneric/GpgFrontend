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
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/GpgData.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreInitTest) {
  auto& ctx = GpgContext::GetInstance(kGpgFrontendDefaultChannel);
  auto& ctx_default = GpgContext::GetInstance();
  ASSERT_TRUE(ctx.Good());
  ASSERT_TRUE(ctx_default.Good());
}

TEST_F(GpgCoreTest, GpgDataTest) {
  auto data_buff = QString(
      "cqEh8fyKWtmiXrW2zzlszJVGJrpXDDpzgP7ZELGxhfZYFi8rMrSVKDwrpFZBSWMG");

  GpgData data(data_buff.data(), data_buff.size());

  auto out_buffer = data.Read2GFBuffer();
  ASSERT_EQ(out_buffer.Size(), 64);
}

TEST_F(GpgCoreTest, GpgKeyTest) {
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key.IsGood());
  ASSERT_TRUE(key.IsPrivateKey());
  ASSERT_TRUE(key.IsHasMasterKey());

  ASSERT_FALSE(key.IsDisabled());
  ASSERT_FALSE(key.IsRevoked());

  ASSERT_EQ(key.Protocol(), "OpenPGP");

  ASSERT_EQ(key.SubKeys().size(), 2);
  ASSERT_EQ(key.UIDs().size(), 1);

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_FALSE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());
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
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
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

TEST_F(GpgCoreTest, GpgUIDTest) {
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
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
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
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
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key.IsGood());
  auto keys = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).FetchKey();

  EXPECT_GT(keys.size(), 0);
  ASSERT_TRUE(std::find(keys.begin(), keys.end(), key) != keys.end());
}

}  // namespace GpgFrontend::Test