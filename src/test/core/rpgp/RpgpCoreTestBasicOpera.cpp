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

#include <algorithm>

#include "RpgpCoreTest.h"
#include "core/GFCoreRust.h"
#include "core/function/GFKeyDatabase.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/FileCryptoOperation.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/function/rpgp/EncryptSignAttribution.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend::Test {

TEST_F(RpgpCoreTest, CoreEncryptDecryptTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, buffer, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));

  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);

  ASSERT_FALSE(encr_out_buffer.Empty());
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(encr_out_buffer);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GFBuffer>()));

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object_0, 1);

  ASSERT_FALSE(decr_result.Recipients().empty());
  ASSERT_EQ(decr_out_buffer, buffer);
}

TEST_F(RpgpCoreTest, CoreEncryptReportsActualRecipientSubKeyTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, buffer, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));

  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);

  // The engine must report the subkey it actually encrypted to, not the
  // certify-only primary key (BDB8BB6BDDFA8497).
  auto recipients = encr_result.Recipients();
  ASSERT_EQ(recipients.size(), 1);
  EXPECT_FALSE(recipients.front().keyid.isEmpty());
  EXPECT_FALSE(recipients.front().pubkey_algo.isEmpty());
  EXPECT_NE(recipients.front().keyid, QString("BDB8BB6BDDFA8497"));

  // The reported recipient subkey must match what decryption reads from the
  // PKESK packets of the produced ciphertext.
  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(encr_out_buffer);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_FALSE(decr_result.Recipients().empty());
  EXPECT_EQ(recipients.front().keyid, decr_result.Recipients().front().keyid);
}

namespace {

// Locate the fixture key's first usable (non-cert, non-revoked) encryption
// subkey; returns its fingerprint + key id, or empty strings if none.
auto FindEncryptionSubkey(const GpgAbstractKeyPtr& key)
    -> std::pair<QString, QString> {
  auto gpg_key = qSharedPointerDynamicCast<GpgKey>(key);
  if (gpg_key == nullptr) return {};
  for (const auto& s : gpg_key->SubKeys()) {
    if (!s.IsHasCertCap() && s.IsHasEncrCap() && !s.IsRevoked()) {
      return {s.Fingerprint(), s.ID()};
    }
  }
  return {};
}

}  // namespace

TEST_F(RpgpCoreTest, CoreEncryptHonorsPinnedSubkeyTest) {
  // Pin a specific encryption subkey via the "<fpr>!" marker and confirm the
  // engine encrypts to exactly that subkey.
  auto base = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                  .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(base != nullptr);

  auto [sub_fpr, sub_keyid] = FindEncryptionSubkey(base);
  ASSERT_FALSE(sub_fpr.isEmpty());

  auto pinned = AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                    .GetKey(sub_fpr + "!");
  ASSERT_TRUE(pinned != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP!"));
  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({pinned}, buffer, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));

  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_EQ(result.Recipients().size(), 1);
  EXPECT_EQ(result.Recipients().front().keyid, sub_keyid);

  // The ciphertext must round-trip.
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(encr_out_buffer);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object_0, 1);
  EXPECT_EQ(decr_out_buffer, buffer);
}

TEST_F(RpgpCoreTest, CoreEncryptSkipsRevokedSubkeyTest) {
  auto& repo = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest);
  auto key = repo.GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  auto [sub_fpr, sub_keyid] = FindEncryptionSubkey(key);
  ASSERT_FALSE(sub_keyid.isEmpty());

  // Baseline: with the subkey valid, encryption selects it.
  {
    auto [err, dobj] =
        MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
            .EncryptSync(
                {repo.GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497")},
                GFBuffer(QString("hi")), true);
    ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
    auto r = ExtractParams<GpgEncryptResult>(dobj, 0);
    ASSERT_EQ(r.Recipients().size(), 1);
    EXPECT_EQ(r.Recipients().front().keyid, sub_keyid);
  }

  // Revoke that encryption subkey.
  int idx = -1;
  auto s_keys = key->SubKeys();
  for (int i = 0; i < s_keys.size(); ++i) {
    if (s_keys[i].ID() == sub_keyid) {
      idx = i;
      break;
    }
  }
  ASSERT_GE(idx, 0);
  ASSERT_TRUE(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                  .RevokeSubkey(key, idx, 0, QString("Test revocation")));
  repo.FlushKeyCache();

  // The revoked subkey must never be used as a recipient. The key carries
  // other, still valid encryption subkeys, so encryption succeeds and picks
  // exactly one of them.
  auto pub = repo.GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(pub != nullptr);

  QStringList usable_encr_keyids;
  for (const auto& s_key : pub->SubKeys()) {
    if (s_key.IsHasEncrCap() && !s_key.IsRevoked()) {
      usable_encr_keyids.append(s_key.ID());
    }
  }
  ASSERT_FALSE(usable_encr_keyids.isEmpty());
  ASSERT_FALSE(usable_encr_keyids.contains(sub_keyid));

  auto [err2, dobj2] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({pub}, GFBuffer(QString("hi")), true);
  ASSERT_EQ(CheckGpgError(err2), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((dobj2->Check<GpgEncryptResult, GFBuffer>()));

  auto r2 = ExtractParams<GpgEncryptResult>(dobj2, 0);
  EXPECT_TRUE(r2.InvalidRecipients().empty());
  ASSERT_EQ(r2.Recipients().size(), 1);
  EXPECT_NE(r2.Recipients().front().keyid, sub_keyid);
  EXPECT_TRUE(usable_encr_keyids.contains(r2.Recipients().front().keyid))
      << r2.Recipients().front().keyid.toStdString();
}

TEST_F(RpgpCoreTest, CoreEncryptRejectsPinnedRevokedSubkeyTest) {
  auto& repo = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest);
  auto key = repo.GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  auto [sub_fpr, sub_keyid] = FindEncryptionSubkey(key);
  ASSERT_FALSE(sub_fpr.isEmpty());

  int idx = -1;
  auto s_keys = key->SubKeys();
  for (int i = 0; i < s_keys.size(); ++i) {
    if (s_keys[i].ID() == sub_keyid) {
      idx = i;
      break;
    }
  }
  ASSERT_GE(idx, 0);
  ASSERT_TRUE(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                  .RevokeSubkey(key, idx, 0, QString("Test revocation")));
  repo.FlushKeyCache();

  // Pinning the now-revoked subkey must be rejected, not honored.
  auto pinned = AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                    .GetKey(sub_fpr + "!");
  ASSERT_TRUE(pinned != nullptr);

  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({pinned}, GFBuffer(QString("hi")), true);

  // No usable recipient: an unusable key, named with its reason, not bad
  // input. Nothing is encrypted.
  EXPECT_EQ(CheckGpgError(err), GPG_ERR_UNUSABLE_PUBKEY);
  ASSERT_TRUE((dobj->Check<GpgEncryptResult, GFBuffer>()));
  EXPECT_EQ(ExtractParams<GpgEncryptResult>(dobj, 0).InvalidRecipients().size(),
            1);
  auto r = ExtractParams<GpgEncryptResult>(dobj, 0);
  EXPECT_TRUE(r.Recipients().empty());
  EXPECT_TRUE(ExtractParams<GFBuffer>(dobj, 1).Empty());
}

