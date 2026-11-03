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

#include <memory>
#include <optional>

#include "core/model/GFBuffer.h"
#include "core/typedef/CoreTypedef.h"
#include "core/typedef/GFError.h"
#include "core/typedef/GpgErrorTypedef.h"

namespace GpgFrontend {

enum class OpenPGPEngine : std::uint8_t {
  kGNUPG,
  kRPGP,
};

/**
 * @brief Why the OpenPGP environment could not be brought up.
 *
 * Carried by CoreSignalStation::SignalBadOpenPGPEnv so the UI can title the
 * failure after what actually went wrong. A single shared signal used to be
 * presented as "No Supported OpenPGP Engine Found" for every cause, which was
 * accurate for exactly one of them and actively misleading for the rest --
 * a missing key database is not a missing engine.
 *
 * Lives here rather than beside the signal because a typed enum declared ahead
 * of a Q_OBJECT class drops the namespace from that class's tr() context under
 * lupdate, breaking its translations at runtime. This header has no Q_OBJECT
 * class in it.
 */
enum class BadOpenPGPEnvReason : std::uint8_t {
  kUNKNOWN,
  kBASIC_PATH_INIT_FAILED,       ///< app data paths could not be established
  kNO_SUPPORTED_ENGINE,          ///< genuinely no usable engine
  kNO_VALID_KEY_DATABASE,        ///< no key database resolved to a directory
  kDEFAULT_CONTEXT_INIT_FAILED,  ///< the channel-0 context would not build
  kKEY_CACHE_INIT_FAILED,        ///< the context built but the keys would not
};

/**
 * @brief Which of the four startup tracks a progress report belongs to.
 *
 * The tracks do not run one after another: modules load on their own task
 * runner while the core is still probing gpgconf. So a single percentage is a
 * weighted blend of all four rather than a position along a line -- see
 * CoreInitProgress.
 */
enum class CoreInitStage : std::uint8_t {
  kCORE,          ///< process-level setup, before any engine exists
  kENGINE,        ///< gpgme/rPGP backend selection and the channel-0 context
  kKEY_DATABASE,  ///< one keyring flush per configured key database
  kMODULES,       ///< discovery, verification and registration of modules
};

/**
 * @brief What the startup sequence is doing right now.
 *
 * Deliberately a code rather than a sentence. The core begins initializing
 * before InitUITranslations() has installed a single translator, so a string
 * built in the core would resolve against an empty catalog and reach the user
 * untranslated. The UI owns every tr() template and maps it from this.
 *
 * kLOADING_KEY_DATABASE and kLOADING_MODULE carry a subject (the database or
 * module name); the rest carry none.
 */
enum class CoreInitStep : std::uint8_t {
  kSTARTING_UP,
  kCHECKING_GNUPG_ENV,
  kCHECKING_RUST_ENGINE,
  kRESOLVING_PATHS,
  kREFRESHING_BACKEND_ENGINE,
  kBUILDING_DEFAULT_CONTEXT,
  kLOADING_KEY_DATABASE,
  kSCANNING_MODULES,
  kVERIFYING_MODULES,
  kLOADING_MODULE,
  kREADY,
};

/// How far a signature, or a key, is trusted.
enum class GpgValidity : std::uint8_t {
  kUNKNOWN,
  kUNDEFINED,
  kNEVER,
  kMARGINAL,
  kFULL,
  kULTIMATE,
};

/// What the checks of a verified signature found, as flags.
enum GpgSigSummary : std::uint32_t {
  kSIGSUM_NONE = 0,
  kSIGSUM_VALID = 1U << 0,        ///< fully valid
  kSIGSUM_GREEN = 1U << 1,        ///< valid, with no problem found
  kSIGSUM_RED = 1U << 2,          ///< bad, or a problem found
  kSIGSUM_KEY_REVOKED = 1U << 3,  ///< the key was revoked
  kSIGSUM_KEY_EXPIRED = 1U << 4,  ///< the key had expired
  kSIGSUM_SIG_EXPIRED = 1U << 5,  ///< the signature had expired
  kSIGSUM_KEY_MISSING = 1U << 6,  ///< the key is not in the keyring
  kSIGSUM_CRL_MISSING = 1U << 7,  ///< no revocation list to check against
};

/// A certification of a user ID, as listed with its key.
struct GFKeySignature {
  bool revoked = false;
  bool expired = false;
  bool invalid = false;
  bool exportable = false;
  GFError status;
  QString key_id;
  QString pubkey_algo;
  qint64 created = 0;  ///< seconds since the epoch
  qint64 expires = 0;  ///< seconds since the epoch, 0 for never
  QString uid;
  QString name;
  QString email;
  QString comment;
};

/// What GnuPG's trust-on-first-use database records for a user ID. Validity
/// and policy are GnuPG's own numbers.
struct GFTofuInfo {
  unsigned validity = 0;
  unsigned policy = 0;
  unsigned long sign_count = 0;
  unsigned long encr_count = 0;
  unsigned long sign_first = 0;
  unsigned long sign_last = 0;
  unsigned long encr_last = 0;
  QString description;
};

struct GFUserId {
  QString name;
  QString email;
  QString comment;
  bool is_primary = false;
  bool is_revoked = false;
  bool is_invalid = false;

