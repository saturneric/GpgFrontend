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
 * Saturneric <eric@bktus.com) starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include "core/function/GFBufferFactory.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgImportInformation.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief Singleton operator for creating and importing encrypted key packages.
 *
 * A key package bundles exported OpenPGP key data encrypted with a randomly
 * generated passphrase. That passphrase is itself encrypted with a caller-
 * supplied PIN and written to a separate key file, so the two files must be
 * kept together to restore the keys.
 */
class GF_CORE_EXPORT KeyPackageOperator
    : public SingletonFunctionObject<KeyPackageOperator> {
 public:
  /**
   * @brief Construct the operator for the given singleton channel.
   *
   * @param channel singleton channel identifier
   */
  explicit KeyPackageOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief Generate a unique key package file name.
   *
   * The name has the form "KeyPackage_XXXXXXXX" where the suffix is derived
   * from a cryptographically random z-base-32 string.
   *
   * @return generated file name string
   */
  auto GenerateKeyPackageName() -> QString;

  /**
   * @brief Export and encrypt a set of keys into a key package file pair.
   *
   * Exports @p keys from GPG, encrypts the key data with a freshly generated
   * random passphrase, then encrypts that passphrase with @p pin. The
   * encrypted key data is written to @p key_package_path and the encrypted
   * passphrase is written to @p key_path. Results are delivered via @p cb.
   *
   * @param key_package_path destination path for the encrypted key data file
   * @param key_path destination path for the encrypted passphrase file
   * @param keys list of keys to export
   * @param pin PIN used to encrypt the package passphrase
   * @param secret if true, include secret key material in the export
   * @param cb callback invoked with the operation result (0 on success, -1 on
   * failure)
   */
  void GenerateKeyPackage(const QString &key_package_path,
                          const QString &key_path,
                          const GpgAbstractKeyPtrList &keys,
                          const GFBuffer &pin, bool secret,
                          const OperationCallback &cb);

  /**
   * @brief Decrypt and import a key package.
   *
   * Reads the encrypted key data from @p key_package_path and the encrypted
   * passphrase from @p key_path, decrypts both using @p pin, then imports
   * the recovered key data into GPG.
   *
   * @param key_package_path path to the encrypted key data file
   * @param key_path path to the encrypted passphrase file
   * @param pin PIN used to decrypt the package passphrase
   * @return tuple of (error message string, import information); error string
   * is empty on success
   */
  auto ImportKeyPackage(const QString &key_package_path,
                        const QString &key_path, const GFBuffer &pin)
      -> std::tuple<QString, QSharedPointer<GpgImportInformation>>;

 private:
  GFBufferFactory &gb_fac_ = GFBufferFactory::GetInstance(
      GetChannel());  ///< Factory used for random passphrase generation and
                      ///< encryption

  /**
   * @brief Generate the unique key package file name (implementation).
   *
   * @return generated file name string
   */
  auto generate_key_package_name() -> QString;
};
}  // namespace GpgFrontend
