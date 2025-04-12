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

#include "GpgCoreTest.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, GenerateSubkeyRSA2048Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo("rsa2048");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "rsa2048");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateSubkeySync(p_key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();

  ASSERT_EQ(s_key.PublicKeyAlgo(), "RSA");
  ASSERT_EQ(s_key.Algo(), "RSA2048");
  ASSERT_EQ(s_key.KeyLength(), 2048);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_TRUE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_TRUE(s_key.IsHasSignCap());
}

TEST_F(GpgCoreTest, GenerateSubkeyDSA2048Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo("dsa2048");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "dsa2048");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateSubkeySync(p_key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();

  ASSERT_EQ(s_key.PublicKeyAlgo(), "DSA");
  ASSERT_EQ(s_key.Algo(), "DSA2048");
  ASSERT_EQ(s_key.KeyLength(), 2048);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_TRUE(s_key.IsHasAuthCap());
  ASSERT_FALSE(s_key.IsHasEncrCap());
  ASSERT_TRUE(s_key.IsHasSignCap());
}

TEST_F(GpgCoreTest, GenerateSubkeyELG2048Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo("elg2048");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "elg2048");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateSubkeySync(p_key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();

  ASSERT_EQ(s_key.PublicKeyAlgo(), "ELG-E");
  ASSERT_EQ(s_key.Algo(), "ELG2048");
  ASSERT_EQ(s_key.KeyLength(), 2048);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());
}

TEST_F(GpgCoreTest, GenerateSubkeyED25519Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo("ed25519");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "ed25519");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateSubkeySync(p_key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();

  ASSERT_EQ(s_key.PublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(s_key.Algo(), "ED25519");
  ASSERT_EQ(s_key.KeyLength(), 255);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_TRUE(s_key.IsHasAuthCap());
  ASSERT_FALSE(s_key.IsHasEncrCap());
  ASSERT_TRUE(s_key.IsHasSignCap());
}

TEST_F(GpgCoreTest, GenerateSubkeyCV25519Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo("cv25519");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "cv25519");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateSubkeySync(p_key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();

  ASSERT_EQ(s_key.PublicKeyAlgo(), QString("ECDH"));
  ASSERT_EQ(s_key.Algo(), QString("CV25519"));
  ASSERT_EQ(s_key.KeyLength(), 255);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());
}

TEST_F(GpgCoreTest, GenerateSubkeyNISTP256Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo("nistp256");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "nistp256");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateSubkeySync(p_key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();

  ASSERT_EQ(s_key.PublicKeyAlgo(), QString("ECDH"));
  ASSERT_EQ(s_key.Algo(), QString("NISTP256"));
  ASSERT_EQ(s_key.KeyLength(), 256);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());
}

TEST_F(GpgCoreTest, GenerateSubkeyBRAINPOOLP256R1Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo("brainpoolp256r1");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "brainpoolp256r1");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateSubkeySync(p_key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();

  ASSERT_EQ(s_key.PublicKeyAlgo(), QString("ECDH"));
  ASSERT_EQ(s_key.Algo(), QString("BRAINPOOLP256R1"));
  ASSERT_EQ(s_key.KeyLength(), 256);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());
}

TEST_F(GpgCoreTest, GenerateSubkeyX448Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo("x448");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "x448");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateSubkeySync(p_key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();

  ASSERT_EQ(s_key.PublicKeyAlgo(), QString("ECDH"));
  ASSERT_EQ(s_key.Algo(), QString("CV448"));
  ASSERT_EQ(s_key.KeyLength(), 448);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());
}

TEST_F(GpgCoreTest, GenerateSubkeySECP256K1Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo("secp256k1");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "secp256k1");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateSubkeySync(p_key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();

  ASSERT_EQ(s_key.PublicKeyAlgo(), QString("ECDH"));
  ASSERT_EQ(s_key.Algo(), QString("SECP256K1"));
  ASSERT_EQ(s_key.KeyLength(), 256);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());
}

}  // namespace GpgFrontend::Test