  /// The user ID exactly as the engine listed it; empty when it keeps only
  /// the parts, which are then assembled.
  QString raw;

  QList<GFKeySignature> signatures;  ///< its certifications, when listed
  QList<GFTofuInfo> tofu;            ///< GnuPG's TOFU records, when listed

  GFUserId() = default;

  explicit GFUserId(const QString& uid);

  GFUserId(const GFUserId&) = default;

  auto operator=(const GFUserId&) -> GFUserId& = default;

  GFUserId(QString name, QString email, QString comment, bool is_primary);

  [[nodiscard]] auto ToString() const -> QString;
};

struct GFSubKeyMetadata {
  int key_ver = 0;
  QString fpr;
  QString key_id;
  int algo = 0;
  unsigned key_length = 0;

  qint64 created_at = 0;
  qint64 expires_at = 0;  ///< Unix epoch seconds; 0 means never expires.
  bool has_secret = false;
  bool is_revoked = false;

  bool can_sign = false;
  bool can_encrypt = false;
  bool can_auth = false;
  bool can_certify = false;

  bool marked = false;

  // As an engine that tracks them reports them; left at their defaults
  // otherwise.
  QString keygrip;
  bool is_disabled = false;
  bool is_card_key = false;
  QString card_serial;
  bool can_renc = false;           ///< an ADSK: may be used to re-encrypt
  std::optional<bool> is_expired;  ///< the engine's verdict; else from the date

  /// The engine's own names for the algorithm: its short name, and the full
  /// algorithm string. Empty when only `algo` names it.
  QString algo_name;
  QString algo_string;
};

struct GFKeyMetadata {
  int key_ver = 0;
  QString fpr;
  QString key_id;

  QList<GFUserId> user_ids;

  int algo = 0;
  unsigned key_length = 0;
  qint64 created_at = 0;
  qint64 expires_at = 0;  ///< Unix epoch seconds; 0 means never expires.
  qint64 update_time = 0;
  bool has_secret = false;
  bool is_revoked = false;
  bool is_disabled = false;

  bool can_sign = false;
  bool can_encrypt = false;
  bool can_auth = false;
  bool can_certify = false;

  QList<GFSubKeyMetadata> subkeys;

  /// How far its owner is trusted; an engine without a trust model trusts its
  /// own keys ultimately.
  GpgValidity owner_trust = GpgValidity::kULTIMATE;
  QString protocol;  ///< the engine's protocol name, if it has one
  std::optional<bool> is_expired;  ///< the engine's verdict; else from the date

