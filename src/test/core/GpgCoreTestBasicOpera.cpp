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
#include "core/GpgModel.h"
#include "core/function/basic/ChannelObject.h"
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreEncryptDecrTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance().GetPubkey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  auto encrypt_text = GFBuffer("Hello GpgFrontend!");
  auto const keys = std::vector<GpgKey>{encrypt_key};

  GpgBasicOperator::GetInstance().Encrypt(
      keys, encrypt_text, true,
      [](GpgError err, const DataObjectPtr& data_obj) {
        ASSERT_TRUE((data_obj->Check<GpgEncryptResult, GFBuffer>()));
        auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
        auto encr_out_buffer = ExtractParams<GFBuffer>(data_obj, 1);
        ASSERT_TRUE(result.InvalidRecipients().empty());
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

        GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
            .Decrypt(encr_out_buffer, [](GpgError err,
                                         const DataObjectPtr& data_obj) {
              auto d_result = ExtractParams<GpgDecryptResult>(data_obj, 0);
              auto decr_out_buffer = ExtractParams<GFBuffer>(data_obj, 1);
              ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
              ASSERT_FALSE(d_result.Recipients().empty());
              ASSERT_EQ(d_result.Recipients()[0].keyid, "6A2764F8298DEB29");

              ASSERT_EQ(decr_out_buffer, GFBuffer("Hello GpgFrontend!"));
            });
      });
}

TEST_F(GpgCoreTest, CoreEncryptDecrTest_KeyNotFound_1) {
  auto encr_out_data = GFBuffer(
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
      "-----END PGP MESSAGE-----");

  GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
      .Decrypt(encr_out_data, [=](GpgError err, const DataObjectPtr& data_obj) {
        auto d_result = ExtractParams<GpgDecryptResult>(data_obj, 0);
        auto decr_out_buffer = ExtractParams<GFBuffer>(data_obj, 1);
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_SECKEY);
        ASSERT_FALSE(d_result.Recipients().empty());
        ASSERT_EQ(d_result.Recipients()[0].keyid, "A50CFD2F6C677D8C");
      });
}

TEST_F(GpgCoreTest, CoreEncryptDecrTest_KeyNotFound_ResultAnalyse) {
  auto encr_out_data = GFBuffer(
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
      "-----END PGP MESSAGE-----");

  GpgDecrResult d_result;
  ByteArrayPtr decr_out_data;
  GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
      .Decrypt(encr_out_data, [=](GpgError err, const DataObjectPtr& data_obj) {
        auto d_result = ExtractParams<GpgDecryptResult>(data_obj, 0);
        auto decr_out_buffer = ExtractParams<GFBuffer>(data_obj, 1);
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_SECKEY);
        ASSERT_FALSE(d_result.Recipients().empty());
        ASSERT_EQ(d_result.Recipients()[0].keyid, "A50CFD2F6C677D8C");

        // GpgDecryptResultAnalyse analyse{err, d_result};
        // analyse.Analyse();
        // ASSERT_EQ(analyse.GetStatus(), -1);
        // ASSERT_FALSE(analyse.GetResultReport().empty());
      });
}

