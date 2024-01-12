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

#include <gtest/gtest.h>
#include <qeventloop.h>

#include "GpgCoreTest.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/function/result_analyse/GpgResultAnalyse.h"
#include "core/model/GpgGenKeyInfo.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, GenerateKeyTest) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_0");
  keygen_info->SetEmail("bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("");
  keygen_info->SetKeyLength(1024);
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[0]));
  keygen_info->SetNonExpired(true);
  keygen_info->SetNonPassPhrase(true);

  std::atomic_bool callback_called_flag{false};

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
      .GenerateKey(keygen_info, [&callback_called_flag](
                                    GpgError err,
                                    const DataObjectPtr& data_object) {
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

        ASSERT_EQ(data_object->GetObjectSize(), 1);
        ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

        auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
        ASSERT_TRUE(result.IsGood());
        auto fpr = result.GetFingerprint();

        auto key =
            GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).GetKey(fpr);
        ASSERT_TRUE(key.IsGood());

        GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel).DeleteKey(fpr);

        callback_called_flag = true;
        ASSERT_FALSE(fpr.isEmpty());
      });

  int retry_count = 1000;
  while (!callback_called_flag && retry_count-- > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_TRUE(callback_called_flag);
}

TEST_F(GpgCoreTest, GenerateKeyTest_1) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_1");
  keygen_info->SetEmail("bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("hello gpgfrontend");
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[0]));
  keygen_info->SetKeyLength(4096);
  keygen_info->SetNonExpired(false);
  keygen_info->SetExpireTime(boost::posix_time::second_clock::local_time() +
                             boost::posix_time::hours(24));
  keygen_info->SetNonPassPhrase(false);

  std::atomic_bool callback_called_flag{false};

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
      .GenerateKey(keygen_info, [&callback_called_flag](
                                    GpgError err,
                                    const DataObjectPtr& data_object) {
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

        ASSERT_EQ(data_object->GetObjectSize(), 1);
        ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());

        auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
        ASSERT_TRUE(result.IsGood());
        auto fpr = result.GetFingerprint();

        auto key =
            GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).GetKey(fpr);
        ASSERT_TRUE(key.IsGood());

        GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel).DeleteKey(fpr);

        callback_called_flag = true;
        ASSERT_FALSE(fpr.isEmpty());
      });

  int retry_count = 2000;
  while (!callback_called_flag && retry_count-- > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_TRUE(callback_called_flag);
}

TEST_F(GpgCoreTest, GenerateKeyTest_4) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_2");
  keygen_info->SetEmail("bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("");
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[1]));
  keygen_info->SetNonExpired(true);
  keygen_info->SetNonPassPhrase(false);

  std::atomic_bool callback_called_flag{false};

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
      .GenerateKey(keygen_info, [&callback_called_flag](
                                    GpgError err,
                                    const DataObjectPtr& data_object) {
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

        auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
        ASSERT_TRUE(result.IsGood());
        auto fpr = result.GetFingerprint();

        auto key =
            GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).GetKey(fpr);
        ASSERT_TRUE(key.IsGood());

        GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel).DeleteKey(fpr);

        callback_called_flag = true;
        ASSERT_FALSE(fpr.isEmpty());
      });

  int retry_count = 2000;
  while (!callback_called_flag && retry_count-- > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_TRUE(callback_called_flag);
}

TEST_F(GpgCoreTest, GenerateKeyTest_5) {
  auto keygen_info = SecureCreateSharedObject<GenKeyInfo>();
  keygen_info->SetName("foo_3");
  keygen_info->SetEmail("bar@gpgfrontend.bktus.com");
  keygen_info->SetComment("");
  keygen_info->SetAlgo(std::get<1>(keygen_info->GetSupportedKeyAlgo()[2]));
  keygen_info->SetNonExpired(true);
  keygen_info->SetNonPassPhrase(false);

  std::atomic_bool callback_called_flag{false};

  GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel)
      .GenerateKey(keygen_info, [&callback_called_flag](
                                    GpgError err,
                                    const DataObjectPtr& data_object) {
        ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

        auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
        ASSERT_TRUE(result.IsGood());
        auto fpr = result.GetFingerprint();

        auto key =
            GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).GetKey(fpr);
        ASSERT_TRUE(key.IsGood());

        GpgKeyOpera::GetInstance(kGpgFrontendDefaultChannel).DeleteKey(fpr);

        callback_called_flag = true;
        ASSERT_FALSE(fpr.isEmpty());
      });

  int retry_count = 1000;
  while (!callback_called_flag && retry_count-- > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_TRUE(callback_called_flag);
}

}  // namespace GpgFrontend::Test
