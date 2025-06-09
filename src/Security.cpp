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

#include "Security.h"

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>

#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"

namespace {
template <typename T>
auto Require(const std::optional<T> &opt, const QString &title,
             const QString &message) -> T {
  if (!opt) {
    QMessageBox::warning(nullptr, title, message, QMessageBox::Ok);
    abort();
  }
  return *opt;
}

void RequireWriteSuccess(bool ok, const QString &key_path) {
  if (!ok) {
    qCritical() << "write app secure key failed: " << key_path;
    QMessageBox::critical(
        nullptr, QObject::tr("Save Key Failed"),
        QObject::tr(
            "Failed to save the secure key to disk at: %1\n"
            "Please check your storage or try running as administrator.")
            .arg(key_path),
        QMessageBox::Ok);
    abort();
  }
}
}  // namespace

namespace GpgFrontend {

auto DeriveKeyArgon2(const GFBuffer &passphrase, const GFBuffer &salt,
                     int key_len, int t_cost, int m_cost, int parallelism)
    -> GFBufferOrNone {
  GFBuffer key(key_len);

  auto *kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
  if (kdf == nullptr) {
    qCritical() << "EVP_KDF_fetch failed";
    return {};
  }

  EVP_KDF_CTX *kctx = EVP_KDF_CTX_new(kdf);
  if (kctx == nullptr) {
    qCritical() << "EVP_KDF_CTX_new failed";
    EVP_KDF_free(kdf);
    return {};
  }

  std::array<OSSL_PARAM, 6> params = {
      {OSSL_PARAM_octet_string("pass", const_cast<char *>(passphrase.Data()),
                               passphrase.Size()),
       OSSL_PARAM_octet_string("salt", const_cast<char *>(salt.Data()),
                               salt.Size()),
       OSSL_PARAM_int("t", &t_cost), OSSL_PARAM_int("m", &m_cost),
       OSSL_PARAM_int("p", &parallelism), OSSL_PARAM_END}};

  int rc = EVP_KDF_derive(kctx, reinterpret_cast<unsigned char *>(key.Data()),
                          key.Size(), params.data());

  EVP_KDF_CTX_free(kctx);
  EVP_KDF_free(kdf);

  if (rc != 1) {
    qCritical() << "EVP_KDF_derive failed";
    return {};
  }

  return key;
}

auto CalculateKeyId(const GFBuffer &pin, const GFBuffer &key) -> GFBuffer {
  auto id = GFBufferFactory::ToHMACSha256(
      pin.Empty() ? GFBuffer("GpgFrontend") : pin, key);
  Q_ASSERT(id.has_value());

  return id.value_or(GFBuffer{});
}

auto FetchTimeRelatedAppSecureKey(const GFBuffer &pin) -> GFBuffer {
  auto &gss = GlobalSettingStation::GetInstance();

  // do rotation per week
  const qint64 timestamp = QDateTime::currentSecsSinceEpoch() /
                           (static_cast<qint64>(60 * 60 * 24 * 7));

  auto salt = Require(
      GFBufferFactory::ToSha256(
          GFBuffer("GF_ROT_KEY" + QString::number(timestamp))),
      QObject::tr("Time Rotation Secure Key Generation Failed"),
      QObject::tr(
          "Failed to generate a salt; falling back to less-secure key."));

  auto key =
      Require(DeriveKeyArgon2(pin, salt.Left(16), 32),
              QObject::tr("Time Rotation Secure Key Generation Failed"),
              QObject::tr("Failed to derive time-rotated key; falling back to "
                          "less-secure key."));

  auto key_id = Require(
      GFBufferFactory::ToHMACSha256(pin, key),
      QObject::tr("Time Rotation Secure Key Generation Failed"),
      QObject::tr(
          "Failed to compute key ID; falling back to less-secure key."));

  Q_ASSERT(!key_id.Empty());

  // set app secure key
  gss.SetActiveKeyId(key_id);

  auto e_key = GFBufferFactory::Encrypt(pin, key);
  if (!e_key) {
    qCritical() << "encrypt app secure key failed! Won't write it to disk.";
    QMessageBox::warning(
        nullptr, QObject::tr("Encrypt Key Failed"),
        QObject::tr("Failed to encrypt the secure key with your PIN. The key "
                    "will not be saved to disk."),
        QMessageBox::Ok);
    return key;
  }

  auto e_key_id = Require(
      GFBufferFactory::ToHMACSha256(pin, *e_key),
      QObject::tr("Time Rotation Secure Key Generation Failed"),
      QObject::tr("Failed to generate a secure application key using OpenSSL. "
                  "A less secure fallback key will be used. Please check your "
                  "system's cryptography support."));
  Q_ASSERT(!e_key_id.Empty());

  const auto key_path = gss.GetAppSecureKeyDir() + "/" +
                        e_key_id.ConvertToQByteArray().toHex().left(16) +
                        ".key";

  if (QFileInfo(key_path).exists()) return key;
  RequireWriteSuccess(GFBufferFactory::ToFile(key_path, *e_key), key_path);
  return key;
}

auto NewLegacyAppSecureKey(const GFBuffer &pin) -> GFBuffer {
  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  auto &gss = GlobalSettingStation::GetInstance();

  auto key = PassphraseGenerator::GenerateBytesByOpenSSL(256);
  if (!key) {
    qCritical()
        << "generate app secure key failed, using qt random generator...";
    if (secure_level > 2) {
      QMessageBox::warning(
          nullptr, QObject::tr("Secure Key Generation Failed"),
          QObject::tr(
              "Failed to generate a secure application key using OpenSSL. "
              "A less secure fallback key will be used. Please check your "
              "system's cryptography support."),
          QMessageBox::Ok);
    }
    key = GFBuffer(QRandomGenerator64::securelySeeded().generate());
  }

  auto r_key = *key;

  if (secure_level > 2 && !pin.Empty()) {
    auto e_key = GFBufferFactory::Encrypt(pin, *key);
    if (!e_key) {
      qCritical() << "encrypt app secure key failed! Won't write it to disk.";
      QMessageBox::critical(
          nullptr, QObject::tr("Encrypt Key Failed"),
          QObject::tr("Failed to encrypt the secure key with your PIN. The key "
                      "will not be saved to disk."),
          QMessageBox::Ok);
      abort();
    }
    key = e_key;
  }

  auto path = gss.GetLegacyAppSecureKeyPath();

  if (!GFBufferFactory::ToFile(path, *key)) {
    qCritical() << "write app secure key failed: " << path;
    if (secure_level > 2) {
      QMessageBox::critical(
          nullptr, QObject::tr("Save Key Failed"),
          QObject::tr(
              "Failed to save the secure key to disk at: %1\n"
              "Please check your storage or try running as administrator.")
              .arg(path),
          QMessageBox::Ok);
      abort();
    }
  }

  return r_key;
}

auto InitLegacyAppSecureKey(const GFBuffer &pin) -> bool {
  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  auto &gss = GlobalSettingStation::GetInstance();

  GFBuffer legacy_key;
  GFBuffer legacy_key_id;
  if (!QFileInfo(gss.GetLegacyAppSecureKeyPath()).exists()) {
    legacy_key = NewLegacyAppSecureKey(pin);
    legacy_key_id = CalculateKeyId(pin, legacy_key);
    Q_ASSERT(!legacy_key.Empty());
  } else {
    auto l_key_path = gss.GetLegacyAppSecureKeyPath();

    auto key = GFBufferFactory::FromFile(l_key_path);
    if (!key) {
      qCritical() << "read app secure key failed: " << l_key_path;
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr(
              "Failed to read the application secure key from disk at: %1\n"
              "Please ensure the key file exists and is accessible, or try "
              "re-initializing the secure key.")
              .arg(l_key_path),
          QMessageBox::Ok);
      return false;
    }
    legacy_key = *key;
    legacy_key_id = CalculateKeyId(pin, *key);
  }

