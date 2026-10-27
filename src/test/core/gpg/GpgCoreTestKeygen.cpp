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
#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/model/GpgSubKey.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, SearchPrimaryKeyAlgoTest) {
  auto [find, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("rsa2048");
  ASSERT_TRUE(find);
  ASSERT_EQ(algo.Id(), "rsa2048");
  ASSERT_EQ(algo.Id(), "rsa2048");
  ASSERT_EQ(algo.Name(), "RSA");
  ASSERT_EQ(algo.Type(), "RSA");
  ASSERT_EQ(algo.KeyLength(), 2048);
}

TEST_F(GpgCoreTest, SearchSubKeyAlgoTest) {
  auto [find, algo] = KeyGenerateInfo::SearchSubKeyAlgo("rsa2048");
  ASSERT_TRUE(find);
  ASSERT_EQ(algo.Id(), "rsa2048");
  ASSERT_EQ(algo.Name(), "RSA");
  ASSERT_EQ(algo.Type(), "RSA");
  ASSERT_EQ(algo.KeyLength(), 2048);
}

TEST_F(GpgCoreTest, IsPostQuantumAlgoTest) {
  // Classical algorithms must not be flagged as post-quantum.
  auto [find_rsa, rsa] = KeyGenerateInfo::SearchPrimaryKeyAlgo("rsa2048");
  ASSERT_TRUE(find_rsa);
  ASSERT_FALSE(rsa.IsPostQuantum());

  auto [find_ed, ed] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  ASSERT_TRUE(find_ed);
  ASSERT_FALSE(ed.IsPostQuantum());

  ASSERT_FALSE(KeyGenerateInfo::kNoneAlgo.IsPostQuantum());

  // SLH-DSA is a stateless hash-based post-quantum signature algorithm.
  auto [find_slh, slh] =
      KeyGenerateInfo::SearchPrimaryKeyAlgo("slhdsashake128s");
  ASSERT_TRUE(find_slh);
  ASSERT_TRUE(slh.IsPostQuantum());

  // Every hybrid primary algorithm (ML-DSA) is post-quantum.
  ASSERT_FALSE(KeyGenerateInfo::kHybridPrimaryKeyAlgo.isEmpty());
  for (const auto& algo : KeyGenerateInfo::kHybridPrimaryKeyAlgo) {
    ASSERT_TRUE(algo.IsPostQuantum()) << algo.Id().toStdString();
  }

  // Every hybrid subkey algorithm (Kyber KEM, ML-DSA sign) is post-quantum.
  ASSERT_FALSE(KeyGenerateInfo::kHybridSubKeyAlgos.isEmpty());
  for (const auto& algo : KeyGenerateInfo::kHybridSubKeyAlgos) {
    ASSERT_TRUE(algo.IsPostQuantum()) << algo.Id().toStdString();
  }
}

// An existing key's algorithm resolves to its KeyAlgo, hybrids by their
// joined id, so what the algorithm IS comes from the KeyAlgo metadata.
TEST(KeyAlgoLookupTest, AnAlgoIdResolvesToItsKeyAlgo) {
  for (const auto* id : {"ky768_cv25519", "kyber1024_x448", "mldsa65_ed25519",
                         "mldsa87_ed448", "slhdsashake128s"}) {
    const auto algo = KeyGenerateInfo::SearchKeyAlgoByAlgoId(id);
    ASSERT_TRUE(algo.has_value()) << id;
    EXPECT_TRUE(algo->IsPostQuantum()) << id;
  }
  for (const auto* id : {"cv25519", "x448", "rsa3072", "nistp256",
                         "brainpoolp256r1", "ed25519"}) {
    const auto algo = KeyGenerateInfo::SearchKeyAlgoByAlgoId(id);
    ASSERT_TRUE(algo.has_value()) << id;
    EXPECT_FALSE(algo->IsPostQuantum()) << id;
  }
  EXPECT_FALSE(KeyGenerateInfo::SearchKeyAlgoByAlgoId("").has_value());
  EXPECT_TRUE(KeyGenerateInfo::SearchKeyAlgoByAlgoId("ky768").has_value());
  EXPECT_FALSE(
      KeyGenerateInfo::SearchKeyAlgoByAlgoId("ky768_unknown").has_value());
}

