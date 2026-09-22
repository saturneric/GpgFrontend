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

#include "GFSDKStorage.h"

#include "GFSdkInternal.h"

/**
 * @file GFSdkStorage.cpp
 * @brief Settings, caches and the register table, module-side.
 *
 * Thin by design. The tier and the type are arguments on the boundary, so
 * there is nothing here to compose: what used to be eight cache functions and
 * five register-table functions with defaults baked in is now a direct
 * forward each, and "or a default" lives in the C++ facade where it belongs.
 */

auto GFStorageSettingsRoot(GFSDKContext* ctx) -> void* {
  GF_SDK_REQUIRE(ctx, storage, "GFStorageSettingsRoot", nullptr);
  return g->settings_root(hctx);
}

auto GFStorageCacheGet(GFSDKContext* ctx, int store, const char* key,
                       GFBufferRef* out) -> int {
  GF_SDK_REQUIRE(ctx, storage, "GFStorageCacheGet", -1);
  return g->cache_get(hctx, store, key, out);
}

auto GFStorageCacheSet(GFSDKContext* ctx, int store, const char* key,
                       GFBufferView value, int64_t ttl_seconds) -> int {
  GF_SDK_REQUIRE(ctx, storage, "GFStorageCacheSet", -1);
  return g->cache_set(hctx, store, key, value, ttl_seconds);
}

auto GFStorageCacheRemove(GFSDKContext* ctx, int store, const char* key)
    -> int {
  GF_SDK_REQUIRE(ctx, storage, "GFStorageCacheRemove", -1);
  return g->cache_remove(hctx, store, key);
}

auto GFStorageStateGetText(GFSDKContext* ctx, const char* ns, const char* key,
                           GFBufferRef* out) -> int {
  GF_SDK_REQUIRE(ctx, storage, "GFStorageStateGetText", -1);
  return g->state_get_text(hctx, ns, key, out);
}

auto GFStorageStateSetText(GFSDKContext* ctx, const char* ns, const char* key,
                           GFBufferView value) -> int {
  GF_SDK_REQUIRE(ctx, storage, "GFStorageStateSetText", -1);
  return g->state_set_text(hctx, ns, key, value);
}

auto GFStorageStateGetBool(GFSDKContext* ctx, const char* ns, const char* key,
                           int* out) -> int {
  GF_SDK_REQUIRE(ctx, storage, "GFStorageStateGetBool", -1);
  return g->state_get_bool(hctx, ns, key, out);
}

auto GFStorageStateSetBool(GFSDKContext* ctx, const char* ns, const char* key,
                           int value) -> int {
  GF_SDK_REQUIRE(ctx, storage, "GFStorageStateSetBool", -1);
  return g->state_set_bool(hctx, ns, key, value);
}

auto GFStorageStateListChildren(GFSDKContext* ctx, const char* ns,
                                const char* key, GFStringListRef* out) -> int {
  GF_SDK_REQUIRE(ctx, storage, "GFStorageStateListChildren", -1);
  return g->state_list_children(hctx, ns, key, out);
}
