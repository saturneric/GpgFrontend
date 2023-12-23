/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "core/GpgFrontendCoreExport.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

struct GPGFRONTEND_CORE_EXPORT GpgRecipient {
  /* The key ID of key for which the text was encrypted.  */
  std::string keyid;

  /* The public key algorithm of the recipient key.  */
  std::string pubkey_algo;

  /* The status of the recipient.  */
  GpgError status;

  GpgRecipient();

  explicit GpgRecipient(gpgme_recipient_t r);
};

}  // namespace GpgFrontend