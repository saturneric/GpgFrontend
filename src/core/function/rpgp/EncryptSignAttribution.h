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

#include "core/typedef/GFTypedef.h"

namespace GpgFrontend {

/**
 * @brief Name the right side of an encrypt+sign that failed with an unusable
 * key.
 *
 * Both halves receive one engine status. An unusable key means the
 * recipients when the engine listed refused recipients, and otherwise a
 * signer that may not sign; both halves then report that one cause.
 *
 * @param unusable_key whether the shared engine status is "unusable key"
 * @param encrypt_result the encrypt half, with its invalid recipients
 * @param encrypt_err the encrypt half's error, corrected in place
 * @param sign_err the sign half's error, corrected in place
 */
inline void AttributeUnusableKey(bool unusable_key,
                                 const GFEncryptResult& encrypt_result,
                                 GpgError& encrypt_err, GpgError& sign_err) {
  if (!unusable_key) return;
  const GpgError cause = encrypt_result.invalid_recipients.empty()
                             ? GPG_ERR_UNUSABLE_SECKEY
                             : GPG_ERR_UNUSABLE_PUBKEY;
  encrypt_err = cause;
  sign_err = cause;
}

}  // namespace GpgFrontend
