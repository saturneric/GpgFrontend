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

#include <gpgme.h>
#include <stddef.h>

#include <QList>
#include <QString>

#include "GFSDKHostApi.h"
#include "GFSDKTypes.h"

/**
 * @file GFSDKGpgInternal.h
 * @brief Gatherers that are no longer part of the module-facing SDK.
 *
 * These used to be public entry points. They are kept because the key-matching
 * and address-deduplication rules inside them are subtle -- every UID rather
 * than only the primary, revoked UIDs excluded, deduplication on the address
 * alone -- and are covered by existing tests. Moving that logic would have
 * risked those behaviours for no gain.
 *
 * What DID change is that they are no longer exported: modules reach this
 * functionality through the opaque list handles in GFSDKGpgList.h, which have
 * one release per list and borrowed element accessors, rather than through a
 * caller-allocated array plus a matching free-the-array function plus a count
 * the caller has to carry around.
 *
 * Internal to gf_sdk. Not installed, not for modules.
 */

/* The gatherers' own aggregates. These were public once; they are not any
   more, because their ownership shape -- an array whose every char* member a
   caller frees individually -- is exactly what the borrowed row structs in
   GFSDKTypes.h replaced. They survive here as the layer between the engine
   and those rows. */

typedef struct GFGpgKeyBrief {
  char* fingerprint;
  char* key_id;
  char* uid;            ///< primary UID, "Name (Comment) <email>"
  char* matched_email;  ///< the UID email that matched the query

  int64_t expires_at;  ///< seconds since the epoch; 0 means never

  /// Mirrors GpgFrontend::GpgKeyStatus:
  /// 0 ok, 1 expiring soon, 2 expired, 3 revoked, 4 disabled.
  int usability;

  int can_encrypt;
  int can_sign;

  /// Whether the matched UID is the key's primary one, and whether that UID
  /// has itself been revoked. Identity-binding facts, not usability ones.
  int matched_uid_is_primary;
  int matched_uid_revoked;
} GFGpgKeyBrief;

typedef struct GFGpgEncRecipient {
  /// As the message names it: an 8-byte key id (v3 PKESK) or a full
  /// fingerprint (v6 PKESK), upper-cased.
  char* key_id;
  char* pub_algo;

  /// Of the key this resolved to; empty when nothing resolved.
  char* fingerprint;
  /// Primary UID of the key this resolved to; empty when nothing resolved.
  char* uid;

  /// Whether @ref key_id names a key this channel's key database holds at all.
  int key_found;

  /// Whether the secret half is held -- the only field that answers "can this
  /// message be opened on this computer". A public key alone cannot decrypt.
  int has_secret;

  /// The sender withheld the recipient key id (`--hidden-recipient`), so this
  /// recipient is deliberately unidentifiable rather than missing. It may
  /// still be the user themselves.
  int hidden;
} GFGpgEncRecipient;

typedef struct GFGpgKeyUID {
  char* name;     ///< Display name from the UID packet.
  char* email;    ///< Email address from the UID packet.
  char* comment;  ///< Optional comment from the UID packet.
} GFGpgKeyUID;

namespace gf_host {

auto GFGpgFindKeysByEmail(int channel, const char* email, GFGpgKeyBrief** keys,
                          int* count) -> int;
void GFGpgFreeKeyBriefs(GFGpgKeyBrief* keys, int count);

auto GFGpgSniffEncryptedRecipients(int channel, const char* data, int size,
                                   GFGpgEncRecipient** out, int* count) -> int;
void GFGpgFreeEncRecipients(GFGpgEncRecipient* out, int count);

auto GFGpgListKeyAddresses(int channel, int secret_only, char*** addresses,
                           int* count) -> int;
void GFGpgFreeStringArray(char** strings, int count);

/**
 * @brief One list element as a borrowed row struct, for the host api groups.
 *
 * The ABI hands a module one pointer per element instead of eleven accessors
 * that differed only in which field they named. The row points INTO the list's
 * own storage and dies with it, exactly as the individual accessors did.
 *
 * Internal: the public SDK spellings (GFGpgKeyBriefFingerprint and friends)
 * are implemented module-side on top of these, in gf_module_runtime.
 */

}  // namespace gf_host

namespace gf_sdk_internal {

auto KeyBriefRowAt(GFGpgKeyBriefListRef l, size_t i) -> const GFGpgKeyBriefRow*;
auto RecipientRowAt(GFGpgRecipientListRef l, size_t i)
    -> const GFGpgRecipientRow*;

/**
 * @brief Wrap @p values in a string list handle, registered like any other.
 *
 * For the host api groups that produce a list of plain strings without going
 * through a gpg gatherer -- the register table's child keys, today. Written
 * here rather than in the caller so that every list handle in the SDK is
 * created by the same code and therefore swept by the same code.
 *
 * @return 0 on success; @p out is untouched on failure.
 */
auto NewStringList(const QList<QString>& values, GFStringListRef* out) -> int;

}  // namespace gf_sdk_internal