TEST_F(RpgpCoreTest, CoreDecryptInvalidDataReportsDetailTest) {
  // Feed data that is not an OpenPGP message at all. The rPGP engine must fail
  // and, unlike a bare status code, expose a human-readable detail so the user
  // learns *why* it failed (mirroring GnuPG's gpg_strerror precision).
  auto garbage = GFBuffer(QString("this is definitely not an OpenPGP message"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(garbage);

  ASSERT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgDecryptResult, GFBuffer>()));

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object, 0);
  ASSERT_FALSE(decr_result.ErrorDetail().isEmpty());
}

TEST_F(RpgpCoreTest, CoreEncryptSymmetricDecryptTest) {
  auto encrypt_text = GFBuffer(QString("Hello RPGP Symmetric!"));
  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSymmetricSync(encrypt_text, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));
  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(encr_result.InvalidRecipients().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(encr_out_buffer);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GFBuffer>()));
  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object_0, 1);

  ASSERT_TRUE(decr_result.Recipients().empty());
  ASSERT_EQ(decr_out_buffer, encrypt_text);
}

TEST_F(RpgpCoreTest, CoreArgon2ProfileResolvesToRfc9106Parameters) {
  // RFC 9106 section 4 first recommendation: t=1, p=4, m=2^21 KiB (2 GiB).
  auto high = RpgpArgon2ParamsOfProfile(kRpgpArgon2ProfileHighMemory);
  ASSERT_EQ(high.t, 1);
  ASSERT_EQ(high.p, 4);
  ASSERT_EQ(high.m_enc, 21);

  // Second recommendation: t=3, p=4, m=2^16 KiB (64 MiB).
  auto low = RpgpArgon2ParamsOfProfile(kRpgpArgon2ProfileLowMemory);
  ASSERT_EQ(low.t, 3);
  ASSERT_EQ(low.p, 4);
  ASSERT_EQ(low.m_enc, 16);

  // An unrecognised or absent token must never silently weaken the
  // derivation, so it resolves to the stronger of the two.
  for (const auto& unknown : {QString(""), QString("rfc9106_third")}) {
    auto params = RpgpArgon2ParamsOfProfile(unknown);
    ASSERT_EQ(params.t, high.t);
    ASSERT_EQ(params.p, high.p);
    ASSERT_EQ(params.m_enc, high.m_enc);
  }
}

TEST_F(RpgpCoreTest, CoreEncryptSymmetricLowMemoryProfileRoundTrips) {
  SetRpgpArgon2S2kParams(
      RpgpArgon2ParamsOfProfile(kRpgpArgon2ProfileLowMemory));

  auto encrypt_text = GFBuffer(QString("Hello RPGP Low Memory Argon2!"));
  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSymmetricSync(encrypt_text, true);

  // The parameters are process-wide, so restore them before any assertion can
  // abort this test and leave them changed for the next one. Restore the
  // *test process* default, which SetupGlobalTestEnv sets to the low-memory
  // profile -- putting back the high-memory one instead would hand every
  // symmetric test that runs after this a 2 GiB derivation, which is exactly
  // the cost the test binary opts out of.
  SetRpgpArgon2S2kParams(
      RpgpArgon2ParamsOfProfile(kRpgpArgon2ProfileLowMemory));

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(encr_out_buffer);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GFBuffer>()));
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object_0, 1);
  ASSERT_EQ(decr_out_buffer, encrypt_text);
}

TEST_F(RpgpCoreTest, CoreSignVerifyNormalTest) {
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);

  auto sign_text = GFBuffer(QString("Hello RPGP!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({sign_key}, sign_text, GPGME_SIG_MODE_NORMAL, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  auto sign_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(sign_out_buffer, GFBuffer());

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "575572EF0DF799AB884EC6C114C6B0B1596A2755");
}

