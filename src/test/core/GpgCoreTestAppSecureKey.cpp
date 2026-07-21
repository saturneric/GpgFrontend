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
 * A PIN participates in the identity, which is what makes high security mode's
 * key set distinct from the unprotected one.
 */
TEST(AppSecureKeyManagerTest, KeyIdDependsOnPin) {
  const auto key = SampleKey();

  EXPECT_NE(AppSecureKeyManager::CalculateKeyId(GFBuffer("pin-a"), key),
            AppSecureKeyManager::CalculateKeyId(GFBuffer("pin-b"), key));
  EXPECT_NE(AppSecureKeyManager::CalculateKeyId(GFBuffer("pin-a"), key),
            AppSecureKeyManager::CalculateKeyId({}, key));
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
 * Below high security mode the key file on disk is the plaintext key. This
 * pins the on-disk format an existing installation already has, so a profile
 * written by an older build still loads.
 */
TEST(AppSecureKeyManagerTest, LegacyKeyFileMatchesLoadedKey) {
  if (qApp->property("GFSecureLevel").toInt() > 2) {
    GTEST_SKIP() << "key file is PIN-encrypted in high security mode";
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

  EXPECT_EQ(result.status, AppKeyWrapStatus::kNotWrapped);
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

  ASSERT_EQ(result.status, AppKeyWrapStatus::kJustEnabled);
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
  ASSERT_EQ(enabled.status, AppKeyWrapStatus::kJustEnabled);

  // A later start finds the file already wrapped and just resolves the secret.
  const auto steady =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(steady.status, AppKeyWrapStatus::kWrapped);
  EXPECT_EQ(steady.secret, enabled.secret);
  EXPECT_TRUE(AESCryptoHelper::IsEncryptedBuffer(file.Read()));
}

TEST(AppSecureKeyWrapTest, DisableRestoresPlaintextAndClearsStore) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true).status,
      AppKeyWrapStatus::kJustEnabled);

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, false);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kJustDisabled);
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
      AppKeyWrapStatus::kJustEnabled);

  // Even while wrapped, the key that comes back out is byte-identical.
  const auto wrapped =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);
  auto unwrapped = GFBufferFactory::DecryptLite(wrapped.secret, file.Read());
  ASSERT_TRUE(unwrapped.has_value());
  EXPECT_EQ(AppSecureKeyManager::CalculateKeyId({}, *unwrapped), id_before);

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, false).status,
      AppKeyWrapStatus::kJustDisabled);

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
  EXPECT_EQ(result.status, AppKeyWrapStatus::kJustEnabled);
  EXPECT_EQ(result.secret.Size(), 32U);
  EXPECT_FALSE(file.Exists());
}

// --- failure paths ----------------------------------------------------------

TEST(AppSecureKeyWrapTest, UnavailableStoreLeavesFilePlaintext) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  store.available = false;

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kStoreUnavailable);
  EXPECT_EQ(file.Read(), key);
  EXPECT_TRUE(store.entries.isEmpty());
}

TEST(AppSecureKeyWrapTest, MissingStoreLeavesFilePlaintext) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), nullptr, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kStoreUnavailable);
  EXPECT_EQ(file.Read(), key);
}

TEST(AppSecureKeyWrapTest, FailedStoreWriteLeavesFilePlaintext) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;
  store.writable = false;

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kStoreUnavailable);
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

  EXPECT_EQ(result.status, AppKeyWrapStatus::kStoreUnavailable);
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
      AppKeyWrapStatus::kJustEnabled);
  const auto wrapped_bytes = file.Read();

  // The keyring was reset, or the profile moved to another machine.
  store.entries.clear();

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kLockedOut);
  // Destroying the only copy of the key here would be unrecoverable.
  EXPECT_EQ(file.Read(), wrapped_bytes);
}

TEST(AppSecureKeyWrapTest, DisableWithLostSecretIsLockedOutNotDataLoss) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true).status,
      AppKeyWrapStatus::kJustEnabled);
  const auto wrapped_bytes = file.Read();
  store.entries.clear();

  // Turning the setting off cannot rescue a file we can no longer decrypt.
  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, false);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kLockedOut);
  EXPECT_EQ(file.Read(), wrapped_bytes);
}

TEST(AppSecureKeyWrapTest, WrongSecretReportsLockedOut) {
  const auto key = SampleKey();
  ScopedKeyFile file(key);
  FakeSecretStore store;

  ASSERT_EQ(
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true).status,
      AppKeyWrapStatus::kJustEnabled);

  store.entries.insert(kAppKeyWrapAccount, GFBuffer(QByteArray(32, '\x01')));

  const auto result =
      AppSecureKeyManager::ResolveWrapSecret(file.Path(), &store, true);

  EXPECT_EQ(result.status, AppKeyWrapStatus::kLockedOut);
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

  EXPECT_EQ(result.status, AppKeyWrapStatus::kNotWrapped);
  EXPECT_EQ(file.Read(), key);
}

/**
 * The feature is opt-in. With nothing enabling it, the running process must
 * report it off and the real key file must be left in its default form, so a
 * changed default or an accidental auto-enable is caught here.
 */
TEST(AppSecureKeyWrapTest, FeatureIsOffUnlessExplicitlyEnabled) {
  EXPECT_FALSE(qApp->property("GFOSSecretStore").toBool());

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
