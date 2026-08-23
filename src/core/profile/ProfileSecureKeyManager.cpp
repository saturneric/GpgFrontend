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

#include "core/profile/ProfileSecureKeyManager.h"

#include "core/profile/ProfileAreaTraits.h"

#include <sodium.h>

#include "core/function/AESCryptoHelper.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/PassphraseGenerator.h"
#include "core/function/SecureRandomGenerator.h"
#include "core/function/SystemSecretStore.h"
#include "core/profile/ProfileMarker.h"

namespace {

/// Label standing in for the HMAC key when no identity PIN is set. Changing it
/// would change every key ID, orphaning all stored data objects.
constexpr auto kEmptyPinLabel = "GpgFrontend";

/// Length of the profile's own key, in bytes.
constexpr int kRootKeyLen = 256;

/// The file the profile's own key is stored in.
/// Shared with ProfileMember, from the area table: see kProfileRootKeyName.
constexpr auto kRootKeyName = GpgFrontend::kProfileRootKeyName;

/// Length of the secret that protects the key file at rest, in bytes.
constexpr size_t kWrapSecretLen = 32;

/// Salt prefix for the weekly rotating key.
constexpr auto kRotatingKeySaltPrefix = "GF_ROT_KEY";

/// Seconds in the rotation period of the time-related key.
constexpr qint64 kRotationPeriodSecs = 60LL * 60 * 24 * 7;

/// Canonical spellings of the protection modes, as they appear in the marker
/// and the settings store. Kept beside the parser so the two cannot drift
/// apart.
constexpr auto kProtectionNone = "none";
constexpr auto kProtectionKeychain = "keychain";
constexpr auto kProtectionPin = "pin";

/// The secure level at which a pre-split profile sealed its key file with a
/// PIN.
constexpr int kLegacyPinSecureLevel = 3;

}  // namespace

namespace GpgFrontend {

auto DeriveAppKeyWrapAccount(ProfileKind kind, const QString& profile_id,
                             const QString& canonical_root,
                             const QString& profile_uuid) -> QString {
  // The one account that must never move: every existing installation already
  // has this entry, and renaming it locks those users out of their own data.
  if (kind == ProfileKind::kINSTALLED_ROOT) {
    return QString::fromLatin1(kAppKeyWrapAccount);
  }

  const auto material = (canonical_root + '\0' + profile_uuid).toUtf8();

  std::array<unsigned char, 8> digest{};
  if (crypto_generichash(
          digest.data(), digest.size(),
          reinterpret_cast<const unsigned char*>(material.constData()),
          material.size(), nullptr, 0) != 0) {
    // Falling back to the shared account would silently reintroduce exactly the
    // collision this function exists to prevent, so refuse instead: the caller
    // treats an empty account as "the credential store is unusable here".
    LOG_E() << "cannot derive the credential account for profile" << profile_id;
    return {};
  }

  return QString("%1.%2.%3")
      .arg(QString::fromLatin1(kAppKeyWrapAccount), profile_id,
           QString::fromLatin1(
               QByteArray(reinterpret_cast<const char*>(digest.data()),
                          digest.size())
                   .toHex()));
}

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

ProfileSecureKeyManager::ProfileSecureKeyManager(
    QSharedPointer<ProfileAccessor> accessor)
    : accessor_(std::move(accessor)) {}

auto ProfileSecureKeyManager::CalculateKeyId(const GFBuffer& pin,
                                             const GFBuffer& key) -> GFBuffer {
  auto id = GFBufferFactory::ToHMACSha256(
      pin.Empty() ? GFBuffer(kEmptyPinLabel) : pin, key);
  Q_ASSERT(id.has_value());

  return id.value_or(GFBuffer{});
}

auto ProfileSecureKeyManager::RegisterKeyIds(QMap<GFBuffer, GFBuffer>& keys,
                                             const GFBuffer& pin,
                                             const GFBuffer& key) -> GFBuffer {
  const auto stable_id = CalculateKeyId({}, key);
  keys.insert(stable_id, key);

  // Only a pre-split profile has objects filed under a PIN-derived ID. Adding
  // the alias unconditionally would be harmless but misleading, so it is added
  // only when there actually is a PIN that could have produced one.
  if (!pin.Empty()) keys.insert(CalculateKeyId(pin, key), key);

  return stable_id;
}

auto ProfileSecureKeyManager::ResolveWrapSecret(const QString& key_path,
                                                SystemSecretStore* store,
                                                bool intent_enabled,
                                                const QString& account)
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

    auto secret = store->Read(account);