TEST_F(RpgpCoreTest, CoreSignVerifyDetachTest) {
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);
  auto sign_text = GFBuffer(QString("Hello RPGP!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({sign_key}, sign_text, GPGME_SIG_MODE_DETACH, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  auto sign_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(sign_text, sign_out_buffer);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "575572EF0DF799AB884EC6C114C6B0B1596A2755");
}

TEST_F(RpgpCoreTest, CoreSignVerifyClearTest) {
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);

  auto sign_text = GFBuffer(QString("Hello RPGP!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({sign_key}, sign_text, GPGME_SIG_MODE_CLEAR, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  auto sign_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(sign_out_buffer, GFBuffer());

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "575572EF0DF799AB884EC6C114C6B0B1596A2755");
}

TEST_F(RpgpCoreTest, CoreEncryptSignDecryptVerifyTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);
  auto encrypt_text = GFBuffer(QString("Hello RPGP!"));

  ASSERT_TRUE(sign_key->IsPrivateKey());
  ASSERT_TRUE(sign_key->IsHasActualSignCap());

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSignSync({encrypt_key}, {sign_key}, encrypt_text, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgEncryptResult, GpgSignResult, GFBuffer>()));
  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 1);
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 2);
  ASSERT_TRUE(encr_result.InvalidRecipients().empty());
  ASSERT_TRUE(sign_result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifySync(encr_out_buffer);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object_0->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 1);
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object_0, 2);

  ASSERT_FALSE(decrypt_result.Recipients().empty());
  ASSERT_EQ(decr_out_buffer, encrypt_text);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "575572EF0DF799AB884EC6C114C6B0B1596A2755");
}

// --- Per-channel operation cancellation -------------------------------------
//
// These cases exercise the per-channel cancel mechanism: each OpenPGP context
// channel owns an independent cancel flag, so a cancel requested on one channel
// must never disturb work on another. The rPGP CancellableReader consults the
// flag for its own channel before every chunk read, aborting the operation with
// GPG_ERR_CANCELED.

// The C++ cancel bookkeeping is keyed by channel: requesting/resetting a cancel
// on one channel leaves other channels untouched.
TEST_F(RpgpCoreTest, CancelStatePerChannelIsolationTest) {
  // Start from a clean slate for both channels.
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
  ResetGpgOperationCancelState(kGpgChannelForUnitTest);
  ASSERT_FALSE(IsGpgOperationCancelRequested(kRpgpChannelForUnitTest));
  ASSERT_FALSE(IsGpgOperationCancelRequested(kGpgChannelForUnitTest));

  // Requesting a cancel on the rPGP channel must not mark the GnuPG channel.
  RequestCancelGpgOperation(kRpgpChannelForUnitTest);
  ASSERT_TRUE(IsGpgOperationCancelRequested(kRpgpChannelForUnitTest));
  ASSERT_FALSE(IsGpgOperationCancelRequested(kGpgChannelForUnitTest));

  // Resetting that channel clears only its own flag.
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
  ASSERT_FALSE(IsGpgOperationCancelRequested(kRpgpChannelForUnitTest));
  ASSERT_FALSE(IsGpgOperationCancelRequested(kGpgChannelForUnitTest));
}

// A cancel pending on a channel aborts an rPGP encryption on that channel, and
// resetting the flag restores normal operation.
TEST_F(RpgpCoreTest, CoreEncryptCancelTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP Cancel!"));

  // With a cancel pending on this channel, the encryption must abort.
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
  RequestCancelGpgOperation(kRpgpChannelForUnitTest);

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, buffer, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_CANCELED);

  // After resetting the flag, the same operation succeeds again.
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, buffer, true);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgEncryptResult, GFBuffer>()));
}

// A cancel pending on a channel aborts an rPGP signing on that channel.
TEST_F(RpgpCoreTest, CoreSignCancelTest) {
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);

  auto sign_text = GFBuffer(QString("Hello RPGP Cancel!"));

  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
  RequestCancelGpgOperation(kRpgpChannelForUnitTest);

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({sign_key}, sign_text, GPGME_SIG_MODE_NORMAL, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_CANCELED);

  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({sign_key}, sign_text, GPGME_SIG_MODE_NORMAL, true);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
}

// A cancelled encrypt-and-sign must report the cancellation, not success with
// an empty ciphertext.
TEST_F(RpgpCoreTest, CoreEncryptSignCancelReportsCanceledTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);

  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
  RequestCancelGpgOperation(kRpgpChannelForUnitTest);

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSignSync({encrypt_key}, {sign_key},
                           GFBuffer(QString("Hello RPGP Cancel!")), true);
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_CANCELED);
}

// A cancelled symmetric encryption must report the cancellation, not a
// generic failure.
TEST_F(RpgpCoreTest, CoreEncryptSymmetricCancelReportsCanceledTest) {
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
  RequestCancelGpgOperation(kRpgpChannelForUnitTest);

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSymmetricSync(GFBuffer(QString("Hello RPGP Cancel!")),
                                true);
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_CANCELED);
}

// A cancel pending on a channel aborts an rPGP decryption on that channel.
TEST_F(RpgpCoreTest, CoreDecryptCancelTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP Cancel!"));

  // Produce a ciphertext while no cancel is pending.
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, buffer, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_FALSE(encr_out_buffer.Empty());

  // With a cancel pending, decryption of that ciphertext must abort.
  RequestCancelGpgOperation(kRpgpChannelForUnitTest);
  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(encr_out_buffer);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_CANCELED);

  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
}

// A cancel pending on a *different* channel must not affect an operation on the
// channel actually doing the work. This proves the Rust-side cancel map is
// keyed per channel rather than process-global.
TEST_F(RpgpCoreTest, CoreEncryptCancelOtherChannelTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP Cancel!"));

  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);

  // Raise the cancel flag for an unrelated channel only (Rust side directly, to
  // avoid materialising a context for a channel we never use).
#ifdef HAS_RUST_SUPPORT
  const int other_channel = kRpgpChannelForUnitTest + 100;
  GpgFrontend::Rust::gfr_set_operation_cancelled(other_channel, true);
#endif

  // The encryption on the test channel must still succeed.
  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, buffer, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));

  // Clean up the unrelated channel's flag.
#ifdef HAS_RUST_SUPPORT
  GpgFrontend::Rust::gfr_set_operation_cancelled(other_channel, false);
#endif
}

