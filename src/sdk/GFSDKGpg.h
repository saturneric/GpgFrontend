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
 * @file GFSDKGpg.h
 * @brief Keys, and what the host can say about a finished operation.
 *
 * Every call takes a `channel`; ask @ref GFGpgCurrentChannel for the one the
 * main window is using. The operations themselves live in GFSDKGpgResult.h
 * and the collections in GFSDKGpgList.h, split by the ownership rules they
 * carry rather than by subject.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The channel the main window is on, or -1 when there is none. */
int GFGpgCurrentChannel(GFSDKContext* ctx);

/**
 * @brief The public key block for @p key_id.
 *
 * @return owned; release with GFBufferRelease. NULL when the key is unknown.
 */
GFBufferRef GFGpgPublicKey(GFSDKContext* ctx, int channel, const char* key_id,
                           int ascii);

/**
 * @brief Export @p key_id as octets.
 * @return 0 on success with @p out owned; negative on failure
 */
int GFGpgExportKey(GFSDKContext* ctx, int channel, const char* key_id,
                   int ascii, GFBufferRef* out);

/**
 * @brief Import key material, showing the standard import dialog.
 *
 * @param parent opaque QWidget to parent the dialog to; may be NULL
 * @return 0 on success
 */
int GFGpgImportKeys(GFSDKContext* ctx, int channel, void* parent,
                    GFBufferView data);

/**
 * @brief The primary UID of @p key_id, split into its parts.
 *
 * Each out-parameter is OWNED and released with GFBufferRelease; pass NULL
 * for a part you do not want. This replaced a struct whose three `char*`
 * members had to be freed one by one, which is exactly the ownership shape
 * the rest of this SDK was rewritten to remove.
 *
 * @return 0 on success, negative when the key is unknown or has no UID
 */
int GFGpgKeyPrimaryUid(GFSDKContext* ctx, int channel, const char* key_id,
                       GFBufferRef* name, GFBufferRef* email,
                       GFBufferRef* comment);

/**
 * @brief Everything the host can say about one finished operation.
 *
 * Engine-neutral: it recovers the full result model from the capsule the
 * operation produced, so it works whether the active engine is GnuPG or
 * rPGP. A raw gpgme handle would not, because an rPGP result has none.
 *
 * Replaces nine entry points, `GFAnalyse{Encrypt,Sign,Decrypt,Verify}Result
 * ByCapsule` and their `*Info*` counterparts, whose signatures were identical
 * and differed only in the operation they named.
 *
 * The capsule is CONSUMED, so ask for everything needed in one call.
 *
 * @param operation one of @ref GFGpgAnalyseOperation
 * @param want a mask of GF_GPG_ANALYSE_WANT_*; a part not asked for comes
 *        back NULL
 * @return positive or zero on success, negative on a bad request
 */
int GFGpgAnalyseResult(GFSDKContext* ctx, int channel, int operation,
                       uint32_t err, const char* capsule_id, uint32_t want,
                       GFGpgAnalysis* out);

#ifdef __cplusplus
}
#endif