// GnuPG names some algorithms differently from the KeyAlgo ids.
TEST(KeyAlgoLookupTest, GnuPGNamesBecomeKeyAlgoIds) {
  EXPECT_EQ(GpgSubKey::AlgoIdFromGnuPGName("ky768_cv25519"), "ky768_cv25519");
  EXPECT_EQ(GpgSubKey::AlgoIdFromGnuPGName("ky1024_cv448"), "kyber1024_x448");
  EXPECT_EQ(GpgSubKey::AlgoIdFromGnuPGName("cv448"), "x448");
  EXPECT_EQ(GpgSubKey::AlgoIdFromGnuPGName("brainpoolP256r1"),
            "brainpoolp256r1");
  EXPECT_EQ(GpgSubKey::AlgoIdFromGnuPGName("rsa3072"), "rsa3072");
}

// Mixing is allowed but warned about: the message is only as strong against
// a quantum attacker as its weakest recipient. A recipient that may be
// encrypted to classically counts as classical; the engine picks the subkey.
TEST(KeyAlgoLookupTest, MixingIsReportedOnlyForPostQuantumBesideClassical) {
  const QList<bool> pq{true};
  const QList<bool> classical{false};
  const QList<bool> both{false, true};

  EXPECT_TRUE(MixesPostQuantumAndClassical({pq, classical}));
  EXPECT_TRUE(MixesPostQuantumAndClassical({pq, both}));
  EXPECT_FALSE(MixesPostQuantumAndClassical({classical, both}));
  EXPECT_FALSE(MixesPostQuantumAndClassical({classical, classical}));
  EXPECT_FALSE(MixesPostQuantumAndClassical({pq}));
  // A recipient with no usable encryption key says nothing either way.
  EXPECT_FALSE(MixesPostQuantumAndClassical({pq, {}}));
}

TEST_F(GpgCoreTest, IsEccAlgoTest) {
  // Non-ECC classical algorithms must not be flagged as ECC.
  auto [find_rsa, rsa] = KeyGenerateInfo::SearchPrimaryKeyAlgo("rsa2048");
  ASSERT_TRUE(find_rsa);
  ASSERT_FALSE(rsa.IsEcc());

  auto [find_dsa, dsa] = KeyGenerateInfo::SearchPrimaryKeyAlgo("dsa2048");
  ASSERT_TRUE(find_dsa);
  ASSERT_FALSE(dsa.IsEcc());

  ASSERT_FALSE(KeyGenerateInfo::kNoneAlgo.IsEcc());

  // EdDSA / ECDSA / ECDH curves are elliptic-curve algorithms.
  auto [find_ed, ed] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  ASSERT_TRUE(find_ed);
  ASSERT_TRUE(ed.IsEcc());

  auto [find_nist, nist] = KeyGenerateInfo::SearchPrimaryKeyAlgo("nistp256");
  ASSERT_TRUE(find_nist);
  ASSERT_TRUE(nist.IsEcc());

  auto [find_cv, cv] = KeyGenerateInfo::SearchSubKeyAlgo("cv25519");
  ASSERT_TRUE(find_cv);
  ASSERT_TRUE(cv.IsEcc());

  // Post-quantum algorithms are not part of the ECC family.
  auto [find_slh, slh] =
      KeyGenerateInfo::SearchPrimaryKeyAlgo("slhdsashake128s");
  ASSERT_TRUE(find_slh);
  ASSERT_FALSE(slh.IsEcc());

  for (const auto& algo : KeyGenerateInfo::kHybridSubKeyAlgos) {
    ASSERT_FALSE(algo.IsEcc()) << algo.Id().toStdString();
  }
}