// Regression: rPGP-backed results carry no native gpgme_*_result handle; their
// data lives in the Gpg*Result model instead. The result analysers must read
// that engine-agnostic model so a successful operation is reported as success.
// Previously the EML flow analysed a null gpgme handle and surfaced a perfectly
// good sign/verify as "failed" (status -1). These analysers are exactly what
// the SDK's analyse-by-capsule path drives, so a non-negative status here is
// the guarantee that path relies on.
TEST_F(RpgpCoreTest, CoreSignVerifyResultAnalyseEngineAgnosticTest) {
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);

  auto sign_text = GFBuffer(QString("Hello RPGP!"));

  // PGP/MIME (and therefore the EML flow this fixes) uses a detached signature,
  // so exercise detached sign + verify here.
  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({sign_key}, sign_text, GPGME_SIG_MODE_DETACH, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 0);
  auto sign_out_buffer = ExtractParams<GFBuffer>(data_object, 1);

  GpgSignResultAnalyse sign_analyse{kRpgpChannelForUnitTest, err, sign_result};
  sign_analyse.Analyse();
  EXPECT_GT(sign_analyse.GetStatus(), 0);
  EXPECT_FALSE(sign_analyse.GetResultReport().isEmpty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(sign_text, sign_out_buffer);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);

  GpgVerifyResultAnalyse verify_analyse{kRpgpChannelForUnitTest, err_0,
                                        verify_result};
  verify_analyse.Analyse();
  // A good signature must never be reported as a failure (-1).
  EXPECT_GE(verify_analyse.GetStatus(), 0);
  EXPECT_FALSE(verify_analyse.GetResultReport().isEmpty());
}

TEST_F(RpgpCoreTest, CoreEncryptDecryptResultAnalyseEngineAgnosticTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, buffer, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);

  GpgEncryptResultAnalyse encr_analyse{kRpgpChannelForUnitTest, err,
                                       encr_result};
  encr_analyse.Analyse();
  EXPECT_GT(encr_analyse.GetStatus(), 0);
  EXPECT_FALSE(encr_analyse.GetResultReport().isEmpty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(encr_out_buffer);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);

  GpgDecryptResultAnalyse decr_analyse{kRpgpChannelForUnitTest, err_0,
                                       decr_result};
  decr_analyse.Analyse();
  // A good decryption must never be reported as a failure (-1).
  EXPECT_GE(decr_analyse.GetStatus(), 0);
  EXPECT_FALSE(decr_analyse.GetResultReport().isEmpty());
}

// An RSA-1024 key, generated once with GnuPG 2.4 (unprotected). RFC 9580
// §12.4: "MUST NOT encrypt, sign, or verify using RSA keys of a size less than
// 2048 bits." The engine refuses to sign with it, and the GUI must learn why.
constexpr const char* kRsa1024Secret = R"(-----BEGIN PGP PRIVATE KEY BLOCK-----

lQHYBGrX0zUBBADD3X9GN8IMYaJYs82kNaX/i09AInAqPcyGzmuWajHGFrQcrv9+
QY1XVYJBywQmBMZfTyYQbKtLuF+Gd+2Cblr7W1pM4xqaCzSx3TeVkzCpWVeSj2gq
X8n1YbrhqJqKRzEE5EY1Cht5hXlnSiWvY2beZWUkijkI6kU23ogR1R4aJwARAQAB
AAP/VRR+GM5XqHEwyD8ycMYtGVg2ULGk73Rwx44bBfeTL2zlSCBsMzfeYbVvI41A
fBNO6E/FRRUvQ3EPbaXiseml8w1w+bVpPFsj6QbBbQMAvlzLRKRw/CM76HTttHHa
APX4ibwSdx50oUBazPhlqySyHma/PQ5Wo19fMfkPx/hjY8ECAMPtlr3WA3qqUt8P
3hOJqSLjmC4kwgTgLrCQ9H4g3LN/i8FWr2AeQKt9tF3Ro7BJveP468Oe9N7u+hGy
aA0IYukCAP/q+X8PHto0gjg5mJir/QiTjAajxEHOOcoJJSzva1HtHUyNJp9YHxaF
ReeXrIBP/zmu8sN5p1w19crplNhryo8CAO6cIgZwvLbaEv9jiYMV9eTi+VF9+A7X
WHMPhMx048TenPoeCn65tf9mxOL4n0lHsXsgGpJYD2paoezsCVe4+A6k+7QfUlNB
IDEwMjQgPHJzYTEwMjRAdGVzdC5leGFtcGxlPojOBBMBCgA4FiEE3k5RkBwVhBtJ
p57+cdGo3WXK0LoFAmrX0zUCGwMFCwkIBwIGFQoJCAsCBBYCAwECHgECF4AACgkQ
cdGo3WXK0LonVgQAv8cQl3rgP2ufnINqEqpUGzE0AGrRNDaTUfOvokKD3MNPGYol
A3Ga3W3TSWMM3+iFUuw66/iXGKMpkDhpAi1htTwi31R7H3uCA0fK9CiyepKOzQya
LIw1mWLtrArMB53aWTVLn0z0pQK4/kAQkFOkuJBjvut8ds/MLgX5qdnwR/ydAdgE
atfTNQEEAJnicjBprzlo+YXYJoNojlvnKxuC2VEyq98QzrMzOtcAGVi7luYCf9IC
7kwKPKgWSutxW17w3laP2X2nFzGygwTBZLENnBF1+OHQf8phV5jCYTeHUloxXcII
gKrQcQrbIJsYVZ+eUuZktYCoHqNW0aIwMl2wNsgbTwkYgZ5e0SyJABEBAAEAA/0R
nStf4kD1y8+/48Foyjd9SoUTJZOVaTzmEAjw1xzVjkPYdcRseH3D/qTjHErcHI0f
zP5d7Cku9FE7Qjxz6lB3tezzDY+xXHmccB7veUFmNH0QI/toyMp0tgUpwxXidZ+K
PJwoXVGdQkCvp6FL8IvIFF0ZNbUiJ9th7wOYFzJpzwIAwS5XrUM0VMzUhZ/RDI9k
pWFOlh4drSqCPDvbgUNFwO5EugfXgsnfiropEg0o/Gtt19QW22zRWC/qIe3iwAFk
ZwIAy+zOptgXDbiNwPlYAjegwkPisg0eSmG2Xw0bD8imzDezNFTy+KRBz0A6yin+
1myqyxhziyY2aJDJU4UJsR7RjwIAhrKxzr+JUIRTh/8LV5j+aYfVvzoMpNioPq45
b00ZSQpl5O7Yqc6Lo3puaCc/ExMATssxr486s4qsK5m/lNEszJ6YiLYEGAEKACAW
IQTeTlGQHBWEG0mnnv5x0ajdZcrQugUCatfTNQIbDAAKCRBx0ajdZcrQuonoA/9Y
xZ2RD21yzuAC5i2j+yyyqL/d1dwAAdbWXpha0dlPbvELHMUCZO9CXuku4kO+ye/L
sXEz2yrRo+4pNpIx9aVIxpu+BoVtMweoFb6XOgQwhm1mCywXRuqUImYCRlrI0WYB
kNl6eEN+fsTGbm+i2S1SGleCuh6yLEONt4TkhMjYIg==
=u2L9
-----END PGP PRIVATE KEY BLOCK-----)";
constexpr const char* kRsa1024Fpr = "DE4E51901C15841B49A79EFE71D1A8DD65CAD0BA";

