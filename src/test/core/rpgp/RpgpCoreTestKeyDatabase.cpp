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

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "GpgFrontendTest.h"
#include "core/function/GFKeyDatabase.h"

namespace GpgFrontend::Test {

// The key database stores key material (including secret keys) in plaintext, so
// Init() must lock down the storage directory and database file with OS
// permissions by default: directory owner-only (0700), file owner rw (0600).
TEST(RpgpKeyDatabaseTest, InitRestrictsStoragePermissions) {
  auto db_home = QDir(QDir::tempPath() + "/" + GenerateRandomString(12));
  if (db_home.exists()) db_home.removeRecursively();
  ASSERT_TRUE(db_home.mkpath("."));

  GFKeyDatabase db;
  ASSERT_TRUE(db.Init(db_home.canonicalPath()));

  const auto dir_perms = QFile(db_home.canonicalPath()).permissions();
  // Owner keeps full access.
  EXPECT_TRUE(dir_perms.testFlag(QFileDevice::ReadOwner));
  EXPECT_TRUE(dir_perms.testFlag(QFileDevice::WriteOwner));
  EXPECT_TRUE(dir_perms.testFlag(QFileDevice::ExeOwner));
  // Group and other are denied any access.
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::ReadGroup));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::WriteGroup));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::ExeGroup));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::ReadOther));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::WriteOther));
  EXPECT_FALSE(dir_perms.testFlag(QFileDevice::ExeOther));

  const auto db_file = db_home.canonicalPath() + "/gf_keydb.sqlite";
  ASSERT_TRUE(QFileInfo::exists(db_file));

  const auto file_perms = QFile(db_file).permissions();
  // Owner keeps read/write, everyone else is denied.
  EXPECT_TRUE(file_perms.testFlag(QFileDevice::ReadOwner));
  EXPECT_TRUE(file_perms.testFlag(QFileDevice::WriteOwner));
  EXPECT_FALSE(file_perms.testFlag(QFileDevice::ReadGroup));
  EXPECT_FALSE(file_perms.testFlag(QFileDevice::WriteGroup));
  EXPECT_FALSE(file_perms.testFlag(QFileDevice::ReadOther));
  EXPECT_FALSE(file_perms.testFlag(QFileDevice::WriteOther));

  db_home.removeRecursively();
}

}  // namespace GpgFrontend::Test
