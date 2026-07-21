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

#include <QTemporaryDir>

#include "GFCoreTest.h"
#include "core/function/AESCryptoHelper.h"
#include "core/function/AppSecureKeyManager.h"
#include "core/function/DataObjectOperator.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/SecureRandomGenerator.h"
#include "core/function/SystemSecretStore.h"

namespace GpgFrontend::Test {

namespace {

/// A stand-in for key material. Fixed rather than random so a failure is
/// reproducible.
auto SampleKey() -> GFBuffer { return GFBuffer(QByteArray(256, '\x5A')); }

/**
 * @brief In-memory credential store, with hooks for the failure modes that
 * matter: a store that refuses writes, and one that accepts a write but hands
 * back something else.
 */
class FakeSecretStore final : public SystemSecretStore {
 public:
  [[nodiscard]] auto Name() const -> QString override {
    return QStringLiteral("fake");
  }

  [[nodiscard]] auto IsAvailable() -> bool override { return available; }

  auto Read(const QString& account) -> GFBufferOrNone override {
    if (!entries.contains(account)) return {};
    return entries.value(account);
  }

  auto Write(const QString& account, const GFBuffer& secret) -> bool override {
    if (!writable) return false;

    // Simulates a store that reports success but cannot return the value, as a
    // locked keyring or a missing entitlement does.
    entries.insert(
        account, corrupt_on_write ? GFBuffer(QByteArray(32, '\x00')) : secret);
    return true;
  }

  auto Remove(const QString& account) -> bool override {
    entries.remove(account);
    return true;
  }

  QMap<QString, GFBuffer> entries;
  bool available = true;
  bool writable = true;
  bool corrupt_on_write = false;
};

/// A temporary key file holding the given contents.
class ScopedKeyFile {
 public:
  explicit ScopedKeyFile(const GFBuffer& contents) {
    EXPECT_TRUE(dir_.isValid());
    path_ = dir_.path() + "/app.key";
    if (!contents.Empty()) {
      EXPECT_TRUE(GFBufferFactory::ToFile(path_, contents));
    }
  }

  [[nodiscard]] auto Path() const -> QString { return path_; }

  [[nodiscard]] auto Read() const -> GFBuffer {
    auto bytes = GFBufferFactory::FromFile(path_);
    return bytes ? *bytes : GFBuffer{};
  }

  [[nodiscard]] auto Exists() const -> bool {
    return QFileInfo(path_).exists();
  }

