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
 * @file GFSDKGpgResult.h
 * @brief The four operations, and the one object each returns.
 *
 * WHAT THIS REPLACED. The older result structs had every `char*` member
 * reclaimed individually, in the right order, on every path including the
 * error paths. Correctness depended on remembering each field of each struct,
 * and it did not survive contact: the verify success path forgot
 * `error_string` and leaked it once per signed region for as long as the code
 * existed, which ASan later confirmed.
 *
 * Here there is exactly ONE release per owned object and NO per-field free at
 * all, so that entire class of bug is unwritable rather than merely fixed.
 *
 * RULE, stated once: whenever an operation returns 0 OR a positive value,
 * @p out holds an owned result the caller must release, on the failure path
 * too, because that is where the explanation lives. Only a negative return
 * means @p out is NULL and there is nothing to release.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Sign @p in. @p sign_mode 0 = inline, 1 = detached. */
int GFGpgSign(GFSDKContext* ctx, int channel, const char* const* key_ids,
              size_t key_ids_size, GFBufferView in, int sign_mode, int ascii,
              GFGpgResultRef* out);

/** @brief Encrypt @p in to @p key_ids. */
int GFGpgEncrypt(GFSDKContext* ctx, int channel, const char* const* key_ids,
                 size_t key_ids_size, GFBufferView in, int ascii,
                 GFGpgResultRef* out);

/** @brief Decrypt @p in with whatever secret key is available. */
int GFGpgDecrypt(GFSDKContext* ctx, int channel, GFBufferView in,
                 GFGpgResultRef* out);

/**
 * @brief Verify @p in against @p signature.
 *
 * Pass NULL for @p signature to verify an inline or clearsigned message.
 */
int GFGpgVerify(GFSDKContext* ctx, int channel, GFBufferView in,
                GFBufferView signature, GFGpgResultRef* out);

/* --- accessors: all BORROWED, valid until GFGpgResultRelease ------------- */

/** @return one of @ref GFGpgResultStatus */
int GFGpgResultStatusOf(GFSDKContext* ctx, GFGpgResultRef r);

/** @brief The raw gpg error code, for callers that map it themselves. */
uint32_t GFGpgResultError(GFSDKContext* ctx, GFGpgResultRef r);

/**
 * @brief The operation's output bytes, or NULL where it has none.
 *
 * A borrowed view: it cannot be released and it dies with the result. Use
 * @ref GFGpgResultTakeData when it must outlive the result.
 */
GFBufferView GFGpgResultData(GFSDKContext* ctx, GFGpgResultRef r);

/**
 * @brief One of the result's text fields.
 *
 * Three accessors with identical signatures became one with a field, because
 * they differed only in which member they named.
 *
 * @param field one of @ref GFGpgResultTextField
 * @return borrowed, never NULL, possibly ""
 */
const char* GFGpgResultText(GFSDKContext* ctx, GFGpgResultRef r, int field);

/**
 * @brief Take the output bytes OUT of the result, transferring ownership.
 *
 * The only accessor here that transfers, and named to say so. Afterwards
 * GFGpgResultData returns NULL.
 */
GFBufferRef GFGpgResultTakeData(GFSDKContext* ctx, GFGpgResultRef r);

/** @brief The one teardown. Releases every field with it. NULL-safe. */
void GFGpgResultRelease(GFSDKContext* ctx, GFGpgResultRef r);

/** @brief Outstanding result handles held by this module. Test hook. */
size_t GFGpgResultOutstandingCount(GFSDKContext* ctx);

#ifdef __cplusplus
}
#endif
