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
#include "core/model/GpgSignature.h"
#include "core/model/GpgVerifyResult.h"

namespace GpgFrontend {

/**
 * @brief Analyses a GpgVerifyResult and formats a human-readable verification
 * report.
 *
 * The generated report covers the overall success or failure, the sign
 * timestamp, and per-signature details including validity, trust level,
 * expiry, revocation, and missing-key conditions. Fingerprints of signatures
 * whose public key is not present in the keyring are collected and available
 * via GetUnknownSignatures().
 */
class GF_CORE_EXPORT GpgVerifyResultAnalyse : public GpgResultAnalyse {
  Q_OBJECT
 public:
  /**
   * @brief Construct the analyser with the verification result to examine.
   *
   * @param channel OpenPGP context channel
   * @param error gpgme error code returned by the verify operation
   * @param result verification result object containing the signature list
   */
  explicit GpgVerifyResultAnalyse(int channel, GpgError error,
                                  GpgVerifyResult result);

  /**
   * @brief Return the raw gpgme signature list from the verify result.
   *
   * @return pointer to the first gpgme_signature_t, or nullptr if the result is
   * invalid
   */
  [[nodiscard]] auto GetSignatures() const -> gpgme_signature_t;

  /**
   * @brief Transfer ownership of the underlying GpgVerifyResult out of this
   * object.
   *
   * @return the GpgVerifyResult held by this analyser
   */
  auto TakeChargeOfResult() -> GpgVerifyResult;

  /**
   * @brief Return fingerprints of signatures whose public key was not found in
   * the keyring.
   *
   * Populated during doAnalyse() for each GPG_ERR_NO_PUBKEY signature.
   *
   * @return list of fingerprint strings for unknown signers
   */
  [[nodiscard]] auto GetUnknownSignatures() const -> QStringList;

 protected:
  /**
   * @brief Write the formatted verification report to stream_.
   *
   * Reports the overall status and, for each signature, its validity,
   * trust level, and any warnings (expired signature/key, revoked key,
   * missing CRL). Collects unknown-signer fingerprints into
   * unknown_signer_fpr_list_.
   */
  void doAnalyse() final;

 private:
  /**
   * @brief Write signer details for a signature whose key is present in the
   * keyring.
   *
   * Looks up the key via AbstractKeyRepository. If found, writes the signer
   * UID, key ID, key creation date, public-key algorithm, hash algorithm, and
   * sign timestamps. If not found, writes the fingerprint and lowers the
   * status to 0.
   *
   * @param stream output stream to write into
   * @param sign signature to describe
   * @return true if the key was found, false if the signer is unknown
   */
  auto print_signer(QTextStream &stream, GpgSignature sign) -> bool;

  /**
   * @brief Write signer details for a signature whose key is absent from the
   * keyring.
   *
   * Writes the fingerprint (or "<unknown>"), public-key algorithm, hash
   * algorithm, and sign timestamps without performing a key lookup.
   *
   * @param stream output stream to write into
   * @param sign signature to describe
   * @return always true
   */
  auto print_signer_without_key(QTextStream &stream, GpgSignature sign) -> bool;

  // gpgme error code from the verify operation
  GpgError error_;
  // Verification result containing the signature list
  GpgVerifyResult result_;
  // Fingerprints of signers whose key was not found
  QStringList unknown_signer_fpr_list_;
};

}  // namespace GpgFrontend