TEST_F(GpgCoreTest, KeyVersionRoundTripTest) {
  auto info = QSharedPointer<KeyGenerateInfo>::create();

  // Default is "engine decides" (0) so callers that never touch the field keep
  // the engine's native behavior.
  ASSERT_EQ(info->GetKeyVersion(), 0);

  info->SetKeyVersion(4);
  ASSERT_EQ(info->GetKeyVersion(), 4);

  info->SetKeyVersion(6);
  ASSERT_EQ(info->GetKeyVersion(), 6);
}

TEST_F(GpgCoreTest, SetNonExpiredFalseKeepsAUsableExpiry) {
  auto info = QSharedPointer<KeyGenerateInfo>::create();

  // A date still ahead survives ticking "never expires" on and off again.
  const auto chosen = QDateTime::currentDateTime().addDays(30);
  info->SetExpireTime(chosen);
  info->SetNonExpired(true);
  info->SetNonExpired(false);
  ASSERT_FALSE(info->IsNonExpired());
  ASSERT_EQ(info->GetExpireTime(), chosen);

  // One already past is replaced by the two-year default, never the epoch.
  info->SetExpireTime(QDateTime::fromSecsSinceEpoch(0));
  info->SetNonExpired(false);
  ASSERT_GT(info->GetExpireTime(),
            QDateTime::currentDateTime().addYears(2).addDays(-1));
}

TEST_F(GpgCoreTest, GenerateKeyRSA2048Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_0");
  p_info->SetEmail("bar@gpgfrontend.bktus.com");
  p_info->SetComment("foobar");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("rsa2048");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "rsa2048");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(true);
  p_info->SetNonPassPhrase(true);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeySync(p_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());
  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);
  auto& key = *p_key;

  ASSERT_EQ(key.Name(), "foo_0");
  ASSERT_EQ(key.Email(), "bar@gpgfrontend.bktus.com");
  ASSERT_EQ(key.Comment(), "foobar");
  ASSERT_EQ(key.PublicKeyAlgo(), "RSA");
  ASSERT_EQ(key.OwnerTrustLevel(), 5);
  ASSERT_EQ(key.PrimaryKeyLength(), 2048);
  ASSERT_EQ(key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_TRUE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());

  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());
  ASSERT_TRUE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

TEST_F(GpgCoreTest, GenerateKeyRSA4096Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_1");
  p_info->SetEmail("bar@gpgfrontend.bktus.com");
  p_info->SetComment("hello gpgfrontend");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("rsa4096");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "rsa4096");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(false);
  p_info->SetNonPassPhrase(true);

  auto expire_time =
      QDateTime::currentDateTime().addSecs(static_cast<qint64>(24 * 3600));
  p_info->SetExpireTime(expire_time);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeySync(p_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(key != nullptr);
  ASSERT_EQ(key->ExpirationTime().date(), expire_time.date());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(key);
}

TEST_F(GpgCoreTest, GenerateKeyDSA2048Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_1");
  p_info->SetEmail("bar_1@gpgfrontend.bktus.com");
  p_info->SetComment("hello gpgfrontend");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("dsa2048");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "dsa2048");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(false);
  p_info->SetNonPassPhrase(true);

  auto expire_time =
      QDateTime::currentDateTime().addSecs(static_cast<qint64>(24 * 3600));
  p_info->SetExpireTime(expire_time);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeySync(p_info);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);
  auto& key = *p_key;

  ASSERT_TRUE(key.IsGood());
  ASSERT_EQ(key.Name(), "foo_1");
  ASSERT_EQ(key.Email(), "bar_1@gpgfrontend.bktus.com");
  ASSERT_EQ(key.Comment(), "hello gpgfrontend");
  ASSERT_EQ(key.PublicKeyAlgo(), "DSA");
  ASSERT_EQ(key.OwnerTrustLevel(), 5);
  ASSERT_EQ(key.PrimaryKeyLength(), 2048);
  ASSERT_GT(key.ExpirationTime(), QDateTime::currentDateTime());

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_FALSE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());

  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());
  ASSERT_FALSE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

