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

#include "GFSDKVisibility.h"

#include <gpgme.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Result of a GPG sign operation.
 *
 * Allocated by GFGpgSignData and must be freed with GFFreeMemory.
 * The embedded @p gpgme_sign_result reference must be released with
 * GFGpgFreeResult before freeing this struct.
 */
typedef struct GFGpgSignResult {
  char* signature;            ///< Signed/armored output data.
  size_t signature_size;      ///< Exact length of @p signature in bytes.
  char* hash_algo;            ///< Hash algorithm used (e.g. "SHA256").
  char* capsule_id;           ///< Opaque ID for UI capsule access.
  char* error_string;         ///< Human-readable error description.
  gpgme_error_t gpgme_error;  ///< Raw GPGME error code.
  gpgme_sign_result_t gpgme_sign_result;  ///< Ref-counted GPGME result handle.
} GFGpgSignResult;

/**
 * @brief Result of a GPG encrypt operation.
 *
 * Allocated by GFGpgEncryptData and must be freed with GFFreeMemory.
 * Release @p gpgme_encrypt_result with GFGpgFreeResult before freeing.
 */
typedef struct GFGpgEncryptionResult {
  char* encrypted_data;        ///< Encrypted output data.
  size_t encrypted_data_size;  ///< Exact length of @p encrypted_data.
  char* capsule_id;            ///< Opaque ID for UI capsule access.
  char* error_string;          ///< Human-readable error description.
  gpgme_error_t gpgme_error;   ///< Raw GPGME error code.
  gpgme_encrypt_result_t
      gpgme_encrypt_result;  ///< Ref-counted GPGME result handle.
} GFGpgEncryptionResult;

/**
 * @brief Result of a GPG decrypt operation.
 *
 * Allocated by GFGpgDecryptData and must be freed with GFFreeMemory.
 * Release @p gpgme_decrypt_result with GFGpgFreeResult before freeing.
 */
typedef struct GFGpgDecryptResult {
  char* decrypted_data;        ///< Plaintext output data.
  size_t decrypted_data_size;  ///< Exact length of @p decrypted_data.
  char* capsule_id;            ///< Opaque ID for UI capsule access.
  char* error_string;          ///< Human-readable error description.
  gpgme_error_t gpgme_error;   ///< Raw GPGME error code.
  gpgme_decrypt_result_t
      gpgme_decrypt_result;  ///< Ref-counted GPGME result handle.
} GFGpgDecryptResult;

/**
 * @brief Result of a GPG signature verification operation.
 *
 * Allocated by GFGpgVerifyData and must be freed with GFFreeMemory.
 * Release @p gpgme_verify_result with GFGpgFreeResult before freeing.
 */
typedef struct GFGpgVerifyResult {
  char* capsule_id;           ///< Opaque ID for UI capsule access.
  char* error_string;         ///< Human-readable error description.
  gpgme_error_t gpgme_error;  ///< Raw GPGME error code.
  gpgme_verify_result_t
      gpgme_verify_result;  ///< Ref-counted GPGME result handle.
} GFGpgVerifyResult;

/**
 * @brief A User ID (UID) associated with a GPG key.
 *
 * Allocated by GFGpgKeyPrimaryUID and must be freed with GFFreeMemory.
 */
typedef struct GFGpgKeyUID {
  char* name;     ///< Display name from the UID packet.
  char* email;    ///< Email address from the UID packet.
  char* comment;  ///< Optional comment from the UID packet.
} GFGpgKeyUID;