TEST_F(RpgpCoreTest, CoreSignWithRsaUnder2048BitsIsUnusableTest) {
  auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                  .ImportKey(GFBuffer(QString::fromLatin1(kRsa1024Secret)));
  ASSERT_TRUE(info != nullptr);
  AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushCache();
  AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest).Fetch();

  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr(kRsa1024Fpr);
  ASSERT_TRUE(key != nullptr);

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({key}, GFBuffer(QString("rsa 1024")), GPGME_SIG_MODE_DETACH,
                    true);
  EXPECT_EQ(CheckGpgError(err), GPG_ERR_UNUSABLE_SECKEY);
}

TEST_F(RpgpCoreTest, EngineBuildInfoTest) {
  const auto info = RustEngineBuildInfo();

  // The engine version is reported (resolved inside gf_core, which is built
  // with Rust support regardless of the test target's own definitions).
  EXPECT_FALSE(info.engine_version.isEmpty());

  // Build environment details are captured at compile time.
  EXPECT_FALSE(info.rustc_version.isEmpty());

  // The rPGP crate (pgp) must be present among the tracked dependencies.
  EXPECT_FALSE(info.dependencies.isEmpty());
  const bool has_pgp =
      std::any_of(info.dependencies.cbegin(), info.dependencies.cend(),
                  [](const auto& dep) {
                    return dep.first == "pgp" && !dep.second.isEmpty();
                  });
  EXPECT_TRUE(has_pgp);
}

namespace {

constexpr const char* kFixtureFpr = "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497";

// A fresh, unprotected Ed25519 + Curve25519 key on the rPGP channel, for tests
// that need a key they may delete without disturbing the shared fixture.
auto GenerateThrowawayRpgpKey(const QString& email) -> GpgKeyPtr {
  auto p_params = QSharedPointer<KeyGenerateInfo>::create();
  p_params->SetName("throwaway");
  p_params->SetEmail(email);
  auto [p_found, p_algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  if (!p_found) return nullptr;
  p_params->SetAlgo(p_algo);
  p_params->SetNonExpired(true);
  p_params->SetNonPassPhrase(true);
  p_params->SetKeyVersion(4);

  auto s_params = QSharedPointer<KeyGenerateInfo>::create();
  s_params->SetIsSubKey(true);
  auto [s_found, s_algo] = KeyGenerateInfo::SearchSubKeyAlgo("cv25519");
  if (!s_found) return nullptr;
  s_params->SetAlgo(s_algo);
  s_params->SetNonExpired(true);
  s_params->SetNonPassPhrase(true);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kRpgpChannelForUnitTest)
          .GenerateKeyWithSubkeySync(p_params, s_params);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return nullptr;

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
  return GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
      .GetKeyPtr(result.GetFingerprint());
}

// A v6 key whose encryption subkey is ML-KEM-768+X25519 (RFC 9980).
auto GeneratePostQuantumRpgpKey(const QString& email) -> GpgKeyPtr {
  auto p_params = QSharedPointer<KeyGenerateInfo>::create();
  p_params->SetName("post-quantum");
  p_params->SetEmail(email);
  auto [p_found, p_algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  if (!p_found) return nullptr;
  p_params->SetAlgo(p_algo);
  p_params->SetNonExpired(true);
  p_params->SetNonPassPhrase(true);
  p_params->SetKeyVersion(6);

  auto s_params = QSharedPointer<KeyGenerateInfo>::create();
  s_params->SetIsSubKey(true);
  for (const auto& algo : KeyGenerateInfo::kHybridSubKeyAlgos) {
    if (algo.Id() != "ky768") continue;
    s_params->SetAlgo(algo);
    for (const auto& sub : algo.SubAlgos(kRpgpChannelForUnitTest)) {
      if (sub.Id() == "cv25519") s_params->SetSubAlgo(sub);
    }
  }
  if (s_params->GetAlgo().Id() != "ky768") return nullptr;
  s_params->SetNonExpired(true);
  s_params->SetNonPassPhrase(true);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kRpgpChannelForUnitTest)
          .GenerateKeyWithSubkeySync(p_params, s_params);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return nullptr;

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
  return GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
      .GetKeyPtr(result.GetFingerprint());
}

auto MakeRpgpSignature(GFSignatureStatus status) -> GFSignature {
  return GFSignature{
      .issuer_fpr = kFixtureFpr,
      .status = status,
      .created_at = 1700000000U,
      .pub_algo = "EdDSA",
      .hash_algo = "SHA256",
      .sig_type = {},
  };
}

}  // namespace

