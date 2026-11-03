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

#include "core/function/gpg/GpgModelConvert.h"

#include "core/function/gpg/GpgErrorMap.h"

namespace GpgFrontend {

auto ConvertKeySignature(gpgme_key_sig_t sig) -> GFKeySignature {
  GFKeySignature out;
  if (sig == nullptr) return out;
  out.revoked = sig->revoked != 0;
  out.expired = sig->expired != 0;
  out.invalid = sig->invalid != 0;
  out.exportable = sig->exportable != 0;
  out.status = FromGpgError(sig->status);
  out.key_id = QString::fromUtf8(sig->keyid);
  out.pubkey_algo = QString::fromUtf8(gpgme_pubkey_algo_name(sig->pubkey_algo));
  out.created = sig->timestamp;
  out.expires = sig->expires;
  out.uid = QString::fromUtf8(sig->uid);
  out.name = QString::fromUtf8(sig->name);
  out.email = QString::fromUtf8(sig->email);
  out.comment = QString::fromUtf8(sig->comment);
  return out;
}

auto ConvertTofuInfo(gpgme_tofu_info_t info) -> GFTofuInfo {
  GFTofuInfo out;
  if (info == nullptr) return out;
  out.validity = info->validity;
  out.policy = info->policy;
  out.sign_count = info->signcount;
  out.encr_count = info->encrcount;
  out.sign_first = info->signfirst;
  out.sign_last = info->signlast;
  out.encr_last = info->encrlast;
  out.description = QString::fromUtf8(info->description);
  return out;
}

}  // namespace GpgFrontend
