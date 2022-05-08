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

#ifndef GPGFRONTEND_GPGFILEOPERA_H
#define GPGFRONTEND_GPGFILEOPERA_H

#include "core/GpgConstants.h"
#include "core/GpgContext.h"
#include "core/GpgModel.h"

namespace GpgFrontend {

/**
 * @brief Executive files related to the basic operations that are provided by
 * GpgBasicOperator
 * @class class: GpgBasicOperator
 */
class GPGFRONTEND_CORE_EXPORT GpgFileOpera
    : public SingletonFunctionObject<GpgFileOpera> {
 public:

  explicit GpgFileOpera(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Encrypted file
   *
   * @param keys Used public key
   * @param in_path The path where the enter file is located
   * @param out_path The path where the output file is located
   * @param result Encrypted results
   * @param _channel Channel in context
   * @return unsigned int error code
   */
  static unsigned int EncryptFile(KeyListPtr keys, const std::string& in_path,
                                  const std::string& out_path,
                                  GpgEncrResult& result,
                                  int _channel = GPGFRONTEND_DEFAULT_CHANNEL);

  /**
   * @brief 运用对称加密算法加密文件
   *
   * @param in_path
   * @param out_path
   * @param result
   * @param _channel
   * @return unsigned int
   */
  static unsigned int EncryptFileSymmetric(
      const std::string& in_path, const std::string& out_path,
      GpgEncrResult& result, int _channel = GPGFRONTEND_DEFAULT_CHANNEL);

  /**
   * @brief
   *
   * @param in_path
   * @param out_path
   * @param result
   * @return GpgError
   */
  static GpgError DecryptFile(const std::string& in_path,
                              const std::string& out_path,
                              GpgDecrResult& result);

  /**
   * @brief
   *
   * @param keys
   * @param in_path
   * @param out_path
   * @param result
   * @param _channel
   * @return GpgError
   */
  static GpgError SignFile(KeyListPtr keys, const std::string& in_path,
                           const std::string& out_path, GpgSignResult& result,
                           int _channel = GPGFRONTEND_DEFAULT_CHANNEL);

  /**
   * @brief
   *
   * @param data_path
   * @param sign_path
   * @param result
   * @param _channel
   * @return GpgError
   */
  static GpgError VerifyFile(const std::string& data_path,
                             const std::string& sign_path,
                             GpgVerifyResult& result,
                             int _channel = GPGFRONTEND_DEFAULT_CHANNEL);

  /**
   * @brief
   *
   * @param keys
   * @param signer_keys
   * @param in_path
   * @param out_path
   * @param encr_res
   * @param sign_res
   * @param _channel
   * @return GpgError
   */
  static GpgError EncryptSignFile(KeyListPtr keys, KeyListPtr signer_keys,
                                  const std::string& in_path,
                                  const std::string& out_path,
                                  GpgEncrResult& encr_res,
                                  GpgSignResult& sign_res,
                                  int _channel = GPGFRONTEND_DEFAULT_CHANNEL);

  /**
   * @brief
   *
   * @param in_path
   * @param out_path
   * @param decr_res
   * @param verify_res
   * @return GpgError
   */
  static GpgError DecryptVerifyFile(const std::string& in_path,
                                    const std::string& out_path,
                                    GpgDecrResult& decr_res,
                                    GpgVerifyResult& verify_res);
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGFILEOPERA_H
