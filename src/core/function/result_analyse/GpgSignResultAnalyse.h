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

#include "GpgResultAnalyse.h"
#include "core/model/GpgSignResult.h"

namespace GpgFrontend {

/**
 * @brief Analyses a GpgSignResult and formats a human-readable signing report.
 *
 * The generated report shows overall success or failure, then for each new
 * signature lists the sign mode, signer identity, key ID, public-key algorithm,
 * hash algorithm, and creation timestamp. Invalid signers are listed with their
 * fingerprint and error reason.
 */
class GF_CORE_EXPORT GpgSignResultAnalyse : public GpgResultAnalyse {
  Q_OBJECT
 public:
  /**
   * @brief Construct the analyser with the signing result to examine.
   *
   * @param channel OpenPGP context channel
   * @param error gpgme error code returned by the sign operation
   * @param result signing result object containing new signatures and invalid
   * signers
   */
  explicit GpgSignResultAnalyse(int channel, GpgError error,
                                GpgSignResult result);

 protected:
  /**
   * @brief Write the formatted signing report to stream_.
   *
   * Reports success or failure. For each valid new signature, writes the sign
   * mode, signer UID, key ID, public-key algorithm, hash algorithm, and sign
   * date (UTC and localized). For each invalid signer, writes the fingerprint
   * and error reason and lowers the status to 0.
   */
  void doAnalyse() override;

 private:
  // gpgme error code from the sign operation
  GpgError error_;
  // Signing result containing new signatures and invalid signers
  GpgSignResult result_;
};

}  // namespace GpgFrontend