/* --- binary-safe entry points ---------------------------------------------
 *
 * The four calls above take NUL-terminated strings, so they cannot express a
 * message octet that happens to be 0x00 -- the data stops there -- and they
 * route bytes through a UTF-8 decode that replaces any ill-formed sequence.
 * Neither is acceptable for OpenPGP: a MIME entity may legitimately be 8bit or
 * binary, and a signature covers exact octets. Verifying a NUL-truncated
 * prefix of what the user is shown reports a good signature over bytes that
 * are not the bytes on screen.
 *
 * These variants carry an explicit length and copy the octets verbatim.
 *
 * OWNERSHIP DIFFERS, deliberately: @p data and @p signature are BORROWED --
 * the caller keeps ownership and must free them itself. The string forms free
 * the buffers they are given; these do not, so a caller can pass a pointer
 * straight into its own QByteArray without an allocator round trip.
 *
 * Output is carried by the @c *_size field alongside each @c char* in the
 * result structs. The buffer is still NUL-terminated for the benefit of
 * callers that treat it as text, but the size is authoritative.
 */





/**
 * @brief Exports the public key block for a given key ID.
 *
 * @param channel GPG context channel index.
 * @param key_id  Fingerprint or key ID to export.
 * @param ascii   Non-zero to produce ASCII-armored output.
 * @return Caller-owned string containing the exported key; free with
 *         GFFreeMemory. Returns nullptr if the key is not found.
 */
GF_SDK_EXPORT char* GFGpgPublicKey(int channel, const char* key_id, int ascii);

/**
 * @brief Retrieves the primary User ID of a key.
 *
 * @param channel    GPG context channel index.
 * @param key_id     Fingerprint or key ID to look up.
 * @param[out] uid   Set to a newly allocated GFGpgKeyUID on success.
 * @return 0 on success, -1 if the key is not found or has no UIDs.
 */
GF_SDK_EXPORT int GFGpgKeyPrimaryUID(int channel, const char* key_id,
                                     GFGpgKeyUID** uid);

/**
 * @brief Imports keys from a binary or ASCII-armored data buffer.
 *
 * Presents the standard key import dialog to the user.
 *
 * @param channel GPG context channel index.
 * @param parent  Optional QWidget pointer used as the dialog parent; may be
 *                nullptr.
 * @param data    Buffer containing the key material to import.
 * @param size    Length of @p data in bytes.
 * @return 0 on success, -1 on failure.
 */
GF_SDK_EXPORT int GFGpgImportKeys(int channel, void* parent, const char* data,
                                  int size);

/**
 * @brief Exports a key to a caller-owned buffer.
 *
 * @param channel    GPG context channel index.
 * @param key_id     Fingerprint or key ID to export.
 * @param ascii      Non-zero to produce ASCII-armored output.
 * @param[out] data  Set to a newly allocated buffer containing the key data;
 *                   free with GFFreeMemory.
 * @param[out] size  Set to the number of bytes written to @p data.
 * @return 0 on success, -1 if the key is not found or export fails.
 */
GF_SDK_EXPORT int GFGpgExportKey(int channel, const char* key_id, int ascii,
                                 char** data, int* size);

/**
 * @brief Returns the GPG context channel index currently active in the main
 *        window.
 * @return Channel index, or -1 if the main window is not available.
 */
GF_SDK_EXPORT int GFGpgCurrentGpgContextChannel();


/**
 * @brief Whether a key can be used, and how well its identity matches.
 *
 * Two independent axes, deliberately kept apart. @ref usability says whether
 * the key is usable at all; it says nothing about whether the key belongs to
 * the person claimed. A perfectly usable key held by the wrong party is the
 * case that matters most, and collapsing the two into one verdict hides it.
 *
 * Allocated by GFGpgFindKeysByEmail and released, as a whole array, by
 * GFGpgFreeKeyBriefs. Never free an individual brief or any of its strings.
 */
typedef struct GFGpgKeyBrief {
  char* fingerprint;
  char* key_id;
  char* uid;            ///< primary UID, "Name (Comment) <email>"
  char* matched_email;  ///< the UID email that matched the query

  int64_t expires_at;  ///< seconds since the epoch; 0 means never

  /// Mirrors GpgFrontend::GpgKeyStatus:
  /// 0 ok, 1 expiring soon, 2 expired, 3 revoked, 4 disabled.
  int usability;

  int can_encrypt;
  int can_sign;

  /// Whether the matched UID is the key's primary one, and whether that UID
  /// has itself been revoked. Identity-binding facts, not usability ones.
  int matched_uid_is_primary;
  int matched_uid_revoked;
} GFGpgKeyBrief;



