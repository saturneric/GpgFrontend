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

#include "GFCoreTest.h"
#include "core/struct/settings_object/KeyDatabaseItemSO.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

auto MakeItem(const QString& name, const QString& path,
              const QString& backend_type = {}, int channel = 0)
    -> KeyDatabaseItemSO {
  KeyDatabaseItemSO item;
  item.name = name;
  item.path = path;
  item.backend_type = backend_type;
  item.channel = channel;
  return item;
}

auto FindByName(const QContainer<KeyDatabaseItemSO>& list, const QString& name)
    -> const KeyDatabaseItemSO* {
  for (const auto& item : list) {
    if (item.name == name) return &item;
  }
  return nullptr;
}

const KeyDatabaseItemSO kDefaultDb =
    MakeItem("DEFAULT", "/app-data/rpgp_db", "rpgp", 0);

const QSet<QString> kLiteBackends = {"rpgp"};         // macOS lite build
const QSet<QString> kFullBackends = {"gnupg", "rpgp"};  // Flathub build

}  // namespace

// With nothing on disk and no stored settings, only the channel-0 DEFAULT
// database survives.
TEST_F(GFCoreTest, KeyDatabaseReconcileEmpty) {
  auto result = ReconcileSandboxKeyDatabaseList(kDefaultDb, {}, {}, kLiteBackends);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].name, QString("DEFAULT"));
  EXPECT_EQ(result[0].channel, 0);
  EXPECT_EQ(result[0].path, QString("/app-data/rpgp_db"));
  EXPECT_EQ(result[0].backend_type, QString("rpgp"));
}

// Databases discovered on disk but absent from settings get sequential channels
// and the default supported backend.
TEST_F(GFCoreTest, KeyDatabaseReconcileDiscoverNew) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("alpha", "/app-data/dbs/alpha"),
      MakeItem("beta", "/app-data/dbs/beta"),
  };

  auto result =
      ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, {}, kLiteBackends);

  ASSERT_EQ(result.size(), 3);

  const auto* alpha = FindByName(result, "alpha");
  const auto* beta = FindByName(result, "beta");
  ASSERT_NE(alpha, nullptr);
  ASSERT_NE(beta, nullptr);

  EXPECT_EQ(alpha->path, QString("/app-data/dbs/alpha"));
  EXPECT_EQ(alpha->backend_type, QString("rpgp"));
  EXPECT_EQ(beta->backend_type, QString("rpgp"));

  // channels are unique and ascending, DEFAULT stays at 0
  EXPECT_EQ(FindByName(result, "DEFAULT")->channel, 0);
  EXPECT_NE(alpha->channel, beta->channel);
  EXPECT_NE(alpha->channel, 0);
  EXPECT_NE(beta->channel, 0);
}

// Metadata (backend type + channel) is recovered from settings by name for a
// database that still exists on disk.
TEST_F(GFCoreTest, KeyDatabaseReconcileRecoverMetadata) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("work", "/app-data/dbs/work"),
  };
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("work", "/stale/old/path", "rpgp", 5),
  };

  auto result = ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, stored,
                                                kLiteBackends);

  const auto* work = FindByName(result, "work");
  ASSERT_NE(work, nullptr);
  // path comes from the scan, not the stale stored value
  EXPECT_EQ(work->path, QString("/app-data/dbs/work"));
  EXPECT_EQ(work->backend_type, QString("rpgp"));
  EXPECT_EQ(work->channel, 5);
}

// A stored database whose directory no longer exists on disk is dropped.
TEST_F(GFCoreTest, KeyDatabaseReconcileDropMissing) {
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("ghost", "/app-data/dbs/ghost", "rpgp", 1),
  };

  auto result =
      ReconcileSandboxKeyDatabaseList(kDefaultDb, {}, stored, kLiteBackends);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(FindByName(result, "ghost"), nullptr);
  EXPECT_NE(FindByName(result, "DEFAULT"), nullptr);
}

// In the rpgp-only lite build a stale "gnupg" backend type carried over in
// settings must be replaced with a supported backend.
TEST_F(GFCoreTest, KeyDatabaseReconcileUnsupportedBackendLite) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("legacy", "/app-data/dbs/legacy"),
  };
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("legacy", "/app-data/dbs/legacy", "gnupg", 2),
  };

  auto result = ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, stored,
                                                kLiteBackends);

  const auto* legacy = FindByName(result, "legacy");
  ASSERT_NE(legacy, nullptr);
  EXPECT_EQ(legacy->backend_type, QString("rpgp"));
}

// When gnupg is available (full build), a stored gnupg type is honoured and the
// default database prefers gnupg.
TEST_F(GFCoreTest, KeyDatabaseReconcileSupportedBackendFull) {
  auto default_db = MakeItem("DEFAULT", "/app-data/db", "gnupg", 0);
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("legacy", "/app-data/dbs/legacy"),
  };
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("legacy", "/app-data/dbs/legacy", "gnupg", 2),
  };

  auto result = ReconcileSandboxKeyDatabaseList(default_db, discovered, stored,
                                                kFullBackends);

  EXPECT_EQ(FindByName(result, "DEFAULT")->backend_type, QString("gnupg"));
  EXPECT_EQ(FindByName(result, "legacy")->backend_type, QString("gnupg"));
}

// A directory accidentally named "DEFAULT" must not shadow the channel-0
// default database.
TEST_F(GFCoreTest, KeyDatabaseReconcileNoDefaultShadow) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("DEFAULT", "/app-data/dbs/DEFAULT"),
  };

  auto result =
      ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, {}, kLiteBackends);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].path, QString("/app-data/rpgp_db"));
  EXPECT_EQ(result[0].channel, 0);
}

// Duplicate channels (DEFAULT at 0 plus a stored entry also at 0) are resolved
// to unique, ascending channels.
TEST_F(GFCoreTest, KeyDatabaseReconcileChannelCollision) {
  QContainer<KeyDatabaseItemSO> discovered{
      MakeItem("dup", "/app-data/dbs/dup"),
  };
  QContainer<KeyDatabaseItemSO> stored{
      MakeItem("dup", "/app-data/dbs/dup", "rpgp", 0),
  };

  auto result =
      ReconcileSandboxKeyDatabaseList(kDefaultDb, discovered, stored, kLiteBackends);

  ASSERT_EQ(result.size(), 2);
  EXPECT_NE(result[0].channel, result[1].channel);
  EXPECT_EQ(FindByName(result, "DEFAULT")->channel, 0);
  EXPECT_GT(FindByName(result, "dup")->channel, 0);
}

}  // namespace GpgFrontend::Test
