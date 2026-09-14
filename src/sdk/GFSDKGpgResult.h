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

#include <stddef.h>
#include <stdint.h>

#include "GFSDKBuffer.h"

/**
 * @file GFSDKGpgResult.h
 * @brief Opaque results and lists for the gpg operations.
 *
 * WHAT THIS REPLACES, AND WHY. The older GFGpgSignResult /
 * GFGpgEncryptionResult / GFGpgDecryptResult / GFGpgVerifyResult are plain
 * structs whose every char* member the caller must reclaim individually, in the
 * right order, on every path including the error paths. Correctness depended on
 * remembering each field of each struct, and it did not survive contact: the
 * verify success path forgot error_string and leaked it once per signed region
 * for as long as the code existed, which ASan later confirmed.
 *
 * Here there is exactly ONE release per owned object and NO per-field free at
 * all, so that entire class of bug is unwritable rather than merely fixed.
 *
 * OWNERSHIP, the same three rules as GFSDKBuffer.h:
 *   - arguments are always borrowed (GFBufferView, const char*);
 *   - owned handles come back only from an out-parameter or a *Take* accessor;
 *   - every accessor returns a BORROWED view valid until the owner is
 *     released. Nothing reachable from a result is separately releasable.
 *
 * The raw gpgme_*_result_t handles are deliberately absent. Every use of them
 * in the tree was a call to free them, they carried no information to any
 * module, and rPGP-backed results have no native handle at all (GetRaw() is
 * null) -- so the capsule id, which both engines produce, is the only route
 * that ever worked for both. Dropping them also takes <gpgme.h> off the module
 * include path.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** One crypto operation's outcome. Release with GFGpgResultRelease. */
typedef struct GFGpgResultImpl* GFGpgResultRef;

/**
 * @brief Status of the operation a result describes.
 *
 * Distinguishing these matters because a caller reacts differently to each,
 * and because the middle case is the one the old API got wrong by accident:
 * the result struct was allocated BEFORE the operation could fail, so a
 * non-zero return did not mean there was nothing to reclaim.
 */
typedef enum {
  GF_GPG_OK = 0,          /**< succeeded; data (where applicable) is present */
  GF_GPG_OP_FAILED = 1,   /**< the operation failed; the result explains why */
  GF_GPG_BAD_REQUEST = 2, /**< arguments were unusable; nothing was attempted */
} GFGpgResultStatus;

/* --- operations ---------------------------------------------------------
 *
 * Each returns 0 when the operation succeeded and non-zero otherwise, and
 * sets @p out. RULE, stated once: whenever these return 0 OR a positive
 * value, @p out holds an owned result the caller must release -- on the
 * failure path too, because that is where the explanation lives. Only a
 * negative return means @p out is NULL and there is nothing to release.
 */

/** @brief Sign @p in. @p sign_mode 0 = inline, 1 = detached. */
GF_SDK_EXPORT int GFGpgSign(int channel, const char* const* key_ids,
                            size_t key_ids_size, GFBufferView in, int sign_mode,
                            int ascii, GFGpgResultRef* out);

/** @brief Encrypt @p in to @p key_ids. */
GF_SDK_EXPORT int GFGpgEncrypt(int channel, const char* const* key_ids,
                               size_t key_ids_size, GFBufferView in, int ascii,
                               GFGpgResultRef* out);

/** @brief Decrypt @p in with whatever secret key is available. */
GF_SDK_EXPORT int GFGpgDecrypt(int channel, GFBufferView in,
                               GFGpgResultRef* out);

/**
 * @brief Verify @p in against @p signature.
 *
 * Pass NULL for @p signature to verify an inline or clearsigned message.
 */
GF_SDK_EXPORT int GFGpgVerify(int channel, GFBufferView in,
                              GFBufferView signature, GFGpgResultRef* out);

/* --- accessors: all BORROWED, valid until GFGpgResultRelease ------------- */

GF_SDK_EXPORT int GFGpgResultStatusOf(GFGpgResultRef r);

/** @brief The raw gpg error code, for callers that map it themselves. */
GF_SDK_EXPORT uint32_t GFGpgResultError(GFGpgResultRef r);

/**
 * @brief The operation's output bytes, or NULL where the operation has none.
 *
 * A borrowed view: it cannot be passed to GFBufferRelease, and it dies with
 * the result. Use GFGpgResultTakeData when it must outlive the result.
 */
GF_SDK_EXPORT GFBufferView GFGpgResultData(GFGpgResultRef r);

/** @brief Opaque id for the analyse-by-capsule calls. Never NULL; may be "". */
GF_SDK_EXPORT const char* GFGpgResultCapsuleId(GFGpgResultRef r);

/** @brief Human-readable explanation. Never NULL; may be "". */
GF_SDK_EXPORT const char* GFGpgResultErrorString(GFGpgResultRef r);

/** @brief Hash algorithm used, for signing. Never NULL; may be "". */
GF_SDK_EXPORT const char* GFGpgResultHashAlgo(GFGpgResultRef r);

/**
 * @brief Take the output bytes OUT of the result, transferring ownership.
 *
 * The only accessor here that transfers, and named to say so. Afterwards
 * GFGpgResultData returns NULL. Release the returned handle yourself.
 */
GF_SDK_EXPORT GF_SDK_MUST_USE GFBufferRef GFGpgResultTakeData(GFGpgResultRef r);

/** @brief The one teardown. Releases every field with it. NULL-safe. */
GF_SDK_EXPORT void GFGpgResultRelease(GFGpgResultRef r);

/** @brief Outstanding result handles; NULL module_id means process-wide. */
GF_SDK_EXPORT size_t GFGpgResultOutstandingCount(const char* module_id);

#ifdef __cplusplus
}
#endif
