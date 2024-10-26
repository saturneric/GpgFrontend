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

#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

/**
 * @brief give the possibility to import or export a key package
 *
 */
class GPGFRONTEND_CORE_EXPORT KeyPackageOperator {
 public:
  /**
   * @brief generate passphrase for key package and save it to file
   *
   * @param phrase_path path to passphrase file
   * @param phrase passphrase generated
   * @return true if passphrase was generated and saved
   * @return false if passphrase was not generated and saved
   */
  static auto GeneratePassphrase(const QString &phrase_path,
                                 QString &phrase) -> bool;

  /**
   * @brief generate the name of the key package
   *
   * @return QString name of the key package
   */
  static auto GenerateKeyPackageName() -> QString;

  /**
   * @brief generate key package
   *
   * @param key_package_path path to key package
   * @param key_package_name name of the key package
   * @param key_ids key ids to export
   * @param phrase passphrase to encrypt key package
   * @param secret true if secret key should be exported
   * @return true if key package was generated
   * @return false if key package was not generated
   */
  static void GenerateKeyPackage(const QString &key_package_path,
                                 const QString &key_package_name, int channel,
                                 const KeyArgsList &keys, QString &phrase,
                                 bool secret, const OperationCallback &cb);

  /**
   * @brief import key package
   *
   * @param key_package_path path to key package
   * @param phrase_path path to passphrase file
   * @param import_info import info
   * @return true if key package was imported
   * @return false if key package was not imported
   */
  static void ImportKeyPackage(const QString &key_package_path,
                               const QString &phrase_path, int channel,
                               const OperationCallback &cb);

 private:
  /**
   * @brief generate key package name
   *
   * @return QString key package name
   */
  static auto generate_key_package_name() -> QString;
};
}  // namespace GpgFrontend
