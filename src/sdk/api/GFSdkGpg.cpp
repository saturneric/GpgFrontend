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

#include "GFSDKGpg.h"

#include "GFSDKGpgResult.h"
#include "GFSdkInternal.h"

/**
 * @file GFSdkGpg.cpp
 * @brief Operations, results and keys, module-side.
 *
 * Nine `GFAnalyse*` spellings and three result-text getters used to live
 * here as separate entry points. They are gone: the operation and the field
 * are arguments now, because that is all they ever were.
 */

/* --- operations ---------------------------------------------------------- */

auto GFGpgCurrentChannel(GFSDKContext* ctx) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgCurrentChannel", -1);
  return g->current_channel(hctx);
}

auto GFGpgSign(GFSDKContext* ctx, int channel, const char* const* key_ids,
               size_t key_ids_size, GFBufferView in, int sign_mode, int ascii,
               GFGpgResultRef* out) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgSign", -1);
  return g->sign(hctx, channel, key_ids, key_ids_size, in, sign_mode, ascii,
                 out);
}

auto GFGpgEncrypt(GFSDKContext* ctx, int channel, const char* const* key_ids,
                  size_t key_ids_size, GFBufferView in, int ascii,
                  GFGpgResultRef* out) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgEncrypt", -1);
  return g->encrypt(hctx, channel, key_ids, key_ids_size, in, ascii, out);
}

auto GFGpgDecrypt(GFSDKContext* ctx, int channel, GFBufferView in,
                  GFGpgResultRef* out) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgDecrypt", -1);
  return g->decrypt(hctx, channel, in, out);
}

auto GFGpgVerify(GFSDKContext* ctx, int channel, GFBufferView in,
                 GFBufferView signature, GFGpgResultRef* out) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgVerify", -1);
  return g->verify(hctx, channel, in, signature, out);
}

/* --- results ------------------------------------------------------------- */

auto GFGpgResultStatusOf(GFSDKContext* ctx, GFGpgResultRef r) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgResultStatusOf", GF_GPG_BAD_REQUEST);
  return g->result_status(hctx, r);
}

auto GFGpgResultError(GFSDKContext* ctx, GFGpgResultRef r) -> uint32_t {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgResultError", 0U);
  return g->result_error(hctx, r);
}

auto GFGpgResultData(GFSDKContext* ctx, GFGpgResultRef r) -> GFBufferView {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgResultData", nullptr);
  return g->result_data(hctx, r);
}

auto GFGpgResultText(GFSDKContext* ctx, GFGpgResultRef r, int field) -> const
    char* {
  // "" rather than nullptr on the denial path: these fields are documented
  // never to be null, and a caller that trusted that would crash on a denial
  // instead of seeing an empty answer.
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgResultText", "");
  const auto* text = g->result_text(hctx, r, field);
  return text == nullptr ? "" : text;
}

auto GFGpgResultTakeData(GFSDKContext* ctx, GFGpgResultRef r) -> GFBufferRef {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgResultTakeData", nullptr);
  return g->result_take_data(hctx, r);
}

void GFGpgResultRelease(GFSDKContext* ctx, GFGpgResultRef r) {
  GF_SDK_REQUIRE_VOID(ctx, gpg, "GFGpgResultRelease");
  g->result_release(hctx, r);
}

auto GFGpgResultOutstandingCount(GFSDKContext* ctx) -> size_t {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgResultOutstandingCount", 0);
  return g->result_outstanding_count(hctx);
}

/* --- keys and analysis --------------------------------------------------- */

auto GFGpgPublicKey(GFSDKContext* ctx, int channel, const char* key_id,
                    int ascii) -> GFBufferRef {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgPublicKey", nullptr);
  return g->public_key(hctx, channel, key_id, ascii);
}

auto GFGpgExportKey(GFSDKContext* ctx, int channel, const char* key_id,
                    int ascii, GFBufferRef* out) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgExportKey", -1);
  return g->export_key(hctx, channel, key_id, ascii, out);
}

auto GFGpgImportKeys(GFSDKContext* ctx, int channel, void* parent,
                     GFBufferView data) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgImportKeys", -1);
  return g->import_keys(hctx, channel, parent, data);
}

auto GFGpgKeyPrimaryUid(GFSDKContext* ctx, int channel, const char* key_id,
                        GFBufferRef* name, GFBufferRef* email,
                        GFBufferRef* comment) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgKeyPrimaryUid", -1);
  return g->key_primary_uid(hctx, channel, key_id, name, email, comment);
}

auto GFGpgAnalyseResult(GFSDKContext* ctx, int channel, int operation,
                        uint32_t err, const char* capsule_id, uint32_t want,
                        GFGpgAnalysis* out) -> int {
  GF_SDK_REQUIRE(ctx, gpg, "GFGpgAnalyseResult", -1);
  return g->analyse_result(hctx, channel, operation, err, capsule_id, want,
                           out);
}
