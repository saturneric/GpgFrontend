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

#ifndef GPGFRONTEND_ZH_CN_TS_BASICOPERATOR_H
#define GPGFRONTEND_ZH_CN_TS_BASICOPERATOR_H

#include "gpg/GpgConstants.h"
#include "gpg/GpgContext.h"
#include "gpg/GpgFunctionObject.h"
#include "gpg/GpgModel.h"

namespace GpgFrontend {

/**
 * @brief Basic operation collection
 *
 */
class BasicOperator : public SingletonFunctionObject<BasicOperator> {
 public:
  /**
   * @brief Construct a new Basic Operator object
   *
   * @param channel Channel corresponding to the context
   */
  explicit BasicOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel())
      : SingletonFunctionObject<BasicOperator>(channel) {}

  /**
   * @brief Call the interface provided by gpgme for encryption operation
   *
   * All incoming data pointers out_buffer will be replaced with new valid
   * values
   *
   * @param keys list of public keys
   * @param in_buffer data that needs to be encrypted
   * @param out_buffer encrypted data
   * @param result the result of the operation
   * @return error code
   */
  gpg_error_t Encrypt(KeyListPtr keys, BypeArrayRef in_buffer,
                      ByteArrayPtr& out_buffer, GpgEncrResult& result);

  /**
   * @brief Call the interface provided by GPGME to symmetrical encryption
   *
   * @param in_buffer Data for encryption
   * @param out_buffer Encrypted data
   * @param result Encrypted results
   * @return gpg_error_t
   */
  gpg_error_t EncryptSymmetric(BypeArrayRef in_buffer, ByteArrayPtr& out_buffer,
                               GpgEncrResult& result);

  /**
   *
   * @brief  Call the interface provided by gpgme to perform encryption and
   * signature operations at the same time.
   *
   * @param keys List of public keys
   * @param signers Private key for signatures
   * @param in_buffer Data for operation
   * @param out_buffer Encrypted data
   * @param encr_result Encrypted results
   * @param sign_result Signature result
   * @return
   */
  gpgme_error_t EncryptSign(KeyListPtr keys, KeyListPtr signers,
                            BypeArrayRef in_buffer, ByteArrayPtr& out_buffer,
                            GpgEncrResult& encr_result,
                            GpgSignResult& sign_result);

  /**
   * @brief Call the interface provided by gpgme for decryption operation
   *
   * @param in_buffer data that needs to be decrypted
   * @param out_buffer decrypted data
   * @param result the result of the operation
   * @return error code
   */
  gpgme_error_t Decrypt(BypeArrayRef in_buffer, ByteArrayPtr& out_buffer,
                        GpgDecrResult& result);

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
  gpgme_error_t DecryptVerify(BypeArrayRef in_buffer, ByteArrayPtr& out_buffer,
                              GpgDecrResult& decrypt_result,
                              GpgVerifyResult& verify_result);

  /**
   * @brief Call the interface provided by gpgme for verification operation
   *
   * @param in_buffer data that needs to be verified
   * @param out_buffer verified data
   * @param result the result of the operation
   * @return error code
   */
  gpgme_error_t Verify(BypeArrayRef in_buffer, ByteArrayPtr& sig_buffer,
                       GpgVerifyResult& result) const;

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
  gpg_error_t Sign(KeyListPtr signers, BypeArrayRef in_buffer,
                   ByteArrayPtr& out_buffer, gpgme_sig_mode_t mode,
                   GpgSignResult& result);

  /**
   * @brief  Set the private key for signatures, this operation is a global
   * operation.
   *
   * @param keys
   */
  void SetSigners(KeyArgsList& signers);

  /**
   * @brief Get a global signature private keys that has been set.
   *
   * @return Intelligent pointer pointing to the private key list
   */
  std::unique_ptr<KeyArgsList> GetSigners();

 private:
  GpgContext& ctx_ = GpgContext::GetInstance(
      SingletonFunctionObject::GetChannel());  ///< Corresponding context
};
}  // namespace GpgFrontend

#endif  // GPGFRONTEND_ZH_CN_TS_BASICOPERATOR_H
