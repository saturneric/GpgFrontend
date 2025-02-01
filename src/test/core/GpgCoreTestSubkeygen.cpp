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
#include "core/model/GpgGenKeyInfo.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, GenerateSubkeyRSA2048Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<GenKeyInfo>::create(true);

  auto [found, algo] = GenKeyInfo::SearchSubKeyAlgo("rsa2048");
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

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();

  ASSERT_EQ(subkey.GetPubkeyAlgo(), "RSA");
  ASSERT_EQ(subkey.GetKeyAlgo(), "RSA2048");
  ASSERT_EQ(subkey.GetKeyLength(), 2048);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_TRUE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_TRUE(subkey.IsHasSigningCapability());
}

TEST_F(GpgCoreTest, GenerateSubkeyDSA2048Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<GenKeyInfo>::create(true);

  auto [found, algo] = GenKeyInfo::SearchSubKeyAlgo("dsa2048");
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

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();

  ASSERT_EQ(subkey.GetPubkeyAlgo(), "DSA");
  ASSERT_EQ(subkey.GetKeyAlgo(), "DSA2048");
  ASSERT_EQ(subkey.GetKeyLength(), 2048);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_TRUE(subkey.IsHasAuthenticationCapability());
  ASSERT_FALSE(subkey.IsHasEncryptionCapability());
  ASSERT_TRUE(subkey.IsHasSigningCapability());
}

TEST_F(GpgCoreTest, GenerateSubkeyELG2048Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<GenKeyInfo>::create(true);

  auto [found, algo] = GenKeyInfo::SearchSubKeyAlgo("elg2048");
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

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();

  ASSERT_EQ(subkey.GetPubkeyAlgo(), "ELG-E");
  ASSERT_EQ(subkey.GetKeyAlgo(), "ELG2048");
  ASSERT_EQ(subkey.GetKeyLength(), 2048);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_FALSE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_FALSE(subkey.IsHasSigningCapability());
}

TEST_F(GpgCoreTest, GenerateSubkeyED25519Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<GenKeyInfo>::create(true);

  auto [found, algo] = GenKeyInfo::SearchSubKeyAlgo("ed25519");
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

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();

  ASSERT_EQ(subkey.GetPubkeyAlgo(), "EdDSA");
  ASSERT_EQ(subkey.GetKeyAlgo(), "ED25519");
  ASSERT_EQ(subkey.GetKeyLength(), 255);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_TRUE(subkey.IsHasAuthenticationCapability());
  ASSERT_FALSE(subkey.IsHasEncryptionCapability());
  ASSERT_TRUE(subkey.IsHasSigningCapability());
}

TEST_F(GpgCoreTest, GenerateSubkeyCV25519Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<GenKeyInfo>::create(true);

  auto [found, algo] = GenKeyInfo::SearchSubKeyAlgo("cv25519");
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

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();

  ASSERT_EQ(subkey.GetPubkeyAlgo(), QString("ECDH"));
  ASSERT_EQ(subkey.GetKeyAlgo(), QString("CV25519"));
  ASSERT_EQ(subkey.GetKeyLength(), 255);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_FALSE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_FALSE(subkey.IsHasSigningCapability());
}

TEST_F(GpgCoreTest, GenerateSubkeyNISTP256Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<GenKeyInfo>::create(true);

  auto [found, algo] = GenKeyInfo::SearchSubKeyAlgo("nistp256");
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

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();

  ASSERT_EQ(subkey.GetPubkeyAlgo(), QString("ECDH"));
  ASSERT_EQ(subkey.GetKeyAlgo(), QString("NISTP256"));
  ASSERT_EQ(subkey.GetKeyLength(), 256);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_FALSE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_FALSE(subkey.IsHasSigningCapability());
}

TEST_F(GpgCoreTest, GenerateSubkeyBRAINPOOLP256R1Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<GenKeyInfo>::create(true);

  auto [found, algo] = GenKeyInfo::SearchSubKeyAlgo("brainpoolp256r1");
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

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();

  ASSERT_EQ(subkey.GetPubkeyAlgo(), QString("ECDH"));
  ASSERT_EQ(subkey.GetKeyAlgo(), QString("BRAINPOOLP256R1"));
  ASSERT_EQ(subkey.GetKeyLength(), 256);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_FALSE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_FALSE(subkey.IsHasSigningCapability());
}

TEST_F(GpgCoreTest, GenerateSubkeyX448Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<GenKeyInfo>::create(true);

  auto [found, algo] = GenKeyInfo::SearchSubKeyAlgo("x448");
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

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();

  ASSERT_EQ(subkey.GetPubkeyAlgo(), QString("ECDH"));
  ASSERT_EQ(subkey.GetKeyAlgo(), QString("CV448"));
  ASSERT_EQ(subkey.GetKeyLength(), 448);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_FALSE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_FALSE(subkey.IsHasSigningCapability());
}

TEST_F(GpgCoreTest, GenerateSubkeySECP256K1Test) {
  auto p_key = GpgKeyGetter::GetInstance().GetKey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(p_key.IsGood());

  auto s_info = QSharedPointer<GenKeyInfo>::create(true);

  auto [found, algo] = GenKeyInfo::SearchSubKeyAlgo("secp256k1");
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

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();

  ASSERT_EQ(subkey.GetPubkeyAlgo(), QString("ECDH"));
  ASSERT_EQ(subkey.GetKeyAlgo(), QString("SECP256K1"));
  ASSERT_EQ(subkey.GetKeyLength(), 256);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_FALSE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_FALSE(subkey.IsHasSigningCapability());
}

}  // namespace GpgFrontend::Test