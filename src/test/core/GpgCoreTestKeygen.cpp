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

#include "GpgCoreTest.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/model/GpgGenKeyInfo.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, GenerateKeyRSA2048Test) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_0");
  keygen_info->SetEmail("bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("foobar");
  keygen_info->SetKeyLength(2048);
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[0]));
  keygen_info->SetAllowAuthentication(true);
  keygen_info->SetAllowCertification(true);
  keygen_info->SetAllowEncryption(true);
  keygen_info->SetAllowSigning(true);
  keygen_info->SetNonExpired(true);
  keygen_info->SetNonPassPhrase(false);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateKeySync(keygen_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());

  ASSERT_EQ(key.GetName(), "foo_0");
  ASSERT_EQ(key.GetEmail(), "bar@gpgfrontend.bktus.com");
  ASSERT_EQ(key.GetComment(), "foobar");
  ASSERT_EQ(key.GetPublicKeyAlgo(), "RSA");
  ASSERT_EQ(key.GetOwnerTrustLevel(), 5);
  ASSERT_EQ(key.GetPrimaryKeyLength(), 2048);
  ASSERT_EQ(key.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_TRUE(key.IsHasCertificationCapability());
  ASSERT_TRUE(key.IsHasAuthenticationCapability());
  ASSERT_TRUE(key.IsHasEncryptionCapability());
  ASSERT_TRUE(key.IsHasSigningCapability());

  ASSERT_TRUE(key.IsHasActualCertificationCapability());
  ASSERT_TRUE(key.IsHasActualAuthenticationCapability());
  ASSERT_TRUE(key.IsHasActualEncryptionCapability());
  ASSERT_TRUE(key.IsHasActualSigningCapability());

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(result.GetFingerprint());
}

TEST_F(GpgCoreTest, GenerateKeyRSA1024NoPassTest) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_1");
  keygen_info->SetEmail("bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("foobar_1");
  keygen_info->SetKeyLength(2048);
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[0]));
  keygen_info->SetAllowAuthentication(false);
  keygen_info->SetAllowCertification(false);
  keygen_info->SetAllowEncryption(false);
  keygen_info->SetAllowSigning(false);
  keygen_info->SetNonExpired(false);
  keygen_info->SetNonPassPhrase(true);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateKeySync(keygen_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());
  ASSERT_EQ(key.GetName(), "foo_1");
  ASSERT_EQ(key.GetEmail(), "bar@gpgfrontend.bktus.com");
  ASSERT_EQ(key.GetComment(), "foobar_1");
  ASSERT_EQ(key.GetPublicKeyAlgo(), "RSA");
  ASSERT_EQ(key.GetOwnerTrustLevel(), 5);
  ASSERT_EQ(key.GetPrimaryKeyLength(), 2048);
  ASSERT_GT(key.GetExpireTime(), QDateTime::currentDateTime());

  ASSERT_TRUE(key.IsHasCertificationCapability());
  ASSERT_FALSE(key.IsHasAuthenticationCapability());
  ASSERT_FALSE(key.IsHasEncryptionCapability());
  ASSERT_FALSE(key.IsHasSigningCapability());

  ASSERT_TRUE(key.IsHasActualCertificationCapability());
  ASSERT_FALSE(key.IsHasActualAuthenticationCapability());
  ASSERT_FALSE(key.IsHasActualEncryptionCapability());
  ASSERT_FALSE(key.IsHasActualSigningCapability());

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(result.GetFingerprint());
}

TEST_F(GpgCoreTest, GenerateKeyRSA4096Test) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_1");
  keygen_info->SetEmail("bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("hello gpgfrontend");
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[0]));
  keygen_info->SetKeyLength(3072);
  keygen_info->SetNonExpired(false);

  auto expire_time =
      QDateTime::currentDateTime().addSecs(static_cast<qint64>(24 * 3600));
  keygen_info->SetExpireTime(expire_time);
  keygen_info->SetNonPassPhrase(false);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateKeySync(keygen_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());
  ASSERT_EQ(key.GetExpireTime().date(), expire_time.date());

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(result.GetFingerprint());
}

