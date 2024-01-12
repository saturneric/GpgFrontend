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
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Executive files related to the basic operations of GPG
 *
 * @class class: GpgBasicOperator
 */
class GPGFRONTEND_CORE_EXPORT GpgFileOpera
    : public SingletonFunctionObject<GpgFileOpera> {
 public:
  /**
   * @brief Construct a new Gpg File Opera object
   *
   * @param channel
   */
  explicit GpgFileOpera(
      int channel = SingletonFunctionObject::GetDefaultChannel());

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
  void EncryptFile(KeyArgsList keys, const QString& in_path, bool ascii,
                   const QString& out_path, const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param keys
   * @param in_path
   * @param ascii
   * @param out_path
   * @param cb
   */
  void EncryptDirectory(KeyArgsList keys, const QString& in_path, bool ascii,
                        const QString& out_path,
                        const GpgOperationCallback& cb);

  /**
   * @brief Encrypted file symmetrically (with password)
   *
   * @param in_path
   * @param out_path
   * @param result
   * @param channel
   * @return unsigned int
   */
  void EncryptFileSymmetric(const QString& in_path, bool ascii,
                            const QString& out_path,
                            const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param in_path
   * @param ascii
   * @param out_path
   * @param cb
   */
  void EncryptDerectorySymmetric(const QString& in_path, bool ascii,
                                 const QString& out_path,
                                 const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param in_path
   * @param out_path
   * @param result
   * @return GpgError
   */
  void DecryptFile(const QString& in_path, const QString& out_path,
                   const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param in_path
   * @param out_path
   * @param cb
   */
  void DecryptArchive(const QString& in_path, const QString& out_path,
                      const GpgOperationCallback& cb);

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
  void SignFile(KeyArgsList keys, const QString& in_path, bool ascii,
                const QString& out_path, const GpgOperationCallback& cb);

  /**
   * @brief Verify file with public key
   *
   * @param data_path The path where the enter file is located
   * @param sign_path The path where the signature file is located
   * @param result Verify results
   * @param channel Channel in context
   * @return GpgError
   */
  void VerifyFile(const QString& data_path, const QString& sign_path,
                  const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param keys
   * @param signer_keys
   * @param in_path
   * @param ascii
   * @param out_path
   * @param cb
   */
  void EncryptSignFile(KeyArgsList keys, KeyArgsList signer_keys,
                       const QString& in_path, bool ascii,
                       const QString& out_path, const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param keys
   * @param signer_keys
   * @param in_path
   * @param ascii
   * @param out_path
   * @param cb
   */
  void EncryptSignDirectory(KeyArgsList keys, KeyArgsList signer_keys,
                            const QString& in_path, bool ascii,
                            const QString& out_path,
                            const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param in_path
   * @param out_path
   * @param decr_res
   * @param verify_res
   * @return GpgError
   */
  void DecryptVerifyFile(const QString& in_path, const QString& out_path,
                         const GpgOperationCallback& cb);

  /**
   * @brief
   *
   * @param in_path
   * @param out_path
   * @param cb
   */
  void DecryptVerifyArchive(const QString& in_path, const QString& out_path,
                            const GpgOperationCallback& cb);

 private:
  GpgContext& ctx_ = GpgContext::GetInstance(
      SingletonFunctionObject::GetChannel());  ///< Corresponding context
};

}  // namespace GpgFrontend
