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

#include "GFSDKBuffer.h"

#include <cstring>

#include "GFSdkInternal.h"

/**
 * @file GFSdkBuffer.cpp
 * @brief Octets and raw memory, module-side.
 *
 * The two arenas are one primitive with an `arena` argument on the boundary
 * and one family of names here, which is the right way round: the host has
 * one rule to implement, and a call site still says which arena it means.
 */

auto GFBufferNewFromBytes(GFSDKContext* ctx, const void* data, size_t size)
    -> GFBufferRef {
  GF_SDK_REQUIRE(ctx, buffer, "GFBufferNewFromBytes", nullptr);
  return g->new_from_bytes(hctx, data, size);
}

auto GFBufferData(GFSDKContext* ctx, GFBufferView buf) -> const void* {
  GF_SDK_REQUIRE(ctx, buffer, "GFBufferData", nullptr);
  return g->data(hctx, buf);
}

auto GFBufferSize(GFSDKContext* ctx, GFBufferView buf) -> size_t {
  GF_SDK_REQUIRE(ctx, buffer, "GFBufferSize", 0);
  return g->size(hctx, buf);
}

void GFBufferZeroize(GFSDKContext* ctx, GFBufferRef buf) {
  GF_SDK_REQUIRE_VOID(ctx, buffer, "GFBufferZeroize");
  g->zeroize(hctx, buf);
}

void GFBufferRelease(GFSDKContext* ctx, GFBufferRef buf) {
  GF_SDK_REQUIRE_VOID(ctx, buffer, "GFBufferRelease");
  g->release(hctx, buf);
}

auto GFBufferOutstandingCount(GFSDKContext* ctx) -> size_t {
  GF_SDK_REQUIRE(ctx, buffer, "GFBufferOutstandingCount", 0);
  return g->outstanding_count(hctx);
}

auto GFMemAlloc(GFSDKContext* ctx, int arena, uint32_t size) -> void* {
  GF_SDK_REQUIRE(ctx, buffer, "GFMemAlloc", nullptr);
  return g->mem_alloc(hctx, arena, size);
}

auto GFMemRealloc(GFSDKContext* ctx, int arena, void* p, uint32_t size)
    -> void* {
  GF_SDK_REQUIRE(ctx, buffer, "GFMemRealloc", nullptr);
  return g->mem_realloc(hctx, arena, p, size);
}

void GFMemFree(GFSDKContext* ctx, int arena, void* p) {
  GF_SDK_REQUIRE_VOID(ctx, buffer, "GFMemFree");
  g->mem_free(hctx, arena, p);
}

auto GFMemStrDup(GFSDKContext* ctx, int arena, const char* s) -> char* {
  GF_SDK_REQUIRE(ctx, buffer, "GFMemStrDup", nullptr);
  return g->mem_strdup(hctx, arena, s);
}
