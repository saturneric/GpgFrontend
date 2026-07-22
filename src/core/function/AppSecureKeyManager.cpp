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

/// Canonical spellings of the protection modes, as they appear in ENV.ini and
/// the settings store. Kept beside the parser so the two cannot drift apart.
constexpr auto kProtectionNone = "none";
constexpr auto kProtectionKeychain = "keychain";
constexpr auto kProtectionPin = "pin";

/// The secure level at which a pre-split profile sealed its key file with a
/// PIN.
constexpr int kLegacyPinSecureLevel = 3;

}  // namespace

namespace GpgFrontend {

auto AppKeyProtectionFromString(const QString& s) -> AppKeyProtection {
  const auto token = s.trimmed().toLower();
  if (token == QLatin1String(kProtectionKeychain)) {
    return AppKeyProtection::kKEYCHAIN;
  }
  if (token == QLatin1String(kProtectionPin)) return AppKeyProtection::kPIN;
  return AppKeyProtection::kNONE;
}

auto AppKeyProtectionToString(AppKeyProtection p) -> QString {
  switch (p) {
    case AppKeyProtection::kKEYCHAIN:
      return QLatin1String(kProtectionKeychain);
    case AppKeyProtection::kPIN:
      return QLatin1String(kProtectionPin);
    case AppKeyProtection::kNONE:
      break;
  }
  return QLatin1String(kProtectionNone);
}

auto AppKeyProtectionFromApp() -> AppKeyProtection {
  if (qApp == nullptr) return AppKeyProtection::kNONE;
  return AppKeyProtectionFromString(
      qApp->property("GFAppKeyProtection").toString());
}

auto ApplyPortableModeRule(AppKeyProtection resolved, bool portable)
    -> AppKeyProtection {
  if (portable && resolved == AppKeyProtection::kKEYCHAIN) {
    return AppKeyProtection::kNONE;
  }
  return resolved;
}

auto ResolveAppKeyProtection(const QVariant& env_protection,
                             const QVariant& env_secure_level,
                             const QVariant& env_os_secret_store,
                             const QVariant& user_protection,
                             const QVariant& user_secure_level,
                             const QVariant& user_os_secret_store)
    -> AppKeyProtection {
  // Each layer is tried in full — its own key, then the two keys it replaced —
  // before falling through, so an ENV.ini that says anything at all about the
  // protection always beats a user setting that says something else.
  const auto layer =
      [](const QVariant& protection, const QVariant& secure_level,
         const QVariant& os_secret_store, AppKeyProtection& out) -> bool {
    if (protection.isValid()) {
      out = AppKeyProtectionFromString(protection.toString());
      return true;
    }
    // A pre-split profile at this level has a PIN-sealed key file, so it has to
    // keep resolving to kPIN or it would fail to open on the next start. Lower
    // levels said nothing about protection and must fall through.
    if (secure_level.isValid() &&
        secure_level.toInt() >= kLegacyPinSecureLevel) {
      out = AppKeyProtection::kPIN;
      return true;
    }
    // An explicit false is an answer, not an absence: it must stop the ladder
    // rather than let a lower layer turn protection back on.
    if (os_secret_store.isValid()) {
      out = os_secret_store.toBool() ? AppKeyProtection::kKEYCHAIN
                                     : AppKeyProtection::kNONE;
      return true;
    }
    return false;
  };

  auto result = AppKeyProtection::kNONE;
  if (layer(env_protection, env_secure_level, env_os_secret_store, result)) {
    return result;
  }
  if (layer(user_protection, user_secure_level, user_os_secret_store, result)) {
    return result;
  }
  return AppKeyProtection::kNONE;
}

AppSecureKeyManager::AppSecureKeyManager(int channel)
    : SingletonFunctionObject<AppSecureKeyManager>(channel) {}

auto AppSecureKeyManager::CalculateKeyId(const GFBuffer& pin,
                                         const GFBuffer& key) -> GFBuffer {
  auto id = GFBufferFactory::ToHMACSha256(
      pin.Empty() ? GFBuffer(kEmptyPinLabel) : pin, key);
  Q_ASSERT(id.has_value());

  return id.value_or(GFBuffer{});
}

auto AppSecureKeyManager::RegisterLegacyKeyIds(QMap<GFBuffer, GFBuffer>& keys,
                                               const GFBuffer& pin,
                                               const GFBuffer& key)
    -> GFBuffer {
  const auto stable_id = CalculateKeyId({}, key);
  keys.insert(stable_id, key);

  // Only a pre-split profile has objects filed under a PIN-derived ID. Adding
  // the alias unconditionally would be harmless but misleading, so it is added
  // only when there actually is a PIN that could have produced one.
  if (!pin.Empty()) keys.insert(CalculateKeyId(pin, key), key);

  return stable_id;
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
      return {AppKeyWrapStatus::kIO_FAILED, {}, key_path};
    }
    on_disk = *bytes;
  }

  const bool wrapped =
      key_exists && AESCryptoHelper::IsEncryptedBuffer(on_disk);

  if (!wrapped && !intent_enabled) return {AppKeyWrapStatus::kNOT_WRAPPED};

  if (wrapped) {
    if (store == nullptr) {
      return {AppKeyWrapStatus::kLOCKED_OUT, {}, backend};
    }

    auto secret = store->Read(kAppKeyWrapAccount);
    if (!secret) {
      LOG_W() << "app secure key is protected but its secret is unavailable";
      return {AppKeyWrapStatus::kLOCKED_OUT, {}, backend};
    }

    auto plain = UnsealKey({}, *secret, on_disk);
    if (!plain) {
      LOG_W() << "app secure key did not decrypt with the stored secret";
      return {AppKeyWrapStatus::kLOCKED_OUT, {}, backend};
    }

    if (intent_enabled) return {AppKeyWrapStatus::kWRAPPED, *secret, backend};

    // Turning protection off: put the plaintext back first, and only drop the
    // store entry once the file no longer depends on it. A crash in between
    // leaves an unused entry, which is harmless; the reverse order would lose
    // the key.
    if (!GFBufferFactory::ToFileAtomic(key_path, *plain)) {
      return {AppKeyWrapStatus::kIO_FAILED, {}, key_path};
    }

    store->Remove(kAppKeyWrapAccount);
    LOG_I() << "app secure key protection disabled, backend:" << backend;
    return {AppKeyWrapStatus::kJUST_DISABLED, {}, backend};
  }

  // Not wrapped, but protection was requested.
  if (store == nullptr || !store->IsAvailable()) {
    LOG_W() << "app secure key protection requested but unavailable, backend:"
            << backend;
    return {AppKeyWrapStatus::kSTORE_UNAVAILABLE, {}, backend};
  }

  auto secret = SecureRandomGenerator::Generate(kWrapSecretLen);
  if (!secret) {
    LOG_E() << "cannot generate a wrap secret: no random source";
    return {AppKeyWrapStatus::kSTORE_UNAVAILABLE, {}, backend};
  }

  if (!store->Write(kAppKeyWrapAccount, *secret)) {
    LOG_W() << "writing the wrap secret failed, backend:" << backend;
    return {AppKeyWrapStatus::kSTORE_UNAVAILABLE, {}, backend};
  }

  // Read it back before anything depends on it. A locked keyring or a missing
  // entitlement can accept a write and still not return it, and finding that
  // out now is the difference between a no-op and an unopenable key file.
  auto verify = store->Read(kAppKeyWrapAccount);
  if (!verify || *verify != *secret) {
    LOG_W() << "wrap secret did not read back intact, backend:" << backend;
    store->Remove(kAppKeyWrapAccount);
    return {AppKeyWrapStatus::kSTORE_UNAVAILABLE, {}, backend};
  }

  // No key file yet: nothing to convert, the caller will create it encrypted.
  if (!key_exists) {
    LOG_I() << "app secure key protection enabled, backend:" << backend;
    return {AppKeyWrapStatus::kJUST_ENABLED, *secret, backend};
  }

  auto encrypted = SealKey({}, *secret, on_disk);
  if (!encrypted) {
    LOG_E() << "encrypting the app secure key failed";
    store->Remove(kAppKeyWrapAccount);
    return {AppKeyWrapStatus::kIO_FAILED, {}, key_path};
  }

  // Prove the ciphertext round-trips before it replaces the only copy of the
  // key. Checking in memory is equivalent to re-reading the file and keeps the
  // replacement itself a single atomic step.
  auto round_trip = UnsealKey({}, *secret, *encrypted);
  if (!round_trip || *round_trip != on_disk) {
    LOG_E() << "app secure key did not survive a wrap round trip";
    store->Remove(kAppKeyWrapAccount);
    return {AppKeyWrapStatus::kIO_FAILED, {}, key_path};
  }

  if (!GFBufferFactory::ToFileAtomic(key_path, *encrypted)) {
    store->Remove(kAppKeyWrapAccount);
    return {AppKeyWrapStatus::kIO_FAILED, {}, key_path};
  }

  LOG_I() << "app secure key protection enabled, backend:" << backend;
  return {AppKeyWrapStatus::kJUST_ENABLED, *secret, backend};
}