TEST_F(GpgCoreTest, GenerateKeyED25519Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_4");
  p_info->SetEmail("bar_ed@gpgfrontend.bktus.com");
  p_info->SetComment("hello gpgfrontend");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "ed25519");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(false);
  p_info->SetNonPassPhrase(true);

  auto expire_time =
      QDateTime::currentDateTime().addSecs(static_cast<qint64>(24 * 3600));
  p_info->SetExpireTime(expire_time);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeySync(p_info);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);
  auto& key = *p_key;

  ASSERT_TRUE(key.IsGood());
  ASSERT_EQ(key.Name(), "foo_4");
  ASSERT_EQ(key.Email(), "bar_ed@gpgfrontend.bktus.com");
  ASSERT_EQ(key.Comment(), "hello gpgfrontend");
  ASSERT_EQ(key.PublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(key.OwnerTrustLevel(), 5);
  ASSERT_EQ(key.PrimaryKeyLength(), 255);
  ASSERT_GT(key.ExpirationTime(), QDateTime::currentDateTime());

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_FALSE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());

  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());
  ASSERT_FALSE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

TEST_F(GpgCoreTest, GenerateKeyED25519CV25519Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_ec");
  p_info->SetEmail("ec_bar@gpgfrontend.bktus.com");
  p_info->SetComment("ecccc");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "ed25519");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(true);
  p_info->SetNonPassPhrase(true);

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  std::tie(found, algo) = KeyGenerateInfo::SearchSubKeyAlgo("cv25519");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "cv25519");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeyWithSubkeySync(p_info, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgGenerateKeyResult, GpgGenerateKeyResult>()));
  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);
  auto& key = *p_key;

  ASSERT_EQ(key.Name(), "foo_ec");
  ASSERT_EQ(key.Email(), "ec_bar@gpgfrontend.bktus.com");
  ASSERT_EQ(key.Comment(), "ecccc");
  ASSERT_EQ(key.PublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(key.Algo(), "ED25519");
  ASSERT_EQ(key.OwnerTrustLevel(), 5);
  ASSERT_EQ(key.PrimaryKeyLength(), 255);
  ASSERT_EQ(key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_TRUE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());

  ASSERT_FALSE(key.SubKeys().empty());
  ASSERT_EQ(key.SubKeys().size(), 2);

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();
  ASSERT_EQ(s_key.PublicKeyAlgo(), "ECDH");
  ASSERT_EQ(s_key.Algo(), "CV25519");
  ASSERT_EQ(s_key.KeyLength(), 255);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());

  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());
  ASSERT_TRUE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

TEST_F(GpgCoreTest, GenerateKeyED25519NISTP256Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_ec2");
  p_info->SetEmail("ec2_bar@gpgfrontend.bktus.com");
  p_info->SetComment("ecccc");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "ed25519");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(true);
  p_info->SetNonPassPhrase(true);

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  std::tie(found, algo) = KeyGenerateInfo::SearchSubKeyAlgo("nistp256");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "nistp256");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeyWithSubkeySync(p_info, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgGenerateKeyResult, GpgGenerateKeyResult>()));
  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);
  auto& key = *p_key;

  ASSERT_EQ(key.Name(), "foo_ec2");
  ASSERT_EQ(key.Email(), "ec2_bar@gpgfrontend.bktus.com");
  ASSERT_EQ(key.Comment(), "ecccc");
  ASSERT_EQ(key.PublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(key.Algo(), "ED25519");
  ASSERT_EQ(key.OwnerTrustLevel(), 5);
  ASSERT_EQ(key.PrimaryKeyLength(), 255);
  ASSERT_EQ(key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_TRUE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());

  ASSERT_FALSE(key.SubKeys().empty());
  ASSERT_EQ(key.SubKeys().size(), 2);

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();
  ASSERT_EQ(s_key.PublicKeyAlgo(), "ECDH");
  ASSERT_EQ(s_key.Algo(), "NISTP256");
  ASSERT_EQ(s_key.KeyLength(), 256);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());

  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());
  ASSERT_TRUE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