// rPGP reports invalid recipients only through the model (GetRaw() is null),
// so the analyser must read them from there: encrypting to one good and one
// unusable recipient is not a clean success.
TEST_F(RpgpCoreTest, CoreEncryptResultAnalyseShowsRpgpInvalidRecipientsTest) {
  const QString bad_fpr = "0123456789ABCDEF0123456789ABCDEF01234567";
  GFEncryptResult gf_result;
  gf_result.invalid_recipients.push_back({bad_fpr, GPG_ERR_CERT_REVOKED});

  GpgEncryptResultAnalyse analyse{kRpgpChannelForUnitTest, GPG_ERR_NO_ERROR,
                                  GpgEncryptResult(gf_result)};
  analyse.Analyse();

  EXPECT_TRUE(analyse.GetResultReport().contains(bad_fpr))
      << analyse.GetResultReport().toStdString();
  EXPECT_TRUE(analyse.GetOpInfo().description.contains(
      "Some recipients could not be added"))
      << analyse.GetOpInfo().description.toStdString();
}

// RFC 9580 §9.3 / §12.2: a message decrypted with a deprecated cipher, or one
// the decrypting key does not prefer, must not read as a clean success.
TEST_F(RpgpCoreTest, CoreDecryptResultAnalyseShowsCipherWarningTest) {
  const QString warning =
      "the message is encrypted with CAST5, a deprecated cipher whose "
      "confidentiality is suspect (RFC 9580 §9.3)";
  GFDecryptResult gf_result;
  gf_result.symkey_algo = "CAST5.CFB";
  gf_result.cipher_warning = warning;
  // A password-only message: no recipient (and so no missing key) lowers the
  // status by itself.

  GpgDecryptResultAnalyse analyse{kRpgpChannelForUnitTest, GPG_ERR_NO_ERROR,
                                  GpgDecryptResult(gf_result)};
  analyse.Analyse();

  EXPECT_EQ(analyse.GetStatus(), 0) << analyse.GetResultReport().toStdString();
  EXPECT_TRUE(analyse.GetResultReport().contains(warning))
      << analyse.GetResultReport().toStdString();
}

// Encrypting to one good recipient and one whose pinned subkey is revoked must
// not read as a clean success: the rejected recipient, with a specific reason,
// has to reach the user-visible result (rPGP has no gpgme handle to read).
TEST_F(RpgpCoreTest, CoreEncryptPartialInvalidRecipientIsReportedTest) {
  auto& repo = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest);
  auto key = GenerateThrowawayRpgpKey("revoked-recp@gpgfrontend.bktus.com");
  ASSERT_TRUE(key != nullptr);
  const auto fpr = key->Fingerprint();

  QString sub_fpr;
  int sub_idx = -1;
  auto s_keys = key->SubKeys();
  for (int i = 0; i < s_keys.size(); ++i) {
    if (!s_keys[i].IsHasCertCap() && s_keys[i].IsHasEncrCap()) {
      sub_fpr = s_keys[i].Fingerprint();
      sub_idx = i;
      break;
    }
  }
  ASSERT_GE(sub_idx, 0);
  ASSERT_TRUE(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                  .RevokeSubkey(key, sub_idx, 0, QString("Test revocation")));
  repo.FlushKeyCache();

  auto pinned = AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                    .GetKey(sub_fpr + "!");
  ASSERT_TRUE(pinned != nullptr);
  auto good = repo.GetPubkeyPtr(kFixtureFpr);
  ASSERT_TRUE(good != nullptr);

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({good, pinned}, GFBuffer(QString("hi")), true);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);

  auto invalid = result.InvalidRecipients();
  ASSERT_FALSE(invalid.empty());
  for (const auto& [inv_fpr, reason] : invalid) {
    EXPECT_NE(gpg_err_code(reason), GPG_ERR_GENERAL)
        << "reason for " << inv_fpr.toStdString() << " was not specific";
  }

  GpgEncryptResultAnalyse analyse{kRpgpChannelForUnitTest, err, result};
  analyse.Analyse();
  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    EXPECT_TRUE(analyse.GetOpInfo().description.contains(
        "Some recipients could not be added"))
        << analyse.GetOpInfo().description.toStdString();
  }
  EXPECT_TRUE(analyse.GetResultReport().contains(std::get<0>(invalid.front())))
      << analyse.GetResultReport().toStdString();

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
      .DeleteKey(repo.GetKeyPtr(fpr));
}

// rPGP's KeyExpired means the key had already expired when it signed, so the
// signature is not valid. It must not read as a good, merely untrusted one.
TEST_F(RpgpCoreTest, CoreVerifyRpgpKeyExpiredAtSigningIsRejectedTest) {
  GFVerifyResult gf_result{};
  gf_result.signatures.push_back(
      MakeRpgpSignature(GFSignatureStatus::kKEY_EXPIRED));

  GpgVerifyResultAnalyse analyse{kRpgpChannelForUnitTest, GPG_ERR_NO_ERROR,
                                 GpgVerifyResult(gf_result)};
  analyse.Analyse();

  EXPECT_LT(analyse.GetStatus(), 0) << analyse.GetResultReport().toStdString();
  ASSERT_EQ(analyse.GetOpInfo().signatures.size(), 1);
  EXPECT_EQ(analyse.GetOpInfo().signatures.front().validity,
            GpgSigValidity::kKEY_EXPIRED);
  EXPECT_TRUE(analyse.GetOpInfo().description.contains("expired"))
      << analyse.GetOpInfo().description.toStdString();
}

