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

#include "KeyPackageOperator.h"

#include <qglobal.h>

#include "core/function/AESCryptoHelper.h"
#include "core/function/KeyPackageOperator.h"
#include "core/function/PassphraseGenerator.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/model/GpgImportInformation.h"
#include "core/typedef/CoreTypedef.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

auto KeyPackageOperator::GeneratePassphrase(const QString& phrase_path,
                                            GFBuffer& phrase) -> bool {
  auto random = PassphraseGenerator::GetInstance().GenerateBytes(256);
  if (!random) return false;

  phrase = *random;
  return WriteFileGFBuffer(phrase_path, phrase);
}

void KeyPackageOperator::GenerateKeyPackage(const QString& key_package_path,
                                            const QString& key_package_name,
                                            int channel,
                                            const GpgAbstractKeyPtrList& keys,
                                            GFBuffer& phrase, bool secret,
                                            const OperationCallback& cb) {
  GpgKeyImportExporter::GetInstance(channel).ExportAllKeys(
      keys, secret, true, [=](GpgError err, const DataObjectPtr& data_obj) {
        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          LOG_W() << "export keys error, reason: "
                  << DescribeGpgErrCode(err).second;
          cb(-1, data_obj);
          return;
        }

        if (CheckGpgError(err) == GPG_ERR_USER_1 || data_obj == nullptr ||
            !data_obj->Check<GFBuffer>()) {
          cb(-1, data_obj);
          return;
        }

        auto gf_buffer = ExtractParams<GFBuffer>(data_obj, 0);

        // AES encrypt
        auto encrypted = AESCryptoHelper::GCMEncrypt(phrase, gf_buffer);
        if (!encrypted) {
          cb(-1, data_obj);
          return;
        }

        cb(WriteFileGFBuffer(key_package_path, *encrypted) ? 0 : -1,
           TransferParams());
        return;
      });
}

void KeyPackageOperator::ImportKeyPackage(const QString& key_package_path,
                                          const QString& phrase_path,
                                          int channel,
                                          const OperationCallback& cb) {
  RunOperaAsync(
      [=](const DataObjectPtr& data_object) -> GFError {
        auto [succ_e, encrypted] = ReadFileGFBuffer(key_package_path);

        if (!succ_e || encrypted.Empty()) {
          LOG_W() << "failed to read key package: " << key_package_path;
          cb(-1, data_object);
          return -1;
        };

        auto [succ_p, passphrase] = ReadFileGFBuffer(phrase_path);
        if (!succ_p || passphrase.Size() != 256) {
          LOG_W() << "passphrase invalid: " << phrase_path;
          cb(-1, data_object);
          return -1;
        }

        auto key_data = AESCryptoHelper::GCMDecrypt(passphrase, encrypted);
        if (!key_data) {
          cb(-1, data_object);
          return -1;
        }

        auto import_info_ptr =
            GpgKeyImportExporter::GetInstance(channel).ImportKey(*key_data);
        if (import_info_ptr == nullptr) return GPG_ERR_NO_DATA;

        auto import_info = *import_info_ptr;
        data_object->Swap({import_info});
        return 0;
      },
      cb, "import_key_package");
}

auto KeyPackageOperator::GenerateKeyPackageName() -> QString {
  return generate_key_package_name();
}

/**
 * @brief generate key package name
 *
 * @return QString key package name
 */
auto KeyPackageOperator::generate_key_package_name() -> QString {
  auto random = SecureRandomGenerator::GetInstance().GnuPGGenerateZBase32();

  if (!random) return {};
  return QString("KeyPackage_%1").arg(random->Left(8).ConvertToQByteArray());
}

}  // namespace GpgFrontend