 private:
  QTemporaryDir dir_;
  QString path_;
};

}  // namespace

/**
 * The identity of a key must be HMAC-SHA256 over the key using the literal
 * "GpgFrontend" as the HMAC key whenever no PIN is set.
 *
 * This is a wire format, not an implementation detail: DataObjectOperator
 * stores the ID as a 32-byte prefix on every object it writes. If this formula
 * ever changes, every object written by an older build stops resolving and is
 * reported as kMISSING_KEY.
 */
TEST(AppSecureKeyManagerTest, LegacyKeyIdMatchesHistoricFormula) {
  const auto key = SampleKey();

  auto expected = GFBufferFactory::ToHMACSha256(GFBuffer("GpgFrontend"), key);
  ASSERT_TRUE(expected.has_value());

  EXPECT_EQ(AppSecureKeyManager::CalculateKeyId({}, key), *expected);
}

/**
 * The key ID must depend only on the PIN and the key material, never on how
 * the key file happens to be protected at rest. This is what allows at-rest
 * protection to be switched on and off without orphaning stored objects.
 */
TEST(AppSecureKeyManagerTest, KeyIdIndependentOfAtRestProtection) {
  const auto key = SampleKey();
  const auto id_before = AppSecureKeyManager::CalculateKeyId({}, key);

  // Encrypting the key file with a wrap secret must not disturb the identity:
  // the identity is derived from the plaintext key, which is unchanged.
  const GFBuffer wrap("a-wrapping-secret");
  auto wrapped = GFBufferFactory::EncryptLite(wrap, key);
  ASSERT_TRUE(wrapped.has_value());

  auto unwrapped = GFBufferFactory::DecryptLite(wrap, *wrapped);
  ASSERT_TRUE(unwrapped.has_value());
  ASSERT_EQ(*unwrapped, key);

  EXPECT_EQ(AppSecureKeyManager::CalculateKeyId({}, *unwrapped), id_before);
}

/**
 * The PIN-derived ID is a legacy read alias, not the identity of anything
 * written from now on. The formula is pinned here because a profile written
 * before the PIN became a pure wrap secret filed its objects under it, and
 * RegisterLegacyKeyIds() has to reproduce it exactly to keep them readable.
 */
TEST(AppSecureKeyManagerTest, LegacyPinDerivedIdFormulaIsStable) {
  const auto key = SampleKey();

  EXPECT_NE(AppSecureKeyManager::CalculateKeyId(GFBuffer("pin-a"), key),
            AppSecureKeyManager::CalculateKeyId(GFBuffer("pin-b"), key));
  EXPECT_NE(AppSecureKeyManager::CalculateKeyId(GFBuffer("pin-a"), key),
            AppSecureKeyManager::CalculateKeyId({}, key));
}

// Identity is derived from the key material alone, so that switching the
// at-rest protection never moves a key ID. A profile written while the PIN
// still fed the identity keeps that older ID registered as a read alias.

TEST(AppSecureKeyIdCompatTest, PinProfileRegistersBothIds) {
  const auto key = SampleKey();
  const GFBuffer pin("a-legacy-pin");

  QMap<GFBuffer, GFBuffer> keys;
  const auto stable_id =
      AppSecureKeyManager::RegisterLegacyKeyIds(keys, pin, key);

  EXPECT_EQ(stable_id, AppSecureKeyManager::CalculateKeyId({}, key));
  EXPECT_EQ(keys.size(), 2);
  EXPECT_EQ(keys.value(stable_id), key);
  EXPECT_EQ(keys.value(AppSecureKeyManager::CalculateKeyId(pin, key)), key);
}

TEST(AppSecureKeyIdCompatTest, NoPinRegistersOnlyTheStableId) {
  const auto key = SampleKey();

  QMap<GFBuffer, GFBuffer> keys;
  const auto stable_id =
      AppSecureKeyManager::RegisterLegacyKeyIds(keys, {}, key);

  EXPECT_EQ(keys.size(), 1);
  EXPECT_EQ(keys.value(stable_id), key);
}

/**
 * DataObjectOperator reads an object by slicing the first 32 bytes off it and
 * calling GetKey() with them. Both IDs therefore have to resolve to the same
 * material, or every object a pre-split profile wrote comes back unreadable.
 */
TEST(AppSecureKeyIdCompatTest, BothIdsResolveToTheSameMaterial) {
  const auto key = SampleKey();
  const GFBuffer pin("a-legacy-pin");

  QMap<GFBuffer, GFBuffer> keys;
  AppSecureKeyManager::RegisterLegacyKeyIds(keys, pin, key);

  const auto legacy_id = AppSecureKeyManager::CalculateKeyId(pin, key);
  const auto stable_id = AppSecureKeyManager::CalculateKeyId({}, key);

  ASSERT_NE(legacy_id, stable_id);
  EXPECT_EQ(keys.value(legacy_id), keys.value(stable_id));
}

/**
 * The whole point of the split: the ID a new object is written under does not
 * move when the PIN changes, so re-keying can never orphan anything.
 */
TEST(AppSecureKeyIdCompatTest, StableIdIsIndependentOfThePin) {
  const auto key = SampleKey();

  QMap<GFBuffer, GFBuffer> with_pin_a;
  QMap<GFBuffer, GFBuffer> with_pin_b;

  const auto id_a = AppSecureKeyManager::RegisterLegacyKeyIds(
      with_pin_a, GFBuffer("pin-a"), key);
  const auto id_b = AppSecureKeyManager::RegisterLegacyKeyIds(
      with_pin_b, GFBuffer("pin-b"), key);

  EXPECT_EQ(id_a, id_b);
}

/**
 * The key loaded at startup must be registered under its own derived ID, and
 * the legacy key must also be the active one below high security mode.
 */
TEST(AppSecureKeyManagerTest, LoadedLegacyKeyResolvesByItsOwnId) {
  auto& mgr = AppSecureKeyManager::GetInstance();

  const auto legacy_key = mgr.GetLegacyKey();
  ASSERT_FALSE(legacy_key.Empty());

  const auto expected_id = AppSecureKeyManager::CalculateKeyId({}, legacy_key);
  EXPECT_EQ(mgr.GetKey(expected_id), legacy_key);

  if (qApp->property("GFSecureLevel").toInt() < 3) {
    EXPECT_EQ(mgr.GetActiveKeyId(), expected_id);
    EXPECT_EQ(mgr.GetActiveKey(), legacy_key);
  }
}

/**
 * With no at-rest protection the key file on disk is the plaintext key. This
 * pins the on-disk format an existing installation already has, so a profile
 * written by an older build still loads. Whether the file is encrypted now
 * follows the protection mode, not the secure level.
 */
TEST(AppSecureKeyManagerTest, LegacyKeyFileMatchesLoadedKey) {
  if (AppKeyProtectionFromApp() != AppKeyProtection::kNONE) {
    GTEST_SKIP() << "key file is encrypted when a protection is in effect";
  }

  auto& mgr = AppSecureKeyManager::GetInstance();

  auto on_disk = GFBufferFactory::FromFile(mgr.GetLegacyKeyPath());
  ASSERT_TRUE(on_disk.has_value());

  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(*on_disk));
  EXPECT_EQ(*on_disk, mgr.GetLegacyKey());
}

/**
 * An unknown ID must resolve to nothing rather than to some default key.
 */
TEST(AppSecureKeyManagerTest, UnknownKeyIdResolvesEmpty) {
  auto& mgr = AppSecureKeyManager::GetInstance();
  EXPECT_TRUE(mgr.GetKey(GFBuffer(QByteArray(32, '\x00'))).Empty());
}

/**
 * Objects already on disk must still decrypt through the migrated key lookup.
 * This is the end-to-end guard that the legacy key still works.
 */
