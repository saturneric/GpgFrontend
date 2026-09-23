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

#include "GFSDKCommand.h"

#include "GFSdkInternal.h"

/**
 * @file GFSdkCommand.cpp
 * @brief Commands, module-side: one forward each.
 *
 * The buffers handed in are transferred whatever happens, so the refusal
 * paths below release them rather than leaking what the caller gave away --
 * a caller cannot tell "the host kept it" from "nobody did".
 */

namespace {

void ReleaseTransferred(GFSDKContext* ctx, GFBufferRef cbor, GFBufferRef* blobs,
                        size_t blob_count) {
  if (ctx == nullptr || ctx->host == nullptr || ctx->host->buffer == nullptr) {
    return;
  }
  const auto* b = ctx->host->buffer;
  if (cbor != nullptr) b->release(ctx->host->context, cbor);
  for (size_t i = 0; blobs != nullptr && i < blob_count; ++i) {
    if (blobs[i] != nullptr) b->release(ctx->host->context, blobs[i]);
  }
}

}  // namespace

auto GFCommandRegister(GFSDKContext* ctx, const GFCommandSpec* spec) -> int {
  GF_SDK_REQUIRE(ctx, command, "GFCommandRegister", GF_CMD_E_UNAVAILABLE);
  return g->register_command(hctx, spec);
}

auto GFCommandUnregister(GFSDKContext* ctx, const char* id) -> int {
  GF_SDK_REQUIRE(ctx, command, "GFCommandUnregister", GF_CMD_E_UNAVAILABLE);
  return g->unregister_command(hctx, id);
}

auto GFCommandInvoke(GFSDKContext* ctx, const char* id, uint32_t flags,
                     GFBufferRef args_cbor, GFBufferRef* blobs,
                     size_t blob_count, GFCommandDoneFn done, void* user,
                     uint64_t* out_call_id) -> int {
  if (ctx == nullptr || ctx->host == nullptr ||
      !GF_SDK_GROUP_HAS(ctx->host, command)) {
    ReleaseTransferred(ctx, args_cbor, blobs, blob_count);
  }
  GF_SDK_REQUIRE(ctx, command, "GFCommandInvoke", GF_CMD_E_UNAVAILABLE);
  return g->invoke(hctx, id, flags, args_cbor, blobs, blob_count, done, user,
                   out_call_id);
}

auto GFCommandComplete(GFSDKContext* ctx, uint64_t call_id, int status,
                       GFBufferRef result_cbor, GFBufferRef* blobs,
                       size_t blob_count, const char* error) -> int {
  if (ctx == nullptr || ctx->host == nullptr ||
      !GF_SDK_GROUP_HAS(ctx->host, command)) {
    ReleaseTransferred(ctx, result_cbor, blobs, blob_count);
  }
  GF_SDK_REQUIRE(ctx, command, "GFCommandComplete", GF_CMD_E_UNAVAILABLE);
  return g->complete(hctx, call_id, status, result_cbor, blobs, blob_count,
                     error);
}

auto GFCommandCancel(GFSDKContext* ctx, uint64_t call_id) -> int {
  GF_SDK_REQUIRE(ctx, command, "GFCommandCancel", GF_CMD_E_UNAVAILABLE);
  return g->cancel(hctx, call_id);
}

auto GFCommandIsCancelled(GFSDKContext* ctx, uint64_t call_id) -> int {
  GF_SDK_REQUIRE(ctx, command, "GFCommandIsCancelled", 0);
  return g->is_cancelled(hctx, call_id);
}

auto GFCommandDescribe(GFSDKContext* ctx, const char* id, GFBufferRef* out)
    -> int {
  GF_SDK_REQUIRE(ctx, command, "GFCommandDescribe", GF_CMD_E_UNAVAILABLE);
  return g->describe(hctx, id, out);
}

auto GFCommandList(GFSDKContext* ctx, const char* prefix, GFStringListRef* out)
    -> int {
  GF_SDK_REQUIRE(ctx, command, "GFCommandList", GF_CMD_E_UNAVAILABLE);
  return g->list(hctx, prefix, out);
}

auto GFCommandQueryState(GFSDKContext* ctx, const char* id,
                         uint32_t* out_bits) -> int {
  GF_SDK_REQUIRE(ctx, command, "GFCommandQueryState", GF_CMD_E_UNAVAILABLE);
  return g->query_state(hctx, id, out_bits);
}