TEST_F(GpgCoreTest, GenerateKeyDSA2048Test) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_1");
  keygen_info->SetEmail("bar_1@gpgfrontend.bktus.com");
  keygen_info->SetComment("hello gpgfrontend");
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[1]));
  keygen_info->SetKeyLength(2048);
  keygen_info->SetAllowAuthentication(true);
  keygen_info->SetAllowCertification(true);
  keygen_info->SetAllowEncryption(true);
  keygen_info->SetAllowSigning(true);
  keygen_info->SetNonExpired(false);

  auto expire_time =
      QDateTime::currentDateTime().addSecs(static_cast<qint64>(24 * 3600));
  keygen_info->SetExpireTime(expire_time);
  keygen_info->SetNonPassPhrase(false);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateKeySync(keygen_info);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());
  ASSERT_EQ(key.GetName(), "foo_1");
  ASSERT_EQ(key.GetEmail(), "bar_1@gpgfrontend.bktus.com");
  ASSERT_EQ(key.GetComment(), "hello gpgfrontend");
  ASSERT_EQ(key.GetPublicKeyAlgo(), "DSA");
  ASSERT_EQ(key.GetOwnerTrustLevel(), 5);
  ASSERT_EQ(key.GetPrimaryKeyLength(), 2048);
  ASSERT_GT(key.GetExpireTime(), QDateTime::currentDateTime());

  ASSERT_TRUE(key.IsHasCertificationCapability());
  ASSERT_TRUE(key.IsHasAuthenticationCapability());
  ASSERT_FALSE(key.IsHasEncryptionCapability());
  ASSERT_TRUE(key.IsHasSigningCapability());

  ASSERT_TRUE(key.IsHasActualCertificationCapability());
  ASSERT_TRUE(key.IsHasActualAuthenticationCapability());
  ASSERT_FALSE(key.IsHasActualEncryptionCapability());
  ASSERT_TRUE(key.IsHasActualSigningCapability());

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(result.GetFingerprint());
}

TEST_F(GpgCoreTest, GenerateKeyED25519Test) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_4");
  keygen_info->SetEmail("bar_ed@gpgfrontend.bktus.com");
  keygen_info->SetComment("hello gpgfrontend");
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[2]));
  keygen_info->SetKeyLength(0);
  keygen_info->SetAllowAuthentication(true);
  keygen_info->SetAllowCertification(true);
  keygen_info->SetAllowEncryption(true);
  keygen_info->SetAllowSigning(true);
  keygen_info->SetNonExpired(false);

  auto expire_time =
      QDateTime::currentDateTime().addSecs(static_cast<qint64>(24 * 3600));
  keygen_info->SetExpireTime(expire_time);
  keygen_info->SetNonPassPhrase(false);

  auto [err, data_object] = GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
                                .GenerateKeySync(keygen_info);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKey(result.GetFingerprint());
  ASSERT_TRUE(key.IsGood());
  ASSERT_EQ(key.GetName(), "foo_4");
  ASSERT_EQ(key.GetEmail(), "bar_ed@gpgfrontend.bktus.com");
  ASSERT_EQ(key.GetComment(), "hello gpgfrontend");
  ASSERT_EQ(key.GetPublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(key.GetOwnerTrustLevel(), 5);
  ASSERT_EQ(key.GetPrimaryKeyLength(), 255);
  ASSERT_GT(key.GetExpireTime(), QDateTime::currentDateTime());

  ASSERT_TRUE(key.IsHasCertificationCapability());
  ASSERT_TRUE(key.IsHasAuthenticationCapability());
  ASSERT_FALSE(key.IsHasEncryptionCapability());
  ASSERT_TRUE(key.IsHasSigningCapability());

  ASSERT_TRUE(key.IsHasActualCertificationCapability());
  ASSERT_TRUE(key.IsHasActualAuthenticationCapability());
  ASSERT_FALSE(key.IsHasActualEncryptionCapability());
  ASSERT_TRUE(key.IsHasActualSigningCapability());

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(result.GetFingerprint());
}

TEST_F(GpgCoreTest, GenerateKeyED25519CV25519Test) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_ec");
  keygen_info->SetEmail("ec_bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("ecccc");
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[3]));
  keygen_info->SetAllowAuthentication(true);
  keygen_info->SetAllowCertification(true);
  keygen_info->SetAllowEncryption(true);
  keygen_info->SetAllowSigning(true);
  keygen_info->SetNonExpired(true);
  keygen_info->SetNonPassPhrase(true);

  auto subkeygen_info = SecureCreateSharedObject<GenKeyInfo>(true);
  subkeygen_info->SetAlgo(std::get<2>(keygen_info->GetSupportedKeyAlgo()[3]));
  subkeygen_info->SetAllowAuthentication(true);
  subkeygen_info->SetAllowCertification(true);
  subkeygen_info->SetAllowEncryption(true);
  subkeygen_info->SetAllowSigning(true);
  subkeygen_info->SetNonExpired(true);
  subkeygen_info->SetNonPassPhrase(true);

  auto [err, data_object] =
      GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeyWithSubkeySync(keygen_info, subkeygen_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgGenerateKeyResult, GpgGenerateKeyResult>()));
  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  auto fpr = result.GetFingerprint();
  ASSERT_FALSE(fpr.isEmpty());

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).GetKey(fpr);
  ASSERT_TRUE(key.IsGood());

  ASSERT_EQ(key.GetName(), "foo_ec");
  ASSERT_EQ(key.GetEmail(), "ec_bar@gpgfrontend.bktus.com");
  ASSERT_EQ(key.GetComment(), "ecccc");
  ASSERT_EQ(key.GetPublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(key.GetKeyAlgo(), "ED25519");
  ASSERT_EQ(key.GetOwnerTrustLevel(), 5);
  ASSERT_EQ(key.GetPrimaryKeyLength(), 255);
  ASSERT_EQ(key.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_TRUE(key.IsHasCertificationCapability());
  ASSERT_TRUE(key.IsHasAuthenticationCapability());
  ASSERT_TRUE(key.IsHasEncryptionCapability());
  ASSERT_TRUE(key.IsHasSigningCapability());

  ASSERT_FALSE(key.GetSubKeys()->empty());
  ASSERT_EQ(key.GetSubKeys()->size(), 2);

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();
  ASSERT_EQ(subkey.GetPubkeyAlgo(), "ECDH");
  ASSERT_EQ(subkey.GetKeyAlgo(), "CV25519");
  ASSERT_EQ(subkey.GetKeyLength(), 255);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_FALSE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_FALSE(subkey.IsHasSigningCapability());

  ASSERT_TRUE(key.IsHasActualCertificationCapability());
  ASSERT_TRUE(key.IsHasActualAuthenticationCapability());
  ASSERT_TRUE(key.IsHasActualEncryptionCapability());
  ASSERT_TRUE(key.IsHasActualSigningCapability());

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel).DeleteKey(fpr);
}

TEST_F(GpgCoreTest, GenerateKeyED25519NISTP256Test) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_ec2");
  keygen_info->SetEmail("ec2_bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("ecccc");
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[4]));
  keygen_info->SetAllowAuthentication(true);
  keygen_info->SetAllowCertification(true);
  keygen_info->SetAllowEncryption(true);
  keygen_info->SetAllowSigning(true);
  keygen_info->SetNonExpired(true);
  keygen_info->SetNonPassPhrase(true);

  auto subkeygen_info = SecureCreateSharedObject<GenKeyInfo>(true);
  subkeygen_info->SetAlgo(std::get<2>(keygen_info->GetSupportedKeyAlgo()[4]));
  subkeygen_info->SetAllowAuthentication(true);
  subkeygen_info->SetAllowCertification(true);
  subkeygen_info->SetAllowEncryption(true);
  subkeygen_info->SetAllowSigning(true);
  subkeygen_info->SetNonExpired(true);
  subkeygen_info->SetNonPassPhrase(true);

  auto [err, data_object] =
      GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeyWithSubkeySync(keygen_info, subkeygen_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgGenerateKeyResult, GpgGenerateKeyResult>()));
  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  auto fpr = result.GetFingerprint();
  ASSERT_FALSE(fpr.isEmpty());

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).GetKey(fpr);
  ASSERT_TRUE(key.IsGood());

  ASSERT_EQ(key.GetName(), "foo_ec2");
  ASSERT_EQ(key.GetEmail(), "ec2_bar@gpgfrontend.bktus.com");
  ASSERT_EQ(key.GetComment(), "ecccc");
  ASSERT_EQ(key.GetPublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(key.GetKeyAlgo(), "ED25519");
  ASSERT_EQ(key.GetOwnerTrustLevel(), 5);
  ASSERT_EQ(key.GetPrimaryKeyLength(), 255);
  ASSERT_EQ(key.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_TRUE(key.IsHasCertificationCapability());
  ASSERT_TRUE(key.IsHasAuthenticationCapability());
  ASSERT_TRUE(key.IsHasEncryptionCapability());
  ASSERT_TRUE(key.IsHasSigningCapability());

  ASSERT_FALSE(key.GetSubKeys()->empty());
  ASSERT_EQ(key.GetSubKeys()->size(), 2);

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();
  ASSERT_EQ(subkey.GetPubkeyAlgo(), "ECDH");
  ASSERT_EQ(subkey.GetKeyAlgo(), "NISTP256");
  ASSERT_EQ(subkey.GetKeyLength(), 256);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_FALSE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_FALSE(subkey.IsHasSigningCapability());

  ASSERT_TRUE(key.IsHasActualCertificationCapability());
  ASSERT_TRUE(key.IsHasActualAuthenticationCapability());
  ASSERT_TRUE(key.IsHasActualEncryptionCapability());
  ASSERT_TRUE(key.IsHasActualSigningCapability());

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel).DeleteKey(fpr);
}