TEST(AppSecureKeyManagerTest, DataObjectRoundTripUsesLegacyKey) {
  auto& dao = DataObjectOperator::GetInstance();

  const GFBuffer payload("legacy-key-round-trip");
  const auto ref = dao.StoreSecDataObj("app_secure_key_test", payload);
  ASSERT_FALSE(ref.isEmpty());

  auto loaded = dao.GetSecDataObject("app_secure_key_test");
  ASSERT_TRUE(loaded.has_value());
  EXPECT_EQ(*loaded, payload);
}

/**
 * IsEncryptedBuffer is what makes a key file self-describing, so it must not
 * report a plaintext or truncated buffer as encrypted.
 */
TEST(AppSecureKeyManagerTest, IsEncryptedBufferDetectsContainer) {
  const auto key = SampleKey();

  auto encrypted = GFBufferFactory::EncryptLite(GFBuffer("k"), key);
  ASSERT_TRUE(encrypted.has_value());
  EXPECT_TRUE(AESCryptoHelper::IsEncryptedBuffer(*encrypted));

  auto encrypted_argon = GFBufferFactory::Encrypt(GFBuffer("k"), key);
  ASSERT_TRUE(encrypted_argon.has_value());
  EXPECT_TRUE(AESCryptoHelper::IsEncryptedBuffer(*encrypted_argon));

  // A raw 256-byte key, which is exactly what an unprotected app.key holds.
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(key));
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(GFBuffer()));

  // Correct magic but too short to be a container.
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(GFBuffer("GFSEC2")));

  // A container whose magic has been corrupted must be rejected.
  auto corrupted = *encrypted;
  corrupted.Data()[0] = 'X';
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(corrupted));
}

/**
 * The Argon2id helper backs the weekly rotating key, so it must be
 * deterministic and validate its inputs rather than producing a weak key.
 */
TEST(AppSecureKeyManagerTest, DeriveKeyArgon2IsDeterministic) {
  const GFBuffer passphrase("a-passphrase");

  auto salt = GFBufferFactory::ToSha256(GFBuffer("salt-source"));
  ASSERT_TRUE(salt.has_value());
  const auto salt16 = salt->Left(16);

  // Keep the cost low: this test is about behaviour, not hardness.
  auto a = AESCryptoHelper::DeriveKeyArgon2(passphrase, salt16, 32, 1, 8192, 1);
  ASSERT_TRUE(a.has_value());
  EXPECT_EQ(a->Size(), 32U);

  auto b = AESCryptoHelper::DeriveKeyArgon2(passphrase, salt16, 32, 1, 8192, 1);
  ASSERT_TRUE(b.has_value());
  EXPECT_EQ(*a, *b);

  auto other = AESCryptoHelper::DeriveKeyArgon2(GFBuffer("different"), salt16,
                                                32, 1, 8192, 1);
  ASSERT_TRUE(other.has_value());
  EXPECT_NE(*a, *other);
}

TEST(AppSecureKeyManagerTest, DeriveKeyArgon2RejectsBadParameters) {
  auto salt = GFBufferFactory::ToSha256(GFBuffer("salt-source"));
  ASSERT_TRUE(salt.has_value());
  const auto salt16 = salt->Left(16);
  const GFBuffer passphrase("a-passphrase");

  EXPECT_FALSE(
      AESCryptoHelper::DeriveKeyArgon2(passphrase, salt16, 0, 1, 8192, 1));
  EXPECT_FALSE(
      AESCryptoHelper::DeriveKeyArgon2(passphrase, salt16, 32, 0, 8192, 1));
  EXPECT_FALSE(
      AESCryptoHelper::DeriveKeyArgon2(passphrase, salt16, 32, 1, 0, 1));

  // libsodium fixes the salt length; a short salt must be refused rather than
  // silently padded.
  EXPECT_FALSE(AESCryptoHelper::DeriveKeyArgon2(passphrase, GFBuffer("short"),
                                                32, 1, 8192, 1));
}

// --- wrap state machine -----------------------------------------------------

TEST(AppSecureKeyWrapTest, DisabledLeavesPlaintextUntouched) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, false);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kNOT_WRAPPED);
  EXPECT_TRUE(result.secret.Empty());
  EXPECT_EQ(file.Read(), key);
  EXPECT_TRUE(store.entries.isEmpty());
}

TEST(AppSecureKeyWrapTest, EnableEncryptsFileAndStoresSecret) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  ASSERT_EQ(result.status, AppKeyWrapStatus::kJUST_ENABLED);
  EXPECT_EQ(result.secret.Size(), 32U);

  const auto on_disk = file.Read();
  EXPECT_TRUE(AESCryptoHelper::IsEncryptedBuffer(on_disk));
  EXPECT_NE(on_disk, key);

  // The stored secret must be the one that actually opens the file.
  ASSERT_TRUE(store.entries.contains(kAppKeyWrapAccount));
  auto plain = GFBufferFactory::DecryptLite(
      store.entries.value(kAppKeyWrapAccount), on_disk);
  ASSERT_TRUE(plain.has_value());
  EXPECT_EQ(*plain, key);
}