auto AppSecureKeyManager::ChangeProtection(
    const QString& key_path, SystemSecretStore* store,
    const GFBuffer& plain_key, AppKeyProtection from, AppKeyProtection to,
    const GFBuffer& new_pin) -> AppKeyProtectionResult {
  const auto backend = store != nullptr ? store->Name() : QString("none");

  // Re-sealing under a new PIN is how a PIN is changed, so it is the one
  // same-mode transition that still has work to do.
  if (from == to && to != AppKeyProtection::kPIN) {
    return {AppKeyProtectionStatus::kUNCHANGED};
  }

  if (plain_key.Empty()) {
    LOG_E() << "refusing to re-protect an empty app secure key";
    return {AppKeyProtectionStatus::kSEAL_FAILED, key_path};
  }

  if (to == AppKeyProtection::kPIN && new_pin.Empty()) {
    return {AppKeyProtectionStatus::kBAD_PIN};
  }

  if (to == AppKeyProtection::kKEYCHAIN &&
      (store == nullptr || !store->IsAvailable())) {
    LOG_W() << "app secure key protection requested but unavailable, backend:"
            << backend;
    return {AppKeyProtectionStatus::kSTORE_UNAVAILABLE, backend};
  }

  // Undoes step 2 and nothing else: the file is not touched until step 5, so
  // there is never anything else to unwind.
  const auto rollback = [&]() {
    if (to == AppKeyProtection::kKEYCHAIN && store != nullptr) {
      store->Remove(kAppKeyWrapAccount);
    }
  };

  GFBuffer secret;
  if (to == AppKeyProtection::kKEYCHAIN) {
    auto generated = SecureRandomGenerator::Generate(kWrapSecretLen);
    if (!generated) {
      LOG_E() << "cannot generate a wrap secret: no random source";
      return {AppKeyProtectionStatus::kSTORE_UNAVAILABLE, backend};
    }

    if (!store->Write(kAppKeyWrapAccount, *generated)) {
      LOG_W() << "writing the wrap secret failed, backend:" << backend;
      return {AppKeyProtectionStatus::kSTORE_UNAVAILABLE, backend};
    }

    // Read it back before anything depends on it. A locked keyring or a missing
    // entitlement can accept a write and still not return it, and finding that
    // out now is the difference between a no-op and an unopenable key file.
    auto verify = store->Read(kAppKeyWrapAccount);
    if (!verify || *verify != *generated) {
      LOG_W() << "wrap secret did not read back intact, backend:" << backend;
      store->Remove(kAppKeyWrapAccount);
      return {AppKeyProtectionStatus::kSTORE_UNAVAILABLE, backend};
    }

    secret = *generated;
  }

  // At most one of the two slots is ever set; SealKey() picks its derivation
  // from whichever it is. Held by value rather than by reference, so nothing
  // depends on the lifetime of a temporary.
  const GFBuffer seal_pin = to == AppKeyProtection::kPIN ? new_pin : GFBuffer{};

  auto sealed = SealKey(seal_pin, secret, plain_key);
  if (!sealed) {
    LOG_E() << "sealing the app secure key for its new protection failed";
    rollback();
    return {AppKeyProtectionStatus::kSEAL_FAILED, key_path};
  }

  // Prove the ciphertext round-trips before it replaces the only copy of the
  // key. Checking in memory is equivalent to re-reading the file and keeps the
  // replacement itself a single atomic step.
  auto round_trip = UnsealKey(seal_pin, secret, *sealed);
  if (!round_trip || *round_trip != plain_key) {
    LOG_E() << "app secure key did not survive a re-protection round trip";
    rollback();
    return {AppKeyProtectionStatus::kIO_FAILED, key_path};
  }

  if (!GFBufferFactory::ToFileAtomic(key_path, *sealed)) {
    LOG_E() << "rewriting the app secure key failed:" << key_path;
    rollback();
    return {AppKeyProtectionStatus::kIO_FAILED, key_path};
  }

  // Only now is the old secret safe to drop: until the file was replaced it was
  // the only thing that could open it.
  if (from == AppKeyProtection::kKEYCHAIN &&
      to != AppKeyProtection::kKEYCHAIN && store != nullptr) {
    store->Remove(kAppKeyWrapAccount);
  }

  LOG_I() << "app secure key protection changed to"
          << AppKeyProtectionToString(to) << "backend:" << backend;
  return {AppKeyProtectionStatus::kOK, to == AppKeyProtection::kKEYCHAIN
                                           ? backend
                                           : AppKeyProtectionToString(to)};
}

