/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
#include "core/function/gpg/GpgContext.h"
#include "core/function/result_analyse/GpgResultAnalyse.h"
#include "core/model/GFBuffer.h"
#include "core/typedef/CoreTypedef.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Basic operation collection
 *
 */
class GPGFRONTEND_CORE_EXPORT GpgBasicOperator
    : public SingletonFunctionObject<GpgBasicOperator> {
 public:
  /**
   * @brief Construct a new Basic Operator object
   *
   * @param channel Channel corresponding to the context
   */
  explicit GpgBasicOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief
   *
   */
  void Encrypt(const KeyArgsList&, const GFBuffer&, bool,
               const GpgOperationCallback&);

  /**
   * @brief
   *
   */
  auto EncryptSync(const KeyArgsList&, const GFBuffer&, bool)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Call the interface provided by GPGME to symmetrical encryption
   *
   * @param in_buffer Data for encryption
   * @param out_buffer Encrypted data
   * @param result Encrypted results
   * @return GpgError
   */
  void EncryptSymmetric(const GFBuffer& in_buffer, bool ascii,
                        const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param in_buffer
   * @param ascii
   * @param cb
   * @return std::tuple<GpgError, DataObjectPtr>
   */
  auto EncryptSymmetricSync(const GFBuffer& in_buffer, bool ascii)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   *
   * @brief  Call the interface provided by gpgme to perform encryption and
   * signature operations at the same time.
   *
   * @param keys List of public keys
   * @param signers Private key for signatures
   * @param in_buffer Data for operation
   * @param ascii ascii mode
   * @return
   */
  void EncryptSign(const KeyArgsList& keys, const KeyArgsList& signers,
                   const GFBuffer& in_buffer, bool ascii,
                   const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param keys
   * @param signers
   * @param in_buffer
   * @param ascii
   * @param cb
   */
  auto EncryptSignSync(const KeyArgsList& keys, const KeyArgsList& signers,
                       const GFBuffer& in_buffer, bool ascii)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Call the interface provided by gpgme for decryption operation
   *
   * @param in_buffer data that needs to be decrypted
   * @param out_buffer decrypted data
   * @param result the result of the operation
   * @return error code
   */
  void Decrypt(const GFBuffer& in_buffer, const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param in_buffer
   */
  auto DecryptSync(const GFBuffer& in_buffer)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief  Call the interface provided by gpgme to perform decryption and
   * verification operations at the same time.
   *
   * @param in_buffer data to be manipulated
   * @param out_buffer data resulting from decryption operation
   * @param decrypt_result the result of the decrypting operation
   * @param verify_result the result of the verifying operation
   * @return error code
   */
  void DecryptVerify(const GFBuffer& in_buffer, const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param in_buffer
   */
  auto DecryptVerifySync(const GFBuffer& in_buffer)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief Call the interface provided by gpgme for verification operation
   *
   * @param in_buffer data that needs to be verified
   * @param out_buffer verified data
   * @param result the result of the operation
   * @return error code
   */
  void Verify(const GFBuffer& in_buffer, const GFBuffer& sig_buffer,
              const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param in_buffer
   * @param sig_buffer
   * @param cb
   * @return std::tuple<GpgError, DataObjectPtr>
   */
  auto VerifySync(const GFBuffer& in_buffer, const GFBuffer& sig_buffer)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief  Call the interface provided by gpgme for signing operation
   *
   * The signing modes are as follows:
   * `GPGME_SIG_MODE_NORMAL'
   *      A normal signature is made, the output includes the plaintext and the
   *      signature.
   * `GPGME_SIG_MODE_DETACH'
   *      A detached signature is made.
   * `GPGME_SIG_MODE_CLEAR'
   *      A clear text signature is made. The ASCII armor and text mode settings
   *      of the context are ignored.
   *
   * @param signers private keys for signing operations
   * @param in_buffer data that needs to be signed
   * @param out_buffer verified data
   * @param mode signing mode
   * @param result the result of the operation
   * @return error code
   */
  void Sign(const KeyArgsList& signers, const GFBuffer& in_buffer,
            GpgSignMode mode, bool ascii, const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param signers
   * @param in_buffer
   * @param mode
   * @param ascii
   * @param cb
   * @return std::tuple<GpgError, DataObjectPtr>
   */
  auto SignSync(const KeyArgsList& signers, const GFBuffer& in_buffer,
                GpgSignMode mode, bool ascii)
      -> std::tuple<GpgError, DataObjectPtr>;

  /**
   * @brief  Set the private key for signatures, this operation is a global
   * operation.
   *
   * @param keys
   */
  void SetSigners(const KeyArgsList& signers, bool ascii);

  /**
   * @brief Get a global signature private keys that has been set.
   *
   * @return Intelligent pointer pointing to the private key list
   */
  auto GetSigners(bool ascii) -> std::unique_ptr<KeyArgsList>;

 private:
  GpgContext& ctx_ = GpgContext::GetInstance(
      SingletonFunctionObject::GetChannel());  ///< Corresponding context
};
}  // namespace GpgFrontend
