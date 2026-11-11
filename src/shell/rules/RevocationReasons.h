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

namespace GpgFrontend::Shell {

/// What a revocation ends: the whole key, one subkey, or one user ID.
enum class RevocationTarget { kKey, kSubkey, kUserId };

/// One reason OpenPGP lets a revocation give, and how it is said.
struct RevocationReason {
  int code;
  QString text;
};

/**
 * @brief The reasons a revocation of @p target may give, in OpenPGP's order,
 * in the words both interfaces offer them.
 *
 * A key or subkey takes codes 0 to 3 (no reason, compromised, superseded,
 * no longer used); a user ID only 0 and 4, no longer valid (RFC 9580
 * 5.2.3.31), and the core refuses any other for it.
 */
auto GF_SHELL_EXPORT RevocationReasonsFor(RevocationTarget target)
    -> QList<RevocationReason>;

/// The reason first offered for @p target: no longer used (3), or no longer
/// valid (4) for a user ID.
auto GF_SHELL_EXPORT DefaultRevocationReason(RevocationTarget target) -> int;

}  // namespace GpgFrontend::Shell
