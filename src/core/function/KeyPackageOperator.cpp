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

#include <boost/format.hpp>

#include "core/function/KeyPackageOperator.h"
#include "core/function/PassphraseGenerator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

auto KeyPackageOperator::GeneratePassphrase(
    const std::filesystem::path& phrase_path, std::string& phrase) -> bool {
  phrase = PassphraseGenerator::GetInstance().Generate(256);
  SPDLOG_DEBUG("generated passphrase: {} bytes", phrase.size());
  return WriteFileStd(phrase_path, phrase);
}

auto KeyPackageOperator::GenerateKeyPackage(
    const std::filesystem::path& key_package_path,
    const std::string& key_package_name, KeyIdArgsListPtr& key_ids,
    std::string& phrase, bool secret) -> bool {
  SPDLOG_DEBUG("generating key package: {}", key_package_name);

  ByteArrayPtr key_export_data = nullptr;
  if (!GpgKeyImportExporter::GetInstance().ExportAllKeys(
          key_ids, key_export_data, secret)) {
    SPDLOG_ERROR("failed to export keys");
    return false;
  }

  auto key = QByteArray::fromStdString(phrase);
  auto data = QString::fromStdString(*key_export_data).toLocal8Bit().toBase64();

  auto hash_key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
  QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                            QAESEncryption::Padding::ISO);
  auto encoded = encryption.encode(data, hash_key);

  SPDLOG_DEBUG("writing key package: {}", key_package_name);
  return WriteFileStd(key_package_path, encoded.toStdString());
}

auto KeyPackageOperator::ImportKeyPackage(
    const std::filesystem::path& key_package_path,
    const std::filesystem::path& phrase_path, GpgImportInformation& import_info)
    -> bool {
  SPDLOG_DEBUG("importing key package: {]", key_package_path.u8string());

  std::string encrypted_data;
  ReadFileStd(key_package_path, encrypted_data);

  if (encrypted_data.empty()) {
    SPDLOG_ERROR("failed to read key package: {}", key_package_path.u8string());
    return false;
  };

  std::string passphrase;
  ReadFileStd(phrase_path, passphrase);
  SPDLOG_DEBUG("passphrase: {} bytes", passphrase.size());
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

  SPDLOG_DEBUG("key data size: {}", key_data.size());
  if (!key_data.startsWith(PGP_PUBLIC_KEY_BEGIN) &&
      !key_data.startsWith(PGP_PRIVATE_KEY_BEGIN)) {
    return false;
  }

  auto key_data_ptr = std::make_unique<ByteArray>(key_data.toStdString());
  import_info =
      GpgKeyImportExporter::GetInstance().ImportKey(std::move(key_data_ptr));
  return true;
}

auto KeyPackageOperator::GenerateKeyPackageName() -> std::string {
  return generate_key_package_name();
}

/**
 * @brief generate key package name
 *
 * @return std::string key package name
 */
auto KeyPackageOperator::generate_key_package_name() -> std::string {
  std::random_device rd_;          ///< Random device
  auto mt_ = std::mt19937(rd_());  ///< Mersenne twister

  std::uniform_int_distribution<int> dist(999, 99999);
  auto file_string = boost::format("KeyPackage_%1%") % dist(mt_);
  return file_string.str();
}

}  // namespace GpgFrontend