    // An empty read does not mean the entry is gone. A keyring that has not
    // been opened since boot answers a lookup with nothing rather than with a
    // locked item, so the platform never gets as far as raising its unlock
    // prompt -- the user is told their key is unrecoverable while the secret
    // sits there, intact, behind a password nobody asked for. Ask for the
    // unlock outright, then look once more.
    //
    // Only once, so a store that unlocks but genuinely holds no entry cannot
    // loop. Only when the unlock succeeded, too: Read() overwrites the store's
    // recorded reason on every call and a missing entry records nothing, so a
    // second read after a declined prompt would replace the explanation with
    // silence, which is the state this is meant to end.
    if (!secret && store->Unlock()) secret = store->Read(account);

    if (!secret) {
      LOG_W() << "app secure key is protected but its secret is unavailable, "
                 "backend:"
              << backend << "reason:" << store->LastError();
      return {AppKeyWrapStatus::kLOCKED_OUT, {}, backend};
    }

    // Not retried through Unlock(): a secret came back, so the store is open
    // and unlocking again would prompt for nothing. This is a mismatch between
    // the key file and the secret, not a locked keyring.
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

    store->Remove(account);
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

  if (!store->Write(account, *secret)) {
    LOG_W() << "writing the wrap secret failed, backend:" << backend;
    return {AppKeyWrapStatus::kSTORE_UNAVAILABLE, {}, backend};
  }

  // Read it back before anything depends on it. A locked keyring or a missing
  // entitlement can accept a write and still not return it, and finding that
  // out now is the difference between a no-op and an unopenable key file.
  auto verify = store->Read(account);
  if (!verify || *verify != *secret) {
    LOG_W() << "wrap secret did not read back intact, backend:" << backend;
    store->Remove(account);
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
    store->Remove(account);
    return {AppKeyWrapStatus::kIO_FAILED, {}, key_path};
  }

  // Prove the ciphertext round-trips before it replaces the only copy of the
  // key. Checking in memory is equivalent to re-reading the file and keeps the
  // replacement itself a single atomic step.
  auto round_trip = UnsealKey({}, *secret, *encrypted);
  if (!round_trip || *round_trip != on_disk) {
    LOG_E() << "app secure key did not survive a wrap round trip";
    store->Remove(account);
    return {AppKeyWrapStatus::kIO_FAILED, {}, key_path};
  }

  if (!GFBufferFactory::ToFileAtomic(key_path, *encrypted)) {
    store->Remove(account);
    return {AppKeyWrapStatus::kIO_FAILED, {}, key_path};
  }

  LOG_I() << "app secure key protection enabled, backend:" << backend;
  return {AppKeyWrapStatus::kJUST_ENABLED, *secret, backend};
}

auto AppKeySinkForFile(const QString& path) -> AppKeySink {
  return {[path](const GFBuffer& sealed) {
            return GFBufferFactory::ToFileAtomic(path, sealed);
          },
          path};
}

auto ProfileSecureKeyManager::KeySink() const -> AppKeySink {
  // The accessor is captured by value: it is shared-owned, and a sink outliving
  // the manager it came from would otherwise write through a dangling one.
  auto accessor = accessor_;
  return {[accessor](const GFBuffer& sealed) {
            return accessor->Write(ProfileArea::kSecure, kRootKeyName, sealed);
          },
          KeyLocationForMessage()};
}

auto ProfileSecureKeyManager::ReadStoredKey() const -> GFBufferOrNone {
  return accessor_->Read(ProfileArea::kSecure, kRootKeyName);
}

