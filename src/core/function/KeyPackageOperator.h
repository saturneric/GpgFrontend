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

#include "core/function/GFBufferFactory.h"
#include "core/model/GFBuffer.h"
#include "core/model/GpgImportInformation.h"
#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend {

/**
 * @brief give the possibility to import or export a key package
 *
 */
class GF_CORE_EXPORT KeyPackageOperator
    : public SingletonFunctionObject<KeyPackageOperator> {
 public:
  explicit KeyPackageOperator(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  /**
   * @brief generate the name of the key package
   *
   * @return QString name of the key package
   */
  auto GenerateKeyPackageName() -> QString;

  /**
   * @brief generate key package
   *
   * @param key_package_path path to key package
   * @param key_package_name name of the key package
   * @param key_ids key ids to export
   * @param phrase passphrase to encrypt key package
   * @param secret true if secret key should be exported
   */
  void GenerateKeyPackage(const QString &key_package_path,
                          const QString &key_path,
                          const GpgAbstractKeyPtrList &keys,
                          const GFBuffer &pin, bool secret,
                          const OperationCallback &cb);

  /**
   * @brief import key package
   *
   * @param key_package_path path to key package
   * @param phrase_path path to passphrase file
   * @param import_info import info
   */
  auto ImportKeyPackage(const QString &key_package_path,
                        const QString &key_path, const GFBuffer &pin)
      -> std::tuple<QString, QSharedPointer<GpgImportInformation>>;

 private:
  GFBufferFactory &gb_fac_ = GFBufferFactory::GetInstance(GetChannel());

  /**
   * @brief generate key package name
   *
   * @return QString key package name
   */
  auto generate_key_package_name() -> QString;
};
}  // namespace GpgFrontend
