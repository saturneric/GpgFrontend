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

#include "GFCoreExport.h"

namespace GpgFrontend {

/**
 * @brief Validity level of a single verified signature.
 *
 * Derived from the GPGME summary bitmask and status error code during
 * GpgVerifyResultAnalyse::doAnalyse(), so UI code never needs gpgme.h.
 */
enum class GpgSigValidity : std::uint8_t {
  kFULLY_VALID,        ///< Cryptographically valid and key is fully trusted
  kVALID_WITH_ISSUES,  ///< Valid but key/sig carries revocation or RED flag
  kVALID_NOT_FULLY_TRUSTED,  ///< Cryptographically valid; trust level below
                             ///< "full"
  kINVALID,                  ///< Signature is cryptographically bad
  kKEY_MISSING,              ///< Public key is absent from the keyring
  kKEY_REVOKED,              ///< Signing key has been revoked
  kSIG_EXPIRED,              ///< Signature timestamp is past its expiry
  kKEY_EXPIRED,              ///< Signing key has expired
  kERROR,                    ///< Internal verification error (GPG_ERR_GENERAL)
  kUNKNOWN,                  ///< Unrecognised status code
};

/**
 * @brief Identity and algorithm details shared by Sign and Verify results.
 *
 * Fields are empty strings / null QDateTime when the signing key is not found
 * in the keyring.
 */
struct GF_CORE_EXPORT GpgSignerInfo {
  QString uid;  ///< Primary UID string "Name <email>", or empty if key absent
  QString fingerprint;  ///< Full fingerprint of the signing key or subkey
  QString keyId;        ///< Short key ID (populated when key is in the keyring)
  QString pubkeyAlgo;   ///< Public-key algorithm name, e.g. "RSA", "Ed25519"
  QString hashAlgo;     ///< Digest algorithm name, e.g. "SHA256", "SHA512"
  QDateTime signTime;   ///< Signature creation timestamp (UTC)
};

/**
 * @brief Per-signature detail produced by a Verify operation.
 *
 * Populated by GpgVerifyResultAnalyse::doAnalyse() — one entry per signature
 * present in the verified data.  @ref warnings mirrors the human-readable
 * strings emitted to the report for the same signature.
 */
struct GF_CORE_EXPORT GpgVerifySigInfo {
  GpgSignerInfo signer;     ///< Signer identity and algorithm details
  GpgSigValidity validity;  ///< Parsed validity level
  QStringList warnings;     ///< Issues found, e.g. "Signature has expired",
                            ///<   "Signing key has been revoked"
};

/**
 * @brief Per-signature detail produced by a Sign operation.
 *
 * Populated by GpgSignResultAnalyse::doAnalyse() — one entry per successfully
 * created signature.
 */
struct GF_CORE_EXPORT GpgNewSigInfo {
  GpgSignerInfo signer;  ///< Signer identity and algorithm details
  QString sigMode;  ///< Signature type string, e.g. "Normal", "Detach", "Clear"
};

/**
 * @brief Per-recipient detail produced by a Decrypt operation.
 *
 * Populated by GpgDecryptResultAnalyse::doAnalyse() — one entry per recipient
 * session-key record in the decrypted message.
 */
struct GF_CORE_EXPORT GpgRecipientInfo {
  QString uid;         ///< Recipient UID "Name <email>", or empty if key absent
  QString fingerprint; ///< Full fingerprint of the recipient key or subkey
  QString keyId;       ///< Key ID reported by GPGME (may be a subkey ID)
  QString pubkeyAlgo;  ///< Public-key algorithm used to encrypt the session key
  bool keyFound;       ///< True when the key was located in the local keyring
};

/**
 * @brief Structured result info exposed by a GpgResultAnalyse subclass.
 *
 * Populated during doAnalyse() so that UI consumers can display operation
 * details without parsing the formatted report text.
 *
 * ## Common fields
 * @ref operation and @ref engine are set by every subclass.
 * @ref details is a flat list of signer/recipient UIDs (or fingerprints when
 * the key is absent) kept for compact single-line status display.
 *
 * ## Sign (@ref newSignatures, @ref invalidSigners)
 * One @ref GpgNewSigInfo per successfully created signature; signer UID, key
 * ID, algorithm pair, timestamp, and signature mode (normal/detach/clear) are
 * all available.  Keys that could not be used are collected in
 * @ref invalidSigners as {fingerprint, error text} pairs.
 *
 * ## Verify (@ref signatures)
 * One @ref GpgVerifySigInfo per signature found in the data.  Each entry
 * carries the full @ref GpgSignerInfo plus a @ref GpgSigValidity level and a
 * list of human-readable @ref GpgVerifySigInfo::warnings (expired sig/key,
 * revoked key, etc.).
 *
 * ## Decrypt (@ref recipients, @ref filename, @ref mimeEncoded,
 *             @ref messageIntegrityProtected, @ref symmetricAlgo)
 * One @ref GpgRecipientInfo per session-key record.  Envelope metadata
 * (filename, MIME flag, MIP flag, symmetric cipher) is stored as individual
 * fields so the UI does not have to parse the formatted report.
 */
struct GF_CORE_EXPORT GpgOpResultInfo {
  // --- common ----------------------------------------------------------------

  int status = 1;  ///< Operation status: >0 success, 0 warning, <0 error
  QString report;  ///< Formatted text report for display

  QString operation;  ///< Human-readable operation name, e.g. "Sign", "Verify"
  QString engine;     ///< Engine version string, e.g. "GPG v2.4.1"

  /// Flat list of signer/recipient UIDs or fingerprints for compact display.
  /// Populated by every subclass; kept for backward-compatible single-line use.
  QStringList details;

  // --- Verify ----------------------------------------------------------------

  /// One entry per signature found in the verified data.
  /// Populated by GpgVerifyResultAnalyse only; empty for all other operations.
  QList<GpgVerifySigInfo> signatures;

  // --- Sign ------------------------------------------------------------------

  /// One entry per successfully created signature.
  /// Populated by GpgSignResultAnalyse only; empty for all other operations.
  QList<GpgNewSigInfo> newSignatures;

  /// {fingerprint, error text} for each signer key that could not be used.
  /// Populated by GpgSignResultAnalyse only; empty for all other operations.
  QList<QPair<QString, QString>> invalidSigners;

  // --- Decrypt ---------------------------------------------------------------

  /// One entry per recipient session-key record in the decrypted message.
  /// Populated by GpgDecryptResultAnalyse only; empty for all other operations.
  QList<GpgRecipientInfo> recipients;

  /// Original filename embedded in the OpenPGP literal packet, if present.
  QString filename;

  /// True when the decrypted content is MIME-encoded.
  bool mimeEncoded = false;

  /// True when the message was protected by a Modification Detection Code.
  /// False indicates a potentially unsafe legacy message.
  bool messageIntegrityProtected = false;

  /// Symmetric cipher used for the session key, e.g. "AES256".
  /// Empty when the algorithm is not reported by the engine.
  QString symmetricAlgo;

  /// SHA-256 hash of the input material (hex-encoded).
  /// Populated for Sign/Verify operations to identify the processed data.
  QString inputHash;

  /// Merge another GpgOpResultInfo into this one.
  /// Used by combined operations (Encrypt+Sign, Decrypt+Verify) to combine
  /// results from two sub-operations into a single displayable info.
  void Merge(const GpgOpResultInfo& other);
};

}  // namespace GpgFrontend
