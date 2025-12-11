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
#include "core/GpgConstants.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/function/gpg/GpgKeyManager.h"
#include "core/function/gpg/GpgKeyOpera.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/GpgUtils.h"

static const char *test_private_key_data = R"(
-----BEGIN PGP PRIVATE KEY BLOCK-----

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
=OHnq
-----END PGP PRIVATE KEY BLOCK-----
)";

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreDeleteSubkeyTestA) {
  auto info =
      GpgKeyImportExporter::GetInstance(kGpgFrontendDefaultChannel)
          .ImportKey(GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key != nullptr);

  auto s_key = key->SubKeys();

  ASSERT_EQ(s_key.size(), 5);
  ASSERT_EQ(s_key[2].ID(), "2D1F9FC59B568A8C");

  ASSERT_WITHIN(GpgKeyManager::GetInstance().DeleteSubkey(key, 2), 3000);

  GpgKeyGetter::GetInstance().FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key != nullptr);

  s_key = key->SubKeys();

  ASSERT_EQ(s_key.size(), 4);
  ASSERT_EQ(s_key[2].ID(), "CE038203C4D03C3D");

  GpgKeyOpera::GetInstance().DeleteKey(key);
}

TEST_F(GpgCoreTest, CoreSetOwnerTrustA) {
  auto info = GpgKeyImportExporter::GetInstance().ImportKey(
      GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key != nullptr);

  ASSERT_WITHIN(GpgKeyManager::GetInstance().SetOwnerTrustLevel(key, 1), 3000);

  GpgKeyGetter::GetInstance().FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key != nullptr);

  // why?
  ASSERT_EQ(key->OwnerTrustLevel(), 0);

  ASSERT_WITHIN(GpgKeyManager::GetInstance().SetOwnerTrustLevel(key, 2), 3000);

  GpgKeyGetter::GetInstance().FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key != nullptr);

  ASSERT_EQ(key->OwnerTrustLevel(), 2);

  ASSERT_WITHIN(GpgKeyManager::GetInstance().SetOwnerTrustLevel(key, 3), 3000);

  GpgKeyGetter::GetInstance().FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key != nullptr);

  ASSERT_EQ(key->OwnerTrustLevel(), 3);

  ASSERT_WITHIN(GpgKeyManager::GetInstance().SetOwnerTrustLevel(key, 4), 3000);

  GpgKeyGetter::GetInstance().FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key != nullptr);

  ASSERT_EQ(key->OwnerTrustLevel(), 4);

  ASSERT_WITHIN(GpgKeyManager::GetInstance().SetOwnerTrustLevel(key, 5), 3000);

  GpgKeyGetter::GetInstance().FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key->IsGood());

  ASSERT_EQ(key->OwnerTrustLevel(), 5);

  ASSERT_FALSE(GpgKeyManager::GetInstance().SetOwnerTrustLevel(key, 0));

  ASSERT_FALSE(GpgKeyManager::GetInstance().SetOwnerTrustLevel(key, -1));
  ASSERT_FALSE(GpgKeyManager::GetInstance().SetOwnerTrustLevel(key, 6));

  GpgKeyOpera::GetInstance().DeleteKey(key);
}

TEST_F(GpgCoreTest, CoreRevokeSubkeyTestA) {
  auto info = GpgKeyImportExporter::GetInstance().ImportKey(
      GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key != nullptr);

  auto s_key = key->SubKeys();

  ASSERT_EQ(s_key.size(), 5);
  ASSERT_EQ(s_key[2].ID(), "2D1F9FC59B568A8C");

  ASSERT_WITHIN(GpgKeyManager::GetInstance().RevokeSubkey(
                    key, 2, 2, QString("H\nE\nLL\nO\n\n")),
                3000);

  GpgKeyGetter::GetInstance().FlushKeyCache();
  key = GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("822D7E13F5B85D7D");
  ASSERT_TRUE(key != nullptr);

  s_key = key->SubKeys();

  ASSERT_EQ(s_key.size(), 5);
  ASSERT_EQ(s_key[2].ID(), "2D1F9FC59B568A8C");

  ASSERT_TRUE(s_key[2].IsRevoked());

  GpgKeyOpera::GetInstance().DeleteKey(key);
}

}  // namespace GpgFrontend::Test