// Whether a recipient is post-quantum comes from its subkey's KeyAlgo
// metadata, read off a real rPGP key, not from the algorithm's display name.
TEST_F(RpgpCoreTest, CoreRecipientMixIsReadFromKeyAlgoMetadataTest) {
  auto pq = GeneratePostQuantumRpgpKey("pq@gpgfrontend.bktus.com");
  ASSERT_TRUE(pq != nullptr);
  auto classical = GenerateThrowawayRpgpKey("classic@gpgfrontend.bktus.com");
  ASSERT_TRUE(classical != nullptr);

  bool found_pq_subkey = false;
  for (const auto& s_key : pq->SubKeys()) {
    if (!s_key.IsHasEncrCap()) continue;
    EXPECT_EQ(s_key.AlgoId(), "ky768_cv25519");
    const auto algo = KeyGenerateInfo::SearchKeyAlgoByAlgoId(s_key.AlgoId());
    ASSERT_TRUE(algo.has_value());
    EXPECT_TRUE(algo->IsPostQuantum());
    found_pq_subkey = true;
  }
  EXPECT_TRUE(found_pq_subkey);

  EXPECT_TRUE(RecipientsMixPostQuantumAndClassical({pq, classical}));
  EXPECT_FALSE(RecipientsMixPostQuantumAndClassical({pq}));
  EXPECT_FALSE(RecipientsMixPostQuantumAndClassical({classical}));

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
      .DeleteKeys({pq, classical});
}

// Encrypt+sign gets one engine status for both halves. An unusable key is the
// recipients' fault only when the engine named refused recipients; otherwise
// the signer may not sign, and neither half may blame the recipients.
TEST_F(RpgpCoreTest, CoreEncryptSignNamesTheSideThatMayNotBeUsedTest) {
  GFEncryptResult no_recipient_refused;
  GpgError enc_err = GPG_ERR_UNUSABLE_PUBKEY;
  GpgError sign_err = GPG_ERR_UNUSABLE_SECKEY;
  AttributeUnusableKey(true, no_recipient_refused, enc_err, sign_err);
  EXPECT_EQ(enc_err, GPG_ERR_UNUSABLE_SECKEY);
  EXPECT_EQ(sign_err, GPG_ERR_UNUSABLE_SECKEY);

  GFEncryptResult refused;
  refused.invalid_recipients.push_back({"ABCD", GPG_ERR_CERT_REVOKED});
  AttributeUnusableKey(true, refused, enc_err, sign_err);
  EXPECT_EQ(enc_err, GPG_ERR_UNUSABLE_PUBKEY);
  EXPECT_EQ(sign_err, GPG_ERR_UNUSABLE_PUBKEY);

  // Any other status is left alone.
  GpgError untouched = GPG_ERR_BAD_PASSPHRASE;
  AttributeUnusableKey(false, refused, untouched, sign_err);
  EXPECT_EQ(untouched, GPG_ERR_BAD_PASSPHRASE);
}

// A signature by a key not authorized to sign is invalid for that reason, not
// because the data was tampered with.
TEST_F(RpgpCoreTest, CoreVerifyKeyNotAuthorizedSummaryNamesCauseTest) {
  GFVerifyResult gf_result{};
  gf_result.signatures.push_back(
      MakeRpgpSignature(GFSignatureStatus::kKEY_NOT_AUTHORIZED));

  GpgVerifyResultAnalyse analyse{kRpgpChannelForUnitTest, GPG_ERR_NO_ERROR,
                                 GpgVerifyResult(gf_result)};
  analyse.Analyse();

  const auto description = analyse.GetOpInfo().description;
  EXPECT_LT(analyse.GetStatus(), 0);
  // Modules see only the validity value: it must not read as a bad signature.
  ASSERT_EQ(analyse.GetOpInfo().signatures.size(), 1);
  EXPECT_EQ(analyse.GetOpInfo().signatures.front().validity,
            GpgSigValidity::kKEY_NOT_AUTHORIZED);
  EXPECT_FALSE(description.contains("tampered")) << description.toStdString();
  EXPECT_TRUE(description.contains("not authorized"))
      << description.toStdString();
}

// A recipient or signer missing from the key database fails before the engine
// runs. The GUI still needs a result object to explain why, as it gets on any
// other failure.
TEST_F(RpgpCoreTest, CoreKeyMissingFromDatabaseDeliversResultTest) {
  auto key = GenerateThrowawayRpgpKey("gone@gpgfrontend.bktus.com");
  ASSERT_TRUE(key != nullptr);
  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();

  auto buffer = GFBuffer(QString("Hello RPGP!"));
  {
    auto [err, data_object] =
        MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
            .EncryptSync({key}, buffer, true);
    EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_PUBKEY);
    ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));
    auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
    EXPECT_FALSE(result.InvalidRecipients().empty());
  }
  {
    auto [err, data_object] =
        MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
            .SignSync({key}, buffer, GPGME_SIG_MODE_NORMAL, true);
    EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_SECKEY);
    EXPECT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  }
  {
    auto [err, data_object] =
        MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
            .EncryptSignSync({key}, {key}, buffer, true);
    EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_PUBKEY);
    EXPECT_TRUE(
        (data_object->Check<GpgEncryptResult, GpgSignResult, GFBuffer>()));
  }

  auto in_file = CreateTempFileAndWriteData(buffer);
  {
    auto [err, data_object] =
        FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
            .EncryptFileSync({key}, in_file, true, GetTempFilePath());
    EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_PUBKEY);
    EXPECT_TRUE((data_object->Check<GpgEncryptResult>()));
  }
  {
    auto [err, data_object] =
        FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
            .SignFileSync({key}, in_file, true, GetTempFilePath());
    EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_SECKEY);
    EXPECT_TRUE((data_object->Check<GpgSignResult>()));
  }
  {
    auto [err, data_object] =
        FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
            .EncryptSignFileSync({key}, {key}, in_file, true,
                                 GetTempFilePath());
    EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_PUBKEY);
    EXPECT_TRUE((data_object->Check<GpgEncryptResult, GpgSignResult>()));
  }
}

// When the secret key is missing, file decryption must still name the
// recipients it was encrypted to, as message decryption does.
TEST_F(RpgpCoreTest, CoreFileDecryptWithoutSecretKeyNamesRecipientsTest) {
  auto key = GenerateThrowawayRpgpKey("absent@gpgfrontend.bktus.com");
  ASSERT_TRUE(key != nullptr);

  auto in_file = CreateTempFileAndWriteData(GFBuffer(QString("Hello RPGP!")));
  auto enc_file = GetTempFilePath();
  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptFileSync({key}, in_file, true, enc_file);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();

  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptFileSync(enc_file, GetTempFilePath());
  EXPECT_EQ(CheckGpgError(err_0), GPG_ERR_NO_SECKEY);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult>()));
  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  EXPECT_FALSE(decr_result.Recipients().empty());
}