auto ProfileSecureKeyManager::ChangeProtection(
    const AppKeySink& sink, SystemSecretStore* store, const GFBuffer& plain_key,
    AppKeyProtection from, AppKeyProtection to, const GFBuffer& new_pin,
    const QString& account) -> AppKeyProtectionResult {
  const auto backend = store != nullptr ? store->Name() : QString("none");
  const auto& key_path = sink.location;

  if (!sink.write) {
    LOG_E() << "refusing to re-protect an app secure key with nowhere to put it";
    return {AppKeyProtectionStatus::kIO_FAILED, key_path};
  }

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
      store->Remove(account);
    }
  };

  GFBuffer secret;
  if (to == AppKeyProtection::kKEYCHAIN) {
    auto generated = SecureRandomGenerator::Generate(kWrapSecretLen);
    if (!generated) {
      LOG_E() << "cannot generate a wrap secret: no random source";
      return {AppKeyProtectionStatus::kSTORE_UNAVAILABLE, backend};
    }

    if (!store->Write(account, *generated)) {
      LOG_W() << "writing the wrap secret failed, backend:" << backend;
      return {AppKeyProtectionStatus::kSTORE_UNAVAILABLE, backend};
    }

    // Read it back before anything depends on it. A locked keyring or a missing
    // entitlement can accept a write and still not return it, and finding that
    // out now is the difference between a no-op and an unopenable key file.
    auto verify = store->Read(account);
    if (!verify || *verify != *generated) {
      LOG_W() << "wrap secret did not read back intact, backend:" << backend;
      store->Remove(account);
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

  if (!sink.write(*sealed)) {
    LOG_E() << "rewriting the app secure key failed:" << key_path;
    rollback();
    return {AppKeyProtectionStatus::kIO_FAILED, key_path};
  }

  // Only now is the old secret safe to drop: until the file was replaced it was
  // the only thing that could open it.
  if (from == AppKeyProtection::kKEYCHAIN &&
      to != AppKeyProtection::kKEYCHAIN && store != nullptr) {
    store->Remove(account);
  }

  LOG_I() << "app secure key protection changed to"
          << AppKeyProtectionToString(to) << "backend:" << backend;
  return {AppKeyProtectionStatus::kOK, to == AppKeyProtection::kKEYCHAIN
                                           ? backend
                                           : AppKeyProtectionToString(to)};
}

auto ProfileSecureKeyManager::KeyPath() const -> QString {
  return accessor_->PathOf(ProfileArea::kSecure, kRootKeyName);
}

auto ProfileSecureKeyManager::KeyLocationForMessage() const -> QString {
  // Every internal use of KeyPath() is a log line or a failure detail shown to
  // the user, and an area held in memory has no path to put there. Naming the
  // storage is the honest answer and is more use than an empty string.
  const auto path = KeyPath();
  return path.isEmpty() ? accessor_->Label() : path;
}

auto ProfileSecureKeyManager::ResetKeyStorage(const QString& key_dir) -> bool {
  const auto path = key_dir + "/" + QString::fromLatin1(kRootKeyName);

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
    if (name == QLatin1StringView(kRootKeyName)) continue;  // already handled above
    if (!dir.remove(name)) {
      LOG_W() << "remove rotated key failed:" << dir.filePath(name);
    }
  }

  LOG_I() << "app secure key storage reset";
  return true;
}

auto ProfileSecureKeyManager::KeyById(const GFBuffer& id) const -> GFBuffer {
  return keys_.value(id, GFBuffer{});
}

auto ProfileSecureKeyManager::ActiveKeyId() const -> GFBuffer {
  return active_key_id_;
}

auto ProfileSecureKeyManager::ActiveKey() const -> GFBuffer {
  auto key = KeyById(active_key_id_);
  Q_ASSERT(!key.Empty());
  return key;
}

auto ProfileSecureKeyManager::RootKey() const -> GFBuffer {
  auto key = KeyById(root_key_id_);
  Q_ASSERT(!key.Empty());
  return key;
}

auto ProfileSecureKeyManager::SealKey(const GFBuffer& pin, const GFBuffer& wrap,
                                      const GFBuffer& plain) -> GFBufferOrNone {
  if (!wrap.Empty()) return GFBufferFactory::EncryptLite(wrap, plain);
  if (!pin.Empty()) return GFBufferFactory::Encrypt(pin, plain);
  return plain;
}

auto ProfileSecureKeyManager::UnsealKey(const GFBuffer& pin,
                                        const GFBuffer& wrap,
                                        const GFBuffer& stored)
    -> GFBufferOrNone {
  if (!wrap.Empty()) return GFBufferFactory::DecryptLite(wrap, stored);
  if (!pin.Empty()) return GFBufferFactory::Decrypt(pin, stored);
  return stored;
}

auto ProfileSecureKeyManager::new_root_key(const GFBuffer& pin,
                                           const GFBuffer& wrap,
                                           ProfileKeyLoadResult& status)
    -> GFBuffer {
  auto key = PassphraseGenerator::GenerateBytesByOpenSSL(kRootKeyLen);
  if (!key) {
    LOG_E() << "generate app secure key failed, using qt random generator...";
    key = GFBuffer(QRandomGenerator64::securelySeeded().generate());
  }

  auto plain_key = *key;

  auto sealed = SealKey(pin, wrap, plain_key);
  if (!sealed) {
    LOG_E() << "encrypt app secure key failed, won't write it to disk";
    status = {ProfileKeyLoadStatus::kWRITE_FAILED,
              QObject::tr("The secure key could not be encrypted, so it was "
                          "not saved to disk.")};
    return plain_key;
  }

  if (!accessor_->Write(ProfileArea::kSecure, kRootKeyName, *sealed)) {
    const auto path = KeyLocationForMessage();
    LOG_E() << "write app secure key failed:" << path;
    status = {ProfileKeyLoadStatus::kWRITE_FAILED, path};
  }

  return plain_key;
}

