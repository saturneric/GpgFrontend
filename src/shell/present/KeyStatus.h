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

#include "GFShellExport.h"
#include "core/model/GpgAbstractKey.h"
#include "core/model/KeyDatabaseInfo.h"
#include "core/typedef/GpgTypedef.h"

// The one condition worth showing about a key, and the built-in key categories.

namespace GpgFrontend {

/**
 * @brief Check whether a key carries no expiry at all.
 *
 * Both GnuPG and the rPGP key model report "no expiry" as the epoch, so an
 * expiration time of 0 must never be read as "expired in 1970".
 *
 * @param key key to inspect
 * @return true if the key never expires
 */
auto GF_SHELL_EXPORT IsKeyNeverExpires(const GpgAbstractKey* key) -> bool;

/**
 * @brief Number of days ahead within which a key counts as expiring soon.
 *
 * Read from the "keys/expiring_soon_days" setting, clamped to a sane range.
 *
 * @return lookahead window in days
 */
auto GF_SHELL_EXPORT GetKeyExpiringSoonDays() -> int;

/**
 * @brief Check whether a key is still usable but expires within the configured
 * lookahead window.
 *
 * Already expired, revoked and disabled keys are excluded: they need a
 * different remedy than a key that is merely about to lapse.
 *
 * @param key key to inspect
 * @return true if the key expires within the lookahead window
 */
auto GF_SHELL_EXPORT IsKeyExpiringSoon(const GpgAbstractKey* key) -> bool;

/**
 * @brief Whether @p id names a built-in key category
 * (MatchesBuiltinKeyCategory()), one decided by what a key is rather than by
 * the keys put into it.
 */
auto GF_SHELL_EXPORT IsBuiltinKeyCategory(const QString& id) -> bool;

/**
 * @brief Whether @p key belongs in the built-in key category @p id, as the
 * desktop's Key Management tabs and the phone's key list decide it.
 *
 * "all", "key_group", "only_public_key", "has_private_key", "expiring_soon",
 * "no_primary_key", "revoked", "expired" and "disabled". A key that is
 * revoked, disabled or expired is in none of "only_public_key",
 * "has_private_key" and "no_primary_key": it needs a different remedy.
 *
 * @param id the category's id
 * @param key key or key group to inspect
 * @return whether it belongs; nullopt when @p id is no built-in category
 */
auto GF_SHELL_EXPORT MatchesBuiltinKeyCategory(const QString& id,
                                               const GpgAbstractKey* key)
    -> std::optional<bool>;

/**
 * @brief The one condition worth reporting about a key, in severity order.
 */
enum class GpgKeyStatus {
  kOk,            ///< usable, and not about to lapse
  kExpiringSoon,  ///< still usable, expires within the lookahead window
  kExpired,       ///< past its expiry date
  kRevoked,       ///< revoked by its owner
  kDisabled,      ///< disabled locally
};

/**
 * @brief Reduce the four key conditions to the single one worth showing.
 *
 * A key can be several of these at once, so the precedence is the actual rule
 * and it is kept here, away from any key object, so it can be pinned down by a
 * test. It reproduces what the row tint has always done: disabled outranks
 * everything, then revoked or expired, then merely expiring soon.
 *
 * @param revoked key is revoked
 * @param disabled key is disabled
 * @param expired key is past its expiry
 * @param expiring_soon key lapses within the lookahead window
 * @return the single status to report
 */
auto GF_SHELL_EXPORT ClassifyKeyStatus(bool revoked, bool disabled,
                                       bool expired, bool expiring_soon)
    -> GpgKeyStatus;

/**
 * @brief Sort rank for a status, healthy first.
 *
 * @param status status to rank
 * @return rank, ascending by severity
 */
auto GF_SHELL_EXPORT KeyStatusSortRank(GpgKeyStatus status) -> int;

/**
 * @brief Translated one-word label for a status.
 *
 * @param status status to describe
 * @return label for the Status column
 */
auto GF_SHELL_EXPORT DescribeKeyStatus(GpgKeyStatus status) -> QString;

/**
 * @brief Reduce several owner-trust levels to the one that ranks the group.
 *
 * The table shows a key group's trust as a single value, "*" when its members
 * disagree. Sorting needs the same reduction as a number. Members that
 * disagree, and a group with no members at all, rank below every real level —
 * neither tells you the group can be trusted.
 *
 * Kept as a free function over plain ints rather than over keys so the rule is
 * testable without a gpg context.
 *
 * @param levels owner-trust level of each member, as GpgKey::OwnerTrustLevel()
 * @return rank, ascending by trust; -1 when mixed or empty
 */
auto GF_SHELL_EXPORT AggregateOwnerTrustRank(const QList<int>& levels) -> int;

/**
 * @brief Classify a key, reading the four conditions off the key itself.
 *
 * @param key key to inspect
 * @return the single status to report
 */
auto GF_SHELL_EXPORT GetKeyStatus(const GpgAbstractKey* key) -> GpgKeyStatus;

}  // namespace GpgFrontend