/**
 * @brief One recipient an encrypted message was encrypted to.
 *
 * @ref key_id is what the message itself names, which is the only thing that
 * decides whether it can be opened. An e-mail address cannot answer that
 * question: a message is encrypted to a KEY, and the key that matters is
 * usually an encryption subkey whose UID address need not appear anywhere in
 * the headers.
 *
 * Allocated by GFGpgSniffEncryptedRecipients and released, as a whole array,
 * by GFGpgFreeEncRecipients. Never free an individual entry or any of its
 * strings.
 */
typedef struct GFGpgEncRecipient {
  /// As the message names it: an 8-byte key id (v3 PKESK) or a full
  /// fingerprint (v6 PKESK), upper-cased.
  char* key_id;
  char* pub_algo;

  /// Of the key this resolved to; empty when nothing resolved.
  char* fingerprint;
  /// Primary UID of the key this resolved to; empty when nothing resolved.
  char* uid;

  /// Whether @ref key_id names a key this channel's key database holds at all.
  int key_found;

  /// Whether the secret half is held -- the only field that answers "can this
  /// message be opened on this computer". A public key alone cannot decrypt.
  int has_secret;

  /// The sender withheld the recipient key id (`--hidden-recipient`), so this
  /// recipient is deliberately unidentifiable rather than missing. It may
  /// still be the user themselves.
  int hidden;
} GFGpgEncRecipient;





/**
 * @brief Analyses an encryption result referenced by capsule ID.
 *
 * Engine-neutral counterpart of GFAnalyseEncryptResult: works for both the
 * native (GnuPG) and rPGP engines, because it recovers the full result model
 * from the capsule produced by GFGpgEncryptData rather than relying on a raw
 * gpgme handle (which the rPGP engine never produces).
 *
 * @param channel       GPG context channel index.
 * @param err           GPGME error code from the encrypt operation.
 * @param capsule_id    Capsule ID from GFGpgEncryptionResult::capsule_id. The
 *                      capsule is consumed (invalidated) by this call.
 * @param[out] analyse  Set to a caller-owned analysis report; free with
 *                      GFFreeMemory.
 * @param[out] cards    Optional; see GFAnalyseEncryptResult. Pass nullptr to
 *                      skip.
 * @return Status code: positive on success, negative on detected errors, -1 if
 *         the capsule is missing or of an unexpected type.
 */
GF_SDK_EXPORT int GFAnalyseEncryptResultByCapsule(int channel,
                                                  gpgme_error_t err,
                                                  const char* capsule_id,
                                                  const char** analyse,
                                                  const char** cards);

/**
 * @brief Analyses a signing result referenced by capsule ID.
 *
 * Engine-neutral counterpart of GFAnalyseSignResult. See
 * GFAnalyseEncryptResultByCapsule for the rationale and ownership rules; the
 * capsule comes from GFGpgSignResult::capsule_id.
 */
GF_SDK_EXPORT int GFAnalyseSignResultByCapsule(int channel, gpgme_error_t err,
                                               const char* capsule_id,
                                               const char** analyse,
                                               const char** cards);

/**
 * @brief Analyses a decryption result referenced by capsule ID.
 *
 * Engine-neutral counterpart of GFAnalyseDecryptResult. See
 * GFAnalyseEncryptResultByCapsule for the rationale and ownership rules; the
 * capsule comes from GFGpgDecryptResult::capsule_id.
 */
GF_SDK_EXPORT int GFAnalyseDecryptResultByCapsule(int channel,
                                                  gpgme_error_t err,
                                                  const char* capsule_id,
                                                  const char** analyse,
                                                  const char** cards);

