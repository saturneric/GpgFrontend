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
#include "core/GpgModel.h"
#include "core/function/gpg/GpgFileOpera.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreFileEncryptDecrTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance().GetPubkey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(encrypt_key.IsGood());

  auto buffer = GFBuffer(QString("Hello GpgFrontend!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] = GpgFileOpera::GetInstance().EncryptFileSync(
      {encrypt_key}, input_file, true, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrpypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] = GpgFileOpera::GetInstance().DecryptFileSync(
      output_file, decrpypt_output_file);

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(decr_result.Recipients().empty());
  ASSERT_EQ(decr_result.Recipients()[0].keyid, "6A2764F8298DEB29");

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrpypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(GpgCoreTest, CoreFileEncryptDecrBinaryTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance().GetPubkey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  ASSERT_TRUE(encrypt_key.IsGood());

  auto buffer = GFBuffer(QString("Hello GpgFrontend!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] = GpgFileOpera::GetInstance().EncryptFileSync(
      {encrypt_key}, input_file, false, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrpypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] = GpgFileOpera::GetInstance().DecryptFileSync(
      output_file, decrpypt_output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult>()));
  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_FALSE(decr_result.Recipients().empty());
  ASSERT_EQ(decr_result.Recipients()[0].keyid, "6A2764F8298DEB29");

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrpypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(GpgCoreTest, CoreFileEncryptSymmetricDecrTest) {
  auto buffer = GFBuffer(QString("Hello GpgFrontend!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      GpgFileOpera::GetInstance().EncryptFileSymmetricSync(input_file, true,
                                                           output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrpypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] = GpgFileOpera::GetInstance().DecryptFileSync(
      output_file, decrpypt_output_file);

  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult>()));

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_TRUE(decrypt_result.Recipients().empty());

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrpypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(GpgCoreTest, CoreFileEncryptSymmetricDecrBinaryTest) {
  auto buffer = GFBuffer(QString("Hello GpgFrontend!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      GpgFileOpera::GetInstance().EncryptFileSymmetricSync(input_file, false,
                                                           output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrpypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] = GpgFileOpera::GetInstance().DecryptFileSync(
      output_file, decrpypt_output_file);

  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult>()));

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_TRUE(decrypt_result.Recipients().empty());

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrpypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(GpgCoreTest, CoreFileSignVerifyNormalTest) {
  auto sign_key = GpgKeyGetter::GetInstance().GetPubkey(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_TRUE(sign_key.IsGood());

  auto input_file = CreateTempFileAndWriteData("Hello GpgFrontend!");
  auto output_file = GetTempFilePath();

  auto [err, data_object] = GpgFileOpera::GetInstance().SignFileSync(
      {sign_key}, input_file, true, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      GpgFileOpera::GetInstance().VerifyFileSync(input_file, output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
}

TEST_F(GpgCoreTest, CoreFileSignVerifyNormalBinaryTest) {
  auto sign_key = GpgKeyGetter::GetInstance().GetPubkey(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_TRUE(sign_key.IsGood());

  auto input_file = CreateTempFileAndWriteData("Hello GpgFrontend!");
  auto output_file = GetTempFilePath();

  auto [err, data_object] = GpgFileOpera::GetInstance().SignFileSync(
      {sign_key}, input_file, false, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      GpgFileOpera::GetInstance().VerifyFileSync(input_file, output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
}

TEST_F(GpgCoreTest, CoreFileEncryptSignDecrVerifyTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance().GetPubkey(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_TRUE(encrypt_key.IsGood());

  auto sign_key = GpgKeyGetter::GetInstance().GetKey(
      "8933EB283A18995F45D61DAC021D89771B680FFB");
  ASSERT_TRUE(sign_key.IsGood());

  auto buffer = GFBuffer(QString("Hello GpgFrontend!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  ASSERT_TRUE(sign_key.IsPrivateKey());
  ASSERT_TRUE(sign_key.IsHasActualSignCap());

  auto [err, data_object] = GpgFileOpera::GetInstance().EncryptSignFileSync(
      {encrypt_key}, {sign_key}, input_file, true, output_file);

  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GpgSignResult>()));
  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 1);
  ASSERT_TRUE(encr_result.InvalidRecipients().empty());
  ASSERT_TRUE(sign_result.InvalidSigners().empty());
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  auto decrpypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      GpgFileOpera::GetInstance().DecryptVerifyFileSync(output_file,
                                                        decrpypt_output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GpgVerifyResult>()));
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto verify_reult = ExtractParams<GpgVerifyResult>(data_object_0, 1);

  ASSERT_FALSE(decrypt_result.Recipients().empty());
  ASSERT_EQ(decrypt_result.Recipients()[0].keyid, "F89C95A05088CC93");
  ASSERT_FALSE(verify_reult.GetSignature().empty());
  ASSERT_EQ(verify_reult.GetSignature().at(0).GetFingerprint(),
            "8933EB283A18995F45D61DAC021D89771B680FFB");

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrpypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(GpgCoreTest, CoreFileEncryptSignDecrVerifyBinaryTest) {
  auto encrypt_key = GpgKeyGetter::GetInstance().GetPubkey(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  ASSERT_TRUE(encrypt_key.IsGood());
  auto sign_key = GpgKeyGetter::GetInstance().GetKey(
      "8933EB283A18995F45D61DAC021D89771B680FFB");
  ASSERT_TRUE(sign_key.IsGood());

  auto buffer = GFBuffer(QString("Hello GpgFrontend!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  ASSERT_TRUE(sign_key.IsPrivateKey());
  ASSERT_TRUE(sign_key.IsHasActualSignCap());

  auto [err, data_object] = GpgFileOpera::GetInstance().EncryptSignFileSync(
      {encrypt_key}, {sign_key}, input_file, false, output_file);

  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GpgSignResult>()));
  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 1);
  ASSERT_TRUE(encr_result.InvalidRecipients().empty());
  ASSERT_TRUE(sign_result.InvalidSigners().empty());
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  auto decrpypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      GpgFileOpera::GetInstance().DecryptVerifyFileSync(output_file,
                                                        decrpypt_output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GpgVerifyResult>()));
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto verify_reult = ExtractParams<GpgVerifyResult>(data_object_0, 1);

  ASSERT_FALSE(decrypt_result.Recipients().empty());
  ASSERT_EQ(decrypt_result.Recipients()[0].keyid, "F89C95A05088CC93");
  ASSERT_FALSE(verify_reult.GetSignature().empty());
  ASSERT_EQ(verify_reult.GetSignature().at(0).GetFingerprint(),
            "8933EB283A18995F45D61DAC021D89771B680FFB");

  const auto [read_success, out_buffer] =
      ReadFileGFBuffer(decrpypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

}  // namespace GpgFrontend::Test
