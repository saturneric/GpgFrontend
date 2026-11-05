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

// What to ask before encrypting to or signing with a set of keys.

namespace GpgFrontend {

/**
 * @brief Whether a message to these recipients mixes post-quantum and
 * classical encryption.
 *
 * Each entry lists, for one recipient, whether each subkey the message may be
 * encrypted to is post-quantum. A recipient counts as post-quantum only if
 * all of them are: which usable subkey is chosen is up to the engine (GnuPG
 * takes the newest, rPGP the first), so this may report a mix the engine
 * would avoid, but never misses one. A recipient with no such subkey says
 * nothing either way. The session key is wrapped for every recipient, so the
 * message is only as resistant to a quantum attacker as its weakest one.
 *
 * Kept as a free function over plain bools so the rule is testable without
 * keys.
 *
 * @param recipients per recipient, whether each candidate subkey is
 * post-quantum
 * @return bool
 */
auto GF_SHELL_EXPORT
MixesPostQuantumAndClassical(const QList<QList<bool>>& recipients) -> bool;

/**
 * @brief MixesPostQuantumAndClassical() for recipient keys, judged by each
 * encryption subkey's KeyAlgo (KeyAlgo::IsPostQuantum()). A pinned (marked)
 * subkey stands alone; otherwise every usable encryption subkey counts. Key
 * groups are not expanded and are left out.
 *
 * @param keys the recipients
 * @return bool
 */
auto GF_SHELL_EXPORT
RecipientsMixPostQuantumAndClassical(const GpgAbstractKeyPtrList& keys) -> bool;

/**
 * @brief Whether any of these keys has more than one usable (not revoked, not
 * expired) signing subkey, the primary key included. The engine would then
 * pick one by itself, so the user should be asked which to sign with. Key
 * groups are left out.
 *
 * @param keys the signers
 * @return bool
 */
auto GF_SHELL_EXPORT
HasAmbiguousSigningSubkey(const GpgAbstractKeyPtrList& keys) -> bool;

/**
 * @brief HasAmbiguousSigningSubkey() for encryption: whether any recipient has
 * more than one usable encryption subkey.
 *
 * @param keys the recipients
 * @return bool
 */
auto GF_SHELL_EXPORT
HasAmbiguousEncryptionSubkey(const GpgAbstractKeyPtrList& keys) -> bool;

}  // namespace GpgFrontend
