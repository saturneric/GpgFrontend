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
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/utils/GpgUtils.h"

static const char* test_private_key_data = R"(
-----BEGIN PGP PRIVATE KEY BLOCK-----

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
=7+Wn
-----END PGP PRIVATE KEY BLOCK-----
)";

namespace GpgFrontend::Test {

TEST_F(RpgpCoreTest, CoreDeleteSubkeyTest) {
  auto info =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ImportKey(GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_TRUE(info != nullptr);
  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  auto s_keys = key->SubKeys();
  auto original_size = s_keys.size();
  ASSERT_GE(original_size, 2);

  ASSERT_TRUE(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                  .DeleteSubkey(key, original_size - 1));

  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
  key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
            .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  s_keys = key->SubKeys();
  ASSERT_EQ(s_keys.size(), static_cast<size_t>(original_size - 1));
}

TEST_F(RpgpCoreTest, CoreRevokeSubkeyTest) {
  auto info =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ImportKey(GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_TRUE(info != nullptr);
  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  auto s_keys = key->SubKeys();
  ASSERT_GE(s_keys.size(), 2);

  ASSERT_TRUE(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                  .RevokeSubkey(key, 1, 0, QString("Test revocation")));

  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
  key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
            .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  s_keys = key->SubKeys();
  ASSERT_GE(s_keys.size(), 2);
  ASSERT_TRUE(s_keys[1].IsRevoked());
}

}  // namespace GpgFrontend::Test
