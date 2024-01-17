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
#include "core/function/gpg/GpgFileOpera.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreFileEncryptDecrTest) {
  std::atomic_bool callback_called_flag{false};

  auto encrypt_key = GpgKeyGetter::GetInstance().GetPubkey(
      "E87C6A2D8D95C818DE93B3AE6A2764F8298DEB29");
  auto input_file = CreateTempFileAndWriteData("Hello GpgFrontend!");
  auto output_file = GetTempFilePath();

  GpgFileOpera::GetInstance().EncryptFile(
      {encrypt_key}, input_file, true, output_file,
      [output_file, &callback_called_flag](GpgError err,
                                           const DataObjectPtr& data_obj) {
        ASSERT_TRUE((data_obj->Check<GpgEncryptResult>()));

        auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
        ASSERT_TRUE(result.InvalidRecipients().empty());
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

        auto decrpypt_output_file = GetTempFilePath();
        GpgFileOpera::GetInstance().DecryptFile(
            output_file, decrpypt_output_file,
            [decrpypt_output_file, &callback_called_flag](
                GpgError err, const DataObjectPtr& data_obj) {
              auto d_result = ExtractParams<GpgDecryptResult>(data_obj, 0);

              ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
              ASSERT_FALSE(d_result.Recipients().empty());
              ASSERT_EQ(d_result.Recipients()[0].keyid, "6A2764F8298DEB29");

              const auto [read_success, buffer] =
                  ReadFileGFBuffer(decrpypt_output_file);
              ASSERT_TRUE(read_success);
              ASSERT_EQ(buffer, GFBuffer(QString("Hello GpgFrontend!")));

              // stop waiting
              callback_called_flag = true;
            });
      });

  int retry_count = 1000;
  while (!callback_called_flag && retry_count-- > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_TRUE(callback_called_flag);
}

