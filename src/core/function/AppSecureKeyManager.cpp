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
#include "core/function/SecureRandomGenerator.h"
#include "core/function/SystemSecretStore.h"

namespace {

/// Label standing in for the HMAC key when no identity PIN is set. Changing it
/// would change every key ID, orphaning all stored data objects.
constexpr auto kEmptyPinLabel = "GpgFrontend";

/// Length of the legacy key, in bytes.
constexpr int kLegacyKeyLen = 256;

/// Length of the secret that protects the key file at rest, in bytes.
constexpr size_t kWrapSecretLen = 32;

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

auto AppSecureKeyManager::ResolveWrapSecret(const QString& key_path,
                                            SystemSecretStore* store,
                                            bool intent_enabled)
    -> AppKeyWrapResult {
  const auto backend = store != nullptr ? store->Name() : QString("none");

  // The file describes its own state: an encrypted container carries a magic
  // prefix, a raw key never does.
  GFBuffer on_disk;
  const bool key_exists = QFileInfo(key_path).exists();
  if (key_exists) {
    auto bytes = GFBufferFactory::FromFile(key_path);
    if (!bytes) {
      LOG_E() << "read app secure key failed:" << key_path;
      return {AppKeyWrapStatus::kIoFailed, {}, key_path};
    }
    on_disk = *bytes;
  }

  const bool wrapped =
      key_exists && AESCryptoHelper::IsEncryptedBuffer(on_disk);

  if (!wrapped && !intent_enabled) return {AppKeyWrapStatus::kNotWrapped};

  if (wrapped) {
    if (store == nullptr) {
      return {AppKeyWrapStatus::kLockedOut, {}, backend};
    }

    auto secret = store->Read(kAppKeyWrapAccount);
    if (!secret) {
      LOG_W() << "app secure key is protected but its secret is unavailable";
      return {AppKeyWrapStatus::kLockedOut, {}, backend};
    }

    auto plain = UnsealKey({}, *secret, on_disk);
    if (!plain) {
      LOG_W() << "app secure key did not decrypt with the stored secret";
      return {AppKeyWrapStatus::kLockedOut, {}, backend};
    }

    if (intent_enabled) return {AppKeyWrapStatus::kWrapped, *secret, backend};

    // Turning protection off: put the plaintext back first, and only drop the
    // store entry once the file no longer depends on it. A crash in between
    // leaves an unused entry, which is harmless; the reverse order would lose
    // the key.
    if (!GFBufferFactory::ToFileAtomic(key_path, *plain)) {
      return {AppKeyWrapStatus::kIoFailed, {}, key_path};
    }

    store->Remove(kAppKeyWrapAccount);
    LOG_I() << "app secure key protection disabled, backend:" << backend;
    return {AppKeyWrapStatus::kJustDisabled, {}, backend};
  }

  // Not wrapped, but protection was requested.
  if (store == nullptr || !store->IsAvailable()) {
    LOG_W() << "app secure key protection requested but unavailable, backend:"
            << backend;
    return {AppKeyWrapStatus::kStoreUnavailable, {}, backend};
  }

  auto secret = SecureRandomGenerator::Generate(kWrapSecretLen);
  if (!secret) {
    LOG_E() << "cannot generate a wrap secret: no random source";
    return {AppKeyWrapStatus::kStoreUnavailable, {}, backend};
  }

  if (!store->Write(kAppKeyWrapAccount, *secret)) {
    LOG_W() << "writing the wrap secret failed, backend:" << backend;
    return {AppKeyWrapStatus::kStoreUnavailable, {}, backend};
  }

  // Read it back before anything depends on it. A locked keyring or a missing
  // entitlement can accept a write and still not return it, and finding that
  // out now is the difference between a no-op and an unopenable key file.
  auto verify = store->Read(kAppKeyWrapAccount);
  if (!verify || *verify != *secret) {
    LOG_W() << "wrap secret did not read back intact, backend:" << backend;
    store->Remove(kAppKeyWrapAccount);
    return {AppKeyWrapStatus::kStoreUnavailable, {}, backend};
  }

  // No key file yet: nothing to convert, the caller will create it encrypted.
  if (!key_exists) {
    LOG_I() << "app secure key protection enabled, backend:" << backend;
    return {AppKeyWrapStatus::kJustEnabled, *secret, backend};
  }

  auto encrypted = SealKey({}, *secret, on_disk);
  if (!encrypted) {
    LOG_E() << "encrypting the app secure key failed";
    store->Remove(kAppKeyWrapAccount);
    return {AppKeyWrapStatus::kIoFailed, {}, key_path};
  }

  // Prove the ciphertext round-trips before it replaces the only copy of the
  // key. Checking in memory is equivalent to re-reading the file and keeps the
  // replacement itself a single atomic step.
  auto round_trip = UnsealKey({}, *secret, *encrypted);
  if (!round_trip || *round_trip != on_disk) {
    LOG_E() << "app secure key did not survive a wrap round trip";
    store->Remove(kAppKeyWrapAccount);
    return {AppKeyWrapStatus::kIoFailed, {}, key_path};
  }

  if (!GFBufferFactory::ToFileAtomic(key_path, *encrypted)) {
    store->Remove(kAppKeyWrapAccount);
    return {AppKeyWrapStatus::kIoFailed, {}, key_path};
  }

  LOG_I() << "app secure key protection enabled, backend:" << backend;
  return {AppKeyWrapStatus::kJustEnabled, *secret, backend};
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

auto AppSecureKeyManager::SealKey(const GFBuffer& pin, const GFBuffer& wrap,
                                  const GFBuffer& plain) -> GFBufferOrNone {
  if (!wrap.Empty()) return GFBufferFactory::EncryptLite(wrap, plain);
  if (!pin.Empty()) return GFBufferFactory::Encrypt(pin, plain);
  return plain;
}

auto AppSecureKeyManager::UnsealKey(const GFBuffer& pin, const GFBuffer& wrap,
                                    const GFBuffer& stored) -> GFBufferOrNone {
  if (!wrap.Empty()) return GFBufferFactory::DecryptLite(wrap, stored);
  if (!pin.Empty()) return GFBufferFactory::Decrypt(pin, stored);
  return stored;
}

auto AppSecureKeyManager::new_legacy_key(const GFBuffer& pin,
                                         const GFBuffer& wrap,
                                         AppSecureKeyInitResult& status)
    -> GFBuffer {
  auto key = PassphraseGenerator::GenerateBytesByOpenSSL(kLegacyKeyLen);
  if (!key) {
    LOG_E() << "generate app secure key failed, using qt random generator...";
    key = GFBuffer(QRandomGenerator64::securelySeeded().generate());
  }

  auto plain_key = *key;

  auto sealed = SealKey(pin, wrap, plain_key);
  if (!sealed) {
    LOG_E() << "encrypt app secure key failed, won't write it to disk";
    status = {AppSecureKeyStatus::kWriteFailed,
              QObject::tr("The secure key could not be encrypted, so it was "
                          "not saved to disk.")};
    return plain_key;
  }

  const auto path = GetLegacyKeyPath();
  if (!GFBufferFactory::ToFile(path, *sealed)) {
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
    legacy_key = new_legacy_key(pin, wrap, result);
    if (legacy_key.Empty()) {
      return {AppSecureKeyStatus::kGenerateFailed, path};
    }
  } else {
    auto key = GFBufferFactory::FromFile(path);
    if (!key) {
      LOG_E() << "read app secure key failed:" << path;
      return {AppSecureKeyStatus::kReadFailed, path};
    }

    auto r_key = UnsealKey(pin, wrap, *key);
    if (!r_key) {
      LOG_W() << "decrypt legacy app secure key failed";
      return {AppSecureKeyStatus::kDecryptFailed, path};
    }
    legacy_key = *r_key;
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