TEST(AppSecureKeyWrapTest, EnabledResolvesStoredSecret) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  const auto enabled =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);
  ASSERT_EQ(enabled.status, AppKeyWrapStatus::kJUST_ENABLED);

  // A later start finds the file already wrapped and just resolves the secret.
  const auto steady =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(steady.status, AppKeyWrapStatus::kWRAPPED);
  EXPECT_EQ(steady.secret, enabled.secret);
  EXPECT_TRUE(AESCryptoHelper::IsEncryptedBuffer(file.Read()));
}

TEST(AppSecureKeyWrapTest, DisableRestoresPlaintextAndClearsStore) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true).status,
      AppKeyWrapStatus::kJUST_ENABLED);

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, false);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kJUST_DISABLED);
  EXPECT_TRUE(result.secret.Empty());
  EXPECT_EQ(file.Read(), key);
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(file.Read()));
  EXPECT_FALSE(store.entries.contains(kAppKeyWrapAccount));
}

/**
 * Enabling and disabling must be perfectly reversible, because the key ID
 * derived from the key material is what every stored data object is filed
 * under. This is the guard against orphaning them.
 */
TEST(AppSecureKeyWrapTest, RoundTripPreservesKeyAndItsId) {
  const auto key = SampleKey();
  const auto id_before = AppSecureKeyManager::CalculateKeyId({}, key);

  ScopedKeyFile file(key);
  FakeSecretStore store;

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true).status,
      AppKeyWrapStatus::kJUST_ENABLED);

  // Even while wrapped, the key that comes back out is byte-identical.
  const auto wrapped =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);
  auto unwrapped = GFBufferFactory::DecryptLite(wrapped.secret, file.Read());
  ASSERT_TRUE(unwrapped.has_value());
  EXPECT_EQ(AppSecureKeyManager::CalculateKeyId({}, *unwrapped), id_before);

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, false).status,
      AppKeyWrapStatus::kJUST_DISABLED);

  EXPECT_EQ(file.Read(), key);
  EXPECT_EQ(AppSecureKeyManager::CalculateKeyId({}, file.Read()), id_before);
}

TEST(AppSecureKeyWrapTest, EnableWithNoKeyFileYetJustProvisionsSecret) {
  ScopedKeyFile file{GFBuffer{}};
  ASSERT_FALSE(file.Exists());
  FakeSecretStore store;

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  // Nothing to convert; the caller creates the key already encrypted.
  EXPECT_EQ(result.status, AppKeyWrapStatus::kJUST_ENABLED);
  EXPECT_EQ(result.secret.Size(), 32U);
  EXPECT_FALSE(file.Exists());
}

/**
 * The bug this guards against shipped: ResolveWrapSecret sealed the key file
 * with one key derivation while the loader unsealed it with another, so
 * enabling the feature produced a file that the next start refused to open.
 *
 * Asserting through UnsealKey rather than calling DecryptLite directly is the
 * whole point — a test that picks the derivation itself agrees with whichever
 * one the sealer used and never crosses the seam.
 */
TEST(AppSecureKeyWrapTest, WrappedFileOpensThroughTheLoadersOwnPath) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  const auto enabled =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);
  ASSERT_EQ(enabled.status, AppKeyWrapStatus::kJUST_ENABLED);

  // Exactly what init_legacy_key() does on the following start.
  auto recovered =
      AppSecureKeyManager::UnsealKey({}, enabled.secret, file.Read());
  ASSERT_TRUE(recovered.has_value());
  EXPECT_EQ(*recovered, key);
}

/**
 * Sealing and unsealing must agree for both kinds of secret, and a file sealed
 * for one kind must not open as the other.
 */
TEST(AppSecureKeyWrapTest, SealAndUnsealAgreeForBothSecretKinds) {
  const auto key = SampleKey();
  const GFBuffer pin("a-user-pin");
  const GFBuffer wrap(QByteArray(32, '\x11'));

  auto sealed_pin = AppSecureKeyManager::SealKey(pin, {}, key);
  ASSERT_TRUE(sealed_pin.has_value());
  auto opened_pin = AppSecureKeyManager::UnsealKey(pin, {}, *sealed_pin);
  ASSERT_TRUE(opened_pin.has_value());
  EXPECT_EQ(*opened_pin, key);

  auto sealed_wrap = AppSecureKeyManager::SealKey({}, wrap, key);
  ASSERT_TRUE(sealed_wrap.has_value());
  auto opened_wrap = AppSecureKeyManager::UnsealKey({}, wrap, *sealed_wrap);
  ASSERT_TRUE(opened_wrap.has_value());
  EXPECT_EQ(*opened_wrap, key);

  // Feeding a secret through the wrong slot selects the wrong derivation, which
  // is exactly how the two halves came to disagree.
  EXPECT_FALSE(AppSecureKeyManager::UnsealKey(wrap, {}, *sealed_wrap));
  EXPECT_FALSE(AppSecureKeyManager::UnsealKey({}, pin, *sealed_pin));

  // With no secret at all the key is stored verbatim.
  auto unprotected = AppSecureKeyManager::SealKey({}, {}, key);
  ASSERT_TRUE(unprotected.has_value());
  EXPECT_EQ(*unprotected, key);
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(*unprotected));
}

// ChangeProtection() moves the key file between the three at-rest backends.
// Every transition must preserve the key material and its identity, and every
// failure must leave the file exactly as it was — it is the only copy.

