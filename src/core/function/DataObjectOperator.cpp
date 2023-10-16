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

#include "DataObjectOperator.h"

#include <qt-aes/qaesencryption.h>

#include <boost/date_time.hpp>

#include "core/function/FileOperator.h"
#include "core/function/PassphraseGenerator.h"

void GpgFrontend::DataObjectOperator::init_app_secure_key() {
  SPDLOG_DEBUG("initializing application secure key");
  FileOperator::WriteFileStd(app_secure_key_path_,
                             PassphraseGenerator::GetInstance().Generate(256));
  std::filesystem::permissions(
      app_secure_key_path_,
      std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
}

GpgFrontend::DataObjectOperator::DataObjectOperator(int channel)
    : SingletonFunctionObject<DataObjectOperator>(channel) {
  if (!is_directory(app_secure_path_)) create_directory(app_secure_path_);

  if (!exists(app_secure_key_path_)) {
    init_app_secure_key();
  }

  std::string key;
  if (!FileOperator::ReadFileStd(app_secure_key_path_.u8string(), key)) {
    SPDLOG_ERROR("failed to read app secure key file: {}",
                 app_secure_key_path_.u8string());
    throw std::runtime_error("failed to read app secure key file");
  }
  hash_key_ = QCryptographicHash::hash(QByteArray::fromStdString(key),
                                       QCryptographicHash::Sha256);
  SPDLOG_DEBUG("app secure key loaded {} bytes", hash_key_.size());

  if (!exists(app_data_objs_path_)) create_directory(app_data_objs_path_);
}

std::string GpgFrontend::DataObjectOperator::SaveDataObj(
    const std::string& _key, const nlohmann::json& value) {
  std::string _hash_obj_key = {};
  if (_key.empty()) {
    _hash_obj_key =
        QCryptographicHash::hash(
            hash_key_ + QByteArray::fromStdString(
                            PassphraseGenerator::GetInstance().Generate(32) +
                            to_iso_extended_string(
                                boost::posix_time::second_clock::local_time())),
            QCryptographicHash::Sha256)
            .toHex()
            .toStdString();
  } else {
    _hash_obj_key =
        QCryptographicHash::hash(hash_key_ + QByteArray::fromStdString(_key),
                                 QCryptographicHash::Sha256)
            .toHex()
            .toStdString();
  }

  const auto obj_path = app_data_objs_path_ / _hash_obj_key;

  QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                            QAESEncryption::Padding::ISO);
  auto encoded =
      encryption.encode(QByteArray::fromStdString(to_string(value)), hash_key_);

  SPDLOG_DEBUG("saving data object {} to {} , size: {} bytes", _hash_obj_key,
               obj_path.u8string(), encoded.size());

  FileOperator::WriteFileStd(obj_path.u8string(), encoded.toStdString());

  return _key.empty() ? _hash_obj_key : std::string();
}

std::optional<nlohmann::json> GpgFrontend::DataObjectOperator::GetDataObject(
    const std::string& _key) {
  try {
    SPDLOG_DEBUG("get data object {}", _key);
    auto _hash_obj_key =
        QCryptographicHash::hash(hash_key_ + QByteArray::fromStdString(_key),
                                 QCryptographicHash::Sha256)
            .toHex()
            .toStdString();

    const auto obj_path = app_data_objs_path_ / _hash_obj_key;

    if (!std::filesystem::exists(obj_path)) {
      SPDLOG_ERROR("data object not found :{}", _key);
      return {};
    }

    std::string buffer;
    if (!FileOperator::ReadFileStd(obj_path.u8string(), buffer)) {
      SPDLOG_ERROR("failed to read data object: {}", _key);
      return {};
    }

    SPDLOG_DEBUG("data object found {}", _key);

    auto encoded = QByteArray::fromStdString(buffer);
    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                              QAESEncryption::Padding::ISO);

    SPDLOG_DEBUG("decrypting data object {} , hash key size: {}",
                 encoded.size(), hash_key_.size());

    auto decoded =
        encryption.removePadding(encryption.decode(encoded, hash_key_));

    SPDLOG_DEBUG("data object decoded: {}", _key);

    return nlohmann::json::parse(decoded.toStdString());
  } catch (...) {
    SPDLOG_ERROR("failed to get data object: {}", _key);
    return {};
  }
}

std::optional<nlohmann::json>
GpgFrontend::DataObjectOperator::GetDataObjectByRef(const std::string& _ref) {
  if (_ref.size() != 64) return {};

  try {
    const auto& _hash_obj_key = _ref;
    const auto obj_path = app_data_objs_path_ / _hash_obj_key;

    if (!std::filesystem::exists(obj_path)) return {};

    std::string buffer;
    if (!FileOperator::ReadFileStd(obj_path.u8string(), buffer)) return {};
    auto encoded = QByteArray::fromStdString(buffer);

    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                              QAESEncryption::Padding::ISO);

    auto decoded =
        encryption.removePadding(encryption.decode(encoded, hash_key_));

    return nlohmann::json::parse(decoded.toStdString());
  } catch (...) {
    return {};
  }
}
