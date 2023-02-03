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

#include "KeyPackageOperator.h"

#include "FileOperator.h"
#include "function/PassphraseGenerator.h"
#include "function/gpg/GpgKeyGetter.h"
#include "function/gpg/GpgKeyImportExporter.h"
#include "qt-aes/qaesencryption.h"

namespace GpgFrontend {

bool KeyPackageOperator::GeneratePassphrase(
    const std::filesystem::path& phrase_path, std::string& phrase) {
  phrase = PassphraseGenerator::GetInstance().Generate(256);
  SPDLOG_INFO("generated passphrase: {} bytes", phrase.size());
  return FileOperator::WriteFileStd(phrase_path, phrase);
}

bool KeyPackageOperator::GenerateKeyPackage(
    const std::filesystem::path& key_package_path,
    const std::string& key_package_name, KeyIdArgsListPtr& key_ids,
    std::string& phrase, bool secret) {
  SPDLOG_INFO("generating key package: {}", key_package_name);

  ByteArrayPtr key_export_data = nullptr;
  if (!GpgKeyImportExporter::GetInstance().ExportKeys(key_ids, key_export_data,
                                                      secret)) {
    SPDLOG_ERROR("failed to export keys");
    return false;
  }

  auto key = QByteArray::fromStdString(phrase);
  auto data = QString::fromStdString(*key_export_data).toLocal8Bit().toBase64();

  auto hash_key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
  QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                            QAESEncryption::Padding::ISO);
  auto encoded = encryption.encode(data, hash_key);

  SPDLOG_INFO("writing key package: {}", key_package_name);
  return FileOperator::WriteFileStd(key_package_path, encoded.toStdString());
}

bool KeyPackageOperator::ImportKeyPackage(
    const std::filesystem::path& key_package_path,
    const std::filesystem::path& phrase_path,
    GpgFrontend::GpgImportInformation& import_info) {
  SPDLOG_INFO("importing key package: {]", key_package_path.u8string());

  std::string encrypted_data;
  FileOperator::ReadFileStd(key_package_path, encrypted_data);

  if (encrypted_data.empty()) {
    SPDLOG_ERROR("failed to read key package: {}", key_package_path.u8string());
    return false;
  };

  std::string passphrase;
  FileOperator::ReadFileStd(phrase_path, passphrase);
  SPDLOG_INFO("passphrase: {} bytes", passphrase.size());
  if (passphrase.size() != 256) {
    SPDLOG_ERROR("failed to read passphrase: {}", phrase_path.u8string());
    return false;
  }

  auto hash_key = QCryptographicHash::hash(
      QByteArray::fromStdString(passphrase), QCryptographicHash::Sha256);
  auto encoded = QByteArray::fromStdString(encrypted_data);

  QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                            QAESEncryption::Padding::ISO);

  auto decoded = encryption.removePadding(encryption.decode(encoded, hash_key));
  auto key_data = QByteArray::fromBase64(decoded);

  SPDLOG_INFO("key data size: {}", key_data.size());
  if (!key_data.startsWith(GpgConstants::PGP_PUBLIC_KEY_BEGIN) &&
      !key_data.startsWith(GpgConstants::PGP_PRIVATE_KEY_BEGIN)) {
    return false;
  }

  auto key_data_ptr = std::make_unique<ByteArray>(key_data.toStdString());
  import_info =
      GpgKeyImportExporter::GetInstance().ImportKey(std::move(key_data_ptr));
  return true;
}

std::string KeyPackageOperator::GenerateKeyPackageName() {
  return generate_key_package_name();
}

}  // namespace GpgFrontend
