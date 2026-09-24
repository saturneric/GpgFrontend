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

#include <QByteArray>
#include <QList>
#include <QString>
#include <optional>

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
 * risked those behaviors for no gain.
 *
 * What DID change is that they are no longer exported: modules reach this
 * functionality through the opaque list handles in GFSDKGpgList.h, which have
 * one release per list and borrowed element accessors, rather than through a
 * caller-allocated array plus a matching free-the-array function plus a count
 * the caller has to carry around.
 *
 * Internal to gf_sdk. Not installed, not for modules.
 */

namespace GpgFrontend {
class GFBuffer;
}  // namespace GpgFrontend

namespace gf_host {

/// One key brief, held as real C++ members.
///
/// Nothing in here is separately allocated, so there is nothing for a caller
/// to free field by field and therefore nothing to forget. The list handles
/// hand out pointers INTO these members.
struct KeyBriefRow {
  QByteArray fingerprint;
  QByteArray key_id;
  QByteArray uid;            ///< primary UID, "Name (Comment) <email>"
  QByteArray matched_email;  ///< the UID email that matched the query
  int64_t expires_at = 0;    ///< seconds since the epoch; 0 means never
  /// Mirrors GpgFrontend::GpgKeyStatus:
  /// 0 ok, 1 expiring soon, 2 expired, 3 revoked, 4 disabled.
  int usability = 0;
  int can_encrypt = 0;
  int can_sign = 0;
  /// Whether the matched UID is the key's primary one, and whether that UID
  /// has itself been revoked. Identity-binding facts, not usability ones.
  int matched_uid_is_primary = 0;
  int matched_uid_revoked = 0;
};

struct RecipientRow {
  /// As the message names it: an 8-byte key id (v3 PKESK) or a full
  /// fingerprint (v6 PKESK), upper-cased.
  QByteArray key_id;
  QByteArray pub_algo;
  /// Of the key this resolved to; empty when nothing resolved.
  QByteArray fingerprint;
  /// Primary UID of the key this resolved to; empty when nothing resolved.
  QByteArray uid;
  /// Whether @ref key_id names a key this channel's key database holds at all.
  int key_found = 0;
  /// Whether the secret half is held -- the only field that answers "can this
  /// message be opened on this computer". A public key alone cannot decrypt.
  int has_secret = 0;
  /// The sender withheld the recipient key id (`--hidden-recipient`), so this
  /// recipient is deliberately unidentifiable rather than missing. It may
  /// still be the user themselves.
  int hidden = 0;
};

/// Every key with a UID whose address is @p email. Nullopt when the request
/// is unusable -- an empty address -- as opposed to matching nothing.
auto FindKeyBriefRows(int channel, const QString& email)
    -> std::optional<QList<KeyBriefRow>>;

/// Who @p data is encrypted to, as far as this channel's keys can tell.
auto SniffRecipientRows(int channel, const GpgFrontend::GFBuffer& data)
    -> QList<RecipientRow>;

/// Every address a key on this channel carries -- only keys with a secret
/// half when @p secret_only -- deduplicated and sorted, "Name <email>".
auto ListKeyAddressRows(int channel, bool secret_only) -> QList<QByteArray>;

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
