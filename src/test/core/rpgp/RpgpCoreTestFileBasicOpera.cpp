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
#include "core/function/openpgp/FileCryptoOperation.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

TEST_F(RpgpCoreTest, CoreFileEncryptDecryptTest) {
  auto encrypt_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                         .GetPubkeyPtr(
                             "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP File!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptFileSync({encrypt_key}, input_file, true, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptFileSync(output_file, decrypt_output_file);

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(decr_result.Recipients().empty());

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(RpgpCoreTest, CoreFileEncryptDecryptBinaryTest) {
  auto encrypt_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                         .GetPubkeyPtr(
                             "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP File Binary!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptFileSync({encrypt_key}, input_file, false, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptFileSync(output_file, decrypt_output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult>()));
  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_FALSE(decr_result.Recipients().empty());

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(RpgpCoreTest, CoreFileEncryptSymmetricDecryptTest) {
  auto buffer = GFBuffer(QString("Hello RPGP Symmetric File!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptFileSymmetricSync(input_file, true, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptFileSync(output_file, decrypt_output_file);

  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult>()));
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_TRUE(decrypt_result.Recipients().empty());

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(RpgpCoreTest, CoreFileSignVerifyNormalTest) {
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetPubkeyPtr(
                          "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);

  auto input_file = CreateTempFileAndWriteData("Hello RPGP File Sign!");
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignFileSync({sign_key}, input_file, true, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifyFileSync(input_file, output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "575572EF0DF799AB884EC6C114C6B0B1596A2755");
}

TEST_F(RpgpCoreTest, CoreFileEncryptSignDecryptVerifyTest) {
  auto encrypt_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                         .GetPubkeyPtr(
                             "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetKeyPtr(
                          "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP File Encrypt Sign!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  ASSERT_TRUE(sign_key->IsPrivateKey());
  ASSERT_TRUE(sign_key->IsHasActualSignCap());

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSignFileSync({encrypt_key}, {sign_key}, input_file, true,
                               output_file);

  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GpgSignResult>()));
  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 1);
  ASSERT_TRUE(encr_result.InvalidRecipients().empty());
  ASSERT_TRUE(sign_result.InvalidSigners().empty());
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  auto decrypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifyFileSync(output_file, decrypt_output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GpgVerifyResult>()));
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 1);

  ASSERT_FALSE(decrypt_result.Recipients().empty());
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "575572EF0DF799AB884EC6C114C6B0B1596A2755");

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

}  // namespace GpgFrontend::Test
