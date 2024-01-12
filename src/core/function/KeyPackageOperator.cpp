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

#include "KeyPackageOperator.h"

#include <qt-aes/qaesencryption.h>

#include "core/function/KeyPackageOperator.h"
#include "core/function/PassphraseGenerator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/typedef/CoreTypedef.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

auto KeyPackageOperator::GeneratePassphrase(
    const std::filesystem::path& phrase_path, QString& phrase) -> bool {
  phrase = PassphraseGenerator::GetInstance().Generate(256);
  GF_CORE_LOG_DEBUG("generated passphrase: {} bytes", phrase.size());
  return WriteFile(phrase_path.c_str(), phrase.toUtf8());
}

void KeyPackageOperator::GenerateKeyPackage(
    const std::filesystem::path& key_package_path,
    const QString& key_package_name, const KeyArgsList& keys, QString& phrase,
    bool secret, const OperationCallback& cb) {
  GF_CORE_LOG_DEBUG("generating key package: {}", key_package_name);

  GpgKeyImportExporter::GetInstance().ExportKeys(
      keys, secret, true, false, false,
      [=](GpgError err, const DataObjectPtr& data_obj) {
        if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
          GF_LOG_ERROR("export keys error, reason: {}",
                       DescribeGpgErrCode(err).second);
          cb(-1, data_obj);
          return;
        }

        if (data_obj == nullptr || !data_obj->Check<GFBuffer>()) {
          throw std::runtime_error("data object doesn't pass checking");
        }

        auto gf_buffer = ExtractParams<GFBuffer>(data_obj, 0);

        auto data = gf_buffer.ConvertToQByteArray().toBase64();
        auto hash_key = QCryptographicHash::hash(phrase.toUtf8(),
                                                 QCryptographicHash::Sha256);
        QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                                  QAESEncryption::Padding::ISO);
        auto encoded_data = encryption.encode(data, hash_key);
        GF_CORE_LOG_DEBUG("writing key package, name: {}", key_package_name);

        cb(WriteFile(key_package_path.c_str(), encoded_data) ? 0 : -1,
           TransferParams());
        return;
      });
}

auto KeyPackageOperator::ImportKeyPackage(
    const std::filesystem::path& key_package_path,
    const std::filesystem::path& phrase_path)
    -> std::tuple<bool, std::shared_ptr<GpgImportInformation>> {
  GF_CORE_LOG_DEBUG("importing key package: {}", key_package_path.u8string());

  QByteArray encrypted_data;
  ReadFile(key_package_path.c_str(), encrypted_data);

  if (encrypted_data.isEmpty()) {
    GF_CORE_LOG_ERROR("failed to read key package: {}",
                      key_package_path.u8string());
    return {false, nullptr};
  };

  QByteArray passphrase;
  ReadFile(phrase_path.c_str(), passphrase);
  GF_CORE_LOG_DEBUG("passphrase: {} bytes", passphrase.size());
  if (passphrase.size() != 256) {
    GF_CORE_LOG_ERROR("failed to read passphrase: {}", phrase_path.u8string());
    return {false, nullptr};
  }

  auto hash_key =
      QCryptographicHash::hash(passphrase, QCryptographicHash::Sha256);
  auto encoded = encrypted_data;

  QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                            QAESEncryption::Padding::ISO);

  auto decoded = encryption.removePadding(encryption.decode(encoded, hash_key));
  auto key_data = QByteArray::fromBase64(decoded);

  GF_CORE_LOG_DEBUG("import key package, read key data size: {}",
                    key_data.size());
  if (!key_data.startsWith(PGP_PUBLIC_KEY_BEGIN) &&
      !key_data.startsWith(PGP_PRIVATE_KEY_BEGIN)) {
    return {false, nullptr};
  }

  auto import_info =
      GpgKeyImportExporter::GetInstance().ImportKey(GFBuffer(key_data));
  return {true, import_info};
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
  return QString("KeyPackage_%1%")
      .arg(QRandomGenerator::global()->bounded(999, 99999));
}

}  // namespace GpgFrontend