/**
 * @brief Analyses a verification result referenced by capsule ID.
 *
 * Engine-neutral counterpart of GFAnalyseVerifyResult. See
 * GFAnalyseEncryptResultByCapsule for the rationale and ownership rules; the
 * capsule comes from GFGpgVerifyResult::capsule_id.
 */
GF_SDK_EXPORT int GFAnalyseVerifyResultByCapsule(int channel, gpgme_error_t err,
                                                 const char* capsule_id,
                                                 const char** analyse,
                                                 const char** cards);

/**
 * @brief As GFAnalyseVerifyResultByCapsule, plus the structured result as JSON.
 *
 * The card and report forms are shaped for display; this one is shaped for
 * decisions. A module that has to reason about individual signatures or
 * recipients -- their fingerprints, algorithms, timestamps, validity, and
 * whether the key was found at all -- cannot get there by re-reading prose.
 *
 * All three out-params are caller-owned; free each with GFFreeMemory. @p cards
 * and @p info_json may be nullptr to skip producing them.
 *
 * The capsule is CONSUMED by this call, exactly as by the non-Info variants,
 * so a result can be analysed once: ask for everything you need here rather
 * than calling both forms.
 *
 * JSON shape (fields absent when the operation does not produce them):
 * @code
 * {
 *   "status": 1, "operation": "Verify", "engine": "GPG v2.4.1",
 *   "description": "...", "details": ["..."], "inputHash": "...",
 *   "signatures": [ { "fingerprint": "...", "keyId": "...", "uid": "...",
 *                     "pubkeyAlgo": "...", "hashAlgo": "...",
 *                     "signTime": "ISO-8601", "validity": 0,
 *                     "warnings": ["..."] } ],
 *   "newSignatures": [ { ..., "sigMode": "Detach" } ],
 *   "invalidSigners": [ { "fingerprint": "...", "error": "..." } ],
 *   "recipients": [ { "fingerprint": "...", "keyId": "...", "uid": "...",
 *                     "pubkeyAlgo": "...", "keyFound": true,
 *                     "algoIsPrimaryKey": false } ],
 *   "filename": "...", "mimeEncoded": false,
 *   "messageIntegrityProtected": true, "symmetricAlgo": "AES256"
 * }
 * @endcode
 *
 * "validity" mirrors GpgFrontend::GpgSigValidity.
 */
GF_SDK_EXPORT int GFAnalyseVerifyResultInfoByCapsule(
    int channel, gpgme_error_t err, const char* capsule_id, const char** analyse,
    const char** cards, const char** info_json);

/**
 * @brief Structured counterpart of GFAnalyseSignResultByCapsule.
 * See GFAnalyseVerifyResultInfoByCapsule for ownership and the JSON shape.
 */
GF_SDK_EXPORT int GFAnalyseSignResultInfoByCapsule(
    int channel, gpgme_error_t err, const char* capsule_id, const char** analyse,
    const char** cards, const char** info_json);

/**
 * @brief Structured counterpart of GFAnalyseEncryptResultByCapsule.
 * See GFAnalyseVerifyResultInfoByCapsule for ownership and the JSON shape.
 */
GF_SDK_EXPORT int GFAnalyseEncryptResultInfoByCapsule(
    int channel, gpgme_error_t err, const char* capsule_id, const char** analyse,
    const char** cards, const char** info_json);

/**
 * @brief Structured counterpart of GFAnalyseDecryptResultByCapsule.
 * See GFAnalyseVerifyResultInfoByCapsule for ownership and the JSON shape.
 */
GF_SDK_EXPORT int GFAnalyseDecryptResultInfoByCapsule(
    int channel, gpgme_error_t err, const char* capsule_id, const char** analyse,
    const char** cards, const char** info_json);

#ifdef __cplusplus
}
#endif  // extern "C"