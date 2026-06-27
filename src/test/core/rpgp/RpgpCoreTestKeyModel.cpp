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

#include "RpgpCoreTest.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

TEST_F(RpgpCoreTest, CoreInitTest) {
  auto& ctx = OpenPGPContext::GetInstance(kRpgpChannelForUnitTest);
  ASSERT_TRUE(ctx.Good());
}

TEST_F(RpgpCoreTest, GpgKeyTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKey("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key.IsGood());
  ASSERT_TRUE(key.IsPrivateKey());
  ASSERT_TRUE(key.IsHasMasterKey());

  ASSERT_FALSE(key.IsDisabled());
  ASSERT_FALSE(key.IsRevoked());

  ASSERT_TRUE(key.Protocol().isEmpty());

  ASSERT_EQ(key.SubKeys().size(), 5);
  ASSERT_EQ(key.UIDs().size(), 2);

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());

  ASSERT_EQ(key.Name(), "uuuuuu");
  ASSERT_EQ(key.Comment(), "uuuuu");
  ASSERT_EQ(key.Email(), "uuuuu@uuu.uuu");
  ASSERT_EQ(key.ID(), "BDB8BB6BDDFA8497");
  ASSERT_EQ(key.Fingerprint(), "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_EQ(key.PublicKeyAlgo(), "ED25519");
  ASSERT_EQ(key.Algo(), "ED25519");
  ASSERT_EQ(key.PrimaryKeyLength(), 255);
  ASSERT_EQ(key.CreationTime(),
            QDateTime::fromSecsSinceEpoch(1782481927));

  ASSERT_EQ(key.OwnerTrust(), "Ultimate");
  ASSERT_FALSE(key.IsExpired());
}

TEST_F(RpgpCoreTest, GpgSubKeyTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKey("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  auto s_keys = key.SubKeys();
  ASSERT_EQ(s_keys.size(), 5);

  auto& p_key = s_keys.front();

  ASSERT_EQ(p_key.ID(), "BDB8BB6BDDFA8497");
  ASSERT_EQ(p_key.Fingerprint(), "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_EQ(p_key.PublicKeyAlgo(), "ED25519");
  ASSERT_EQ(p_key.Algo(), "ED25519");
  ASSERT_EQ(p_key.KeyLength(), 255);
  ASSERT_EQ(p_key.CreationTime(),
            QDateTime::fromSecsSinceEpoch(1782481927));
  ASSERT_FALSE(p_key.IsCardKey());

  auto& s_key = s_keys.back();

  ASSERT_FALSE(s_key.IsRevoked());
  ASSERT_FALSE(s_key.IsDisabled());
  ASSERT_FALSE(s_key.IsCardKey());
  ASSERT_TRUE(s_key.IsPrivateKey());
  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_FALSE(s_key.IsHasSignCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());

  ASSERT_FALSE(s_key.IsExpired());
}

TEST_F(RpgpCoreTest, GpgUIDTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKey("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  auto uids = key.UIDs();
  ASSERT_EQ(uids.size(), 2);

  auto& uid0 = uids.front();
  ASSERT_EQ(uid0.GetName(), "uuuuuu");
  ASSERT_EQ(uid0.GetComment(), "uuuuu");
  ASSERT_EQ(uid0.GetEmail(), "uuuuu@uuu.uuu");
  ASSERT_EQ(uid0.GetUID(), "uuuuuu (uuuuu) <uuuuu@uuu.uuu>");
  ASSERT_FALSE(uid0.GetInvalid());
  ASSERT_FALSE(uid0.GetRevoked());

  auto& uid1 = uids.back();
  ASSERT_EQ(uid1.GetName(), "gggggg");
  ASSERT_EQ(uid1.GetComment(), "ggggg");
  ASSERT_EQ(uid1.GetEmail(), "ggggg@ggg.gggggg");
  ASSERT_EQ(uid1.GetUID(), "gggggg (ggggg) <ggggg@ggg.gggggg>");
  ASSERT_FALSE(uid1.GetInvalid());
  ASSERT_FALSE(uid1.GetRevoked());
}

TEST_F(RpgpCoreTest, GpgKeySignatureTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKey("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  auto uids = key.UIDs();
  ASSERT_EQ(uids.size(), 2);

  auto& uid = uids.front();
  auto signatures = uid.GetSignatures();
  if (signatures->size() > 0) {
    auto& signature = signatures->front();

    ASSERT_EQ(signature.GetName(), "uuuuuu");
    ASSERT_EQ(signature.GetComment(), "uuuuu");
    ASSERT_EQ(signature.GetEmail(), "uuuuu@uuu.uuu");

    ASSERT_FALSE(signature.IsRevoked());
    ASSERT_FALSE(signature.IsInvalid());
    ASSERT_EQ(CheckGpgError(signature.GetStatus()), GPG_ERR_NO_ERROR);
  }
}

TEST_F(RpgpCoreTest, GpgKeyGetterTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);
  auto keys =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).Fetch();

  EXPECT_GT(keys.size(), 0);
  ASSERT_TRUE(std::find(keys.begin(), keys.end(), key) != keys.end());
}

TEST_F(RpgpCoreTest, GpgPublicKeyTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKey("3BEDAB48EAAAA195006330414DD9733454846D0C");
  ASSERT_TRUE(key.IsGood());
  ASSERT_FALSE(key.IsPrivateKey());

  ASSERT_EQ(key.Name(), "kkkkkk");
  ASSERT_EQ(key.Comment(), "kkkkkk");
  ASSERT_EQ(key.Email(), "kkkkk@kkk.kk");
  ASSERT_EQ(key.ID(), "4DD9733454846D0C");
  ASSERT_EQ(key.Fingerprint(), "3BEDAB48EAAAA195006330414DD9733454846D0C");
  ASSERT_EQ(key.PublicKeyAlgo(), "NIST P-384");
  ASSERT_EQ(key.PrimaryKeyLength(), 384);

  ASSERT_EQ(key.SubKeys().size(), 2);
  ASSERT_EQ(key.UIDs().size(), 1);
}

TEST_F(RpgpCoreTest, GpgKey3Test) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKey("C54DF5E9E6AD3278C77F5438DA6A97C428EC96C8");
  ASSERT_TRUE(key.IsGood());
  ASSERT_FALSE(key.IsPrivateKey());

  ASSERT_EQ(key.Name(), "bbbbbb");
  ASSERT_EQ(key.Comment(), "bbbbbb");
  ASSERT_EQ(key.Email(), "bbbbbb@bbb.bbb");
  ASSERT_EQ(key.ID(), "DA6A97C428EC96C8");
  ASSERT_EQ(key.Fingerprint(), "C54DF5E9E6AD3278C77F5438DA6A97C428EC96C8");
  ASSERT_EQ(key.PublicKeyAlgo(), "ED25519");

  ASSERT_EQ(key.SubKeys().size(), 2);
  ASSERT_EQ(key.UIDs().size(), 1);
}

}  // namespace GpgFrontend::Test