TEST_F(GpgCoreTest, GenerateKeyED25519BRAINPOOLP256R1Test) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_ec3");
  keygen_info->SetEmail("ec3_bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("ecccc3");
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[5]));
  keygen_info->SetAllowAuthentication(true);
  keygen_info->SetAllowCertification(true);
  keygen_info->SetAllowEncryption(true);
  keygen_info->SetAllowSigning(true);
  keygen_info->SetNonExpired(true);
  keygen_info->SetNonPassPhrase(true);

  auto subkeygen_info = SecureCreateSharedObject<GenKeyInfo>(true);
  subkeygen_info->SetAlgo(std::get<2>(keygen_info->GetSupportedKeyAlgo()[5]));
  subkeygen_info->SetAllowAuthentication(true);
  subkeygen_info->SetAllowCertification(true);
  subkeygen_info->SetAllowEncryption(true);
  subkeygen_info->SetAllowSigning(true);
  subkeygen_info->SetNonExpired(true);
  subkeygen_info->SetNonPassPhrase(true);

  auto [err, data_object] =
      GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeyWithSubkeySync(keygen_info, subkeygen_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgGenerateKeyResult, GpgGenerateKeyResult>()));
  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  auto fpr = result.GetFingerprint();
  ASSERT_FALSE(fpr.isEmpty());

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).GetKey(fpr);
  ASSERT_TRUE(key.IsGood());

  ASSERT_EQ(key.GetName(), "foo_ec3");
  ASSERT_EQ(key.GetEmail(), "ec3_bar@gpgfrontend.bktus.com");
  ASSERT_EQ(key.GetComment(), "ecccc3");
  ASSERT_EQ(key.GetPublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(key.GetKeyAlgo(), "ED25519");
  ASSERT_EQ(key.GetOwnerTrustLevel(), 5);
  ASSERT_EQ(key.GetPrimaryKeyLength(), 255);
  ASSERT_EQ(key.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_TRUE(key.IsHasCertificationCapability());
  ASSERT_TRUE(key.IsHasAuthenticationCapability());
  ASSERT_TRUE(key.IsHasEncryptionCapability());
  ASSERT_TRUE(key.IsHasSigningCapability());

  ASSERT_FALSE(key.GetSubKeys()->empty());
  ASSERT_EQ(key.GetSubKeys()->size(), 2);

  auto subkeys = key.GetSubKeys();
  auto& subkey = subkeys->back();
  ASSERT_EQ(subkey.GetPubkeyAlgo(), "ECDH");
  ASSERT_EQ(subkey.GetKeyAlgo(), "BRAINPOOLP256R1");
  ASSERT_EQ(subkey.GetKeyLength(), 256);
  ASSERT_EQ(subkey.GetExpireTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(subkey.IsHasCertificationCapability());
  ASSERT_FALSE(subkey.IsHasAuthenticationCapability());
  ASSERT_TRUE(subkey.IsHasEncryptionCapability());
  ASSERT_FALSE(subkey.IsHasSigningCapability());

  ASSERT_TRUE(key.IsHasActualCertificationCapability());
  ASSERT_TRUE(key.IsHasActualAuthenticationCapability());
  ASSERT_TRUE(key.IsHasActualEncryptionCapability());
  ASSERT_TRUE(key.IsHasActualSigningCapability());

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel).DeleteKey(fpr);
}

}  // namespace GpgFrontend::Test
