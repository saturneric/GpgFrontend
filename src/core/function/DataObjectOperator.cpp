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

#include "core/function/PassphraseGenerator.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

void DataObjectOperator::init_app_secure_key() {
  GF_CORE_LOG_INFO("initializing application secure key...");
  WriteFile(app_secure_key_path_,
            PassphraseGenerator::GetInstance().Generate(256).toUtf8());
  QFile::setPermissions(app_secure_key_path_,
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

DataObjectOperator::DataObjectOperator(int channel)
    : SingletonFunctionObject<DataObjectOperator>(channel) {
  if (!QDir(app_secure_path_).exists()) QDir(app_secure_path_).mkpath(".");
  if (!QFileInfo(app_secure_key_path_).exists()) init_app_secure_key();

  QByteArray key;
  if (!ReadFile(app_secure_key_path_, key)) {
    GF_CORE_LOG_ERROR("failed to read app secure key file: {}",
                      app_secure_key_path_);
    // unsafe mode
    key = {};
  }

  hash_key_ = QCryptographicHash::hash(key, QCryptographicHash::Sha256);

  if (!QDir(app_data_objs_path_).exists()) {
    QDir(app_data_objs_path_).mkpath(".");
  }
}

auto DataObjectOperator::SaveDataObj(const QString& key,
                                     const QJsonDocument& value) -> QString {
  QByteArray hash_obj_key = {};
  if (key.isEmpty()) {
    hash_obj_key =
        QCryptographicHash::hash(
            hash_key_ +
                PassphraseGenerator::GetInstance().Generate(32).toUtf8() +
                QDateTime::currentDateTime().toString().toUtf8(),
            QCryptographicHash::Sha256)
            .toHex();
  } else {
    hash_obj_key = QCryptographicHash::hash(hash_key_ + key.toUtf8(),
                                            QCryptographicHash::Sha256)
                       .toHex();
  }

  const auto target_obj_path = app_data_objs_path_ + "/" + hash_obj_key;
  auto encoded_data =
      QAESEncryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                     QAESEncryption::Padding::ISO)
          .encode(value.toJson(), hash_key_);
  GF_CORE_LOG_TRACE("saving data object {} to disk {} , size: {} bytes",
                    hash_obj_key, target_obj_path, encoded_data.size());

  // recreate if not exists
  if (!QDir(app_data_objs_path_).exists()) {
    QDir(app_data_objs_path_).mkpath(".");
  }

  if (!WriteFile(target_obj_path, encoded_data)) {
    GF_CORE_LOG_ERROR("failed to write data object to disk: {}", key);
  }
  return key.isEmpty() ? hash_obj_key : QString();
}

auto DataObjectOperator::GetDataObject(const QString& key)
    -> std::optional<QJsonDocument> {
  try {
    GF_CORE_LOG_TRACE("try to get data object from disk, key: {}", key);
    auto hash_obj_key = QCryptographicHash::hash(hash_key_ + key.toUtf8(),
                                                 QCryptographicHash::Sha256)
                            .toHex();

    const auto obj_path = app_data_objs_path_ + "/" + hash_obj_key;
    if (!QFileInfo(obj_path).exists()) {
      GF_CORE_LOG_WARN("data object not found from disk, key: {}", key);
      return {};
    }

    QByteArray encoded_data;
    if (!ReadFile(obj_path, encoded_data)) {
      GF_CORE_LOG_ERROR("failed to read data object from disk, key: {}", key);
      return {};
    }

    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                              QAESEncryption::Padding::ISO);

    auto decoded_data =
        encryption.removePadding(encryption.decode(encoded_data, hash_key_));
    GF_CORE_LOG_TRACE("data object has been decoded, key: {}, data: {}", key,
                      decoded_data);
    return QJsonDocument::fromJson(decoded_data);
  } catch (...) {
    GF_CORE_LOG_ERROR("failed to get data object, caught exception: {}", key);
    return {};
  }
}

auto DataObjectOperator::GetDataObjectByRef(const QString& _ref)
    -> std::optional<QJsonDocument> {
  if (_ref.size() != 64) return {};

  try {
    const auto& hash_obj_key = _ref;
    const auto obj_path = app_data_objs_path_ + "/" + hash_obj_key;

    if (!QFileInfo(obj_path).exists()) return {};

    QByteArray encoded_data;
    if (!ReadFile(obj_path, encoded_data)) return {};

    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::ECB,
                              QAESEncryption::Padding::ISO);

    auto decoded_data =
        encryption.removePadding(encryption.decode(encoded_data, hash_key_));

    return QJsonDocument::fromJson(decoded_data);
  } catch (...) {
    return {};
  }
}
}  // namespace GpgFrontend