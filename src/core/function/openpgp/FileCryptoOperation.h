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

#include "core/function/basic/GpgFunctionObject.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Singleton for file and directory OpenPGP crypto operations.
 *
 * Each method has an async variant (callback-based) and a synchronous variant.
 * Async methods post to the GPG task runner; sync methods execute on the
 * calling thread. Archive/directory operations pack contents before encryption.
 */
class GF_CORE_EXPORT FileCryptoOperation
    : public SingletonFunctionObject<FileCryptoOperation> {
 public:
  /**
   * @brief Construct the operation handler for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit FileCryptoOperation(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Encrypt a file with the given public keys (async).
   *
   * @param keys recipient public keys
   * @param in_path path to the plaintext file
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the ciphertext file
   * @param cb callback invoked on completion with (GpgError, DataObjectPtr)
   */
  void EncryptFile(const GpgAbstractKeyPtrList& keys, const QString& in_path,
                   bool ascii, const QString& out_path,
                   const GpgOperationCallback& cb);

  /**
   * @brief Encrypt a file with the given public keys (sync).
   *
   * @param keys recipient public keys
   * @param in_path path to the plaintext file
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the ciphertext file
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto EncryptFileSync(const GpgAbstractKeyPtrList& keys,
                       const QString& in_path, bool ascii,
                       const QString& out_path)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Pack and encrypt a directory with the given public keys (async).
   *
   * @param keys recipient public keys
   * @param in_path path to the source directory
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the encrypted archive
   * @param cb callback invoked on completion
   */
  void EncryptDirectory(const GpgAbstractKeyPtrList& keys,
                        const QString& in_path, bool ascii,
                        const QString& out_path,
                        const GpgOperationCallback& cb);

  /**
   * @brief Encrypt a file symmetrically (password-based, async).
   *
   * @param in_path path to the plaintext file
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the ciphertext file
   * @param cb callback invoked on completion
   */
  void EncryptFileSymmetric(const QString& in_path, bool ascii,
                            const QString& out_path,
                            const GpgOperationCallback& cb);

  /**
   * @brief Encrypt a file symmetrically (sync).
   *
   * @param in_path path to the plaintext file
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the ciphertext file
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto EncryptFileSymmetricSync(const QString& in_path, bool ascii,
                                const QString& out_path)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Pack and encrypt a directory symmetrically (async).
   *
   * @param in_path path to the source directory
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the encrypted archive
   * @param cb callback invoked on completion
   */
  void EncryptDirectorySymmetric(const QString& in_path, bool ascii,
                                 const QString& out_path,
                                 const GpgOperationCallback& cb);

  /**
   * @brief Decrypt a file (async).
   *
   * @param in_path path to the ciphertext file
   * @param out_path destination path for the decrypted file
   * @param cb callback invoked on completion
   */
  void DecryptFile(const QString& in_path, const QString& out_path,
                   const GpgOperationCallback& cb);

  /**
   * @brief Decrypt a file (sync).
   *
   * @param in_path path to the ciphertext file
   * @param out_path destination path for the decrypted file
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto DecryptFileSync(const QString& in_path, const QString& out_path)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Decrypt an encrypted archive and extract its contents (async).
   *
   * @param in_path path to the encrypted archive
   * @param out_path destination directory for the extracted files
   * @param cb callback invoked on completion
   */
  void DecryptArchive(const QString& in_path, const QString& out_path,
                      const GpgOperationCallback& cb);

  /**
   * @brief Create a detached or inline signature for a file (async).
   *
   * @param keys signing private keys
   * @param in_path path to the file to sign
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the signature
   * @param cb callback invoked on completion
   */
  void SignFile(const GpgAbstractKeyPtrList& keys, const QString& in_path,
                bool ascii, const QString& out_path,
                const GpgOperationCallback& cb);

  /**
   * @brief Create a detached or inline signature for a file (sync).
   *
   * @param keys signing private keys
   * @param in_path path to the file to sign
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the signature
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto SignFileSync(const GpgAbstractKeyPtrList& keys, const QString& in_path,
                    bool ascii, const QString& out_path)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Verify a file signature (async).
   *
   * @param data_path path to the signed data file
   * @param sign_path path to the detached signature file; empty for inline
   * signatures
   * @param cb callback invoked on completion
   */
  void VerifyFile(const QString& data_path, const QString& sign_path,
                  const GpgOperationCallback& cb);

  /**
   * @brief Verify a file signature (sync).
   *
   * @param data_path path to the signed data file
   * @param sign_path path to the detached signature file; empty for inline
   * signatures
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto VerifyFileSync(const QString& data_path, const QString& sign_path)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Encrypt and sign a file simultaneously (async).
   *
   * @param keys recipient public keys
   * @param signer_keys signing private keys
   * @param in_path path to the plaintext file
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the encrypted+signed file
   * @param cb callback invoked on completion
   */
  void EncryptSignFile(const GpgAbstractKeyPtrList& keys,
                       const GpgAbstractKeyPtrList& signer_keys,
                       const QString& in_path, bool ascii,
                       const QString& out_path, const GpgOperationCallback& cb);

  /**
   * @brief Encrypt and sign a file simultaneously (sync).
   *
   * @param keys recipient public keys
   * @param signer_keys signing private keys
   * @param in_path path to the plaintext file
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the encrypted+signed file
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto EncryptSignFileSync(const GpgAbstractKeyPtrList& keys,
                           const GpgAbstractKeyPtrList& signer_keys,
                           const QString& in_path, bool ascii,
                           const QString& out_path)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Pack, encrypt, and sign a directory simultaneously (async).
   *
   * @param keys recipient public keys
   * @param signer_keys signing private keys
   * @param in_path path to the source directory
   * @param ascii if true, produce ASCII-armored output
   * @param out_path destination path for the encrypted+signed archive
   * @param cb callback invoked on completion
   */
  void EncryptSignDirectory(const GpgAbstractKeyPtrList& keys,
                            const GpgAbstractKeyPtrList& signer_keys,
                            const QString& in_path, bool ascii,
                            const QString& out_path,
                            const GpgOperationCallback& cb);

  /**
   * @brief Decrypt and verify a file simultaneously (async).
   *
   * @param in_path path to the encrypted+signed ciphertext file
   * @param out_path destination path for the decrypted file
   * @param cb callback invoked on completion with decrypt and verify results
   */
  void DecryptVerifyFile(const QString& in_path, const QString& out_path,
                         const GpgOperationCallback& cb);

  /**
   * @brief Decrypt and verify a file simultaneously (sync).
   *
   * @param in_path path to the encrypted+signed ciphertext file
   * @param out_path destination path for the decrypted file
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto DecryptVerifyFileSync(const QString& in_path, const QString& out_path)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Decrypt and verify an encrypted archive simultaneously (async).
   *
   * @param in_path path to the encrypted+signed archive
   * @param out_path destination directory for the extracted and verified files
   * @param cb callback invoked on completion
   */
  void DecryptVerifyArchive(const QString& in_path, const QString& out_path,
                            const GpgOperationCallback& cb);

 private:
  OpenPGPContext& ctx_ = OpenPGPContext::GetInstance(
      SingletonFunctionObject::GetChannel());  ///< OpenPGP context for this
                                               ///< channel
};

}  // namespace GpgFrontend
