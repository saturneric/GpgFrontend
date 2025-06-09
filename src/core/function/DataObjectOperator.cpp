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

#include <openssl/evp.h>
#include <openssl/kdf.h>

#include "core/function/GFBufferFactory.h"
#include "core/function/PassphraseGenerator.h"
#include "core/utils/IOUtils.h"

namespace {

auto DeriveObjectKey(const GpgFrontend::GFBuffer& key,
                     const GpgFrontend::GFBuffer& context)
    -> GpgFrontend::GFBufferOrNone {
  GpgFrontend::GFBuffer out(32);
  auto outlen = out.Size();

  EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
  if (pctx == nullptr) {
    LOG_E() << "EVP_PKEY_CTX_new_id failed";
    return {};
  }

  if (EVP_PKEY_derive_init(pctx) <= 0) {
    LOG_E() << "EVP_PKEY_derive_init failed";
    EVP_PKEY_CTX_free(pctx);
    return {};
  }

  if (EVP_PKEY_CTX_set_hkdf_md(pctx, EVP_sha256()) <= 0) {
    LOG_E() << "EVP_PKEY_CTX_set_hkdf_md failed";
    EVP_PKEY_CTX_free(pctx);
    return {};
  }

  if (EVP_PKEY_CTX_set1_hkdf_salt(pctx, nullptr, 0) <= 0) {
    LOG_E() << "EVP_PKEY_CTX_set1_hkdf_salt failed";
    EVP_PKEY_CTX_free(pctx);
    return {};
  }

  if (EVP_PKEY_CTX_set1_hkdf_key(
          pctx, reinterpret_cast<const unsigned char*>(key.Data()),
          static_cast<int>(key.Size())) <= 0) {
    LOG_E() << "EVP_PKEY_CTX_set1_hkdf_key failed";
    EVP_PKEY_CTX_free(pctx);
    return {};
  }

  if (EVP_PKEY_CTX_add1_hkdf_info(
          pctx, reinterpret_cast<const unsigned char*>(context.Data()),
          static_cast<int>(context.Size())) <= 0) {
    LOG_E() << "EVP_PKEY_CTX_add1_hkdf_info failed";
    EVP_PKEY_CTX_free(pctx);
    return {};
  }

  if (EVP_PKEY_derive(pctx, reinterpret_cast<unsigned char*>(out.Data()),
                      &outlen) <= 0) {
    LOG_E() << "EVP_PKEY_derive failed";
    EVP_PKEY_CTX_free(pctx);
    return {};
  }

  EVP_PKEY_CTX_free(pctx);
  out.Resize(static_cast<ssize_t>(outlen));
  return out;
}
}  // namespace

namespace GpgFrontend {
DataObjectOperator::DataObjectOperator(int channel)
    : SingletonFunctionObject<DataObjectOperator>(channel) {
  auto key = gss_.GetAppSecureKey();

  Q_ASSERT(!key.Empty());
  if (!key.Empty()) key_ = key;
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
  const auto ref_hex = ref.ConvertToQByteArray().toHex();
  const auto ref_path = gss_.GetDataObjectsDir() + "/" + ref_hex.left(32);

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

  if (!WriteFileGFBuffer(ref_path, *encrypted)) {
    LOG_E() << "failed to write data object to disk: " << key;
  }

  return ref_hex;
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

    return GFBufferFactory::ToHMACSha256(key_, random).value_or(GFBuffer{});
  }

  return GFBufferFactory::ToHMACSha256(key_, GFBuffer(obj_name))
      .value_or(GFBuffer{});
}

auto DataObjectOperator::read_decr_object(const GFBuffer& ref)
    -> GFBufferOrNone {
  const auto ref_hex = ref.ConvertToQByteArray().toHex();
  const auto ref_path = gss_.GetDataObjectsDir() + "/" + ref_hex.left(32);
  if (!QFileInfo(ref_path).exists()) {
    LOG_W() << "data object not found from disk, ref: " << ref_path;
    return {};
  }

  auto [succ, encrypted] = ReadFileGFBuffer(ref_path);
  if (!succ) {
    LOG_W() << "failed to read data object from disk, ref: " << ref_hex;
    return {};
  }

  if (encrypted.Empty()) {
    LOG_W() << "data object from disk is empty, ref: " << ref_hex;
    return {};
  }

  auto drv_key = DeriveObjectKey(key_, ref);
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

}  // namespace GpgFrontend