TEST_F(GpgCoreTest, GenerateKeyED25519BRAINPOOLP256R1Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_ec3");
  p_info->SetEmail("ec3_bar@gpgfrontend.bktus.com");
  p_info->SetComment("ecccc3");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "ed25519");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(true);
  p_info->SetNonPassPhrase(true);

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);

  std::tie(found, algo) = KeyGenerateInfo::SearchSubKeyAlgo("brainpoolp256r1");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "brainpoolp256r1");
  s_info->SetAlgo(algo);

  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeyWithSubkeySync(p_info, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgGenerateKeyResult, GpgGenerateKeyResult>()));
  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);
  auto& key = *p_key;

  ASSERT_EQ(key.Name(), "foo_ec3");
  ASSERT_EQ(key.Email(), "ec3_bar@gpgfrontend.bktus.com");
  ASSERT_EQ(key.Comment(), "ecccc3");
  ASSERT_EQ(key.PublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(key.Algo(), "ED25519");
  ASSERT_EQ(key.OwnerTrustLevel(), 5);
  ASSERT_EQ(key.PrimaryKeyLength(), 255);
  ASSERT_EQ(key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_TRUE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());

  ASSERT_FALSE(key.SubKeys().empty());
  ASSERT_EQ(key.SubKeys().size(), 2);

  auto s_keys = key.SubKeys();
  auto& s_key = s_keys.back();
  ASSERT_EQ(s_key.PublicKeyAlgo(), "ECDH");
  ASSERT_EQ(s_key.Algo(), "BRAINPOOLP256R1");
  ASSERT_EQ(s_key.KeyLength(), 256);
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));

  ASSERT_FALSE(s_key.IsHasCertCap());
  ASSERT_FALSE(s_key.IsHasAuthCap());
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());

  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());
  ASSERT_TRUE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

TEST_F(GpgCoreTest, GenerateKeyNISTP256Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_4");
  p_info->SetEmail("bar_ed@gpgfrontend.bktus.com");
  p_info->SetComment("hello gpgfrontend");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("nistp256");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "nistp256");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(false);
  p_info->SetNonPassPhrase(true);

  auto expire_time =
      QDateTime::currentDateTime().addSecs(static_cast<qint64>(24 * 3600));
  p_info->SetExpireTime(expire_time);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeySync(p_info);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);
  auto& key = *p_key;

  ASSERT_EQ(key.Name(), "foo_4");
  ASSERT_EQ(key.Email(), "bar_ed@gpgfrontend.bktus.com");
  ASSERT_EQ(key.Comment(), "hello gpgfrontend");
  ASSERT_EQ(key.PublicKeyAlgo(), "ECDSA");
  ASSERT_EQ(key.Algo(), "NISTP256");
  ASSERT_EQ(key.OwnerTrustLevel(), 5);
  ASSERT_EQ(key.PrimaryKeyLength(), 256);
  ASSERT_GT(key.ExpirationTime(), QDateTime::currentDateTime());

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_FALSE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());

  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());
  ASSERT_FALSE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

TEST_F(GpgCoreTest, GenerateKeyED448Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_4");
  p_info->SetEmail("bar_ed@gpgfrontend.bktus.com");
  p_info->SetComment("hello gpgfrontend");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed448");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "ed448");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(false);
  p_info->SetNonPassPhrase(true);

  auto expire_time =
      QDateTime::currentDateTime().addSecs(static_cast<qint64>(24 * 3600));
  p_info->SetExpireTime(expire_time);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeySync(p_info);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    GTEST_SKIP() << "ed448 algorithm is not support by this GnuPG version";
  }
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);
  auto& key = *p_key;

  ASSERT_TRUE(key.IsGood());
  ASSERT_EQ(key.Name(), "foo_4");
  ASSERT_EQ(key.Email(), "bar_ed@gpgfrontend.bktus.com");
  ASSERT_EQ(key.Comment(), "hello gpgfrontend");
  ASSERT_EQ(key.PublicKeyAlgo(), "EdDSA");
  ASSERT_EQ(key.Algo(), "ED448");
  ASSERT_EQ(key.OwnerTrustLevel(), 5);
  ASSERT_EQ(key.PrimaryKeyLength(), 448);
  ASSERT_GT(key.ExpirationTime(), QDateTime::currentDateTime());

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_FALSE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());

  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());
  ASSERT_FALSE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