auto AppSecureKeyManager::GetKeyDir() const -> QString {
  return GetGSS().GetAppDataPath() + "/secure";
}

auto AppSecureKeyManager::GetLegacyKeyPath() const -> QString {
  return GetKeyDir() + "/app.key";
}

auto AppSecureKeyManager::ResetKeyStorage(const QString& key_dir) -> bool {
  const auto path = key_dir + "/app.key";

  // The app key file is the one that must go: without it Initialize() generates
  // a fresh key. Treat "already absent" as success, so a reset stays idempotent
  // if a previous attempt got half way.
  if (QFileInfo::exists(path) && !QFile::remove(path)) {
    LOG_E() << "remove app secure key failed:" << path;
    return false;
  }

  // Sweep the rotated <keyId>.key files derived from the discarded key. They
  // are re-derivable and keyed to the old key, so leaving them behind only
  // litters the secure directory with material the new key will never
  // reference.
  QDir dir(key_dir);
  for (const auto& name : dir.entryList({"*.key"}, QDir::Files)) {
    if (name == "app.key") continue;  // already handled above
    if (!dir.remove(name)) {
      LOG_W() << "remove rotated key failed:" << dir.filePath(name);
    }
  }

  LOG_I() << "app secure key storage reset";
  return true;
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
    status = {AppSecureKeyStatus::kWRITE_FAILED,
              QObject::tr("The secure key could not be encrypted, so it was "
                          "not saved to disk.")};
    return plain_key;
  }

  const auto path = GetLegacyKeyPath();
  if (!GFBufferFactory::ToFile(path, *sealed)) {
    LOG_E() << "write app secure key failed:" << path;
    status = {AppSecureKeyStatus::kWRITE_FAILED, path};
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
      return {AppSecureKeyStatus::kGENERATE_FAILED, path};
    }
  } else {
    auto key = GFBufferFactory::FromFile(path);
    if (!key) {
      LOG_E() << "read app secure key failed:" << path;
      return {AppSecureKeyStatus::kREAD_FAILED, path};
    }

    auto r_key = UnsealKey(pin, wrap, *key);
    if (!r_key) {
      LOG_W() << "decrypt legacy app secure key failed";
      return {AppSecureKeyStatus::kDECRYPT_FAILED, path};
    }
    legacy_key = *r_key;
  }

  // The identity comes from the key material alone, never from whatever
  // protects it at rest: the ID is stored as a prefix on every data object, so
  // deriving it from the protection would orphan every object each time that
  // protection changed. A profile written while the PIN still fed the identity
  // keeps its old ID registered too, so its objects stay readable.
  const auto legacy_key_id = RegisterLegacyKeyIds(keys_, pin, legacy_key);
  Q_ASSERT(!legacy_key_id.Empty());

  active_key_id_ = legacy_key_id;
  legacy_key_id_ = legacy_key_id;

  return result;
}