TEST_F(GpgCoreTest, CoreSignVerifyNormalTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                         .GetPubkey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ByteArray sign_text = "Hello GpgFrontend!";
  ByteArrayPtr sign_out_data;
  GpgSignResult s_result;
  KeyListPtr keys = std::make_unique<KeyArgsList>();
  keys->push_back(std::move(encrypt_key));
  auto err = GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
                 .Sign(std::move(keys), sign_text, sign_out_data,
                       GPGME_SIG_MODE_NORMAL, s_result);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(s_result->invalid_signers, nullptr);

  GpgVerifyResult v_result;
  ByteArrayPtr sign_buff = nullptr;
  err = GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
            .Verify(*sign_out_data, sign_buff, v_result);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_NE(v_result->signatures, nullptr);
  ASSERT_EQ(std::string(v_result->signatures->fpr),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_EQ(v_result->signatures->next, nullptr);
}

TEST_F(GpgCoreTest, CoreSignVerifyDetachTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                         .GetPubkey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ByteArray sign_text = "Hello GpgFrontend!";
  ByteArrayPtr sign_out_data;
  GpgSignResult s_result;
  KeyListPtr keys = std::make_unique<KeyArgsList>();
  keys->push_back(std::move(encrypt_key));
  auto err = GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
                 .Sign(std::move(keys), sign_text, sign_out_data,
                       GPGME_SIG_MODE_DETACH, s_result);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(s_result->invalid_signers, nullptr);

  GpgVerifyResult v_result;
  err = GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
            .Verify(sign_text, sign_out_data, v_result);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_NE(v_result->signatures, nullptr);
  ASSERT_EQ(std::string(v_result->signatures->fpr),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_EQ(v_result->signatures->next, nullptr);
}

TEST_F(GpgCoreTest, CoreSignVerifyClearTest) {
  auto sign_key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                      .GetKey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ByteArray sign_text = "Hello GpgFrontend!";
  ByteArrayPtr sign_out_data;
  GpgSignResult s_result;
  KeyListPtr keys = std::make_unique<KeyArgsList>();
  keys->push_back(std::move(sign_key));
  auto err = GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
                 .Sign(std::move(keys), sign_text, sign_out_data,
                       GPGME_SIG_MODE_CLEAR, s_result);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(s_result->invalid_signers, nullptr);

  GpgVerifyResult v_result;
  ByteArrayPtr sign_buff = nullptr;
  err = GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
            .Verify(*sign_out_data, sign_buff, v_result);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_NE(v_result->signatures, nullptr);
  ASSERT_EQ(std::string(v_result->signatures->fpr),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_EQ(v_result->signatures->next, nullptr);
}

TEST_F(GpgCoreTest, CoreEncryptSignDecrVerifyTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                         .GetPubkey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  auto sign_key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                      .GetKey("8933EB283A18995F45D61DAC021D89771B680FFB");
  //   Question?
  //   ASSERT_FALSE(encrypt_key.is_private_key());
  ASSERT_TRUE(sign_key.IsPrivateKey());
  ASSERT_TRUE(sign_key.IsHasActualSigningCapability());
  ByteArray encrypt_text = "Hello GpgFrontend!";
  ByteArrayPtr encr_out_data;
  GpgEncrResult e_result;
  GpgSignResult s_result;

  KeyListPtr keys = std::make_unique<KeyArgsList>();
  KeyListPtr sign_keys = std::make_unique<KeyArgsList>();

  keys->push_back(std::move(encrypt_key));
  sign_keys->push_back(std::move(sign_key));

  auto err = GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
                 .EncryptSign(std::move(keys), std::move(sign_keys),
                              encrypt_text, encr_out_data, e_result, s_result);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(e_result->invalid_recipients, nullptr);
  ASSERT_EQ(s_result->invalid_signers, nullptr);

  GpgDecrResult d_result;
  GpgVerifyResult v_result;
  ByteArrayPtr decr_out_data = nullptr;
  err = GpgBasicOperator::GetInstance(kGpgFrontendDefaultChannel)
            .DecryptVerify(*encr_out_data, decr_out_data, d_result, v_result);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_NE(d_result->recipients, nullptr);
  ASSERT_EQ(std::string(d_result->recipients->keyid), "F89C95A05088CC93");
  ASSERT_EQ(*decr_out_data, encrypt_text);
  ASSERT_NE(v_result->signatures, nullptr);
  ASSERT_EQ(std::string(v_result->signatures->fpr),
            "8933EB283A18995F45D61DAC021D89771B680FFB");
  ASSERT_EQ(v_result->signatures->next, nullptr);
}

}  // namespace GpgFrontend::Test
