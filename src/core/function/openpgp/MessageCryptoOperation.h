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
#include "core/model/GFBuffer.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Singleton for in-memory OpenPGP message crypto operations.
 *
 * Each method has an async variant (callback-based) and a synchronous variant.
 * Async methods post to the GPG task runner; sync methods execute on the
 * calling thread.
 */
class GF_CORE_EXPORT MessageCryptoOperation
    : public SingletonFunctionObject<MessageCryptoOperation> {
 public:
  /**
   * @brief Construct the operation handler for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit MessageCryptoOperation(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Encrypt a message buffer with the given recipient keys (async).
   *
   * @param keys recipient public keys
   * @param in_buffer plaintext data to encrypt
   * @param ascii if true, produce ASCII-armored output
   * @param cb callback invoked on completion with (GpgError, DataObjectPtr)
   */
  void Encrypt(const GpgAbstractKeyPtrList& keys, const GFBuffer& in_buffer,
               bool ascii, const GpgOperationCallback& cb);

  /**
   * @brief Encrypt a message buffer with the given recipient keys (sync).
   *
   * @param keys recipient public keys
   * @param in_buffer plaintext data to encrypt
   * @param ascii if true, produce ASCII-armored output
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto EncryptSync(const GpgAbstractKeyPtrList& keys, const GFBuffer& in_buffer,
                   bool ascii) -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Encrypt a message buffer symmetrically (password-based, async).
   *
   * @param in_buffer plaintext data to encrypt
   * @param ascii if true, produce ASCII-armored output
   * @param cb callback invoked on completion
   */
  void EncryptSymmetric(const GFBuffer& in_buffer, bool ascii,
                        const GpgOperationCallback& cb);

  /**
   * @brief Encrypt a message buffer symmetrically (sync).
   *
   * @param in_buffer plaintext data to encrypt
   * @param ascii if true, produce ASCII-armored output
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto EncryptSymmetricSync(const GFBuffer& in_buffer, bool ascii)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Encrypt and sign a message buffer simultaneously (async).
   *
   * @param keys recipient public keys for encryption
   * @param signers private keys for signing
   * @param in_buffer plaintext data
   * @param ascii if true, produce ASCII-armored output
   * @param cb callback invoked on completion
   */
  void EncryptSign(const GpgAbstractKeyPtrList& keys,
                   const GpgAbstractKeyPtrList& signers,
                   const GFBuffer& in_buffer, bool ascii,
                   const GpgOperationCallback& cb);

  /**
   * @brief Encrypt and sign a message buffer simultaneously (sync).
   *
   * @param keys recipient public keys for encryption
   * @param signers private keys for signing
   * @param in_buffer plaintext data
   * @param ascii if true, produce ASCII-armored output
   * @return tuple of (GpgError, DataObjectPtr)
   */
  auto EncryptSignSync(const GpgAbstractKeyPtrList& keys,
                       const GpgAbstractKeyPtrList& signers,
                       const GFBuffer& in_buffer, bool ascii)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Decrypt an encrypted message buffer (async).
   *
   * @param in_buffer ciphertext data to decrypt
   * @param cb callback invoked on completion with decryption result
   */
  void Decrypt(const GFBuffer& in_buffer, const GpgOperationCallback& cb);

  /**
   * @brief Decrypt an encrypted message buffer (sync).
   *
   * @param in_buffer ciphertext data to decrypt
   * @return tuple of (GpgError, DataObjectPtr) containing the decryption result
   */
  auto DecryptSync(const GFBuffer& in_buffer)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Decrypt and verify a combined encrypted+signed message buffer
   * (async).
   *
   * @param in_buffer encrypted and signed data
   * @param cb callback invoked on completion with both decrypt and verify
   * results
   */
  void DecryptVerify(const GFBuffer& in_buffer, const GpgOperationCallback& cb);

  /**
   * @brief Decrypt and verify a combined encrypted+signed message buffer
   * (sync).
   *
   * @param in_buffer encrypted and signed data
   * @return tuple of (GpgError, DataObjectPtr) containing both results
   */
  auto DecryptVerifySync(const GFBuffer& in_buffer)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Verify the signature on a message buffer (async).
   *
   * @param in_buffer signed data (or plaintext for detached signatures)
   * @param sig_buffer detached signature buffer; pass empty GFBuffer for inline
   * signatures
   * @param cb callback invoked on completion with verification result
   */
  void Verify(const GFBuffer& in_buffer, const GFBuffer& sig_buffer,
              const GpgOperationCallback& cb);

  /**
   * @brief Verify the signature on a message buffer (sync).
   *
   * @param in_buffer signed data
   * @param sig_buffer detached signature buffer; pass empty GFBuffer for inline
   * signatures
   * @return tuple of (GpgError, DataObjectPtr) containing the verification
   * result
   */
  auto VerifySync(const GFBuffer& in_buffer, const GFBuffer& sig_buffer)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Sign a message buffer with the given private keys (async).
   *
   * Signing modes:
   * - GPGME_SIG_MODE_NORMAL: output includes both plaintext and signature.
   * - GPGME_SIG_MODE_DETACH: produces a detached signature only.
   * - GPGME_SIG_MODE_CLEAR: clear-text signature; ignores ASCII armor and text
   * mode settings.
   *
   * @param signers private keys to sign with
   * @param in_buffer data to sign
   * @param mode signing mode (normal, detached, or clear)
   * @param ascii if true, produce ASCII-armored output (ignored in CLEAR mode)
   * @param cb callback invoked on completion with sign result
   */
  void Sign(const GpgAbstractKeyPtrList& signers, const GFBuffer& in_buffer,
            GpgSignMode mode, bool ascii, const GpgOperationCallback& cb);

  /**
   * @brief Sign a message buffer with the given private keys (sync).
   *
   * @param signers private keys to sign with
   * @param in_buffer data to sign
   * @param mode signing mode
   * @param ascii if true, produce ASCII-armored output
   * @return tuple of (GpgError, DataObjectPtr) containing the sign result
   */
  auto SignSync(const GpgAbstractKeyPtrList& signers, const GFBuffer& in_buffer,
                GpgSignMode mode, bool ascii)
      -> std::tuple<GpgError, DataObjectPtr>;

 private:
  OpenPGPContext& ctx_ = OpenPGPContext::GetInstance(
      SingletonFunctionObject::GetChannel());  ///< OpenPGP context for this
                                               ///< channel
};
}  // namespace GpgFrontend