auto AppSecureKeyManager::DeriveRotatedKey(const GFBuffer& app_key,
                                           qint64 period) -> GFBuffer {
  auto salt = GFBufferFactory::ToSha256(
      GFBuffer(kRotatingKeySaltPrefix + QString::number(period)));
  if (!salt) {
    LOG_E() << "generate rotating key salt failed";
    return {};
  }

  // HMAC-SHA256 over the period's salt, keyed by the application secure key.
  // Deriving from the app key rather than a PIN is what makes rotation
  // independent of the at-rest protection: setting, changing or removing a PIN
  // must never orphan a rotated key. The app key is 256 bytes of entropy, so
  // there is nothing for Argon2id to stretch and the cheap construction is
  // both sufficient and ~100ms/start faster — the same trade SealKey() makes.
  auto key = GFBufferFactory::ToHMACSha256(app_key, *salt);
  if (!key) {
    LOG_E() << "derive time-rotated key failed";
    return {};
  }

  return *key;
}

auto AppSecureKeyManager::fetch_time_related_key(const GFBuffer& app_key)
    -> GFBuffer {
  const qint64 period =
      QDateTime::currentSecsSinceEpoch() / kRotationPeriodSecs;

  auto key = DeriveRotatedKey(app_key, period);
  if (key.Empty()) return {};

  const auto key_id = CalculateKeyId({}, key);
  if (key_id.Empty()) {
    LOG_E() << "compute time-rotated key id failed";
    return {};
  }

  active_key_id_ = key_id;

  const auto key_path = GetKeyDir() + "/" +
                        key_id.ConvertToQByteArray().toHex().left(16) + ".key";

  if (QFileInfo(key_path).exists()) return key;

  auto e_key = GFBufferFactory::EncryptLite(app_key, key);
  if (!e_key) {
    LOG_E() << "encrypt time-rotated key failed, won't write it to disk";
    return key;
  }

  if (!GFBufferFactory::ToFile(key_path, *e_key)) {
    LOG_E() << "write time-rotated key failed:" << key_path;
  }

  return key;
}