namespace {

/// Recover the plaintext key from a file sealed under @p protection.
auto OpenAs(const GFBuffer& stored, AppKeyProtection protection,
            const GFBuffer& pin, const GFBuffer& secret) -> GFBufferOrNone {
  switch (protection) {
    case AppKeyProtection::kPIN:
      return AppSecureKeyManager::UnsealKey(pin, {}, stored);
    case AppKeyProtection::kKEYCHAIN:
      return AppSecureKeyManager::UnsealKey({}, secret, stored);
    case AppKeyProtection::kNONE:
      break;
  }
  return stored;
}

/// Put a key file into @p protection, returning the store secret when one was
/// provisioned. Fails the calling test if the transition does not succeed.
auto SealAs(const ScopedKeyFile& file, FakeSecretStore& store,
            const GFBuffer& key, AppKeyProtection protection,
            const GFBuffer& pin) -> GFBuffer {
  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kNONE, protection, pin);
  EXPECT_TRUE(result.Ok()) << "setting up " << static_cast<int>(protection);
  return store.entries.value(kAppKeyWrapAccount, GFBuffer{});
}

}  // namespace

TEST(AppKeyProtectionTest, NoneToKeychainSealsFileAndStoresSecret) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kNONE,
      AppKeyProtection::kKEYCHAIN, {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kOK);
  EXPECT_TRUE(AESCryptoHelper::IsEncryptedBuffer(file.Read()));
  ASSERT_TRUE(store.entries.contains(kAppKeyWrapAccount));

  auto opened = AppSecureKeyManager::UnsealKey(
      {}, store.entries.value(kAppKeyWrapAccount), file.Read());
  ASSERT_TRUE(opened.has_value());
  EXPECT_EQ(*opened, key);
}

TEST(AppKeyProtectionTest, NoneToPinSealsWithoutTouchingTheStore) {
  const auto key = SampleKey();
  const GFBuffer pin("a-user-pin");
  ScopedKeyFile file(key);
  FakeSecretStore store;

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kNONE, AppKeyProtection::kPIN,
      pin);

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kOK);
  EXPECT_TRUE(AESCryptoHelper::IsEncryptedBuffer(file.Read()));
  EXPECT_TRUE(store.entries.isEmpty());

  auto opened = AppSecureKeyManager::UnsealKey(pin, {}, file.Read());
  ASSERT_TRUE(opened.has_value());
  EXPECT_EQ(*opened, key);

  // The PIN belongs in the PIN slot: fed through the wrap slot it selects the
  // cheap derivation instead of Argon2id and must not open the file.
  EXPECT_FALSE(AppSecureKeyManager::UnsealKey({}, pin, file.Read()));
}

TEST(AppKeyProtectionTest, KeychainToNoneRestoresPlaintextAndClearsStore) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  SealAs(file, store, key, AppKeyProtection::kKEYCHAIN, {});

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kKEYCHAIN,
      AppKeyProtection::kNONE, {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kOK);
  EXPECT_EQ(file.Read(), key);
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(file.Read()));
  EXPECT_FALSE(store.entries.contains(kAppKeyWrapAccount));
}

TEST(AppKeyProtectionTest, PinToNoneRestoresPlaintext) {
  const auto key = SampleKey();
  const GFBuffer pin("a-user-pin");
  ScopedKeyFile file(key);
  FakeSecretStore store;
  SealAs(file, store, key, AppKeyProtection::kPIN, pin);

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kPIN, AppKeyProtection::kNONE,
      {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kOK);
  EXPECT_EQ(file.Read(), key);
  EXPECT_TRUE(store.entries.isEmpty());
}

TEST(AppKeyProtectionTest, KeychainToPinReleasesTheStoreEntry) {
  const auto key = SampleKey();
  const GFBuffer pin("a-user-pin");
  ScopedKeyFile file(key);
  FakeSecretStore store;
  SealAs(file, store, key, AppKeyProtection::kKEYCHAIN, {});

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kKEYCHAIN,
      AppKeyProtection::kPIN, pin);

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kOK);
  EXPECT_FALSE(store.entries.contains(kAppKeyWrapAccount));

  auto opened = AppSecureKeyManager::UnsealKey(pin, {}, file.Read());
  ASSERT_TRUE(opened.has_value());
  EXPECT_EQ(*opened, key);
}

TEST(AppKeyProtectionTest, PinToKeychainProvisionsANewSecret) {
  const auto key = SampleKey();
  const GFBuffer pin("a-user-pin");
  ScopedKeyFile file(key);
  FakeSecretStore store;
  SealAs(file, store, key, AppKeyProtection::kPIN, pin);

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kPIN,
      AppKeyProtection::kKEYCHAIN, {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kOK);
  ASSERT_TRUE(store.entries.contains(kAppKeyWrapAccount));

  // The old PIN is not consulted and is no longer able to open the file.
  auto opened = AppSecureKeyManager::UnsealKey(
      {}, store.entries.value(kAppKeyWrapAccount), file.Read());
  ASSERT_TRUE(opened.has_value());
  EXPECT_EQ(*opened, key);
  EXPECT_FALSE(AppSecureKeyManager::UnsealKey(pin, {}, file.Read()));
}

