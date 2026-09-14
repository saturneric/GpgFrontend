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

#include "GFSDKVisibility.h"

#include <stddef.h>
#include <stdint.h>

#include "GFSDKBuffer.h"

/**
 * @file GFSDKGpgList.h
 * @brief Opaque, distinctly-typed collections returned by the gpg calls.
 *
 * WHAT THIS REPLACES. GFGpgFreeKeyBriefs, GFGpgFreeEncRecipients and
 * GFGpgFreeStringArray each exist for one reason: to walk a struct's char*
 * members and free them. Every new returned aggregate needed another one, and
 * the caller had to know which walker went with which array AND pass back the
 * right count. Here each list is one opaque handle with one release, and the
 * elements are reached through borrowed accessors -- so there is no nested
 * allocation layout for a caller to get wrong, and no count to carry around.
 *
 * WHY STILL THREE TYPES rather than one generic list. Funnelling unrelated
 * collections through a single GFGpgListRelease(void*) would buy nothing and
 * would throw away the C type checking that makes the rest of this safe: it
 * would become possible to hand a key-brief list to a recipient accessor and
 * have it compile. One teardown per OWNING OBJECT is the goal; one teardown
 * for every object in the SDK is not.
 *
 * Every accessor below returns a BORROWED pointer valid until the owning list
 * is released. Never free one.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GFGpgKeyBriefListImpl* GFGpgKeyBriefListRef;
typedef struct GFGpgRecipientListImpl* GFGpgRecipientListRef;
typedef struct GFStringListImpl* GFStringListRef;

/* --- key briefs ---------------------------------------------------------- */

/**
 * @brief Keys whose UID e-mail matches @p email.
 *
 * @p email is BORROWED, like every SDK argument: the caller keeps it.
 * @param[out] out owned list, released with GFGpgKeyBriefListRelease.
 * @return 0 on success; negative on a bad request. An empty list is success.
 */
GF_SDK_EXPORT int GFGpgFindKeys(int channel, const char* email,
                                GFGpgKeyBriefListRef* out);

GF_SDK_EXPORT size_t GFGpgKeyBriefListCount(GFGpgKeyBriefListRef l);

/* All borrowed, valid until the list is released. Index out of range yields
   "" or 0 rather than undefined behaviour. */
GF_SDK_EXPORT const char* GFGpgKeyBriefFingerprint(GFGpgKeyBriefListRef l,
                                                   size_t i);
GF_SDK_EXPORT const char* GFGpgKeyBriefKeyId(GFGpgKeyBriefListRef l, size_t i);
GF_SDK_EXPORT const char* GFGpgKeyBriefUid(GFGpgKeyBriefListRef l, size_t i);
GF_SDK_EXPORT const char* GFGpgKeyBriefMatchedEmail(GFGpgKeyBriefListRef l,
                                                    size_t i);
GF_SDK_EXPORT int64_t GFGpgKeyBriefExpiresAt(GFGpgKeyBriefListRef l, size_t i);
GF_SDK_EXPORT int GFGpgKeyBriefUsability(GFGpgKeyBriefListRef l, size_t i);
GF_SDK_EXPORT int GFGpgKeyBriefCanEncrypt(GFGpgKeyBriefListRef l, size_t i);
GF_SDK_EXPORT int GFGpgKeyBriefCanSign(GFGpgKeyBriefListRef l, size_t i);
GF_SDK_EXPORT int GFGpgKeyBriefMatchedUidIsPrimary(GFGpgKeyBriefListRef l,
                                                   size_t i);
GF_SDK_EXPORT int GFGpgKeyBriefMatchedUidRevoked(GFGpgKeyBriefListRef l,
                                                 size_t i);

GF_SDK_EXPORT void GFGpgKeyBriefListRelease(GFGpgKeyBriefListRef l);

/* --- encrypted-message recipients ---------------------------------------- */

/**
 * @brief Who @p in was encrypted to, without decrypting it.
 *
 * Reads only the PKESK packets, so nothing is decrypted and no passphrase is
 * requested: safe to call to decide what to TELL the user before they ask for
 * a decryption. A message with no PKESK at all yields an empty list rather
 * than an error -- "encrypted to nobody we can name" is an answer.
 */
GF_SDK_EXPORT int GFGpgSniffRecipients(int channel, GFBufferView in,
                                       GFGpgRecipientListRef* out);

GF_SDK_EXPORT size_t GFGpgRecipientListCount(GFGpgRecipientListRef l);

GF_SDK_EXPORT const char* GFGpgRecipientKeyId(GFGpgRecipientListRef l,
                                              size_t i);
GF_SDK_EXPORT const char* GFGpgRecipientPubAlgo(GFGpgRecipientListRef l,
                                                size_t i);
GF_SDK_EXPORT const char* GFGpgRecipientFingerprint(GFGpgRecipientListRef l,
                                                    size_t i);
GF_SDK_EXPORT const char* GFGpgRecipientUid(GFGpgRecipientListRef l, size_t i);
GF_SDK_EXPORT int GFGpgRecipientKeyFound(GFGpgRecipientListRef l, size_t i);
GF_SDK_EXPORT int GFGpgRecipientHasSecret(GFGpgRecipientListRef l, size_t i);

/** @brief The sender withheld this recipient (--hidden-recipient). */
GF_SDK_EXPORT int GFGpgRecipientHidden(GFGpgRecipientListRef l, size_t i);

GF_SDK_EXPORT void GFGpgRecipientListRelease(GFGpgRecipientListRef l);

/* --- plain string lists --------------------------------------------------- */

/** @brief Every e-mail address in the keyring; @p secret_only limits to keys
 *         whose secret half is held. */
GF_SDK_EXPORT int GFGpgListAddresses(int channel, int secret_only,
                                     GFStringListRef* out);

GF_SDK_EXPORT size_t GFStringListCount(GFStringListRef l);
GF_SDK_EXPORT const char* GFStringListAt(GFStringListRef l, size_t i);
GF_SDK_EXPORT void GFStringListRelease(GFStringListRef l);

/** @brief Outstanding list handles of every kind; NULL means process-wide. */
GF_SDK_EXPORT size_t GFGpgListOutstandingCount(const char* module_id);

#ifdef __cplusplus
}
#endif
