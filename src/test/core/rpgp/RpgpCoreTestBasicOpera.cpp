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

#include "RpgpCoreTest.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

TEST_F(RpgpCoreTest, CoreEncryptDecryptTest) {
  auto encrypt_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                         .GetPubkeyPtr(
                             "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
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
                      .GetPubkeyPtr(
                          "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
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
                      .GetPubkeyPtr(
                          "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
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
                      .GetPubkeyPtr(
                          "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
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
  auto encrypt_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                         .GetPubkeyPtr(
                             "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetKeyPtr(
                          "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
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

}  // namespace GpgFrontend::Test