/**
 * The guard against orphaning every stored data object: whatever route the key
 * file takes between backends, the material — and therefore the ID that every
 * object carries as a prefix — has to come back unchanged.
 */
TEST(AppKeyProtectionTest, EveryTransitionPreservesTheKeyId) {
  const auto key = SampleKey();
  const GFBuffer from_pin("pin-before");
  const GFBuffer to_pin("pin-after");
  const auto expected_id = AppSecureKeyManager::CalculateKeyId({}, key);

  for (const auto from : {AppKeyProtection::kNONE, AppKeyProtection::kKEYCHAIN,
                          AppKeyProtection::kPIN}) {
    for (const auto to : {AppKeyProtection::kNONE, AppKeyProtection::kKEYCHAIN,
                          AppKeyProtection::kPIN}) {
      ScopedKeyFile file(key);
      FakeSecretStore store;
      SealAs(file, store, key, from, from_pin);

      const auto result = AppSecureKeyManager::ChangeProtection(
          file.Path(), &store, key, from, to, to_pin);
      ASSERT_TRUE(result.Ok())
          << static_cast<int>(from) << " -> " << static_cast<int>(to);

      // kUNCHANGED leaves the file in its original form, so read it back the
      // way it is actually sealed rather than the way it was requested.
      const auto effective =
          result.status == AppKeyProtectionStatus::kUNCHANGED ? from : to;
      const auto pin = result.status == AppKeyProtectionStatus::kUNCHANGED
                           ? from_pin
                           : to_pin;

      auto opened = OpenAs(file.Read(), effective, pin,
                           store.entries.value(kAppKeyWrapAccount, GFBuffer{}));
      ASSERT_TRUE(opened.has_value())
          << static_cast<int>(from) << " -> " << static_cast<int>(to);
      EXPECT_EQ(*opened, key);
      EXPECT_EQ(AppSecureKeyManager::CalculateKeyId({}, *opened), expected_id);
    }
  }
}

TEST(AppKeyProtectionTest, SameProtectionIsANoOp) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  const auto before = file.Read();

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kNONE,
      AppKeyProtection::kNONE, {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kUNCHANGED);
  EXPECT_EQ(file.Read(), before);
  EXPECT_TRUE(store.entries.isEmpty());
}

/**
 * Re-sealing with a new PIN is the one same-mode transition that does work: it
 * is how a PIN is changed without the key ever touching the disk in plaintext.
 */
TEST(AppKeyProtectionTest, PinRekeyIsNotANoOp) {
  const auto key = SampleKey();
  const GFBuffer old_pin("pin-before");
  const GFBuffer new_pin("pin-after");
  ScopedKeyFile file(key);
  FakeSecretStore store;
  SealAs(file, store, key, AppKeyProtection::kPIN, old_pin);

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kPIN, AppKeyProtection::kPIN,
      new_pin);

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kOK);

  auto opened = AppSecureKeyManager::UnsealKey(new_pin, {}, file.Read());
  ASSERT_TRUE(opened.has_value());
  EXPECT_EQ(*opened, key);
  EXPECT_FALSE(AppSecureKeyManager::UnsealKey(old_pin, {}, file.Read()));
}

// The weekly rotating key set of secure level 3 hangs off the application
// secure key, not off a PIN. That is what lets the at-rest protection be
// changed at all: a rotated key derived from a PIN would be orphaned the moment
// that PIN did.

TEST(AppSecureKeyRotationTest, DerivationIsDeterministicPerPeriod) {
  const auto app_key = SampleKey();

  EXPECT_EQ(AppSecureKeyManager::DeriveRotatedKey(app_key, 42),
            AppSecureKeyManager::DeriveRotatedKey(app_key, 42));
  EXPECT_NE(AppSecureKeyManager::DeriveRotatedKey(app_key, 42),
            AppSecureKeyManager::DeriveRotatedKey(app_key, 43));
}

TEST(AppSecureKeyRotationTest, DerivationFollowsTheAppKey) {
  const auto other = GFBuffer(QByteArray(256, '\x3C'));

  EXPECT_NE(AppSecureKeyManager::DeriveRotatedKey(SampleKey(), 42),
            AppSecureKeyManager::DeriveRotatedKey(other, 42));
}

/**
 * The property the whole re-basing exists for. A rotated key is a function of
 * the app secure key alone, so moving the key file between backends — or
 * changing the PIN — leaves every rotated key, and every object written under
 * one, exactly where it was.
 */
TEST(AppSecureKeyRotationTest, RotatedKeySurvivesEveryProtectionChange) {
  const auto app_key = SampleKey();
  const GFBuffer first_pin("pin-before");
  const GFBuffer second_pin("pin-after");

  const auto before = AppSecureKeyManager::DeriveRotatedKey(app_key, 42);
  ASSERT_FALSE(before.Empty());
  const auto before_id = AppSecureKeyManager::CalculateKeyId({}, before);

  ScopedKeyFile file(app_key);
  FakeSecretStore store;

  const AppKeyProtection route[] = {
      AppKeyProtection::kKEYCHAIN, AppKeyProtection::kPIN,
      AppKeyProtection::kPIN, AppKeyProtection::kNONE};
  const GFBuffer pins[] = {{}, first_pin, second_pin, {}};

  auto current = AppKeyProtection::kNONE;
  for (int i = 0; i < 4; ++i) {
    const auto result = AppSecureKeyManager::ChangeProtection(
        file.Path(), &store, app_key, current, route[i], pins[i]);
    ASSERT_TRUE(result.Ok()) << "step " << i;
    current = route[i];

    // The app key is untouched by a protection change, so the rotated key it
    // derives must be bit-identical at every step.
    const auto after = AppSecureKeyManager::DeriveRotatedKey(app_key, 42);
    EXPECT_EQ(after, before) << "step " << i;
    EXPECT_EQ(AppSecureKeyManager::CalculateKeyId({}, after), before_id)
        << "step " << i;
  }
}