TEST_F(GpgCoreTest, CoreFileEncryptSymmetricDecrTest) {
  std::atomic_bool callback_called_flag{false};

  auto input_file = CreateTempFileAndWriteData("Hello GpgFrontend!");
  auto output_file = GetTempFilePath();

  GpgFileOpera::GetInstance().EncryptFileSymmetric(
      input_file, true, output_file,
      [&callback_called_flag, output_file](GpgError err,
                                           const DataObjectPtr& data_obj) {
        ASSERT_TRUE((data_obj->Check<GpgEncryptResult>()));
        auto result = ExtractParams<GpgEncryptResult>(data_obj, 0);
        ASSERT_TRUE(result.InvalidRecipients().empty());
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

        auto decrpypt_output_file = GetTempFilePath();
        GpgFileOpera::GetInstance().DecryptFile(
            output_file, decrpypt_output_file,
            [&callback_called_flag, decrpypt_output_file](
                GpgError err, const DataObjectPtr& data_obj) {
              ASSERT_TRUE((data_obj->Check<GpgDecryptResult>()));

              auto decrypt_result =
                  ExtractParams<GpgDecryptResult>(data_obj, 0);
              ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
              ASSERT_TRUE(decrypt_result.Recipients().empty());

              const auto [read_success, buffer] =
                  ReadFileGFBuffer(decrpypt_output_file);
              ASSERT_TRUE(read_success);
              ASSERT_EQ(buffer, GFBuffer(QString("Hello GpgFrontend!")));

              // stop waiting
              callback_called_flag = true;
            });
      });

  int retry_count = 1000;
  while (!callback_called_flag && retry_count-- > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_TRUE(callback_called_flag);
}

TEST_F(GpgCoreTest, CoreFileSignVerifyNormalTest) {
  std::atomic_bool callback_called_flag{false};

  auto sign_key = GpgKeyGetter::GetInstance().GetPubkey(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  auto input_file = CreateTempFileAndWriteData("Hello GpgFrontend!");
  auto output_file = GetTempFilePath();

  GpgFileOpera::GetInstance().SignFile(
      {sign_key}, input_file, true, output_file,
      [&callback_called_flag, input_file, output_file](
          GpgError err, const DataObjectPtr& data_obj) {
        ASSERT_TRUE((data_obj->Check<GpgSignResult>()));
        auto result = ExtractParams<GpgSignResult>(data_obj, 0);
        ASSERT_TRUE(result.InvalidSigners().empty());
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

        GpgFileOpera::GetInstance().VerifyFile(
            input_file, output_file,
            [&callback_called_flag](GpgError err,
                                    const DataObjectPtr& data_obj) {
              auto d_result = ExtractParams<GpgVerifyResult>(data_obj, 0);
              ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
              ASSERT_FALSE(d_result.GetSignature().empty());
              ASSERT_EQ(d_result.GetSignature().at(0).GetFingerprint(),
                        "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");

              // stop waiting
              callback_called_flag = true;
            });
      });

  int retry_count = 1000;
  while (!callback_called_flag && retry_count-- > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_TRUE(callback_called_flag);
}

TEST_F(GpgCoreTest, CoreFileEncryptSignDecrVerifyTest) {
  std::atomic_bool callback_called_flag{false};

  auto encrypt_key = GpgKeyGetter::GetInstance().GetPubkey(
      "467F14220CE8DCF780CF4BAD8465C55B25C9B7D1");
  auto sign_key = GpgKeyGetter::GetInstance().GetKey(
      "8933EB283A18995F45D61DAC021D89771B680FFB");
  auto input_file = CreateTempFileAndWriteData("Hello GpgFrontend!");
  auto output_file = GetTempFilePath();

  ASSERT_TRUE(sign_key.IsPrivateKey());
  ASSERT_TRUE(sign_key.IsHasActualSigningCapability());

  GpgFileOpera::GetInstance().EncryptSignFile(
      {encrypt_key}, {sign_key}, input_file, true, output_file,
      [&callback_called_flag, output_file](GpgError err,
                                           const DataObjectPtr& data_obj) {
        ASSERT_TRUE((data_obj->Check<GpgEncryptResult, GpgSignResult>()));
        auto encr_result = ExtractParams<GpgEncryptResult>(data_obj, 0);
        auto sign_result = ExtractParams<GpgSignResult>(data_obj, 1);
        ASSERT_TRUE(encr_result.InvalidRecipients().empty());
        ASSERT_TRUE(sign_result.InvalidSigners().empty());
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

        auto decrpypt_output_file = GetTempFilePath();
        GpgFileOpera::GetInstance().DecryptVerifyFile(
            output_file, decrpypt_output_file,
            [&callback_called_flag, decrpypt_output_file](
                GpgError err, const DataObjectPtr& data_obj) {
              ASSERT_TRUE(
                  (data_obj->Check<GpgDecryptResult, GpgVerifyResult>()));
              auto decrypt_result =
                  ExtractParams<GpgDecryptResult>(data_obj, 0);
              auto verify_reult = ExtractParams<GpgVerifyResult>(data_obj, 1);

              ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

              ASSERT_FALSE(decrypt_result.Recipients().empty());
              ASSERT_EQ(decrypt_result.Recipients()[0].keyid,
                        "F89C95A05088CC93");
              ASSERT_FALSE(verify_reult.GetSignature().empty());
              ASSERT_EQ(verify_reult.GetSignature().at(0).GetFingerprint(),
                        "8933EB283A18995F45D61DAC021D89771B680FFB");

              const auto [read_success, buffer] =
                  ReadFileGFBuffer(decrpypt_output_file);
              ASSERT_TRUE(read_success);
              ASSERT_EQ(buffer, GFBuffer(QString("Hello GpgFrontend!")));

              // stop waiting
              callback_called_flag = true;
            });
      });

  int retry_count = 1000;
  while (!callback_called_flag && retry_count-- > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_TRUE(callback_called_flag);
}

}  // namespace GpgFrontend::Test