auto ProfileSecureKeyManager::init_root_key(const GFBuffer& pin,
                                            const GFBuffer& wrap)
    -> ProfileKeyLoadResult {
  ProfileKeyLoadResult result;

  GFBuffer root_key;
  const auto path = KeyLocationForMessage();
  LOG_D() << "profile secure key kept in:" << path;

  if (!accessor_->Exists(ProfileArea::kSecure, kRootKeyName)) {
    root_key = new_root_key(pin, wrap, result);
    if (root_key.Empty()) {
      return {ProfileKeyLoadStatus::kGENERATE_FAILED, path};
    }
  } else {
    auto key = accessor_->Read(ProfileArea::kSecure, kRootKeyName);
    if (!key) {
      LOG_E() << "read app secure key failed:" << path;
      return {ProfileKeyLoadStatus::kREAD_FAILED, path};
    }

    auto r_key = UnsealKey(pin, wrap, *key);
    if (!r_key) {
      LOG_W() << "decrypt profile secure key failed";
      return {ProfileKeyLoadStatus::kDECRYPT_FAILED, path};
    }
    root_key = *r_key;
  }

  // The identity comes from the key material alone, never from whatever
  // protects it at rest: the ID is stored as a prefix on every data object, so
  // deriving it from the protection would orphan every object each time that
  // protection changed. A profile written while the PIN still fed the identity
  // keeps its old ID registered too, so its objects stay readable.
  const auto root_key_id = RegisterKeyIds(keys_, pin, root_key);
  Q_ASSERT(!root_key_id.Empty());

  active_key_id_ = root_key_id;
  root_key_id_ = root_key_id;

  return result;
}

auto ProfileSecureKeyManager::DeriveRotatedKey(const GFBuffer& app_key,
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

auto ProfileSecureKeyManager::fetch_time_related_key(const GFBuffer& app_key)
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

  const auto key_name = key_id.ConvertToQByteArray().toHex().left(16) + ".key";

  if (accessor_->Exists(ProfileArea::kSecure, key_name)) return key;

  auto e_key = GFBufferFactory::EncryptLite(app_key, key);
  if (!e_key) {
    LOG_E() << "encrypt time-rotated key failed, won't write it to disk";
    return key;
  }

  if (!accessor_->Write(ProfileArea::kSecure, key_name, *e_key)) {
    LOG_E() << "write time-rotated key failed:" << key_name;
  }

  return key;
}

auto ProfileSecureKeyManager::Load(const GFBuffer& pin, const GFBuffer& wrap,
                                   bool rotating) -> ProfileKeyLoadResult {
  auto result = init_root_key(pin, wrap);
  if (!result.Ok()) return result;

  // The classical case: one key encrypts everything and opens everything.
  if (!rotating) {
    mode_ = ProfileKeyMode::kSINGLE;
    return result;
  }

  // Rotation hangs off the profile's own key, so it is available only once the
  // root key above has been loaded.
  const auto app_key = RootKey();

  auto t_key = fetch_time_related_key(app_key);
  if (t_key.Empty()) {
    return {ProfileKeyLoadStatus::kGENERATE_FAILED, KeyLocationForMessage()};
  }
  RegisterKeyIds(keys_, pin, t_key);

  // Everything earlier periods wrote is still ours to read, so every rotated
  // key that opens is listed beside the active one. A key belonging to neither
  // form simply fails to decrypt and is skipped, which is how rotated keys from
  // earlier weeks have always survived.
  // The name itself, not a name derived from a path. A driver that holds this
  // area in memory has no path to take a filename from, and the empty string
  // that produced would stop this guard matching anything -- feeding the root
  // key into the trial-decrypt loop below, where for a PIN-protected package it
  // would succeed and be registered as a rotated key.
  const auto root_key_file = QString::fromLatin1(kRootKeyName);

  for (const auto& key_file : accessor_->List(ProfileArea::kSecure, "*.key")) {
    // The root key is not a rotated key and was already registered above; it is
    // also the one file here that may be sealed by the credential store.
    if (key_file == root_key_file) continue;

    auto stored = accessor_->Read(ProfileArea::kSecure, key_file);
    if (!stored) {
      LOG_E() << "read app secure key failed:" << key_file;
      return {ProfileKeyLoadStatus::kREAD_FAILED, KeyLocationForMessage()};
    }

    // Trial-decrypt: first the way rotated keys are written now, then the way a
    // profile written before rotation was re-based on the app key wrote them.
    auto r_key = GFBufferFactory::DecryptLite(app_key, *stored);
    if (!r_key && !pin.Empty()) r_key = GFBufferFactory::Decrypt(pin, *stored);
    if (!r_key) continue;

    RegisterKeyIds(keys_, pin, *r_key);
  }

  Q_ASSERT(!active_key_id_.Empty());
  mode_ = ProfileKeyMode::kROTATING;
  return result;
}

}  // namespace GpgFrontend