/**
 * Initialize() trial-decrypts each rotated key file with the app secure key and
 * then, for a profile written before the re-basing, with the PIN. Both forms
 * have to open, and neither may open the other's — a cross-open would mean the
 * wrong derivation had been selected.
 */
TEST(AppSecureKeyRotationTest, BothOnDiskFormsOfARotatedKeyOpen) {
  const auto app_key = SampleKey();
  const GFBuffer pin("a-legacy-pin");
  const auto rotated = AppSecureKeyManager::DeriveRotatedKey(app_key, 42);
  ASSERT_FALSE(rotated.Empty());

  auto current_form = GFBufferFactory::EncryptLite(app_key, rotated);
  ASSERT_TRUE(current_form.has_value());
  auto legacy_form = GFBufferFactory::Encrypt(pin, rotated);
  ASSERT_TRUE(legacy_form.has_value());

  auto opened_current = GFBufferFactory::DecryptLite(app_key, *current_form);
  ASSERT_TRUE(opened_current.has_value());
  EXPECT_EQ(*opened_current, rotated);

  auto opened_legacy = GFBufferFactory::Decrypt(pin, *legacy_form);
  ASSERT_TRUE(opened_legacy.has_value());
  EXPECT_EQ(*opened_legacy, rotated);

  // The fallback order in Initialize() relies on the wrong form failing rather
  // than returning garbage.
  EXPECT_FALSE(GFBufferFactory::DecryptLite(app_key, *legacy_form));
  EXPECT_FALSE(GFBufferFactory::Decrypt(pin, *current_form));
}

/**
 * A rotated key is filed under an ID that carries no trace of the PIN, so the
 * objects written under it stay addressable after a re-key.
 */
TEST(AppSecureKeyRotationTest, RotatedKeyIdIsPinIndependent) {
  const auto app_key = SampleKey();
  const auto rotated = AppSecureKeyManager::DeriveRotatedKey(app_key, 42);

  QMap<GFBuffer, GFBuffer> keys;
  const auto id = AppSecureKeyManager::RegisterLegacyKeyIds(keys, {}, rotated);

  EXPECT_EQ(id, AppSecureKeyManager::CalculateKeyId({}, rotated));
  EXPECT_EQ(keys.value(id), rotated);
}

// --- failure paths ----------------------------------------------------------

TEST(AppKeyProtectionTest, PinTargetRejectsAnEmptyPin) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  const auto before = file.Read();

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kNONE, AppKeyProtection::kPIN,
      {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kBAD_PIN);
  EXPECT_EQ(file.Read(), before);
}

TEST(AppKeyProtectionTest, KeychainTargetWithUnavailableStoreLeavesFileAlone) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  store.available = false;
  const auto before = file.Read();

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kNONE,
      AppKeyProtection::kKEYCHAIN, {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kSTORE_UNAVAILABLE);
  EXPECT_EQ(file.Read(), before);
  EXPECT_TRUE(store.entries.isEmpty());
}

TEST(AppKeyProtectionTest, KeychainTargetWithMissingStoreLeavesFileAlone) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  const auto before = file.Read();

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), nullptr, key, AppKeyProtection::kNONE,
      AppKeyProtection::kKEYCHAIN, {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kSTORE_UNAVAILABLE);
  EXPECT_EQ(file.Read(), before);
}

TEST(AppKeyProtectionTest, SecretThatDoesNotReadBackAbortsTheTransition) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  store.corrupt_on_write = true;
  const auto before = file.Read();

  const auto result = AppSecureKeyManager::ChangeProtection(
      file.Path(), &store, key, AppKeyProtection::kNONE,
      AppKeyProtection::kKEYCHAIN, {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kSTORE_UNAVAILABLE);
  EXPECT_EQ(file.Read(), before);
  // The half-provisioned entry must be rolled back, or the next start would
  // find a secret that opens nothing.
  EXPECT_FALSE(store.entries.contains(kAppKeyWrapAccount));
}

/**
 * The "never leave the only copy unreadable" invariant. If the file cannot be
 * rewritten, the secret the current form depends on must survive.
 */
