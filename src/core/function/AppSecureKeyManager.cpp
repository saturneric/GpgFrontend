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

#include "core/function/AppSecureKeyManager.h"

#include "core/function/AESCryptoHelper.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/PassphraseGenerator.h"

namespace {

/// Label standing in for the HMAC key when no identity PIN is set. Changing it
/// would change every key ID, orphaning all stored data objects.
constexpr auto kEmptyPinLabel = "GpgFrontend";

/// Length of the legacy key, in bytes.
constexpr int kLegacyKeyLen = 256;

/// Salt prefix for the weekly rotating key.
constexpr auto kRotatingKeySaltPrefix = "GF_ROT_KEY";

/// Seconds in the rotation period of the time-related key.
constexpr qint64 kRotationPeriodSecs = 60LL * 60 * 24 * 7;

}  // namespace

namespace GpgFrontend {

AppSecureKeyManager::AppSecureKeyManager(int channel)
    : SingletonFunctionObject<AppSecureKeyManager>(channel) {}

auto AppSecureKeyManager::CalculateKeyId(const GFBuffer& pin,
                                         const GFBuffer& key) -> GFBuffer {
  auto id = GFBufferFactory::ToHMACSha256(
      pin.Empty() ? GFBuffer(kEmptyPinLabel) : pin, key);
  Q_ASSERT(id.has_value());

  return id.value_or(GFBuffer{});
}

auto AppSecureKeyManager::GetKeyDir() const -> QString {
  return GetGSS().GetAppDataPath() + "/secure";
}

auto AppSecureKeyManager::GetLegacyKeyPath() const -> QString {
  return GetKeyDir() + "/app.key";
}

auto AppSecureKeyManager::GetKey(const GFBuffer& id) const -> GFBuffer {
  return keys_.value(id, GFBuffer{});
}

auto AppSecureKeyManager::GetActiveKeyId() const -> GFBuffer {
  return active_key_id_;
}

auto AppSecureKeyManager::GetActiveKey() const -> GFBuffer {
  auto key = GetKey(active_key_id_);
  Q_ASSERT(!key.Empty());
  return key;
}

auto AppSecureKeyManager::GetLegacyKey() const -> GFBuffer {
  auto key = GetKey(legacy_key_id_);
  Q_ASSERT(!key.Empty());
  return key;
}

auto AppSecureKeyManager::new_legacy_key(const GFBuffer& wrap,
                                         AppSecureKeyInitResult& status)
    -> GFBuffer {
  auto key = PassphraseGenerator::GenerateBytesByOpenSSL(kLegacyKeyLen);
  if (!key) {
    LOG_E() << "generate app secure key failed, using qt random generator...";
    key = GFBuffer(QRandomGenerator64::securelySeeded().generate());
  }

  auto plain_key = *key;

  if (!wrap.Empty()) {
    auto e_key = GFBufferFactory::Encrypt(wrap, plain_key);
    if (!e_key) {
      LOG_E() << "encrypt app secure key failed, won't write it to disk";
      status = {AppSecureKeyStatus::kWriteFailed,
                QObject::tr("The secure key could not be encrypted, so it was "
                            "not saved to disk.")};
      return plain_key;
    }
    key = e_key;
  }

  const auto path = GetLegacyKeyPath();
  if (!GFBufferFactory::ToFile(path, *key)) {
    LOG_E() << "write app secure key failed:" << path;
    status = {AppSecureKeyStatus::kWriteFailed, path};
  }

  return plain_key;
}

auto AppSecureKeyManager::init_legacy_key(const GFBuffer& pin,
                                          const GFBuffer& wrap)
    -> AppSecureKeyInitResult {
  AppSecureKeyInitResult result;

  GFBuffer legacy_key;
  const auto path = GetLegacyKeyPath();
  LOG_D() << "legacy app secure key path:" << path;

  if (!QFileInfo(path).exists()) {
    legacy_key = new_legacy_key(wrap, result);
    if (legacy_key.Empty()) {
      return {AppSecureKeyStatus::kGenerateFailed, path};
    }
  } else {
    auto key = GFBufferFactory::FromFile(path);
    if (!key) {
      LOG_E() << "read app secure key failed:" << path;
      return {AppSecureKeyStatus::kReadFailed, path};
    }
    legacy_key = *key;

    if (!wrap.Empty()) {
      auto r_key = GFBufferFactory::Decrypt(wrap, legacy_key);
      if (!r_key) {
        LOG_W() << "decrypt legacy app secure key failed";
        return {AppSecureKeyStatus::kDecryptFailed, path};
      }
      legacy_key = *r_key;
    }
  }

  // The identity always comes from the PIN, never from the wrap secret: the ID
  // is stored as a prefix on every data object, so deriving it from the at-rest
  // protection would orphan every object whenever that protection changed.
  const auto legacy_key_id = CalculateKeyId(pin, legacy_key);
  Q_ASSERT(!legacy_key_id.Empty());

  active_key_id_ = legacy_key_id;
  legacy_key_id_ = legacy_key_id;
  keys_.insert(legacy_key_id, legacy_key);

  return result;
}

auto AppSecureKeyManager::fetch_time_related_key(const GFBuffer& pin)
    -> GFBuffer {
  const qint64 period =
      QDateTime::currentSecsSinceEpoch() / kRotationPeriodSecs;

  auto salt = GFBufferFactory::ToSha256(
      GFBuffer(kRotatingKeySaltPrefix + QString::number(period)));
  if (!salt) {
    LOG_E() << "generate rotating key salt failed";
    return {};
  }

  auto key = AESCryptoHelper::DeriveKeyArgon2(pin, salt->Left(16), 32);
  if (!key) {
    LOG_E() << "derive time-rotated key failed";
    return {};
  }

  auto key_id = GFBufferFactory::ToHMACSha256(pin, *key);
  if (!key_id) {
    LOG_E() << "compute time-rotated key id failed";
    return {};
  }

  active_key_id_ = *key_id;

  const auto key_path = GetKeyDir() + "/" +
                        key_id->ConvertToQByteArray().toHex().left(16) + ".key";

  if (QFileInfo(key_path).exists()) return *key;

  auto e_key = GFBufferFactory::Encrypt(pin, *key);
  if (!e_key) {
    LOG_E() << "encrypt time-rotated key failed, won't write it to disk";
    return *key;
  }

  if (!GFBufferFactory::ToFile(key_path, *e_key)) {
    LOG_E() << "write time-rotated key failed:" << key_path;
  }

  return *key;
}

auto AppSecureKeyManager::Initialize(const GFBuffer& pin, const GFBuffer& wrap)
    -> AppSecureKeyInitResult {
  const auto secure_level = qApp->property("GFSecureLevel").toInt();

  auto result = init_legacy_key(pin, wrap);
  if (!result.Ok()) return result;

  // Below high security mode there is only ever the legacy key.
  if (secure_level < 3) return result;

  QMap<GFBuffer, GFBuffer> keys;

  auto t_key = fetch_time_related_key(pin);
  if (t_key.Empty()) {
    return {AppSecureKeyStatus::kGenerateFailed, GetKeyDir()};
  }
  keys.insert(CalculateKeyId(pin, t_key), t_key);

  QDir dir(GetKeyDir());
  for (const auto& key_file : dir.entryList({"*.key"}, QDir::Files)) {
    const auto key_path = dir.absoluteFilePath(key_file);
    auto key = GFBufferFactory::FromFile(key_path);
    if (!key) {
      LOG_E() << "read app secure key failed:" << key_path;
      return {AppSecureKeyStatus::kReadFailed, key_path};
    }
    keys.insert(CalculateKeyId(pin, *key), *key);
  }

  // Trial-decrypt every candidate: keys that belong to another PIN simply fail
  // and are skipped, which is how rotated keys from earlier weeks survive.
  for (auto it = keys.constBegin(); it != keys.constEnd(); ++it) {
    auto r_key = GFBufferFactory::Decrypt(pin, it.value());
    if (!r_key) continue;

    keys_.insert(CalculateKeyId(pin, *r_key), *r_key);
  }

  Q_ASSERT(!active_key_id_.Empty());
  return result;
}

}  // namespace GpgFrontend
