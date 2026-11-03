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

#include "core/function/gpg/GpgErrorMap.h"

#include <array>
#include <utility>

namespace GpgFrontend {

namespace {

/// Each of our codes beside the GnuPG code it corresponds to.
constexpr std::array<std::pair<GFErrorCode, gpg_err_code_t>, 32> kGpgCodes{{
    {GFErrorCode::kCANCELED, GPG_ERR_CANCELED},
    // GnuPG never reports it; GpgFrontend has used it to mark its own stop.
    {GFErrorCode::kABORTED, GPG_ERR_USER_1},
    {GFErrorCode::kGENERAL, GPG_ERR_GENERAL},
    {GFErrorCode::kNOT_SUPPORTED, GPG_ERR_NOT_SUPPORTED},
    {GFErrorCode::kUNSUPPORTED_OPERATION, GPG_ERR_UNSUPPORTED_OPERATION},
    {GFErrorCode::kINVALID_ARGUMENT, GPG_ERR_INV_ARG},
    {GFErrorCode::kINVALID_VALUE, GPG_ERR_INV_VALUE},
    {GFErrorCode::kINVALID_DATA, GPG_ERR_INV_DATA},
    {GFErrorCode::kBAD_DATA, GPG_ERR_BAD_DATA},
    {GFErrorCode::kNO_DATA, GPG_ERR_NO_DATA},
    {GFErrorCode::kBAD_ARMOR, GPG_ERR_INV_ARMOR},
    {GFErrorCode::kNO_PUBLIC_KEY, GPG_ERR_NO_PUBKEY},
    {GFErrorCode::kNO_SECRET_KEY, GPG_ERR_NO_SECKEY},
    {GFErrorCode::kNO_KEY, GPG_ERR_NO_KEY},
    {GFErrorCode::kKEY_EXPIRED, GPG_ERR_KEY_EXPIRED},
    {GFErrorCode::kKEY_REVOKED, GPG_ERR_CERT_REVOKED},
    {GFErrorCode::kUNUSABLE_PUBLIC_KEY, GPG_ERR_UNUSABLE_PUBKEY},
    {GFErrorCode::kUNUSABLE_SECRET_KEY, GPG_ERR_UNUSABLE_SECKEY},
    {GFErrorCode::kWRONG_KEY_USAGE, GPG_ERR_WRONG_KEY_USAGE},
    {GFErrorCode::kUNSUPPORTED_ALGORITHM, GPG_ERR_UNSUPPORTED_ALGORITHM},
    {GFErrorCode::kNO_PASSPHRASE, GPG_ERR_NO_PASSPHRASE},
    {GFErrorCode::kBAD_PASSPHRASE, GPG_ERR_BAD_PASSPHRASE},
    {GFErrorCode::kINVALID_PASSPHRASE, GPG_ERR_INV_PASSPHRASE},
    {GFErrorCode::kDECRYPT_FAILED, GPG_ERR_DECRYPT_FAILED},
    {GFErrorCode::kBAD_SIGNATURE, GPG_ERR_BAD_SIGNATURE},
    {GFErrorCode::kSIGNATURE_EXPIRED, GPG_ERR_SIG_EXPIRED},
    {GFErrorCode::kNOT_FOUND, GPG_ERR_NOT_FOUND},
    {GFErrorCode::kNOT_OPERATIONAL, GPG_ERR_NOT_OPERATIONAL},
    {GFErrorCode::kTIMEOUT, GPG_ERR_TIMEOUT},
    {GFErrorCode::kIO, GPG_ERR_EIO},
    {GFErrorCode::kINTERNAL, GPG_ERR_INTERNAL},
    {GFErrorCode::kBUG, GPG_ERR_BUG},
}};

}  // namespace

auto FromGpgError(gpgme_error_t err) -> GFError {
  const auto code = gpg_err_code(err);
  if (code == GPG_ERR_NO_ERROR) return {};

  auto mapped = GFErrorCode::kGENERAL;
  for (const auto& [ours, theirs] : kGpgCodes) {
    if (theirs == code) {
      mapped = ours;
      break;
    }
  }
  return {mapped,
          mapped == GFErrorCode::kABORTED ? GFErrorOrigin::kAPPLICATION
                                          : GFErrorOrigin::kGNUPG,
          static_cast<std::int32_t>(err)};
}

auto ToGpgError(const GFError& err) -> gpgme_error_t {
  if (err.origin == GFErrorOrigin::kGNUPG) {
    return static_cast<gpgme_error_t>(err.native);
  }
  if (err.code == GFErrorCode::kOK) return GPG_ERR_NO_ERROR;

  for (const auto& [ours, theirs] : kGpgCodes) {
    if (ours == err.code) return theirs;
  }
  return GPG_ERR_GENERAL;
}

}  // namespace GpgFrontend
