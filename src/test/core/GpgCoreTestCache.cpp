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

#include <chrono>
#include <thread>

#include "GFCoreTest.h"
#include "core/function/CacheManager.h"
#include "core/function/DataObjectOperator.h"

namespace GpgFrontend::Test {

namespace {

// Mirrors CacheManager::Impl's private naming scheme. Reproduced here so the
// tests can observe what actually landed on disk, which is the whole point:
// the durable-reset bug was invisible from the in-memory API alone.
constexpr auto kCacheDataPrefix = "__cache_data_";
constexpr auto kCacheRegistryKey = "__cache_manage_data_register_key_list";

auto DataObjectForCacheKey(const QString& key) -> GFBufferOrNone {
  return DataObjectOperator::GetInstance().GetSecDataObject(kCacheDataPrefix +
                                                            key);
}

auto RegistryContains(const QString& key) -> bool {
  auto doc = DataObjectOperator::GetInstance().GetDataObject(kCacheRegistryKey);
  if (!doc.has_value() || !doc->isArray()) return false;

  const auto arr = doc->array();
  return std::any_of(arr.begin(), arr.end(), [&](const auto& v) -> bool {
    return v.toString() == key;
  });
}

}  // namespace

TEST_F(GFCoreTest, CoreCacheTestA) {
  CacheManager::GetInstance().SaveCache("ABC", "DEF");
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABC"), QString("DEF"));
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABCGG"), QString());
}

TEST_F(GFCoreTest, CoreCacheTestB) {
  CacheManager::GetInstance().SaveCache("ABCDE", "DEFG", 3);
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABCDE"), QString("DEFG"));
}

TEST_F(GFCoreTest, CoreCacheTestC) {
  CacheManager::GetInstance().SaveCache("ABCDEF", "DEFEEE", 2);
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABCDEF"), QString("DEFEEE"));
  std::this_thread::sleep_for(std::chrono::milliseconds(4000));
  ASSERT_EQ(CacheManager::GetInstance().LoadCache("ABCDEF"), QString(""));
}

TEST_F(GFCoreTest, DurableCacheSaveReachesDisk) {
  auto& cm = CacheManager::GetInstance();
  const QString key = "durable-reaches-disk";

  cm.SaveSecDurableCache(key, GFBuffer("durable-value"), true);

  auto stored = DataObjectForCacheKey(key);
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(*stored, GFBuffer("durable-value"));
  EXPECT_TRUE(RegistryContains(key));
}

TEST_F(GFCoreTest, DurableCacheOverwriteUpdatesDiskAndKeepsRegistry) {
  auto& cm = CacheManager::GetInstance();
  const QString key = "durable-overwrite";

  cm.SaveSecDurableCache(key, GFBuffer("v1"), true);
  ASSERT_TRUE(RegistryContains(key));

  cm.SaveSecDurableCache(key, GFBuffer("v2"), true);

  auto stored = DataObjectForCacheKey(key);
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(*stored,
            GFBuffer("v2"));  // per-key flush rewrote the changed value
  EXPECT_TRUE(
      RegistryContains(key));  // registry still intact after value-only update
}

TEST_F(GFCoreTest, DurableCacheSkipsWriteWhenValueUnchanged) {
  auto& cm = CacheManager::GetInstance();
  const QString key = "durable-unchanged";

  cm.SaveSecDurableCache(key, GFBuffer("same"), true);
  ASSERT_TRUE(DataObjectForCacheKey(key).has_value());

  // Drop the on-disk object behind the cache's back, then save the identical
  // value again. Because the value is unchanged, the save must be a no-op and
  // must NOT rewrite the object -- so the disk copy stays absent.
  DataObjectOperator::GetInstance().RemoveDataObj(kCacheDataPrefix + key);
  ASSERT_FALSE(DataObjectForCacheKey(key).has_value());

  cm.SaveSecDurableCache(key, GFBuffer("same"), true);
  EXPECT_FALSE(DataObjectForCacheKey(key).has_value());

  // A genuinely different value must still be written through.
  cm.SaveSecDurableCache(key, GFBuffer("different"), true);
  auto stored = DataObjectForCacheKey(key);
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(*stored, GFBuffer("different"));
}

TEST_F(GFCoreTest, ResetDurableCacheRemovesBackingDataObject) {
  auto& cm = CacheManager::GetInstance();
  const QString key = "durable-reset-disk";

  cm.SaveSecDurableCache(key, GFBuffer("secret-to-clear"), true);
  ASSERT_TRUE(DataObjectForCacheKey(key).has_value());

  EXPECT_TRUE(cm.ResetDurableCache(key));

  // the encrypted object must be gone from disk, not just from the in-memory
  // map. this is the assertion that pins the bug: previously the on-disk copy
  // survived and was reloaded on the next start.
  EXPECT_FALSE(DataObjectForCacheKey(key).has_value());
}

TEST_F(GFCoreTest, ResetDurableCacheRemovesKeyFromRegistry) {
  auto& cm = CacheManager::GetInstance();
  const QString key = "durable-reset-registry";

  cm.SaveSecDurableCache(key, GFBuffer("registry-value"), true);
  ASSERT_TRUE(RegistryContains(key));

  EXPECT_TRUE(cm.ResetDurableCache(key));

  // load_all_cache_storage() rebuilds the in-memory map by walking this
  // registry, so a stale entry here resurrects the value on next start.
  EXPECT_FALSE(RegistryContains(key));
}

TEST_F(GFCoreTest, ResetDurableCacheClearsInMemoryValue) {
  auto& cm = CacheManager::GetInstance();
  const QString key = "durable-reset-memory";

  cm.SaveSecDurableCache(key, GFBuffer("in-memory-value"), true);
  ASSERT_EQ(cm.LoadSecDurableCache(key), GFBuffer("in-memory-value"));

  EXPECT_TRUE(cm.ResetDurableCache(key));

  EXPECT_TRUE(cm.LoadSecDurableCache(key).Empty());
  EXPECT_EQ(cm.LoadSecDurableCache(key, GFBuffer("fallback")),
            GFBuffer("fallback"));
}

TEST_F(GFCoreTest, ResetDurableCacheOnMissingKeyIsHarmless) {
  auto& cm = CacheManager::GetInstance();

  EXPECT_NO_FATAL_FAILURE((void)cm.ResetDurableCache("never-saved-key"));
  EXPECT_FALSE(DataObjectForCacheKey("never-saved-key").has_value());
}

}  // namespace GpgFrontend::Test