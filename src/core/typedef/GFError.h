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

#include <cstdint>

namespace GpgFrontend {

/**
 * @brief What went wrong, in GpgFrontend's own terms.
 *
 * The same list whichever engine ran the operation. Each engine's own codes
 * are mapped onto it where they leave that engine, and kept beside it in
 * GFError::native for diagnostics.
 */
enum class GFErrorCode : std::uint16_t {
  kOK = 0,
  kCANCELED,               ///< the user stopped it
  kABORTED,                ///< GpgFrontend stopped it before a result
  kGENERAL,                ///< failed, for no reason more specific below
  kNOT_SUPPORTED,          ///< a feature the engine does not offer
  kUNSUPPORTED_OPERATION,  ///< an operation the engine does not offer
  kINVALID_ARGUMENT,
  kINVALID_VALUE,
  kINVALID_DATA,
  kBAD_DATA,
  kNO_DATA,
  kBAD_ARMOR,
  kNO_PUBLIC_KEY,
  kNO_SECRET_KEY,
  kNO_KEY,
  kKEY_EXPIRED,
  kKEY_REVOKED,
  kUNUSABLE_PUBLIC_KEY,
  kUNUSABLE_SECRET_KEY,
  kWRONG_KEY_USAGE,
  kUNSUPPORTED_ALGORITHM,
  kNO_PASSPHRASE,
  kBAD_PASSPHRASE,
  kINVALID_PASSPHRASE,
  kDECRYPT_FAILED,
  kBAD_SIGNATURE,
  kSIGNATURE_EXPIRED,
  kNOT_FOUND,
  kNOT_OPERATIONAL,
  kTIMEOUT,
  kIO,
  kINTERNAL,
  kBUG,  ///< the engine failed in a way it should never fail
};

/// Who reported the error.
enum class GFErrorOrigin : std::uint8_t {
  kNONE,         ///< nobody: there is no error
  kAPPLICATION,  ///< GpgFrontend itself
  kGNUPG,        ///< the GnuPG engine; native holds its gpg_error_t
  kRPGP,         ///< the rPGP engine; native holds its status
};

/**
 * @brief An operation's outcome: success, cancellation, abortion or failure.
 *
 * Deliberately not a number. The code says what happened; the origin and
 * native value say who said so and in their own words, for diagnostics.
 */
struct GFError {
  GFErrorCode code = GFErrorCode::kOK;
  GFErrorOrigin origin = GFErrorOrigin::kNONE;
  std::int32_t native = 0;  ///< the engine's own value, 0 when none

  [[nodiscard]] auto Ok() const -> bool { return code == GFErrorCode::kOK; }

  [[nodiscard]] auto Canceled() const -> bool {
    return code == GFErrorCode::kCANCELED;
  }

  [[nodiscard]] auto Aborted() const -> bool {
    return code == GFErrorCode::kABORTED;
  }

  /// Neither a success, nor stopped by the user or by GpgFrontend.
  [[nodiscard]] auto Failed() const -> bool {
    return !Ok() && !Canceled() && !Aborted();
  }

  /// An error GpgFrontend itself reports.
  static auto Application(GFErrorCode code) -> GFError {
    return {code,
            code == GFErrorCode::kOK ? GFErrorOrigin::kNONE
                                     : GFErrorOrigin::kAPPLICATION,
            0};
  }
};

}  // namespace GpgFrontend
