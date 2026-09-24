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

#pragma once

#include <stdint.h>

#include "GFSDKContext.h"

/**
 * @file GFSDKStorage.h
 * @brief Settings, caches and the shared runtime register table.
 *
 * ## Why the application settings live here and not under "UI"
 *
 * The settings calls below are scoped to the calling module. Reading
 * configuration is not drawing, and a module that only wants to know a
 * preference should not have to ask for the ability to open dialogs. The
 * capability that grants everything in this header is "storage", and the
 * names now say so.
 *
 * ## Absence is reported, not papered over
 *
 * The reads below distinguish "no such key" from "the key holds this value".
 * The old entry points took a default and returned it on a miss, which made a
 * stored value equal to the default indistinguishable from nothing at all.
 * "Or a default" is arithmetic; it lives in the C++ facade.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* --- settings ------------------------------------------------------------
 *
 * The application's settings, by key, as CBOR values. A module names a key
 * and the Host decides where it lives: GF_SETTING_MODULE is the module's own
 * group, which no other module can see; GF_SETTING_HOST is one of the few
 * Host settings the Host shares on purpose, read-only unless it says so.
 * A key is relative -- "servers/default", never "/x" or "a/../b".
 */

/** @return 0 and an owned CBOR buffer in @p out, or -1 when absent. */
int GFStorageSettingGet(GFSDKContext* ctx, int scope, const char* key,
                        GFBufferRef* out);
int GFStorageSettingSet(GFSDKContext* ctx, int scope, const char* key,
                        GFBufferView cbor);
int GFStorageSettingRemove(GFSDKContext* ctx, int scope, const char* key);

/* --- caches --------------------------------------------------------------
 *
 * Three tiers, one set of calls. They differ in how long a value lives and
 * whether it is wiped, which is a property of the store and not of the call.
 * @p store is one of @ref GFStorageStore.
 *
 * Keys are private to the calling module and to the store: two modules, or
 * two stores, never see each other's values under the same key. Values are
 * octets and may contain NUL. An empty value is the same as no value.
 *
 * The secure tier used to CONSUME both of its arguments and free them through
 * two different allocators, a rule nothing in the signature hinted at. These
 * borrow, like every other argument in this SDK.
 */

/** @return 0 on a hit with @p out owned; negative when absent. */
int GFStorageCacheGet(GFSDKContext* ctx, int store, const char* key,
                      GFBufferRef* out);

/** @param ttl_seconds <= 0 means no expiry. */
int GFStorageCacheSet(GFSDKContext* ctx, int store, const char* key,
                      GFBufferView value, int64_t ttl_seconds);

int GFStorageCacheRemove(GFSDKContext* ctx, int store, const char* key);

/* --- the runtime register table ------------------------------------------
 *
 * Shared state, visible to the host and to other modules. Text and bool are
 * kept apart because the table really does store typed values and a lookup of
 * the wrong type is a miss, not a conversion.
 */

/** @return 0 on a hit with @p out owned; negative when absent or not text. */
int GFStorageStateGetText(GFSDKContext* ctx, const char* ns, const char* key,
                          GFBufferRef* out);
int GFStorageStateSetText(GFSDKContext* ctx, const char* ns, const char* key,
                          GFBufferView value);

/** @return 0 on a hit; negative when absent or not a bool. */
int GFStorageStateGetBool(GFSDKContext* ctx, const char* ns, const char* key,
                          int* out);
int GFStorageStateSetBool(GFSDKContext* ctx, const char* ns, const char* key,
                          int value);

/** @brief Direct children of ns/key. @p out owned; release through the list
 *         accessors in GFSDKGpgList.h. */
int GFStorageStateListChildren(GFSDKContext* ctx, const char* ns,
                               const char* key, GFStringListRef* out);

#ifdef __cplusplus
}
#endif
