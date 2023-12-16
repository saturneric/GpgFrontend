/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
  auto data_buff = std::string(
      "cqEh8fyKWtmiXrW2zzlszJVGJrpXDDpzgP7ZELGxhfZYFi8rMrSVKDwrpFZBSWMG");

  GpgData data(data_buff.data(), data_buff.size());

  auto out_buffer = data.Read2Buffer();
  ASSERT_EQ(out_buffer->size(), 64);
}

TEST_F(GpgCoreTest, GpgKeyTest) {
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_TRUE(key.IsGood());
  ASSERT_TRUE(key.IsPrivateKey());
  ASSERT_TRUE(key.IsHasMasterKey());

  ASSERT_FALSE(key.IsDisabled());
  ASSERT_FALSE(key.IsRevoked());

  ASSERT_EQ(key.GetProtocol(), "OpenPGP");

  ASSERT_EQ(key.GetSubKeys()->size(), 2);
  ASSERT_EQ(key.GetUIDs()->size(), 1);

  ASSERT_TRUE(key.IsHasCertificationCapability());
  ASSERT_FALSE(key.IsHasEncryptionCapability());
  ASSERT_TRUE(key.IsHasSigningCapability());
  ASSERT_FALSE(key.IsHasAuthenticationCapability());
  ASSERT_FALSE(key.IsHasActualCertificationCapability());
  ASSERT_FALSE(key.IsHasActualEncryptionCapability());
  ASSERT_FALSE(key.IsHasActualSigningCapability());
  ASSERT_FALSE(key.IsHasActualAuthenticationCapability());

  ASSERT_EQ(key.GetName(), "GpgFrontendTest");
  ASSERT_TRUE(key.GetComment().empty());
  ASSERT_EQ(key.GetEmail(), "gpgfrontend@gpgfrontend.pub");
  ASSERT_EQ(key.GetId(), "81704859182661FB");
  ASSERT_EQ(key.GetFingerprint(), "9490795B78F8AFE9F93BD09281704859182661FB");
  ASSERT_EQ(key.GetExpireTime(),
            boost::posix_time::from_iso_string("20230905T040000"));
  ASSERT_EQ(key.GetPublicKeyAlgo(), "RSA");
  ASSERT_EQ(key.GetPrimaryKeyLength(), 3072);
  ASSERT_EQ(key.GetLastUpdateTime(),
            boost::posix_time::from_iso_string("19700101T000000"));
  ASSERT_EQ(key.GetCreateTime(),
            boost::posix_time::from_iso_string("20210905T060153"));

  ASSERT_EQ(key.GetOwnerTrust(), "Unknown");

  ASSERT_EQ(key.IsExpired(), key.GetExpireTime() <
                                 boost::posix_time::second_clock::local_time());
}

TEST_F(GpgCoreTest, GpgSubKeyTest) {
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto sub_keys = key.GetSubKeys();
  ASSERT_EQ(sub_keys->size(), 2);

  auto& sub_key = sub_keys->back();

  ASSERT_FALSE(sub_key.IsRevoked());
  ASSERT_FALSE(sub_key.IsDisabled());
  ASSERT_EQ(sub_key.GetCreateTime(),
            boost::posix_time::from_iso_string("20210905T060153"));

  ASSERT_FALSE(sub_key.IsCardKey());
  ASSERT_TRUE(sub_key.IsPrivateKey());
  ASSERT_EQ(sub_key.GetID(), "2B36803235B5E25B");
  ASSERT_EQ(sub_key.GetFingerprint(),
            "50D37E8F8EE7340A6794E0592B36803235B5E25B");
  ASSERT_EQ(sub_key.GetKeyLength(), 3072);
  ASSERT_EQ(sub_key.GetPubkeyAlgo(), "RSA");
  ASSERT_FALSE(sub_key.IsHasCertificationCapability());
  ASSERT_FALSE(sub_key.IsHasAuthenticationCapability());
  ASSERT_FALSE(sub_key.IsHasSigningCapability());
  ASSERT_TRUE(sub_key.IsHasEncryptionCapability());
  ASSERT_EQ(key.GetExpireTime(),
            boost::posix_time::from_iso_string("20230905T040000"));

  ASSERT_EQ(
      sub_key.IsExpired(),
      sub_key.GetExpireTime() < boost::posix_time::second_clock::local_time());
}

TEST_F(GpgCoreTest, GpgUIDTest) {
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto uids = key.GetUIDs();
  ASSERT_EQ(uids->size(), 1);
  auto& uid = uids->front();

  ASSERT_EQ(uid.GetName(), "GpgFrontendTest");
  ASSERT_TRUE(uid.GetComment().empty());
  ASSERT_EQ(uid.GetEmail(), "gpgfrontend@gpgfrontend.pub");
  ASSERT_EQ(uid.GetUID(), "GpgFrontendTest <gpgfrontend@gpgfrontend.pub>");
  ASSERT_FALSE(uid.GetInvalid());
  ASSERT_FALSE(uid.GetRevoked());
}

TEST_F(GpgCoreTest, GpgKeySignatureTest) {
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey("9490795B78F8AFE9F93BD09281704859182661FB");
  auto uids = key.GetUIDs();
  ASSERT_EQ(uids->size(), 1);
  auto& uid = uids->front();

  auto signatures = uid.GetSignatures();
  ASSERT_EQ(signatures->size(), 1);
  auto& signature = signatures->front();

  ASSERT_EQ(signature.GetName(), "GpgFrontendTest");
  ASSERT_TRUE(signature.GetComment().empty());
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

  EXPECT_GT(keys->size(), 0);
  ASSERT_TRUE(find(keys->begin(), keys->end(), key) != keys->end());
}

}  // namespace GpgFrontend::Test