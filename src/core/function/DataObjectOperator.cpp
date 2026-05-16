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

#include <sodium.h>

#include <cstring>

#include "core/function/GFBufferFactory.h"
#include "core/function/PassphraseGenerator.h"
#include "core/utils/IOUtils.h"

namespace {

constexpr std::string_view kObjectKeyDeriveDomain = "GpgFrontend.DataObject.v2";

auto EnsureSodiumInit() -> bool {
  static const int kRc = sodium_init();
  if (kRc < 0) {
    LOG_E() << "sodium_init failed";
    return false;
  }
  return true;
}

auto DeriveObjectKey(const GpgFrontend::GFBuffer& key,
                     const GpgFrontend::GFBuffer& context)
    -> GpgFrontend::GFBufferOrNone {
  if (!EnsureSodiumInit()) return {};

  if (key.Empty() || context.Empty()) {
    LOG_E() << "DeriveObjectKey received empty key or context";
    return {};
  }

  GpgFrontend::GFBuffer normalized_key(crypto_generichash_KEYBYTES);

  if (key.Size() == crypto_generichash_KEYBYTES) {
    std::memcpy(normalized_key.Data(), key.Data(), normalized_key.Size());
  } else {
    const int key_hash_rc = crypto_generichash(
        reinterpret_cast<unsigned char*>(normalized_key.Data()),
        normalized_key.Size(),
        reinterpret_cast<const unsigned char*>(key.Data()),
        static_cast<unsigned long long>(key.Size()), nullptr, 0);

    if (key_hash_rc != 0) {
      LOG_E() << "crypto_generichash normalize key failed";
      return {};
    }
  }

  GpgFrontend::GFBuffer input;
  input.Combine({
      GpgFrontend::GFBuffer(kObjectKeyDeriveDomain.data(),
                            kObjectKeyDeriveDomain.size()),
      context,
  });

  GpgFrontend::GFBuffer out(crypto_aead_xchacha20poly1305_ietf_KEYBYTES);

  const int rc = crypto_generichash(
      reinterpret_cast<unsigned char*>(out.Data()), out.Size(),
      reinterpret_cast<const unsigned char*>(input.Data()),
      static_cast<unsigned long long>(input.Size()),
      reinterpret_cast<const unsigned char*>(normalized_key.Data()),
      normalized_key.Size());

  sodium_memzero(normalized_key.Data(), normalized_key.Size());

  if (rc != 0) {
    LOG_E() << "crypto_generichash derive object key failed";
    return {};
  }

  return out;
}

}  // namespace