TEST_F(GpgCoreTest, GenerateKeySECP256K1Test) {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName("foo_4");
  p_info->SetEmail("bar_ed@gpgfrontend.bktus.com");
  p_info->SetComment("hello gpgfrontend");

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("secp256k1");
  ASSERT_TRUE(found);
  ASSERT_EQ(algo.Id(), "secp256k1");
  p_info->SetAlgo(algo);

  p_info->SetNonExpired(false);
  p_info->SetNonPassPhrase(true);

  auto expire_time =
      QDateTime::currentDateTime().addSecs(static_cast<qint64>(24 * 3600));
  p_info->SetExpireTime(expire_time);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeySync(p_info);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(data_object->GetObjectSize(), 1);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  ASSERT_FALSE(result.GetFingerprint().isEmpty());

  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);
  auto& key = *p_key;

  ASSERT_EQ(key.Name(), "foo_4");
  ASSERT_EQ(key.Email(), "bar_ed@gpgfrontend.bktus.com");
  ASSERT_EQ(key.Comment(), "hello gpgfrontend");
  ASSERT_EQ(key.PublicKeyAlgo(), "ECDSA");
  ASSERT_EQ(key.Algo(), "SECP256K1");
  ASSERT_EQ(key.OwnerTrustLevel(), 5);
  ASSERT_EQ(key.PrimaryKeyLength(), 256);
  ASSERT_GT(key.ExpirationTime(), QDateTime::currentDateTime());

  ASSERT_TRUE(key.IsHasCertCap());
  ASSERT_TRUE(key.IsHasAuthCap());
  ASSERT_FALSE(key.IsHasEncrCap());
  ASSERT_TRUE(key.IsHasSignCap());

  ASSERT_TRUE(key.IsHasActualCertCap());
  ASSERT_TRUE(key.IsHasActualAuthCap());
  ASSERT_FALSE(key.IsHasActualEncrCap());
  ASSERT_TRUE(key.IsHasActualSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

namespace {

std::atomic<int> g_passphrase_prompts{0};

auto CountingPassphraseCb(void*, const char*, const char*, int, int fd)
    -> gpgme_error_t {
  g_passphrase_prompts++;
  const QByteArray passphrase("abcdefg\n");
  return gpgme_io_writen(fd, passphrase.constData(), passphrase.size()) == 0
             ? GPG_ERR_NO_ERROR
             : GPG_ERR_CANCELED;
}

auto MakeKeyWithSubkeyInfo(const QString& name, const QString& algo_id,
                           const QString& sub_algo_id)
    -> std::pair<QSharedPointer<KeyGenerateInfo>,
                 QSharedPointer<KeyGenerateInfo>> {
  auto p_info = QSharedPointer<KeyGenerateInfo>::create();
  p_info->SetName(name);
  p_info->SetEmail(name + "@gpgfrontend.bktus.com");
  p_info->SetAlgo(std::get<1>(KeyGenerateInfo::SearchPrimaryKeyAlgo(algo_id)));
  p_info->SetNonExpired(true);
  p_info->SetNonPassPhrase(true);

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);
  s_info->SetAlgo(std::get<1>(KeyGenerateInfo::SearchSubKeyAlgo(sub_algo_id)));
  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);
  return {p_info, s_info};
}

}  // namespace