  // no we don't
  // we have to decrypt all the app secure keys
  QMap<GFBuffer, GFBuffer> r_keys;
  if (secure_level > 2 && !pin.Empty()) {
    auto r_key = GFBufferFactory::Decrypt(pin, legacy_key);

    if (!r_key) {
      qWarning() << "decrypt legacy app secure key failed";

      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr("Failed to decrypt the application secure key. Your PIN "
                      "may be incorrect, or the key file may be "
                      "corrupted.Please clear the secure key and try again."),
          QMessageBox::Ok);
      return false;
    }

    legacy_key = *r_key;
    legacy_key_id = CalculateKeyId(pin, *r_key);
  }

  gss.SetActiveKeyId(legacy_key_id);
  gss.SetLegacyKeyId(legacy_key_id);

  Q_ASSERT(!gss.GetActiveKeyId().Empty());
  gss.AppendAppSecureKeys({{legacy_key_id, legacy_key}});
  return true;
}

auto InitAppSecureKey(const GFBuffer &pin) -> bool {
  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  auto &gss = GlobalSettingStation::GetInstance();

  if (!InitLegacyAppSecureKey(pin)) return false;

  // stop here for security level less than 3
  if (secure_level < 3) return true;

  // now we are at the high security mode

  QMap<GFBuffer, GFBuffer> keys;

  auto t_key = FetchTimeRelatedAppSecureKey(pin);
  Q_ASSERT(!t_key.Empty());

  // add active key to list
  keys.insert(CalculateKeyId(pin, t_key), t_key);

  QDir dir(gss.GetAppSecureKeyDir());
  auto key_files = dir.entryList({"*.key"}, QDir::Files);

  // read all the keys from disks
  for (const QString &key_file : key_files) {
    const auto key_path = dir.absoluteFilePath(key_file);
    auto key = GFBufferFactory::FromFile(key_path);

    if (!key) {
      qCritical() << "read app secure key failed: " << key_path;
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr(
              "Failed to read the application secure key from disk at: %1\n"
              "Please ensure the key file exists and is accessible, or try "
              "re-initializing the secure key.")
              .arg(key_path),
          QMessageBox::Ok);
      return false;
    }

    auto e_key_id = CalculateKeyId(pin, *key);
    keys.insert(e_key_id, *key);
  }

  // we have to decrypt all the app secure keys
  QMap<GFBuffer, GFBuffer> r_keys;

  for (auto it = keys.constBegin(); it != keys.constEnd(); ++it) {
    auto r_key = GFBufferFactory::Decrypt(pin, it.value());

    // maybe that's not my key
    if (!r_key) continue;

    // recalculate the real key id
    auto key_id = CalculateKeyId(pin, *r_key);
    r_keys[key_id] = *r_key;
  }

  keys = r_keys;

  Q_ASSERT(!gss.GetActiveKeyId().Empty());
  gss.AppendAppSecureKeys(keys);
  return true;
}

};  // namespace GpgFrontend