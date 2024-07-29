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
#include <qt-aes/qaesencryption.h>

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
                                            QString& phrase) -> bool {
  phrase = PassphraseGenerator::GetInstance().Generate(256);
  qCDebug(core, "generated passphrase: %lld bytes", phrase.size());
  return WriteFile(phrase_path, phrase.toUtf8());
}

void KeyPackageOperator::GenerateKeyPackage(const QString& key_package_path,
                                            const QString& key_package_name,
                                            const KeyArgsList& keys,
                                            QString& phrase, bool secret,
                                            const OperationCallback& cb) {
  GpgKeyImportExporter::GetInstance().ExportAllKeys(
      keys, secret, true, [=](GpgError err, const DataObjectPtr& data_obj) {
        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          qCWarning(core) << "export keys error, reason: "
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

        auto data = gf_buffer.ConvertToQByteArray().toBase64();
        auto hash_key = QCryptographicHash::hash(phrase.toUtf8(),
                                                 QCryptographicHash::Sha256);
        QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                                  QAESEncryption::Padding::ISO);
        auto encoded_data = encryption.encode(data, hash_key);

        cb(WriteFile(key_package_path, encoded_data) ? 0 : -1,
           TransferParams());
        return;
      });
}

void KeyPackageOperator::ImportKeyPackage(const QString& key_package_path,
                                          const QString& phrase_path,
                                          const OperationCallback& cb) {
  RunOperaAsync(
      [=](const DataObjectPtr& data_object) -> GFError {
        QByteArray encrypted_data;
        ReadFile(key_package_path, encrypted_data);

        if (encrypted_data.isEmpty()) {
          qCWarning(core) << "failed to read key package: " << key_package_path;
          return -1;
        };

        QByteArray passphrase;
        ReadFile(phrase_path, passphrase);
        if (passphrase.size() != 256) {
          qCWarning(core) << "passphrase size mismatch: " << phrase_path;
          return -1;
        }

        auto hash_key =
            QCryptographicHash::hash(passphrase, QCryptographicHash::Sha256);
        auto encoded = encrypted_data;

        QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                                  QAESEncryption::Padding::ISO);

        auto decoded =
            encryption.removePadding(encryption.decode(encoded, hash_key));
        auto key_data = QByteArray::fromBase64(decoded);
        if (!key_data.startsWith(PGP_PUBLIC_KEY_BEGIN) &&
            !key_data.startsWith(PGP_PRIVATE_KEY_BEGIN)) {
          return -1;
        }

        auto import_info_ptr =
            GpgKeyImportExporter::GetInstance().ImportKey(GFBuffer(key_data));
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
  return QString("KeyPackage_%1")
      .arg(QRandomGenerator::global()->bounded(999, 99999));
}

}  // namespace GpgFrontend
