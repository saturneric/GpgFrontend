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
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreEncryptDecrTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance().GetPubkeyPtr(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello GpgFrontend!"));

  auto [err, data_object] =
      GpgBasicOperator::GetInstance().EncryptSync({encrypt_key}, buffer, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));

  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);

  ASSERT_FALSE(encr_out_buffer.Empty());
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto [err_0, data_object_0] =
      GpgBasicOperator::GetInstance().DecryptSync(encr_out_buffer);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GFBuffer>()));

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object_0, 1);

  ASSERT_FALSE(decr_result.Recipients().empty());
  ASSERT_EQ(decr_result.Recipients()[0].keyid, "6A2764F8298DEB29");
  ASSERT_EQ(decr_out_buffer, buffer);
}

TEST_F(GpgCoreTest, CoreEncryptSymmetricDecrTest) {
  auto encrypt_text = GFBuffer(QString("Hello GpgFrontend!"));
  auto [err, data_object] =
      GpgBasicOperator::GetInstance().EncryptSymmetricSync(encrypt_text, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));
  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(encr_result.InvalidRecipients().empty());

  auto [err_0, data_object_0] =
      GpgBasicOperator::GetInstance().DecryptSync(encr_out_buffer);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GFBuffer>()));
  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object_0, 1);

  ASSERT_TRUE(decr_result.Recipients().empty());
  ASSERT_EQ(decr_out_buffer, encrypt_text);
}

TEST_F(GpgCoreTest, CoreEncryptDecrTest_KeyNotFound_1) {
  auto encr_out_data = GFBuffer(QString(
      "-----BEGIN PGP MESSAGE-----\n"
      "\n"
      "hQEMA6UM/S9sZ32MAQf9Fb6gp6nvgKTQBv2mmjXia6ODXYq6kNeLsPVzLCbHyWOs\n"
      "0GDED11R1NksA3EQxFf4fzLkDpbo68r5bWy7c28c99Fr68IRET19Tw6Gu65MQezD\n"
      "Rdzo1oVqmK9sfKqOT3+0S2H+suFYw5kfBztMZLVGGl9R9fOXdKcj0fqGs2br3e9D\n"
      "ArBFqq07Bae2DD1J8mckWB2x9Uem4vjRiY+vEJcEdAS1N5xu1n7qzzyDgcRcS34X\n"
      "PNBQeTrFMc2RS7mnip2DbyZVEjORobhguK6xZyqXXbvFacStGWDLptV3dcCn4JRO\n"
      "dIORyt5wugqAtgE4qEGTvr/pJ/oXPw4Wve/trece/9I/AR38vW8ntVmDa/hV75iZ\n"
      "4QGAhQ8grD4kq31GHXHUOmBX51XXW9SINmplC8elEx3R460EUZJjjb0OvTih+eZH\n"
      "=8n2H\n"
      "-----END PGP MESSAGE-----"));

  auto [err, data_object] =
      GpgBasicOperator::GetInstance().DecryptSync(encr_out_data);

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object, 0);
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_SECKEY);

  ASSERT_FALSE(decr_result.Recipients().empty());
  ASSERT_EQ(decr_result.Recipients()[0].keyid, "A50CFD2F6C677D8C");
}

TEST_F(GpgCoreTest, CoreEncryptDecrTest_KeyNotFound_ResultAnalyse) {
  auto encr_out_data = GFBuffer(QString(
      "-----BEGIN PGP MESSAGE-----\n"
      "\n"
      "hQEMA6UM/S9sZ32MAQf9Fb6gp6nvgKTQBv2mmjXia6ODXYq6kNeLsPVzLCbHyWOs\n"
      "0GDED11R1NksA3EQxFf4fzLkDpbo68r5bWy7c28c99Fr68IRET19Tw6Gu65MQezD\n"
      "Rdzo1oVqmK9sfKqOT3+0S2H+suFYw5kfBztMZLVGGl9R9fOXdKcj0fqGs2br3e9D\n"
      "ArBFqq07Bae2DD1J8mckWB2x9Uem4vjRiY+vEJcEdAS1N5xu1n7qzzyDgcRcS34X\n"
      "PNBQeTrFMc2RS7mnip2DbyZVEjORobhguK6xZyqXXbvFacStGWDLptV3dcCn4JRO\n"
      "dIORyt5wugqAtgE4qEGTvr/pJ/oXPw4Wve/trece/9I/AR38vW8ntVmDa/hV75iZ\n"
      "4QGAhQ8grD4kq31GHXHUOmBX51XXW9SINmplC8elEx3R460EUZJjjb0OvTih+eZH\n"
      "=8n2H\n"
      "-----END PGP MESSAGE-----"));

  auto [err, data_object] =
      GpgBasicOperator::GetInstance().DecryptSync(encr_out_data);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_SECKEY);
  ASSERT_TRUE((data_object->Check<GpgDecryptResult, GFBuffer>()));

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object, 0);
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);

  ASSERT_FALSE(decr_result.Recipients().empty());
  ASSERT_EQ(decr_result.Recipients()[0].keyid, "A50CFD2F6C677D8C");

  GpgDecryptResultAnalyse analyse{kGpgFrontendDefaultChannel, err, decr_result};
  analyse.Analyse();
  ASSERT_EQ(analyse.GetStatus(), -1);
  ASSERT_FALSE(analyse.GetResultReport().isEmpty());
}

