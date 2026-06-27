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

#include "GpgCoreEngineTest.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

// ---------------------------------------------------------------------------
// Key generation
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, GenerateKeyWithSubkey) {
  auto key = GenerateFullKey("genkey");
  ASSERT_TRUE(key != nullptr);

  EXPECT_TRUE(key->IsGood());
  EXPECT_TRUE(key->IsPrivateKey());
  EXPECT_TRUE(key->IsHasActualSignCap());
  EXPECT_TRUE(key->IsHasActualEncrCap());
  EXPECT_FALSE(key->SubKeys().empty());

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, GenerateSubkey) {
  auto key = GenerateFullKey("addsubkey");
  ASSERT_TRUE(key != nullptr);
  auto original_size = key->SubKeys().size();

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);
  s_info->SetAlgo(PickEncryptionSubAlgo());
  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(Channel())
          .GenerateSubkeySync(key, s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());
  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());

  GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
  auto reloaded =
      GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(key->Fingerprint());
  ASSERT_TRUE(reloaded != nullptr);
  EXPECT_EQ(reloaded->SubKeys().size(), original_size + 1);

  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Encrypt / Decrypt
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, EncryptDecrypt) {
  auto key = GenerateFullKey("encdec");
  ASSERT_TRUE(key != nullptr);

  auto plain = GFBuffer(QString("Hello GpgFrontend engine independent test!"));

  auto [err, data_object] = MessageCryptoOperation::GetInstance(Channel())
                                .EncryptSync({key}, plain, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));
  auto enc_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto cipher = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(enc_result.InvalidRecipients().empty());
  ASSERT_FALSE(cipher.Empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(Channel()).DecryptSync(cipher);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GFBuffer>()));
  auto out = ExtractParams<GFBuffer>(data_object_0, 1);
  ASSERT_EQ(out, plain);

  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Sign / Verify
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, SignVerifyDetached) {
  auto key = GenerateFullKey("signverify");
  ASSERT_TRUE(key != nullptr);

  auto text = GFBuffer(QString("Hello GpgFrontend engine independent test!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(Channel())
          .SignSync({key}, text, GPGME_SIG_MODE_DETACH, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 0);
  auto signature = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(sign_result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(Channel())
          .VerifySync(text, signature);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  EXPECT_EQ(verify_result.GetSignature().at(0).GetFingerprint().toUpper(),
            key->Fingerprint().toUpper());

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, SignVerifyInline) {
  auto key = GenerateFullKey("signverify_inline");
  ASSERT_TRUE(key != nullptr);

  auto text = GFBuffer(QString("Hello GpgFrontend engine independent test!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(Channel())
          .SignSync({key}, text, GPGME_SIG_MODE_NORMAL, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 0);
  auto signed_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(sign_result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(Channel())
          .VerifySync(signed_buffer, GFBuffer());
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  EXPECT_EQ(verify_result.GetSignature().at(0).GetFingerprint().toUpper(),
            key->Fingerprint().toUpper());

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, EncryptSignDecryptVerify) {
  auto key = GenerateFullKey("encsign");
  ASSERT_TRUE(key != nullptr);

  auto plain = GFBuffer(QString("Hello GpgFrontend engine independent test!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(Channel())
          .EncryptSignSync({key}, {key}, plain, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgEncryptResult, GpgSignResult, GFBuffer>()));
  auto cipher = ExtractParams<GFBuffer>(data_object, 2);

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(Channel()).DecryptVerifySync(cipher);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object_0->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 1);
  auto out = ExtractParams<GFBuffer>(data_object_0, 2);
  ASSERT_EQ(out, plain);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  EXPECT_EQ(verify_result.GetSignature().at(0).GetFingerprint().toUpper(),
            key->Fingerprint().toUpper());

  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Stress: repeat the full encrypt -> decrypt -> sign -> verify cycle.
// Bump the iteration count with the GF_STRESS_ITER environment variable, e.g.
//   GF_STRESS_ITER=10000 ./GpgFrontendTest --gtest_filter=*RPGP*Stress*
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, Stress) {
  auto key = GenerateFullKey("stress");
  ASSERT_TRUE(key != nullptr);

  const int iterations = StressIterations();
  for (int i = 0; i < iterations; ++i) {
    auto plain = GFBuffer(GenerateRandomString(256));

    auto [enc_err, enc_obj] = MessageCryptoOperation::GetInstance(Channel())
                                  .EncryptSync({key}, plain, true);
    ASSERT_EQ(CheckGpgError(enc_err), GPG_ERR_NO_ERROR)
        << "encrypt failed at iteration " << i;
    auto cipher = ExtractParams<GFBuffer>(enc_obj, 1);

    auto [dec_err, dec_obj] =
        MessageCryptoOperation::GetInstance(Channel()).DecryptSync(cipher);
    ASSERT_EQ(CheckGpgError(dec_err), GPG_ERR_NO_ERROR)
        << "decrypt failed at iteration " << i;
    auto decrypted = ExtractParams<GFBuffer>(dec_obj, 1);
    ASSERT_EQ(decrypted, plain) << "roundtrip mismatch at iteration " << i;

    auto [sign_err, sign_obj] =
        MessageCryptoOperation::GetInstance(Channel())
            .SignSync({key}, plain, GPGME_SIG_MODE_DETACH, true);
    ASSERT_EQ(CheckGpgError(sign_err), GPG_ERR_NO_ERROR)
        << "sign failed at iteration " << i;
    auto signature = ExtractParams<GFBuffer>(sign_obj, 1);

    auto [verify_err, verify_obj] =
        MessageCryptoOperation::GetInstance(Channel())
            .VerifySync(plain, signature);
    ASSERT_EQ(CheckGpgError(verify_err), GPG_ERR_NO_ERROR)
        << "verify failed at iteration " << i;
  }

  DeleteKey(key);
}

}  // namespace GpgFrontend::Test
