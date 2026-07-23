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
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"
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
            .EncryptSync({repo.GetPubkeyPtr(
                             "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497")},
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

  // The revoked subkey must never be used as a recipient. With no other
  // encryption subkey and a certify-only primary, the recipient is invalid.
  auto pub = repo.GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(pub != nullptr);
  auto [err2, dobj2] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({pub}, GFBuffer(QString("hi")), true);

  if (CheckGpgError(err2) == GPG_ERR_NO_ERROR &&
      dobj2->Check<GpgEncryptResult, GFBuffer>()) {
    auto r2 = ExtractParams<GpgEncryptResult>(dobj2, 0);
    for (const auto& rec : r2.Recipients()) {
      EXPECT_NE(rec.keyid, sub_keyid);
    }
  } else {
    // No usable encryption key remained: the operation must have failed.
    EXPECT_NE(CheckGpgError(err2), GPG_ERR_NO_ERROR);
  }
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

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR &&
      dobj->Check<GpgEncryptResult, GFBuffer>()) {
    auto r = ExtractParams<GpgEncryptResult>(dobj, 0);
    for (const auto& rec : r.Recipients()) {
      EXPECT_NE(rec.keyid, sub_keyid);
    }
    EXPECT_FALSE(r.InvalidRecipients().empty());
  } else {
    EXPECT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
  }
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
// good sign/verify as "failed" (status -1). These analysers are exactly what the
// SDK's analyse-by-capsule path drives, so a non-negative status here is the
// guarantee that path relies on.
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

}  // namespace GpgFrontend::Test
