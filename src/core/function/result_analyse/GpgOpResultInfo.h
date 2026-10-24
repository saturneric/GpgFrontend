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
#include "core/typedef/GpgTypedef.h"

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
  kKEY_NOT_AUTHORIZED,       ///< Mathematically valid, but the certificate
                             ///<   does not authorize the key to sign
                             ///<   (appended so the values modules already
                             ///<   know keep their meaning)
};

/**
 * @brief The operation two merged results describe together.
 *
 * {Encrypt, Sign} and {Decrypt, Verify} combine; an operation merged with
 * itself or with kUNKNOWN stays itself; any other pair is kUNKNOWN.
 */
auto GF_CORE_EXPORT CombineGpgOperations(GpgOperation a, GpgOperation b)
    -> GpgOperation;

/// Display name of @p op, for presentation only.
auto GF_CORE_EXPORT GpgOperationName(GpgOperation op) -> QString;

/// Where a recorded error came from.
enum class GpgErrorOrigin : std::uint8_t {
  kENGINE,       ///< the code an engine call returned
  kAPPLICATION,  ///< synthesized by GpgFrontend: no code and no usable result
};

/**
 * @brief One error recorded for an operation, kept verbatim.
 *
 * @ref scope is the single stage the error is known to belong to, or a combined
 * operation when only the combined engine call is known to have failed.
 */
struct GF_CORE_EXPORT GpgOpStageError {
  GpgOperation scope = GpgOperation::kNONE;
  GpgErrorOrigin origin = GpgErrorOrigin::kENGINE;
  unsigned int code = 0;  ///< gpg_err_code_t; non-zero for kENGINE
  QString codeText;  ///< kENGINE: text derived from the code (gpgme_strerror);
                     ///< kAPPLICATION: GpgFrontend's own message
  /// Detail text an engine result carried, verbatim, tagged with the stage
  /// whose result held it.
  QList<QPair<GpgOperation, QString>> engineDetails;
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
  bool rejected = false;    ///< The signature must not be relied on although
                            ///<   @ref validity names only a key state (rPGP:
                            ///<   the key had expired before it signed)
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
  bool algoIsPrimaryKey =
      false;  ///< True when pubkeyAlgo/keyId describe the primary key rather
              ///< than the encryption subkey actually used (GnuPG engine, which
              ///< does not report the recipient subkey)
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

  /// Operation status: >0 success, 0 valid output with non-fatal warnings
  /// (kept), <0 failure (output suppressed). See GpgResultAnalyse.
  int status = 1;
  QString report;  ///< Formatted text report for display

  /// Stable identity of the operation; what logic compares.
  GpgOperation op = GpgOperation::kNONE;
  /// Display name of @ref op, e.g. "Sign"; kept for the SDK's JSON. Never
  /// compared.
  QString operation;
  QString description;  ///< Plain-language summary set by result analyse for UI display
  QString engine;       ///< Engine version string, e.g. "GPG v2.4.1"

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

  /// Every error recorded for this operation, as reported. Never collapsed to
  /// one: a combined operation keeps each stage's error.
  QList<GpgOpStageError> stageErrors;

  /// Merge another GpgOpResultInfo into this one.
  /// Used by combined operations (Encrypt+Sign, Decrypt+Verify) to combine
  /// results from two sub-operations into a single displayable info.
  void Merge(const GpgOpResultInfo& other);

  /**
   * @brief Re-attribute an error both stages received from one engine call.
   *
   * Each stage's analyser records the call's error as its own, which would
   * claim two failures where the evidence shows one. Entries with
   * @p call_error_code collapse into one entry scoped to @p combined; the
   * detail text each stage's result held is kept, still tagged by stage.
   * Entries with any other code are left as they are.
   */
  void CollapseSharedCallError(unsigned int call_error_code,
                               GpgOperation combined);
};

}  // namespace GpgFrontend