  /// The engine's own handle for the key, for that engine's backend alone to
  /// hand back to it; null when it keeps none.
  std::shared_ptr<void> native;
};

struct GFKeyBlocks {
  GFBuffer public_key;
  GFBuffer secret_key;
};

struct GFKey {
  GFKeyMetadata metadata;
  GFKeyBlocks blocks;
};

enum class GFSignatureStatus : uint8_t {
  kVALID = 0,
  kBAD_SIGNATURE = 1,
  kNO_KEY = 2,
  kUNKNOWN_ERROR = 3,
  kKEY_REVOKED = 4,         ///< valid, but the key was revoked as of it
  kKEY_EXPIRED = 5,         ///< valid, but made after the key expired
  kKEY_NOT_AUTHORIZED = 6,  ///< valid, but the key may not sign
};

struct GFSignature {
  QString issuer_fpr;
  GFSignatureStatus status = GFSignatureStatus::kUNKNOWN_ERROR;
  uint32_t created_at = 0;
  QString pub_algo;
  QString hash_algo;
  QString sig_type;

  // As an engine with a trust model reports them; derived from `status`
  // otherwise.
  std::optional<GFError> engine_status;
  std::optional<GpgValidity> validity;
  std::optional<std::uint32_t> summary;  ///< GpgSigSummary flags
  qint64 expires_at = 0;                 ///< 0 when it does not expire
  bool has_trust_model = false;
};

struct GFRecipient {
  QString key_id;
  QString pub_algo;
  GFError status;
};

struct GFInvalidRecipient {
  QString fpr;
  GFError reason;
};

struct GFEncryptResult {
  GFBuffer data;
  QList<GFInvalidRecipient> invalid_recipients;
  /// Subkeys the session key was actually encrypted to (rPGP). Empty for GPGME,
  /// which does not report the recipient subkey used.
  QList<GFRecipient> recipients;
  QString
      error_detail;  ///< Engine-specific failure detail (rPGP), may be empty.
};

struct GFDecryptResult {
  GFBuffer data;
  QString filename;
  QString symkey_algo;  ///< Cipher and mode, as gpgme's (e.g. "AES256.OCB").
  QString cipher_warning;  ///< Why the cipher deserves a warning (RFC 9580
                           ///< §9.3, §12.2), or empty.
  QList<GFRecipient> recipients;
  QString
      error_detail;  ///< Engine-specific failure detail (rPGP), may be empty.
};

struct GFSignResult {
  GFBuffer data;
  QList<GFSignature> signatures;
  QList<GFInvalidRecipient> invalid_signers;  ///< signers that were refused
  QString
      error_detail;  ///< Engine-specific failure detail (rPGP), may be empty.
};

struct GFVerifyResult {
  QByteArray data;
  QList<GFSignature> signatures;
  bool is_verified = false;
  QString
      error_detail;  ///< Engine-specific failure detail (rPGP), may be empty.
};

struct GFEncryptAndSignResult {
  GFBuffer data;
  GFSignResult sign_result;
  GFEncryptResult encrypt_result;
};

struct GFDecryptAndVerifyResult {
  GFBuffer data;
  GFDecryptResult decrypt_result;
  GFVerifyResult verify_result;
};

}  // namespace GpgFrontend

// SignalBadOpenPGPEnv is emitted from a task runner and received on the main
// thread, so every delivery is queued. Without this (and the matching
// qRegisterMetaType in CoreSignalStation) the emission can be dropped --
// and since that signal is what releases the startup event loop, the app would
// sit on the loading dialog forever in precisely the broken-environment case
// this reporting exists to explain.
Q_DECLARE_METATYPE(GpgFrontend::BadOpenPGPEnvReason)

// Same reasoning for SignalCoreInitProgress, which is emitted from three
// different task runner threads and received by the startup dialog.
Q_DECLARE_METATYPE(GpgFrontend::CoreInitStep)