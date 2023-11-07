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

#include "core/function/result_analyse/GpgResultAnalyse.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Executive files related to the basic operations of GPG
 *
 * @class class: GpgBasicOperator
 */
class GPGFRONTEND_CORE_EXPORT GpgFileOpera {
 public:
  /**
   * @brief Encrypted file with public key
   *
   * @param keys Used public key
   * @param in_path The path where the enter file is located
   * @param out_path The path where the output file is located
   * @param result Encrypted results
   * @param channel Channel in context
   * @return unsigned int error code
   */
  static auto EncryptFile(KeyListPtr keys, const std::string& in_path,
                          const std::string& out_path, GpgEncrResult& result,
                          int channel = kGpgfrontendDefaultChannel)
      -> unsigned int;

  /**
   * @brief Encrypted file symmetrically (with password)
   *
   * @param in_path
   * @param out_path
   * @param result
   * @param channel
   * @return unsigned int
   */
  static auto EncryptFileSymmetric(const std::string& in_path,
                                   const std::string& out_path,
                                   GpgEncrResult& result,
                                   int channel = kGpgfrontendDefaultChannel)
      -> unsigned int;

  /**
   * @brief
   *
   * @param in_path
   * @param out_path
   * @param result
   * @return GpgError
   */
  static auto DecryptFile(const std::string& in_path,
                          const std::string& out_path, GpgDecrResult& result)
      -> GpgError;

  /**
   * @brief Sign file with private key
   *
   * @param keys
   * @param in_path
   * @param out_path
   * @param result
   * @param channel
   * @return GpgError
   */
  static auto SignFile(KeyListPtr keys, const std::string& in_path,
                       const std::string& out_path, GpgSignResult& result,
                       int channel = kGpgfrontendDefaultChannel) -> GpgError;

  /**
   * @brief Verify file with public key
   *
   * @param data_path The path where the enter file is located
   * @param sign_path The path where the signature file is located
   * @param result Verify results
   * @param channel Channel in context
   * @return GpgError
   */
  static auto VerifyFile(const std::string& data_path,
                         const std::string& sign_path, GpgVerifyResult& result,
                         int channel = kGpgfrontendDefaultChannel) -> GpgError;

  /**
   * @brief Encrypt and sign file with public key and private key
   *
   * @param keys
   * @param signer_keys
   * @param in_path
   * @param out_path
   * @param encr_res
   * @param sign_res
   * @param channel
   * @return GpgError
   */
  static auto EncryptSignFile(KeyListPtr keys, KeyListPtr signer_keys,
                              const std::string& in_path,
                              const std::string& out_path,
                              GpgEncrResult& encr_res, GpgSignResult& sign_res,
                              int channel = kGpgfrontendDefaultChannel)
      -> GpgError;

  /**
   * @brief
   *
   * @param in_path
   * @param out_path
   * @param decr_res
   * @param verify_res
   * @return GpgError
   */
  static auto DecryptVerifyFile(const std::string& in_path,
                                const std::string& out_path,
                                GpgDecrResult& decr_res,
                                GpgVerifyResult& verify_res) -> GpgError;
};

}  // namespace GpgFrontend