namespace GpgFrontend {
DataObjectOperator::DataObjectOperator(int channel)
    : SingletonFunctionObject<DataObjectOperator>(channel) {
  key_ = gss_.GetActiveAppSecureKey();
  Q_ASSERT(!key_.Empty());

  key_id_ = gss_.GetActiveKeyId();
  Q_ASSERT(!key_id_.Empty());

  l_key_ = gss_.GetLegacyAppSecureKey();
  Q_ASSERT(!l_key_.Empty());
}

auto DataObjectOperator::StoreDataObj(const QString& key,
                                      const QJsonDocument& value) -> QString {
  return StoreSecDataObj(key, GFBuffer(value.toJson()));
}

auto DataObjectOperator::GetDataObject(const QString& key)
    -> std::optional<QJsonDocument> {
  if (key_.Empty()) return {};
  return read_decr_json_object(get_object_ref(key));
}

auto DataObjectOperator::GetDataObjectByRef(const QString& ref)
    -> std::optional<QJsonDocument> {
  if (key_.Empty() || ref.size() != 64) return {};
  return read_decr_json_object(GFBuffer(QByteArray::fromHex(ref.toLatin1())));
}

auto DataObjectOperator::StoreSecDataObj(const QString& key,
                                         const GFBuffer& value) -> QString {
  if (key_.Empty()) return {};

  // recreate if not exists
  if (!QDir(gss_.GetDataObjectsDir()).exists()) {
    QDir(gss_.GetDataObjectsDir()).mkpath(".");
  }

  auto ref = get_object_ref(key);
  return write_encr_object(ref, value);
}

auto DataObjectOperator::GetSecDataObject(const QString& key)
    -> GFBufferOrNone {
  if (key_.Empty()) return {};
  return read_decr_object(get_object_ref(key));
}

auto DataObjectOperator::GetSecDataObjectByRef(const QString& ref)
    -> GFBufferOrNone {
  if (key_.Empty() || ref.size() != 64) return {};
  return read_decr_object(GFBuffer(QByteArray::fromHex(ref.toLatin1())));
}

auto DataObjectOperator::get_object_ref(const QString& obj_name) -> GFBuffer {
  if (obj_name.isEmpty()) {
    auto random =
        PassphraseGenerator::GetInstance().Generate(32).value_or(GFBuffer(
            QString::number(QRandomGenerator64::securelySeeded().generate())));

    return GFBufferFactory::ToHMACSha256(l_key_, random).value_or(GFBuffer{});
  }

  return GFBufferFactory::ToHMACSha256(l_key_, GFBuffer(obj_name))
      .value_or(GFBuffer{});
}

auto DataObjectOperator::read_decr_object(const GFBuffer& ref)
    -> GFBufferOrNone {
  const auto ref_hex = ref.ConvertToQByteArray().toHex();
  const auto ref_path = gss_.GetDataObjectsDir() + "/" + ref_hex;
  if (!QFileInfo(ref_path).exists()) {
    LOG_W() << "data object not found from disk, ref: " << ref_hex;
    return {};
  }

  auto [succ, data] = ReadFileGFBuffer(ref_path);
  if (!succ) {
    LOG_W() << "failed to read data object from disk, ref: " << ref_hex;
    return {};
  }

  auto key_id = data.Left(32);
  auto key = gss_.GetAppSecureKey(key_id);

  if (key.Empty()) {
    LOG_W() << "fail to find key of data object, key"
            << key_id.ConvertToQByteArray().toHex() << " ref: " << ref_hex;
    return {};
  }

  auto encrypted = data.Right(static_cast<int>(data.Size() - key_id.Size()));
  if (encrypted.Empty()) {
    LOG_W() << "data object from disk is empty, ref: " << ref_hex;
    return {};
  }

  auto drv_key = DeriveObjectKey(key, ref);
  if (!drv_key) {
    LOG_W() << "failed to derive key from ref: " << ref_hex;
    return {};
  }

  auto plaintext = GFBufferFactory::DecryptLite(*drv_key, encrypted);
  if (!plaintext) {
    LOG_W() << "failed to decrypt data object ref: " << ref_hex;
    return {};
  }

  return plaintext;
}

auto DataObjectOperator::read_decr_json_object(const GFBuffer& ref)
    -> std::optional<QJsonDocument> {
  auto plaintext = read_decr_object(ref);
  if (!plaintext) return {};

  try {
    return QJsonDocument::fromJson(plaintext->ConvertToQByteArray());
  } catch (...) {
    const auto ref_hex = ref.ConvertToQByteArray().toHex();
    LOG_W() << "failed to get data object:" << ref_hex << " caught exception.";
    return {};
  }
}

auto DataObjectOperator::write_encr_object(const GFBuffer& ref,
                                           const GFBuffer& value) -> QString {
  const auto ref_hex = ref.ConvertToQByteArray().toHex();
  const auto ref_path = gss_.GetDataObjectsDir() + "/" + ref_hex;

  auto drv_key = DeriveObjectKey(key_, ref);
  if (!drv_key) {
    LOG_W() << "failed to derive key from ref: " << ref_hex;
    return {};
  }

  auto encrypted = GFBufferFactory::EncryptLite(*drv_key, value);
  if (!encrypted) {
    LOG_E() << "failed to encrypt data object: " << ref_hex;
    return {};
  }

  GFBuffer data;
  data.Combine({key_id_, *encrypted});

  if (!WriteFileGFBuffer(ref_path, data)) {
    LOG_E() << "failed to write data object to disk: " << ref_hex;
  }

  return ref_hex;
}
}  // namespace GpgFrontend