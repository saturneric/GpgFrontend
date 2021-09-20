/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include <gtest/gtest.h>
#include "GpgFrontendTest.h"

#include <string>
#include <vector>

#include "gpg/GpgConstants.h"
#include "gpg/function/BasicOperator.h"
#include "gpg/function/GpgKeyGetter.h"
#include "gpg/model/GpgKey.h"

using namespace GpgFrontend;

TEST_F(GpgCoreTest, CoreEncryptDecrTest) {
  auto encrpyt_key = GpgKeyGetter::GetInstance(default_channel)
                         .GetPubkey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ByteArray encrypt_text = "Hello GpgFrontend!";
  BypeArrayPtr encr_out_data;
  GpgEncrResult e_result;
  std::vector<GpgKey> keys;
  keys.push_back(std::move(encrpyt_key));
  auto err =
      BasicOperator::GetInstance(default_channel)
          .Encrypt(std::move(keys), encrypt_text, encr_out_data, e_result);
  ASSERT_EQ(e_result->invalid_recipients, nullptr);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);

  GpgDecrResult d_result;
  BypeArrayPtr decr_out_data;
  err = BasicOperator::GetInstance(default_channel)
            .Decrypt(*encr_out_data, decr_out_data, d_result);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_NE(d_result->recipients, nullptr);
  ASSERT_EQ(std::string(d_result->recipients->keyid), "F89C95A05088CC93");
  ASSERT_EQ(*decr_out_data, encrypt_text);
}

TEST_F(GpgCoreTest, CoreSignVerifyNormalTest) {
  auto encrpyt_key = GpgKeyGetter::GetInstance(default_channel)
                         .GetPubkey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ByteArray sign_text = "Hello GpgFrontend!";
  BypeArrayPtr sign_out_data;
  GpgSignResult s_result;
  std::vector<GpgKey> keys;
  keys.push_back(std::move(encrpyt_key));
  auto err = BasicOperator::GetInstance(default_channel)
                 .Sign(std::move(keys), sign_text, sign_out_data,
                       GPGME_SIG_MODE_NORMAL, s_result);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(s_result->invalid_signers, nullptr);

  GpgVerifyResult v_result;
  BypeArrayPtr sign_buff = nullptr;
  err = BasicOperator::GetInstance(default_channel)
            .Verify(*sign_out_data, sign_buff, v_result);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_NE(v_result->signatures, nullptr);
  ASSERT_EQ(std::string(v_result->signatures->fpr),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_EQ(v_result->signatures->next, nullptr);
}

TEST_F(GpgCoreTest, CoreSignVerifyDetachTest) {
  auto encrpyt_key = GpgKeyGetter::GetInstance(default_channel)
                         .GetPubkey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ByteArray sign_text = "Hello GpgFrontend!";
  BypeArrayPtr sign_out_data;
  GpgSignResult s_result;
  std::vector<GpgKey> keys;
  keys.push_back(std::move(encrpyt_key));
  auto err = BasicOperator::GetInstance(default_channel)
                 .Sign(std::move(keys), sign_text, sign_out_data,
                       GPGME_SIG_MODE_DETACH, s_result);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(s_result->invalid_signers, nullptr);

  GpgVerifyResult v_result;
  err = BasicOperator::GetInstance(default_channel)
            .Verify(sign_text, sign_out_data, v_result);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_NE(v_result->signatures, nullptr);
  ASSERT_EQ(std::string(v_result->signatures->fpr),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_EQ(v_result->signatures->next, nullptr);
}

TEST_F(GpgCoreTest, CoreSignVerifyClearTest) {
  auto sign_key = GpgKeyGetter::GetInstance(default_channel)
                      .GetKey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ByteArray sign_text = "Hello GpgFrontend!";
  BypeArrayPtr sign_out_data;
  GpgSignResult s_result;
  std::vector<GpgKey> keys;
  keys.push_back(std::move(sign_key));
  auto err = BasicOperator::GetInstance(default_channel)
                 .Sign(std::move(keys), sign_text, sign_out_data,
                       GPGME_SIG_MODE_CLEAR, s_result);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(s_result->invalid_signers, nullptr);

  GpgVerifyResult v_result;
  BypeArrayPtr sign_buff = nullptr;
  err = BasicOperator::GetInstance(default_channel)
            .Verify(*sign_out_data, sign_buff, v_result);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_NE(v_result->signatures, nullptr);
  ASSERT_EQ(std::string(v_result->signatures->fpr),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_EQ(v_result->signatures->next, nullptr);
}

TEST_F(GpgCoreTest, CoreEncryptSignDecrVerifyTest) {
  auto encrpyt_key = GpgKeyGetter::GetInstance(default_channel)
                         .GetPubkey("467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  auto sign_key = GpgKeyGetter::GetInstance(default_channel)
                      .GetKey("8933EB283A18995F45D61DAC021D89771B680FFB");
  //   Question?
  //   ASSERT_FALSE(encrpyt_key.is_private_key());
  ASSERT_TRUE(sign_key.is_private_key());
  ASSERT_TRUE(sign_key.CanSignActual());
  ByteArray encrypt_text = "Hello GpgFrontend!";
  BypeArrayPtr encr_out_data;
  GpgEncrResult e_result;
  GpgSignResult s_result;

  std::vector<GpgKey> keys, sign_keys;
  keys.push_back(std::move(encrpyt_key));
  sign_keys.push_back(std::move(sign_key));

  auto err = BasicOperator::GetInstance(default_channel)
                 .EncryptSign(std::move(keys), std::move(sign_keys),
                              encrypt_text, encr_out_data, e_result, s_result);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(e_result->invalid_recipients, nullptr);
  ASSERT_EQ(s_result->invalid_signers, nullptr);

  GpgDecrResult d_result;
  GpgVerifyResult v_result;
  BypeArrayPtr decr_out_data = nullptr;
  err = BasicOperator::GetInstance(default_channel)
            .DecryptVerify(*encr_out_data, decr_out_data, d_result, v_result);
  ASSERT_EQ(check_gpg_error_2_err_code(err), GPG_ERR_NO_ERROR);
  ASSERT_NE(d_result->recipients, nullptr);
  ASSERT_EQ(std::string(d_result->recipients->keyid), "F89C95A05088CC93");
  ASSERT_EQ(*decr_out_data, encrypt_text);
  ASSERT_NE(v_result->signatures, nullptr);
  ASSERT_EQ(std::string(v_result->signatures->fpr),
            "8933EB283A18995F45D61DAC021D89771B680FFB");
  ASSERT_EQ(v_result->signatures->next, nullptr);
}
