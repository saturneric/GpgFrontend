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

#include "GFSDKContext.h"

/**
 * @file GFSDKGpgList.h
 * @brief Collections the host returns, and the one row accessor each has.
 *
 * WHAT THIS REPLACED, TWICE. First, a family of `Free*` array walkers, one
 * per aggregate, each existing only to walk a struct's `char*` members; the
 * caller had to know which walker went with which array AND carry the count.
 * Now each list is one opaque handle with one release.
 *
 * Then, nineteen per-field accessors across two list types, which differed
 * only in which member they named. Now each list hands back a borrowed row
 * struct, so a new field costs an append rather than a new entry point.
 *
 * Every row and every string in it is BORROWED and dies with its list.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* --- key briefs ---------------------------------------------------------- */

/**
 * @brief Keys whose UID e-mail matches @p email.
 *
 * @return 0 on success with @p out owned. An empty list is success.
 */
int GFGpgFindKeys(GFSDKContext* ctx, int channel, const char* email,
                  GFGpgKeyBriefListRef* out);

size_t GFGpgKeyBriefCount(GFSDKContext* ctx, GFGpgKeyBriefListRef l);

/** @return borrowed, valid until release; NULL when @p i is out of range */
const GFGpgKeyBriefRow* GFGpgKeyBriefAt(GFSDKContext* ctx,
                                        GFGpgKeyBriefListRef l, size_t i);

void GFGpgKeyBriefRelease(GFSDKContext* ctx, GFGpgKeyBriefListRef l);

/* --- encrypted-message recipients ---------------------------------------- */

/**
 * @brief Who @p in was encrypted to, without decrypting it.
 *
 * Reads only the PKESK packets, so nothing is decrypted and no passphrase is
 * requested: safe to call to decide what to TELL the user before they ask for
 * a decryption. A message with no PKESK yields an empty list rather than an
 * error, because "encrypted to nobody we can name" is an answer.
 */
int GFGpgSniffRecipients(GFSDKContext* ctx, int channel, GFBufferView in,
                         GFGpgRecipientListRef* out);

size_t GFGpgRecipientCount(GFSDKContext* ctx, GFGpgRecipientListRef l);

const GFGpgRecipientRow* GFGpgRecipientAt(GFSDKContext* ctx,
                                          GFGpgRecipientListRef l, size_t i);

void GFGpgRecipientRelease(GFSDKContext* ctx, GFGpgRecipientListRef l);

/* --- plain string lists ---------------------------------------------------
 *
 * The generic container. Two different capabilities produce one: "gpg"
 * returns keyring addresses, "storage" returns the register table's child
 * keys. The accessors are therefore always available, because a module able
 * to obtain a list but not to release it would leak because of a permission
 * boundary.
 */

/** @brief Every e-mail address in the keyring; @p secret_only limits it to
 *         keys whose secret half is held. */
int GFGpgListAddresses(GFSDKContext* ctx, int channel, int secret_only,
                       GFStringListRef* out);

size_t GFStringListCount(GFSDKContext* ctx, GFStringListRef l);
const char* GFStringListAt(GFSDKContext* ctx, GFStringListRef l, size_t i);
void GFStringListRelease(GFSDKContext* ctx, GFStringListRef l);

/** @brief Outstanding list handles of every kind held by this module. */
size_t GFListOutstandingCount(GFSDKContext* ctx);

#ifdef __cplusplus
}
#endif
