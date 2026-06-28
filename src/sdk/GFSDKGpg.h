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

#include <gpgme.h>

extern "C" {

/**
 * @brief Result of a GPG sign operation.
 *
 * Allocated by GFGpgSignData and must be freed with GFFreeMemory.
 * The embedded @p gpgme_sign_result reference must be released with
 * GFGpgFreeResult before freeing this struct.
 */
struct GFGpgSignResult {
  char* signature;            ///< Signed/armored output data.
  char* hash_algo;            ///< Hash algorithm used (e.g. "SHA256").
  char* capsule_id;           ///< Opaque ID for UI capsule access.
  char* error_string;         ///< Human-readable error description.
  gpgme_error_t gpgme_error;  ///< Raw GPGME error code.
  gpgme_sign_result_t gpgme_sign_result;  ///< Ref-counted GPGME result handle.
};

/**
 * @brief Result of a GPG encrypt operation.
 *
 * Allocated by GFGpgEncryptData and must be freed with GFFreeMemory.
 * Release @p gpgme_encrypt_result with GFGpgFreeResult before freeing.
 */
struct GFGpgEncryptionResult {
  char* encrypted_data;       ///< Encrypted output data.
  char* capsule_id;           ///< Opaque ID for UI capsule access.
  char* error_string;         ///< Human-readable error description.
  gpgme_error_t gpgme_error;  ///< Raw GPGME error code.
  gpgme_encrypt_result_t
      gpgme_encrypt_result;  ///< Ref-counted GPGME result handle.
};

/**
 * @brief Result of a GPG decrypt operation.
 *
 * Allocated by GFGpgDecryptData and must be freed with GFFreeMemory.
 * Release @p gpgme_decrypt_result with GFGpgFreeResult before freeing.
 */
struct GFGpgDecryptResult {
  char* decrypted_data;       ///< Plaintext output data.
  char* capsule_id;           ///< Opaque ID for UI capsule access.
  char* error_string;         ///< Human-readable error description.
  gpgme_error_t gpgme_error;  ///< Raw GPGME error code.
  gpgme_decrypt_result_t
      gpgme_decrypt_result;  ///< Ref-counted GPGME result handle.
};

/**
 * @brief Result of a GPG signature verification operation.
 *
 * Allocated by GFGpgVerifyData and must be freed with GFFreeMemory.
 * Release @p gpgme_verify_result with GFGpgFreeResult before freeing.
 */
struct GFGpgVerifyResult {
  char* capsule_id;           ///< Opaque ID for UI capsule access.
  char* error_string;         ///< Human-readable error description.
  gpgme_error_t gpgme_error;  ///< Raw GPGME error code.
  gpgme_verify_result_t
      gpgme_verify_result;  ///< Ref-counted GPGME result handle.
};

/**
 * @brief A User ID (UID) associated with a GPG key.
 *
 * Allocated by GFGpgKeyPrimaryUID and must be freed with GFFreeMemory.
 */
struct GFGpgKeyUID {
  char* name;     ///< Display name from the UID packet.
  char* email;    ///< Email address from the UID packet.
  char* comment;  ///< Optional comment from the UID packet.
};

/**
 * @brief Signs data using one or more GPG keys.
 *
 * @param channel      GPG context channel index.
 * @param key_ids      Array of key fingerprints or IDs to sign with.
 * @param key_ids_size Number of entries in @p key_ids.
 * @param data         Null-terminated plaintext to sign.
 * @param sign_mode    0 for normal (inline) signature, 1 for detached.
 * @param ascii        Non-zero to produce ASCII-armored output.
 * @param[out] result  Set to a newly allocated GFGpgSignResult on success.
 * @return 0 on success, -1 on failure (result->error_string is set).
 */
auto GF_SDK_EXPORT GFGpgSignData(int channel, char** key_ids, int key_ids_size,
                                 char* data, int sign_mode, int ascii,
                                 GFGpgSignResult** result) -> int;

/**
 * @brief Encrypts data for one or more recipients.
 *
 * @param channel      GPG context channel index.
 * @param key_ids      Array of recipient key fingerprints or IDs.
 * @param key_ids_size Number of entries in @p key_ids.
 * @param data         Null-terminated plaintext to encrypt.
 * @param ascii        Non-zero to produce ASCII-armored output.
 * @param[out] result  Set to a newly allocated GFGpgEncryptionResult on
 * success.
 * @return 0 on success, -1 on failure (result->error_string is set).
 */
auto GF_SDK_EXPORT GFGpgEncryptData(int channel, char** key_ids,
                                    int key_ids_size, char* data, int ascii,
                                    GFGpgEncryptionResult** result) -> int;

/**
 * @brief Decrypts GPG-encrypted data using available secret keys.
 *
 * @param channel     GPG context channel index.
 * @param data        Null-terminated encrypted (PGP) message.
 * @param[out] result Set to a newly allocated GFGpgDecryptResult.
 * @return 0 on success, -1 on failure.
 */
auto GF_SDK_EXPORT GFGpgDecryptData(int channel, char* data,
                                    GFGpgDecryptResult** result) -> int;

/**
 * @brief Verifies a detached or inline GPG signature.
 *
 * @param channel      GPG context channel index.
 * @param data         Null-terminated original plaintext (for detached sig)
 *                     or signed message (for inline sig).
 * @param signature    Null-terminated detached signature data; pass an empty
 *                     string for inline/clearsign messages.
 * @param[out] result  Set to a newly allocated GFGpgVerifyResult.
 * @return 0 on success, -1 on failure.
 */
auto GF_SDK_EXPORT GFGpgVerifyData(int channel, char* data, char* signature,
                                   GFGpgVerifyResult** result) -> int;

/**
 * @brief Exports the public key block for a given key ID.
 *
 * @param channel GPG context channel index.
 * @param key_id  Fingerprint or key ID to export.
 * @param ascii   Non-zero to produce ASCII-armored output.
 * @return Caller-owned string containing the exported key; free with
 *         GFFreeMemory. Returns nullptr if the key is not found.
 */
auto GF_SDK_EXPORT GFGpgPublicKey(int channel, char* key_id, int ascii)
    -> char*;

/**
 * @brief Retrieves the primary User ID of a key.
 *
 * @param channel    GPG context channel index.
 * @param key_id     Fingerprint or key ID to look up.
 * @param[out] uid   Set to a newly allocated GFGpgKeyUID on success.
 * @return 0 on success, -1 if the key is not found or has no UIDs.
 */
auto GF_SDK_EXPORT GFGpgKeyPrimaryUID(int channel, char* key_id,
                                      GFGpgKeyUID** uid) -> int;

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
auto GF_SDK_EXPORT GFGpgImportKeys(int channel, void* parent, const char* data,
                                   int size) -> int;

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
auto GF_SDK_EXPORT GFGpgExportKey(int channel, char* key_id, int ascii,
                                  char** data, int* size) -> int;

/**
 * @brief Returns the GPG context channel index currently active in the main
 *        window.
 * @return Channel index, or -1 if the main window is not available.
 */
auto GF_SDK_EXPORT GFGpgCurrentGpgContextChannel() -> int;

/**
 * @brief Releases a ref-counted GPGME result object.
 *
 * Call this on the raw GPGME result handle (e.g. gpgme_sign_result) stored
 * inside a GFGpgSignResult / GFGpgEncryptionResult / etc. before freeing the
 * enclosing struct.
 *
 * @param r GPGME result pointer to dereference; no-op if nullptr.
 */
auto GF_SDK_EXPORT GFGpgFreeResult(void* r) -> void;

/**
 * @brief Analyses a GPGME encryption result and produces a human-readable
 *        report.
 *
 * @param channel       GPG context channel index.
 * @param err           GPGME error code from the encrypt operation.
 * @param result        GPGME encryption result handle.
 * @param[out] analyse  Set to a caller-owned string with the analysis report;
 *                      free with GFFreeMemory.
 * @param[out] cards    Optional; when non-null, set to a caller-owned JSON
 *                      array string of Info Board cards for the result (free
 *                      with GFFreeMemory). Pass nullptr to skip.
 * @return Status code: positive on success, negative on detected errors.
 */
auto GF_SDK_EXPORT GFAnalyseEncryptResult(int channel, gpgme_error_t err,
                                          gpgme_encrypt_result_t result,
                                          const char** analyse,
                                          const char** cards) -> int;

/**
 * @brief Analyses a GPGME signing result and produces a human-readable report.
 *
 * @param channel       GPG context channel index.
 * @param err           GPGME error code from the sign operation.
 * @param result        GPGME sign result handle.
 * @param[out] analyse  Set to a caller-owned string with the analysis report;
 *                      free with GFFreeMemory.
 * @param[out] cards    Optional; when non-null, set to a caller-owned JSON
 *                      array string of Info Board cards for the result (free
 *                      with GFFreeMemory). Pass nullptr to skip.
 * @return Status code: positive on success, negative on detected errors.
 */
auto GF_SDK_EXPORT GFAnalyseSignResult(int channel, gpgme_error_t err,
                                       gpgme_sign_result_t result,
                                       const char** analyse, const char** cards)
    -> int;

/**
 * @brief Analyses a GPGME decryption result and produces a human-readable
 *        report.
 *
 * @param channel       GPG context channel index.
 * @param err           GPGME error code from the decrypt operation.
 * @param result        GPGME decrypt result handle.
 * @param[out] analyse  Set to a caller-owned string with the analysis report;
 *                      free with GFFreeMemory.
 * @param[out] cards    Optional; when non-null, set to a caller-owned JSON
 *                      array string of Info Board cards for the result (free
 *                      with GFFreeMemory). Pass nullptr to skip.
 * @return Status code: positive on success, negative on detected errors.
 */
auto GF_SDK_EXPORT GFAnalyseDecryptResult(int channel, gpgme_error_t err,
                                          gpgme_decrypt_result_t result,
                                          const char** analyse,
                                          const char** cards) -> int;

/**
 * @brief Analyses a GPGME verification result and produces a human-readable
 *        report.
 *
 * @param channel       GPG context channel index.
 * @param err           GPGME error code from the verify operation.
 * @param result        GPGME verify result handle.
 * @param[out] analyse  Set to a caller-owned string with the analysis report;
 *                      free with GFFreeMemory.
 * @param[out] cards    Optional; when non-null, set to a caller-owned JSON
 *                      array string of Info Board cards for the result (free
 *                      with GFFreeMemory). Pass nullptr to skip.
 * @return Status code: positive on success, negative on detected errors.
 */
auto GF_SDK_EXPORT GFAnalyseVerifyResult(int channel, gpgme_error_t err,
                                         gpgme_verify_result_t result,
                                         const char** analyse,
                                         const char** cards) -> int;

}  // extern "C"