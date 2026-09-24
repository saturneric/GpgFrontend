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

#include <QByteArray>
#include <array>
#include <optional>

#include "GpgFrontendTest.h"
#include "core/SdkTestContext.h"
#include "core/function/CacheManager.h"
#include "sdk/GFSDK.hpp"
#include "sdk/GFSDKBuffer.hpp"
#include "sdk/GFSDKStorage.h"

/**
 * @file GpgCoreTestSdkStorage.cpp
 * @brief The module cache stores, through the public SDK.
 *
 * Each of these pins a promise the C header makes: values are octets, an
 * absent key reads as absent, and a key belongs to one module and one store.
 */

namespace GpgFrontend::Test {

namespace {

auto CtxA() -> GFSDKContext* {
  static SdkTestContext context("com.example.sdk.storage.a");
  return context.get();
}

auto CtxB() -> GFSDKContext* {
  static SdkTestContext context("com.example.sdk.storage.b");
  return context.get();
}

constexpr std::array<int, 3> kStores = {GF_STORE_SESSION, GF_STORE_DURABLE,
                                        GF_STORE_SECURE_DURABLE};

auto Set(GFSDKContext* ctx, int store, const char* key, const QByteArray& value)
    -> int {
  auto buf = GFBuf::Copy(ctx, value);
  return GFStorageCacheSet(ctx, store, key, buf.View(), 0);
}

/// The value, or std::nullopt when the store reports it absent.
auto Get(GFSDKContext* ctx, int store, const char* key)
    -> std::optional<QByteArray> {
  GFBuf out(ctx);
  if (GFStorageCacheGet(ctx, store, key, out.Out()) != 0) return std::nullopt;
  return QByteArray(out.Data(), static_cast<qsizetype>(out.Size()));
}

}  // namespace

TEST(SdkStorageTest, ValuesAreOctetsInEveryStore) {
  const QByteArray value(
      "a\0b\xff\xfe"
      "c",
      6);
  for (const auto store : kStores) {
    ASSERT_EQ(Set(CtxA(), store, "octets", value), 0) << "store " << store;
    const auto got = Get(CtxA(), store, "octets");
    ASSERT_TRUE(got.has_value()) << "store " << store;
    EXPECT_EQ(*got, value) << "store " << store;
    EXPECT_EQ(GFStorageCacheRemove(CtxA(), store, "octets"), 0);
  }
}

// The durable store used to parse its value as JSON and store nothing when
// that failed, so plain text vanished.
TEST(SdkStorageTest, TheDurableStoreKeepsPlainText) {
  gf::sdk::SetCacheText(CtxA(), GF_STORE_DURABLE, "plain", "not json");
  EXPECT_EQ(gf::sdk::CacheText(CtxA(), GF_STORE_DURABLE, "plain"),
            QString("not json"));
  gf::sdk::RemoveCache(CtxA(), GF_STORE_DURABLE, "plain");
}

TEST(SdkStorageTest, AnAbsentKeyReadsAsAbsentInEveryStore) {
  for (const auto store : kStores) {
    EXPECT_FALSE(Get(CtxA(), store, "never-written").has_value())
        << "store " << store;
  }
}

TEST(SdkStorageTest, RemovingAKeyMakesItAbsent) {
  for (const auto store : kStores) {
    ASSERT_EQ(Set(CtxA(), store, "removed", "value"), 0);
    ASSERT_EQ(GFStorageCacheRemove(CtxA(), store, "removed"), 0);
    EXPECT_FALSE(Get(CtxA(), store, "removed").has_value())
        << "store " << store;
  }
}

TEST(SdkStorageTest, AnEmptyValueIsTheSameAsNoValue) {
  for (const auto store : kStores) {
    ASSERT_EQ(Set(CtxA(), store, "emptied", "value"), 0);
    ASSERT_EQ(Set(CtxA(), store, "emptied", QByteArray()), 0);
    EXPECT_FALSE(Get(CtxA(), store, "emptied").has_value())
        << "store " << store;
  }
}

// Any module granted "storage" could read another module's secure entries
// by guessing the key.
TEST(SdkStorageTest, AKeyBelongsToOneModule) {
  for (const auto store : kStores) {
    ASSERT_EQ(Set(CtxA(), store, "shared-name", "from a"), 0);
    EXPECT_FALSE(Get(CtxB(), store, "shared-name").has_value())
        << "store " << store;

    ASSERT_EQ(Set(CtxB(), store, "shared-name", "from b"), 0);
    EXPECT_EQ(Get(CtxA(), store, "shared-name"), QByteArray("from a"));

    GFStorageCacheRemove(CtxA(), store, "shared-name");
    GFStorageCacheRemove(CtxB(), store, "shared-name");
  }
}

// The durable and secure stores used to share one key space.
TEST(SdkStorageTest, AKeyBelongsToOneStore) {
  ASSERT_EQ(Set(CtxA(), GF_STORE_SECURE_DURABLE, "secret", "credential"), 0);
  EXPECT_FALSE(Get(CtxA(), GF_STORE_DURABLE, "secret").has_value());
  GFStorageCacheRemove(CtxA(), GF_STORE_SECURE_DURABLE, "secret");
}

// A value written under the old shared key moves to the module's own key
// the first time it is read, and is gone from the old one.
TEST(SdkStorageTest, ALegacyValueIsMigratedOnFirstRead) {
  auto& cache = CacheManager::GetInstance();
  const QString legacy = "__module_legacy-credential";
  cache.SaveSecDurableCache(legacy, GFBuffer(QByteArray("old value")), true);

  EXPECT_EQ(Get(CtxA(), GF_STORE_SECURE_DURABLE, "legacy-credential"),
            QByteArray("old value"));
  EXPECT_TRUE(cache.LoadSecDurableCache(legacy).Empty());

  // Still there on the next read, now from the scoped key.
  EXPECT_EQ(Get(CtxA(), GF_STORE_SECURE_DURABLE, "legacy-credential"),
            QByteArray("old value"));
  GFStorageCacheRemove(CtxA(), GF_STORE_SECURE_DURABLE, "legacy-credential");
}

TEST(SdkStorageTest, RemovingAKeyAlsoDropsAnUnmigratedLegacyValue) {
  auto& cache = CacheManager::GetInstance();
  const QString legacy = "__module_legacy-removed";
  cache.SaveSecDurableCache(legacy, GFBuffer(QByteArray("old value")), true);

  ASSERT_EQ(GFStorageCacheRemove(CtxA(), GF_STORE_DURABLE, "legacy-removed"),
            0);
  EXPECT_TRUE(cache.LoadSecDurableCache(legacy).Empty());
  EXPECT_FALSE(Get(CtxA(), GF_STORE_DURABLE, "legacy-removed").has_value());
}

TEST(SdkStorageTest, AnUnknownStoreIsRefused) {
  EXPECT_NE(Set(CtxA(), 99, "key", "value"), 0);
  EXPECT_FALSE(Get(CtxA(), 99, "key").has_value());
  EXPECT_NE(GFStorageCacheRemove(CtxA(), 99, "key"), 0);
}

// The register table carries the host's own `core` values -- startup reads
// some of them back -- next to every module's. A module writes only its own
// namespace; reading is not restricted.
TEST(SdkStorageTest, AModuleWritesOnlyItsOwnStateNamespace) {
  EXPECT_NE(GFStorageStateSetBool(CtxA(), "core", "env.state.probe", 1), 0);
  EXPECT_NE(
      GFStorageStateSetBool(CtxA(), "com.example.sdk.storage.b", "probe", 1),
      0);
  EXPECT_EQ(
      GFStorageStateSetBool(CtxA(), "com.example.sdk.storage.a", "probe", 1),
      0);

  int value = 0;
  // Readable by the other module, and by the one that wrote it.
  EXPECT_EQ(GFStorageStateGetBool(CtxB(), "com.example.sdk.storage.a", "probe",
                                  &value),
            0);
  EXPECT_EQ(value, 1);
}

TEST(SdkStorageTest, StateKeysAreOneCaseForReadingAndWriting) {
  // A mixed-case read used to miss a value the write had lower-cased.
  ASSERT_EQ(GFStorageStateSetBool(CtxA(), "com.example.sdk.storage.a",
                                  "Mixed.Case", 1),
            0);
  int value = 0;
  EXPECT_EQ(GFStorageStateGetBool(CtxA(), "com.example.sdk.storage.a",
                                  "Mixed.Case", &value),
            0);
  EXPECT_EQ(value, 1);
}

}  // namespace GpgFrontend::Test