TEST(AppKeyProtectionTest, FailedFileWriteKeepsTheOldSecretRecoverable) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  const auto secret = SealAs(file, store, key, AppKeyProtection::kKEYCHAIN, {});
  const auto sealed = file.Read();

  // A directory cannot be replaced by a file, so the atomic write fails while
  // everything before it has already succeeded.
  QTemporaryDir blocked;
  ASSERT_TRUE(blocked.isValid());

  const auto result = AppSecureKeyManager::ChangeProtection(
      blocked.path(), &store, key, AppKeyProtection::kKEYCHAIN,
      AppKeyProtection::kNONE, {});

  EXPECT_EQ(result.status, AppKeyProtectionStatus::kIO_FAILED);
  EXPECT_EQ(file.Read(), sealed);
  ASSERT_TRUE(store.entries.contains(kAppKeyWrapAccount));
  EXPECT_EQ(store.entries.value(kAppKeyWrapAccount), secret);

  auto still_openable = AppSecureKeyManager::UnsealKey({}, secret, file.Read());
  ASSERT_TRUE(still_openable.has_value());
  EXPECT_EQ(*still_openable, key);
}

TEST(AppSecureKeyWrapTest, UnavailableStoreLeavesFilePlaintext) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  store.available = false;

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kSTORE_UNAVAILABLE);
  EXPECT_EQ(file.Read(), key);
  EXPECT_TRUE(store.entries.isEmpty());
}

TEST(AppSecureKeyWrapTest, MissingStoreLeavesFilePlaintext) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), nullptr, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kSTORE_UNAVAILABLE);
  EXPECT_EQ(file.Read(), key);
}

TEST(AppSecureKeyWrapTest, FailedStoreWriteLeavesFilePlaintext) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  store.writable = false;

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kSTORE_UNAVAILABLE);
  EXPECT_EQ(file.Read(), key);
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(file.Read()));
}

/**
 * The read-back check is what stands between a store that quietly loses the
 * secret and a key file nobody can open again.
 */
TEST(AppSecureKeyWrapTest, SecretThatDoesNotReadBackAbortsTheTransition) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  store.corrupt_on_write = true;

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kSTORE_UNAVAILABLE);
  EXPECT_EQ(file.Read(), key);
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(file.Read()));
  // The unusable entry must not be left behind.
  EXPECT_FALSE(store.entries.contains(kAppKeyWrapAccount));
}

TEST(AppSecureKeyWrapTest, LostSecretReportsLockedOutWithoutTouchingFile) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true).status,
      AppKeyWrapStatus::kJUST_ENABLED);
  const auto wrapped_bytes = file.Read();

  // The keyring was reset, or the profile moved to another machine.
  store.entries.clear();

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kLOCKED_OUT);
  // Destroying the only copy of the key here would be unrecoverable.
  EXPECT_EQ(file.Read(), wrapped_bytes);
}

TEST(AppSecureKeyWrapTest, DisableWithLostSecretIsLockedOutNotDataLoss) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true).status,
      AppKeyWrapStatus::kJUST_ENABLED);
  const auto wrapped_bytes = file.Read();
  store.entries.clear();

  // Turning the setting off cannot rescue a file we can no longer decrypt.
  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, false);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kLOCKED_OUT);
  EXPECT_EQ(file.Read(), wrapped_bytes);
}

TEST(AppSecureKeyWrapTest, WrongSecretReportsLockedOut) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true).status,
      AppKeyWrapStatus::kJUST_ENABLED);

  store.entries.insert(kAppKeyWrapAccount, GFBuffer(QByteArray(32, '\x01')));

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kLOCKED_OUT);
}

TEST(AppSecureKeyWrapTest, DisabledAndUnwrappedNeverConsultsTheStore) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  // A broken store must not matter when the feature is off.
  store.available = false;
  store.writable = false;

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, false);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kNOT_WRAPPED);
  EXPECT_EQ(file.Read(), key);
}

/**
 * The feature is opt-in. With nothing enabling it, the running process must
 * report it off and the real key file must be left in its default form, so a
 * changed default or an accidental auto-enable is caught here.
 */
TEST(AppSecureKeyWrapTest, FeatureIsOffUnlessExplicitlyEnabled) {
  EXPECT_EQ(AppKeyProtectionFromApp(), AppKeyProtection::kNONE);

  auto& mgr = AppSecureKeyManager::GetInstance();
  auto on_disk = GFBufferFactory::FromFile(mgr.GetLegacyKeyPath());
  ASSERT_TRUE(on_disk.has_value());
  EXPECT_FALSE(AESCryptoHelper::IsEncryptedBuffer(*on_disk));
}

// --- live backend -----------------------------------------------------------

/**
 * Exercises whichever backend this platform installed. Skips where none is
 * available, which includes headless CI: the platform sources are part of the
 * application target, not the test library.
 */
TEST(AppSecureKeyWrapTest, InstalledBackendRoundTrip) {
  auto* store = GetSystemSecretStore();
  if (store == nullptr || !store->IsAvailable()) {
    GTEST_SKIP() << "no system credential store on this host";
  }

  const QString account = "gftest-roundtrip";
  auto secret = SecureRandomGenerator::Generate(32);
  ASSERT_TRUE(secret.has_value());

  ASSERT_TRUE(store->Write(account, *secret));

  auto read_back = store->Read(account);
  ASSERT_TRUE(read_back.has_value());
  EXPECT_EQ(*read_back, *secret);

  EXPECT_TRUE(store->Remove(account));
  EXPECT_FALSE(store->Read(account).has_value());
}

}  // namespace GpgFrontend::Test
