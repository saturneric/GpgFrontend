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

#include <array>
#include <cstdint>
#include <utility>

#include "core/typedef/GFError.h"

/**
 * @file GpgErrorCodes.h
 * @brief GnuPG's numbers for GpgFrontend's error codes, without GnuPG.
 *
 * The module SDK has always handed modules GnuPG's error values, and errors
 * are shown numbered as GnuPG numbers them. A build without the GnuPG engine
 * has no gpg-error.h to take the numbers from, so they are written out here,
 * and GpgErrorMap.cpp checks each one against gpg-error.h wherever it is
 * compiled.
 */

namespace GpgFrontend {

/// The code part of a GnuPG error value; the rest names its source.
constexpr std::uint32_t kGnuPGCodeMask = 0xFFFF;

/// Each of our codes beside the GnuPG code number it corresponds to.
constexpr std::array<std::pair<GFErrorCode, std::uint32_t>, 32> kGnuPGCodes{{
    {GFErrorCode::kCANCELED, 99},  // GPG_ERR_CANCELED
    // GnuPG never reports it; GpgFrontend has used it to mark its own stop.
    {GFErrorCode::kABORTED, 1024},      // GPG_ERR_USER_1
    {GFErrorCode::kGENERAL, 1},         // GPG_ERR_GENERAL
    {GFErrorCode::kNOT_SUPPORTED, 60},  // GPG_ERR_NOT_SUPPORTED
    {GFErrorCode::kUNSUPPORTED_OPERATION,
     124},                                      // GPG_ERR_UNSUPPORTED_OPERATION
    {GFErrorCode::kINVALID_ARGUMENT, 45},       // GPG_ERR_INV_ARG
    {GFErrorCode::kINVALID_VALUE, 55},          // GPG_ERR_INV_VALUE
    {GFErrorCode::kINVALID_DATA, 79},           // GPG_ERR_INV_DATA
    {GFErrorCode::kBAD_DATA, 89},               // GPG_ERR_BAD_DATA
    {GFErrorCode::kNO_DATA, 58},                // GPG_ERR_NO_DATA
    {GFErrorCode::kBAD_ARMOR, 15},              // GPG_ERR_INV_ARMOR
    {GFErrorCode::kNO_PUBLIC_KEY, 9},           // GPG_ERR_NO_PUBKEY
    {GFErrorCode::kNO_SECRET_KEY, 17},          // GPG_ERR_NO_SECKEY
    {GFErrorCode::kNO_KEY, 221},                // GPG_ERR_NO_KEY
    {GFErrorCode::kKEY_EXPIRED, 153},           // GPG_ERR_KEY_EXPIRED
    {GFErrorCode::kKEY_REVOKED, 94},            // GPG_ERR_CERT_REVOKED
    {GFErrorCode::kUNUSABLE_PUBLIC_KEY, 53},    // GPG_ERR_UNUSABLE_PUBKEY
    {GFErrorCode::kUNUSABLE_SECRET_KEY, 54},    // GPG_ERR_UNUSABLE_SECKEY
    {GFErrorCode::kWRONG_KEY_USAGE, 125},       // GPG_ERR_WRONG_KEY_USAGE
    {GFErrorCode::kUNSUPPORTED_ALGORITHM, 84},  // GPG_ERR_UNSUPPORTED_ALGORITHM
    {GFErrorCode::kNO_PASSPHRASE, 177},         // GPG_ERR_NO_PASSPHRASE
    {GFErrorCode::kBAD_PASSPHRASE, 11},         // GPG_ERR_BAD_PASSPHRASE
    {GFErrorCode::kINVALID_PASSPHRASE, 31},     // GPG_ERR_INV_PASSPHRASE
    {GFErrorCode::kDECRYPT_FAILED, 152},        // GPG_ERR_DECRYPT_FAILED
    {GFErrorCode::kBAD_SIGNATURE, 8},           // GPG_ERR_BAD_SIGNATURE
    {GFErrorCode::kSIGNATURE_EXPIRED, 154},     // GPG_ERR_SIG_EXPIRED
    {GFErrorCode::kNOT_FOUND, 27},              // GPG_ERR_NOT_FOUND
    {GFErrorCode::kNOT_OPERATIONAL, 176},       // GPG_ERR_NOT_OPERATIONAL
    {GFErrorCode::kTIMEOUT, 62},                // GPG_ERR_TIMEOUT
    {GFErrorCode::kIO, 32817},                  // GPG_ERR_EIO
    {GFErrorCode::kINTERNAL, 63},               // GPG_ERR_INTERNAL
    {GFErrorCode::kBUG, 59},                    // GPG_ERR_BUG
}};

/// GnuPG's code number for @p code; 0 for success, general failure for none.
constexpr auto GnuPGCodeOf(GFErrorCode code) -> std::uint32_t {
  if (code == GFErrorCode::kOK) return 0;
  for (const auto& [ours, theirs] : kGnuPGCodes) {
    if (ours == code) return theirs;
  }
  return 1;  // GPG_ERR_GENERAL
}

/// Our code for GnuPG's code number @p gnupg_code (the code part of an error
/// value); a code with no counterpart is a general failure.
constexpr auto GFErrorCodeOfGnuPG(std::uint32_t gnupg_code) -> GFErrorCode {
  if (gnupg_code == 0) return GFErrorCode::kOK;
  for (const auto& [ours, theirs] : kGnuPGCodes) {
    if (theirs == gnupg_code) return ours;
  }
  return GFErrorCode::kGENERAL;
}

}  // namespace GpgFrontend