TEST_F(GpgCoreTest, CoreSignVerifyNormalTest) {
  auto sign_key = GpgKeyGetter::GetInstance().GetPubkeyPtr(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_TRUE(sign_key != nullptr);

  auto sign_text = GFBuffer(QString("Hello GpgFrontend!"));

  auto [err, data_object] = GpgBasicOperator::GetInstance().SignSync(
      {sign_key}, sign_text, GPGME_SIG_MODE_NORMAL, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  auto sign_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      GpgBasicOperator::GetInstance().VerifySync(sign_out_buffer, GFBuffer());

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
}

TEST_F(GpgCoreTest, CoreSignVerifyDetachTest) {
  auto sign_key = GpgKeyGetter::GetInstance().GetPubkeyPtr(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_TRUE(sign_key != nullptr);
  auto sign_text = GFBuffer(QString("Hello GpgFrontend!"));

  auto [err, data_object] = GpgBasicOperator::GetInstance().SignSync(
      {sign_key}, sign_text, GPGME_SIG_MODE_DETACH, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  auto sign_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      GpgBasicOperator::GetInstance().VerifySync(sign_text, sign_out_buffer);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
}

TEST_F(GpgCoreTest, CoreSignVerifyClearTest) {
  auto sign_key = GpgKeyGetter::GetInstance().GetPubkeyPtr(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_TRUE(sign_key != nullptr);

  auto sign_text = GFBuffer(QString("Hello GpgFrontend!"));

  auto [err, data_object] = GpgBasicOperator::GetInstance().SignSync(
      {sign_key}, sign_text, GPGME_SIG_MODE_CLEAR, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  auto sign_out_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      GpgBasicOperator::GetInstance().VerifySync(sign_out_buffer, GFBuffer());

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto verify_reult = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_reult.GetSignature().empty());
  ASSERT_EQ(verify_reult.GetSignature().at(0).GetFingerprint(),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
}

TEST_F(GpgCoreTest, CoreEncryptSignDecrVerifyTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance().GetPubkeyPtr(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_TRUE(encrypt_key != nullptr);
  auto sign_key = GpgKeyGetter::GetInstance().GetKeyPtr(
      "8933EB283A18995F45D61DAC021D89771B680FFB");
  ASSERT_TRUE(sign_key != nullptr);
  auto encrypt_text = GFBuffer(QString("Hello GpgFrontend!"));

  ASSERT_TRUE(sign_key->IsPrivateKey());
  ASSERT_TRUE(sign_key->IsHasActualSignCap());

  auto [err, data_object] = GpgBasicOperator::GetInstance().EncryptSignSync(
      {encrypt_key}, {sign_key}, encrypt_text, true);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgEncryptResult, GpgSignResult, GFBuffer>()));
  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 1);
  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 2);
  ASSERT_TRUE(encr_result.InvalidRecipients().empty());
  ASSERT_TRUE(sign_result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      GpgBasicOperator::GetInstance().DecryptVerifySync(encr_out_buffer);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object_0->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto verify_reult = ExtractParams<GpgVerifyResult>(data_object_0, 1);
  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object_0, 2);

  ASSERT_FALSE(decrypt_result.Recipients().empty());
  ASSERT_EQ(decr_out_buffer, encrypt_text);
  ASSERT_EQ(decrypt_result.Recipients()[0].keyid, "F89C95A05088CC93");
  ASSERT_FALSE(verify_reult.GetSignature().empty());
  ASSERT_EQ(verify_reult.GetSignature().at(0).GetFingerprint(),
            "8933EB283A18995F45D61DAC021D89771B680FFB");
}

}  // namespace GpgFrontend::Test