auto AppSecureKeyManager::Initialize(const GFBuffer& pin, const GFBuffer& wrap)
    -> AppSecureKeyInitResult {
  const auto secure_level = qApp->property("GFSecureLevel").toInt();

  auto result = init_legacy_key(pin, wrap);
  if (!result.Ok()) return result;

  // Below high security mode there is only ever the legacy key.
  if (secure_level < 3) return result;

  // Rotation hangs off the app secure key, so it is available only once the
  // legacy key above has been loaded.
  const auto app_key = GetLegacyKey();

  auto t_key = fetch_time_related_key(app_key);
  if (t_key.Empty()) {
    return {AppSecureKeyStatus::kGENERATE_FAILED, GetKeyDir()};
  }
  RegisterLegacyKeyIds(keys_, pin, t_key);

  const auto legacy_key_file = QFileInfo(GetLegacyKeyPath()).fileName();

  QDir dir(GetKeyDir());
  for (const auto& key_file : dir.entryList({"*.key"}, QDir::Files)) {
    // The legacy key is not a rotated key and was already registered above; it
    // is also the one file here that may be sealed by the credential store.
    if (key_file == legacy_key_file) continue;

    const auto key_path = dir.absoluteFilePath(key_file);
    auto stored = GFBufferFactory::FromFile(key_path);
    if (!stored) {
      LOG_E() << "read app secure key failed:" << key_path;
      return {AppSecureKeyStatus::kREAD_FAILED, key_path};
    }

    // Trial-decrypt: first the way rotated keys are written now, then the way a
    // profile written before rotation was re-based on the app key wrote them.
    // Keys belonging to neither simply fail and are skipped, which is how
    // rotated keys from earlier weeks have always survived.
    auto r_key = GFBufferFactory::DecryptLite(app_key, *stored);
    if (!r_key && !pin.Empty()) r_key = GFBufferFactory::Decrypt(pin, *stored);
    if (!r_key) continue;

    RegisterLegacyKeyIds(keys_, pin, *r_key);
  }

  Q_ASSERT(!active_key_id_.Empty());
  return result;
}

}  // namespace GpgFrontend