TEST_F(GpgCoreTest, GenerateKeyWithSubkeyAsksPassphraseOnceTest) {
  // A key and its subkey are made in one gpg run, so the user sets the
  // passphrase once instead of setting it and then unlocking the new key for
  // the subkey.
  auto [p_info, s_info] =
      MakeKeyWithSubkeyInfo("foo_once", "ed25519", "cv25519");
  p_info->SetComment("one prompt");
  p_info->SetNonExpired(false);
  p_info->SetExpireTime(QDateTime::currentDateTime().addDays(1));
  p_info->SetNonPassPhrase(false);
  p_info->SetAllowAuth(false);
  s_info->SetNonPassPhrase(false);

  auto& ctx = GpgCtx(OpenPGPContext::GetInstance(kGpgFrontendDefaultChannel));
  gpgme_passphrase_cb_t prev_cb = nullptr;
  void* prev_hook = nullptr;
  gpgme_get_passphrase_cb(ctx.DefaultContext(), &prev_cb, &prev_hook);
  gpgme_set_passphrase_cb(ctx.DefaultContext(), CountingPassphraseCb,
                          prev_hook);
  g_passphrase_prompts = 0;

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
          .GenerateKeyWithSubkeySync(p_info, s_info);

  gpgme_set_passphrase_cb(ctx.DefaultContext(), prev_cb, prev_hook);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(g_passphrase_prompts.load(), 1);
  ASSERT_TRUE(
      (data_object->Check<GpgGenerateKeyResult, GpgGenerateKeyResult>()));

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  auto s_result = ExtractParams<GpgGenerateKeyResult>(data_object, 1);
  auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                   .GetKeyPtr(result.GetFingerprint());
  ASSERT_TRUE(p_key != nullptr);

  ASSERT_EQ(p_key->Name(), "foo_once");
  ASSERT_EQ(p_key->Comment(), "one prompt");
  ASSERT_GT(p_key->ExpirationTime(), QDateTime::currentDateTime());

  auto s_keys = p_key->SubKeys();
  ASSERT_EQ(s_keys.size(), 2);
  ASSERT_TRUE(s_keys.front().IsHasCertCap());
  ASSERT_TRUE(s_keys.front().IsHasSignCap());
  ASSERT_FALSE(s_keys.front().IsHasAuthCap());
  ASSERT_FALSE(s_keys.front().IsHasEncrCap());

  const auto& s_key = s_keys.back();
  ASSERT_EQ(s_result.GetFingerprint(), s_key.Fingerprint());
  ASSERT_EQ(s_key.Algo(), "CV25519");
  ASSERT_EQ(s_key.ExpirationTime(), QDateTime::fromMSecsSinceEpoch(0));
  ASSERT_TRUE(s_key.IsHasEncrCap());
  ASSERT_FALSE(s_key.IsHasSignCap());

  KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
      .DeleteKey(p_key);
}

TEST_F(GpgCoreTest, GenerateKeyWithSubkeyCurveNamesTest) {
  // GnuPG's parameter block wants libgcrypt's spelling of the curve names,
  // which is not always the id the algorithm is listed under.
  const QList<std::tuple<QString, QString, QString, QString>> pairs{
      {"nistp384", "brainpoolp256r1", "NISTP384", "BRAINPOOLP256R1"},
      {"ed448", "x448", "ED448", "CV448"},
  };

  for (const auto& [algo_id, sub_algo_id, algo, sub_algo] : pairs) {
    auto [p_info, s_info] =
        MakeKeyWithSubkeyInfo("foo_curve", algo_id, sub_algo_id);

    auto [err, data_object] =
        KeyGenerationOperation::GetInstance(kGpgFrontendDefaultChannel)
            .GenerateKeyWithSubkeySync(p_info, s_info);
    if (CheckGpgError(err) == GPG_ERR_UNKNOWN_CURVE ||
        CheckGpgError(err) == GPG_ERR_INV_VALUE) {
      GTEST_SKIP() << algo_id.toStdString() << " / "
                   << sub_algo_id.toStdString()
                   << " is not supported by this GnuPG version";
    }
    ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR)
        << algo_id.toStdString() << " / " << sub_algo_id.toStdString();

    auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
    auto p_key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                     .GetKeyPtr(result.GetFingerprint());
    ASSERT_TRUE(p_key != nullptr);
    ASSERT_EQ(p_key->Algo(), algo);

    auto s_keys = p_key->SubKeys();
    ASSERT_EQ(s_keys.size(), 2);
    ASSERT_EQ(s_keys.back().Algo(), sub_algo);

    KeyManagementOperation::GetInstance(kGpgFrontendDefaultChannel)
        .DeleteKey(p_key);
  }
}

}  // namespace GpgFrontend::Test