// Both halves of a combined operation fail together; each must carry the
// engine's reason, not only the half that happened to read it first.
TEST_F(RpgpCoreTest, CoreDecryptVerifyFailureDetailOnBothHalvesTest) {
  auto garbage = GFBuffer(QString("this is definitely not an OpenPGP message"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifySync(garbage);
  ASSERT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object, 0);
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object, 1);
  EXPECT_FALSE(decr_result.ErrorDetail().isEmpty());
  EXPECT_FALSE(verify_result.ErrorDetail().isEmpty());
}

// The signature mode ("Mode" on a sign result card) is reported by the engine
// and must reach the analyser.
TEST_F(RpgpCoreTest, CoreSignResultCarriesSignatureModeTest) {
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetKeyPtr(kFixtureFpr);
  ASSERT_TRUE(sign_key != nullptr);

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({sign_key}, GFBuffer(QString("Hello RPGP!")),
                    GPGME_SIG_MODE_DETACH, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  GpgSignResultAnalyse analyse{kRpgpChannelForUnitTest, err,
                               ExtractParams<GpgSignResult>(data_object, 0)};
  analyse.Analyse();
  ASSERT_FALSE(analyse.GetOpInfo().newSignatures.isEmpty());
  EXPECT_EQ(analyse.GetOpInfo().newSignatures.front().sigMode, "Detach");
}

// A cancel belongs to the batch it was requested for. Once that batch is over,
// later work on the same channel (key edits, module calls) whose wait carries
// no cancel channel of its own must not be skipped as cancelled.
TEST_F(RpgpCoreTest, CancelScopeEndsWithItsBatchTest) {
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
  {
    const GpgOperationCancelScope scope(kRpgpChannelForUnitTest);
    RequestCancelGpgOperation(kRpgpChannelForUnitTest);
    ASSERT_TRUE(IsGpgOperationCancelRequested(kRpgpChannelForUnitTest));
  }
  EXPECT_FALSE(IsGpgOperationCancelRequested(kRpgpChannelForUnitTest));

  // The engine's own per-channel flag is cleared too: a streaming operation
  // after the batch runs to completion.
  auto encrypt_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                         .GetPubkeyPtr(kFixtureFpr);
  ASSERT_TRUE(encrypt_key != nullptr);
  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, GFBuffer(QString("after cancel")), true);
  EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  // A scope without a channel (a non-cancelable wait) leaves a pending cancel
  // alone: it is not its own to clear.
  RequestCancelGpgOperation(kRpgpChannelForUnitTest);
  {
    const GpgOperationCancelScope scope(-1);
  }
  EXPECT_TRUE(IsGpgOperationCancelRequested(kRpgpChannelForUnitTest));
  ResetGpgOperationCancelState(kRpgpChannelForUnitTest);
}

// The engine reports a wrong passphrase as ErrorBadPassphrase. ErrorNoKey for a
// recipient whose secret key is in the database means that key could not be
// used (here: a stored secret block the engine cannot parse), and must not be
// shown as a wrong passphrase.
TEST_F(RpgpCoreTest, CoreDecryptUnusableStoredSecretIsNotBadPassphraseTest) {
  auto encrypt_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                         .GetPubkeyPtr(kFixtureFpr);
  ASSERT_TRUE(encrypt_key != nullptr);
  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, GFBuffer(QString("no key")), true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto cipher = ExtractParams<GFBuffer>(data_object, 1);

  // Keep the metadata (which says the secret is present) but store a block
  // that is no secret key at all.
  auto key_db =
      OpenPGPContext::GetInstance(kRpgpChannelForUnitTest).KeyDatabase();
  ASSERT_TRUE(key_db != nullptr);
  auto meta = key_db->GetKeyMetadata(kFixtureFpr);
  auto blocks = key_db->GetKeyBlocks(kFixtureFpr);
  ASSERT_TRUE(meta.has_value() && blocks.has_value());
  ASSERT_TRUE(meta->has_secret);
  blocks->secret_key = blocks->public_key;
  ASSERT_TRUE(key_db->SaveKey(*meta, *blocks));

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(cipher);
  EXPECT_NE(CheckGpgError(err_0), GPG_ERR_BAD_PASSPHRASE);
  EXPECT_EQ(CheckGpgError(err_0), GPG_ERR_UNUSABLE_SECKEY);
  auto result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_FALSE(result.Recipients().empty());
  for (const auto& rec : result.Recipients()) {
    EXPECT_NE(gpg_err_code(rec.status), GPG_ERR_BAD_PASSPHRASE)
        << rec.keyid.toStdString();
  }
}

// rPGP has no trust model, so a good rPGP signature is valid, but the summary
// must not claim the signer is trusted.
TEST_F(RpgpCoreTest, CoreVerifyRpgpValidSignatureDoesNotClaimTrustTest) {
  GFVerifyResult gf_result{};
  gf_result.signatures.push_back(MakeRpgpSignature(GFSignatureStatus::kVALID));

  GpgVerifyResultAnalyse analyse{kRpgpChannelForUnitTest, GPG_ERR_NO_ERROR,
                                 GpgVerifyResult(gf_result)};
  analyse.Analyse();

  EXPECT_GT(analyse.GetStatus(), 0);
  const auto description = analyse.GetOpInfo().description;
  EXPECT_FALSE(description.contains("trusted")) << description.toStdString();
  EXPECT_TRUE(description.contains("valid")) << description.toStdString();
  EXPECT_FALSE(analyse.GetResultReport().contains("trusted"))
      << analyse.GetResultReport().toStdString();
}

}  // namespace GpgFrontend::Test
