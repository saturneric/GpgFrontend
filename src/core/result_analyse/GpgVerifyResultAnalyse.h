/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef GPGFRONTEND_GPGVERIFYRESULTANALYSE_H
#define GPGFRONTEND_GPGVERIFYRESULTANALYSE_H

#include "GpgResultAnalyse.h"
#include "core/model/GpgKeySignature.h"

namespace GpgFrontend {
/**
 * @brief
 *
 */
class GpgVerifyResultAnalyse : public GpgResultAnalyse {
 public:
  /**
   * @brief Construct a new Verify Result Analyse object
   *
   * @param error
   * @param result
   */
  explicit GpgVerifyResultAnalyse(GpgError error, GpgVerifyResult result);

  /**
   * @brief Get the Signatures object
   *
   * @return gpgme_signature_t
   */
  gpgme_signature_t GetSignatures() const;

  /**
   * @brief
   *
   * @return GpgVerifyResult
   */
  GpgVerifyResult TakeChargeOfResult();

 private:
  /**
   * @brief
   *
   */
  void do_analyse();

 private:
  /**
   * @brief
   *
   * @param stream
   * @param sign
   * @return true
   * @return false
   */
  bool print_signer(std::stringstream &stream, gpgme_signature_t sign);

  GpgError error_;          ///<
  GpgVerifyResult result_;  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGVERIFYRESULTANALYSE_H
