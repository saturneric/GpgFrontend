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

#include "DataObjectOperator.h"

#include "core/function/AESCryptoHelper.h"
#include "core/function/PassphraseGenerator.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

void DataObjectOperator::init_app_secure_key() {
  auto key = PassphraseGenerator::GenerateBytesByOpenSSL(256);
  if (!key) {
    LOG_E() << "generate app secure key failed";
    return;
  }

  key_ = *key;
  WriteFileGFBuffer(app_secure_key_path_, *key);
  QFile::setPermissions(app_secure_key_path_,
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

DataObjectOperator::DataObjectOperator(int channel)
    : SingletonFunctionObject<DataObjectOperator>(channel) {
  if (!QDir(app_secure_path_).exists()) QDir(app_secure_path_).mkpath(".");
  if (!QFileInfo(app_secure_key_path_).exists()) init_app_secure_key();

  auto [succ, key_] = ReadFileGFBuffer(app_secure_key_path_);
  if (!succ || key_.Empty()) {
    LOG_E() << "failed to read app secure key file: " << app_secure_key_path_;
    // regenerate secure key
    init_app_secure_key();
  }

  Q_ASSERT(!key_.Empty());

  hash_key_.clear();
  if (!key_.Empty()) {
    hash_key_ = QCryptographicHash::hash(key_.ConvertToQByteArray(),
                                         QCryptographicHash::Sha256);
  }

  if (!QDir(app_data_objs_path_).exists()) {
    QDir(app_data_objs_path_).mkpath(".");
  }
}

auto DataObjectOperator::StoreDataObj(const QString& key,
                                      const QJsonDocument& value) -> QString {
  if (hash_key_.isEmpty()) return {};

  QByteArray hash_obj_key = get_object_ref(key);

  const auto target_obj_path = app_data_objs_path_ + "/" + hash_obj_key;
  auto encrypted = AESCryptoHelper::GCMEncrypt(key_, GFBuffer(value.toJson()));

  if (!encrypted) {
    LOG_E() << "failed to encrypt data object";
    return {};
  }

  // recreate if not exists
  if (!QDir(app_data_objs_path_).exists()) {
    QDir(app_data_objs_path_).mkpath(".");
  }

  if (!WriteFileGFBuffer(target_obj_path, *encrypted)) {
    LOG_E() << "failed to write data object to disk: " << key;
  }
  return key.isEmpty() ? hash_obj_key : QString();
}

auto DataObjectOperator::GetDataObject(const QString& key)
    -> std::optional<QJsonDocument> {
  if (hash_key_.isEmpty()) return {};
  return read_decr_object(get_object_ref(key));
}

auto DataObjectOperator::GetDataObjectByRef(const QString& ref)
    -> std::optional<QJsonDocument> {
  if (hash_key_.isEmpty() || ref.size() != 64) return {};
  return read_decr_object(ref);
}

auto DataObjectOperator::get_object_ref(const QString& key) -> QByteArray {
  if (key.isEmpty()) {
    auto random = PassphraseGenerator::GetInstance().Generate(32);
    if (!random) return {};

    return QCryptographicHash::hash(
               random->ConvertToQByteArray() +
                   QDateTime::currentDateTime().toString().toUtf8(),
               QCryptographicHash::Sha256)
        .toHex();
  }

  return QCryptographicHash::hash(hash_key_ + key.toUtf8(),
                                  QCryptographicHash::Sha256)
      .toHex();
}

auto DataObjectOperator::read_decr_object(const QString& ref)
    -> std::optional<QJsonDocument> {
  const auto obj_path = app_data_objs_path_ + "/" + ref;
  if (!QFileInfo(obj_path).exists()) {
    LOG_W() << "data object not found from disk, ref: " << ref;
    return {};
  }

  auto [succ, encrypted] = ReadFileGFBuffer(obj_path);
  if (!succ) {
    LOG_W() << "failed to read data object from disk, ref: " << ref;
    return {};
  }

  auto plaintext = AESCryptoHelper::GCMDecrypt(key_, encrypted);
  if (!plaintext) {
    LOG_W() << "failed to decrypt data object ref: " << ref;
    return {};
  }

  try {
    return QJsonDocument::fromJson(plaintext->ConvertToQByteArray());
  } catch (...) {
    LOG_W() << "failed to get data object:" << ref << " caught exception.";
    return {};
  }
}
}  // namespace GpgFrontend