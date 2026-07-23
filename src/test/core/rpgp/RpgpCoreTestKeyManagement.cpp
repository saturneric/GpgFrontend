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

#include "RpgpCoreTest.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/function/rpgp/PasswordFetcher.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/utils/GpgUtils.h"

static const char* test_private_key_data = R"(
-----BEGIN PGP PRIVATE KEY BLOCK-----

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
=7+Wn
-----END PGP PRIVATE KEY BLOCK-----
)";

namespace GpgFrontend::Test {

TEST_F(RpgpCoreTest, CoreDeleteSubkeyTest) {
  auto info =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ImportKey(GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_TRUE(info != nullptr);
  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  auto s_keys = key->SubKeys();
  auto original_size = s_keys.size();
  ASSERT_GE(original_size, 2);

  ASSERT_TRUE(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                  .DeleteSubkey(key, original_size - 1));

  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
  key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
            .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  s_keys = key->SubKeys();
  ASSERT_EQ(s_keys.size(), static_cast<size_t>(original_size - 1));
}

TEST_F(RpgpCoreTest, CoreRevokeSubkeyTest) {
  auto info =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ImportKey(GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_TRUE(info != nullptr);
  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  auto s_keys = key->SubKeys();
  ASSERT_GE(s_keys.size(), 2);

  ASSERT_TRUE(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                  .RevokeSubkey(key, 1, 0, QString("Test revocation")));

  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
  key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
            .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);

  s_keys = key->SubKeys();
  ASSERT_GE(s_keys.size(), 2);
  ASSERT_TRUE(s_keys[1].IsRevoked());
}

namespace {

// Fingerprint of a fixture key whose passphrase is kDefaultPassphrase.
const char* const kFixtureKeyFpr = "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497";
const char* const kDefaultPassphrase = "123456";
const char* const kNewPassphrase = "654321";

// Answers unlock prompts from `unlock` (by fingerprint, falling back to
// `unlock_default`) and new-passphrase prompts with `fresh`. Lets a test assert
// which key is actually protected by which passphrase.
struct ScriptedPassphrases {
  QMap<QString, QString> unlock;
  QString unlock_default;
  QString fresh;
};

void InstallPassphraseScript(const ScriptedPassphrases& script) {
  SetChannelPasswordFetcher(
      kRpgpChannelForUnitTest, [script](const PassphraseState& s) -> GFBuffer {
        if (s.ask_for_new) return GFBuffer(script.fresh);
        auto it = script.unlock.find(s.fpr.toUpper());
        return GFBuffer(it != script.unlock.end() ? it.value()
                                                  : script.unlock_default);
      });
}

// Restore what RpgpCoreTest::SetUpTestSuite installs, so a test that reshuffles
// passphrases doesn't leak its script into the rest of the suite.
void RestoreDefaultPassphraseFetcher() {
  SetChannelPasswordFetcher(kRpgpChannelForUnitTest,
                            [](const PassphraseState&) -> GFBuffer {
                              return GFBuffer(QString(kDefaultPassphrase));
                            });
}

// Encrypt to the fixture key, then decrypt again. Decryption unlocks the
// *encryption subkey*, so this is what proves a passphrase change reached the
// subkeys and not just the primary key packet.
auto EncryptThenDecryptRoundTrip() -> GpgError {
  auto encrypt_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                         .GetPubkeyPtr(kFixtureKeyFpr);
  if (encrypt_key == nullptr) return GPG_ERR_NO_PUBKEY;

  auto buffer = GFBuffer(QString("Hello RPGP!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({encrypt_key}, buffer, true);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return err;

  auto encr_out_buffer = ExtractParams<GFBuffer>(data_object, 1);

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(encr_out_buffer);
  if (CheckGpgError(err_0) != GPG_ERR_NO_ERROR) return err_0;

  auto decr_out_buffer = ExtractParams<GFBuffer>(data_object_0, 1);
  return decr_out_buffer == buffer ? GPG_ERR_NO_ERROR : GPG_ERR_GENERAL;
}

}  // namespace

// A whole-key change must re-protect the subkeys too, matching what
// `gpg --passwd` does. Decrypting afterwards with only the new passphrase
// available is the assertion: it unlocks the encryption subkey.
TEST_F(RpgpCoreTest, CoreModifyKeyPassphraseCoversSubkeysTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr(kFixtureKeyFpr);
  ASSERT_TRUE(key != nullptr);
  ASSERT_GE(key->SubKeys().size(), 2);

  InstallPassphraseScript(
      {.unlock_default = kDefaultPassphrase, .fresh = kNewPassphrase});

  auto [err, _] = KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                      .ModifyPasswordSync(key);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  // Only the new passphrase is on offer from here on.
  InstallPassphraseScript({.unlock_default = kNewPassphrase});
  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();

  EXPECT_EQ(CheckGpgError(EncryptThenDecryptRoundTrip()), GPG_ERR_NO_ERROR);

  RestoreDefaultPassphraseFetcher();
}

// A per-subkey change must touch only the named subkey. The encryption subkey
// moves to the new passphrase while the primary stays on the old one.
TEST_F(RpgpCoreTest, CoreModifySubkeyPassphraseTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr(kFixtureKeyFpr);
  ASSERT_TRUE(key != nullptr);

  // The primary key is certify-only here; encryption runs on a subkey.
  QString encr_subkey_fpr;
  for (const auto& s_key : key->SubKeys()) {
    if (!s_key.IsHasCertCap() && s_key.IsHasEncrCap()) {
      encr_subkey_fpr = s_key.Fingerprint();
      break;
    }
  }
  ASSERT_FALSE(encr_subkey_fpr.isEmpty());

  InstallPassphraseScript(
      {.unlock_default = kDefaultPassphrase, .fresh = kNewPassphrase});

  auto [err, _] = KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                      .ModifySubkeyPasswordSync(key, encr_subkey_fpr);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();

  // The encryption subkey now needs the new passphrase; offering only the old
  // one must fail.
  InstallPassphraseScript({.unlock_default = kDefaultPassphrase});
  EXPECT_NE(CheckGpgError(EncryptThenDecryptRoundTrip()), GPG_ERR_NO_ERROR);

  InstallPassphraseScript({.unlock_default = kNewPassphrase});
  EXPECT_EQ(CheckGpgError(EncryptThenDecryptRoundTrip()), GPG_ERR_NO_ERROR);

  // ...while the primary key was left alone: it still unlocks with the old
  // passphrase. A wrong one here would fail the unlock.
  InstallPassphraseScript(
      {.unlock_default = kDefaultPassphrase, .fresh = kDefaultPassphrase});
  auto [err_p, _p] =
      KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
          .ModifySubkeyPasswordSync(key, key->Fingerprint());
  EXPECT_EQ(CheckGpgError(err_p), GPG_ERR_NO_ERROR);

  RestoreDefaultPassphraseFetcher();
}

// A mistyped current passphrase must cost a re-prompt, not the whole operation.
TEST_F(RpgpCoreTest, CoreModifySubkeyPassphraseRetriesBadUnlockTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr(kFixtureKeyFpr);
  ASSERT_TRUE(key != nullptr);

  QString encr_subkey_fpr;
  for (const auto& s_key : key->SubKeys()) {
    if (!s_key.IsHasCertCap() && s_key.IsHasEncrCap()) {
      encr_subkey_fpr = s_key.Fingerprint();
      break;
    }
  }
  ASSERT_FALSE(encr_subkey_fpr.isEmpty());

  // Get the unlock wrong once, then right -- as a user fixing a typo would.
  int unlock_attempts = 0;
  bool first_prompt_flagged_retry = true;
  bool second_prompt_flagged_retry = false;
  SetChannelPasswordFetcher(kRpgpChannelForUnitTest,
                            [&](const PassphraseState& s) -> GFBuffer {
                              if (s.ask_for_new)
                                return GFBuffer(QString(kNewPassphrase));

                              // PassphraseDialog renders this flag as "the
                              // passphrase you entered was incorrect", so the
                              // re-prompt must carry it and the first must not.
                              if (++unlock_attempts == 1) {
                                first_prompt_flagged_retry = s.retry;
                                return GFBuffer(QString("wrong-passphrase"));
                              }
                              if (unlock_attempts == 2)
                                second_prompt_flagged_retry = s.retry;
                              return GFBuffer(QString(kDefaultPassphrase));
                            });

  auto [err, _] = KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                      .ModifySubkeyPasswordSync(key, encr_subkey_fpr);
  EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_FALSE(first_prompt_flagged_retry)
      << "the initial prompt must not claim a previous attempt was wrong";
  EXPECT_TRUE(second_prompt_flagged_retry)
      << "the re-prompt must be flagged as a retry so the dialog can say so";
  EXPECT_GE(unlock_attempts, 2)
      << "the rejected passphrase was not re-prompted";

  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
  InstallPassphraseScript({.unlock_default = kNewPassphrase});
  EXPECT_EQ(CheckGpgError(EncryptThenDecryptRoundTrip()), GPG_ERR_NO_ERROR);

  RestoreDefaultPassphraseFetcher();
}

// Once the retries are spent, the user must be told the passphrase was wrong --
// not handed a bare "General error".
TEST_F(RpgpCoreTest, CoreModifySubkeyPassphraseReportsBadPassphraseTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr(kFixtureKeyFpr);
  ASSERT_TRUE(key != nullptr);

  QString encr_subkey_fpr;
  for (const auto& s_key : key->SubKeys()) {
    if (!s_key.IsHasCertCap() && s_key.IsHasEncrCap()) {
      encr_subkey_fpr = s_key.Fingerprint();
      break;
    }
  }
  ASSERT_FALSE(encr_subkey_fpr.isEmpty());

  InstallPassphraseScript(
      {.unlock_default = "always-wrong", .fresh = kNewPassphrase});

  auto [err, _] = KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                      .ModifySubkeyPasswordSync(key, encr_subkey_fpr);
  EXPECT_EQ(CheckGpgError(err), GPG_ERR_BAD_PASSPHRASE);

  RestoreDefaultPassphraseFetcher();
}

// An empty subkey fingerprint is a caller error, not a silent whole-key change.
TEST_F(RpgpCoreTest, CoreModifySubkeyPassphraseRejectsEmptyFprTest) {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr(kFixtureKeyFpr);
  ASSERT_TRUE(key != nullptr);

  auto [err, _] = KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                      .ModifySubkeyPasswordSync(key, {});
  EXPECT_EQ(CheckGpgError(err), GPG_ERR_INV_ARG);
}

namespace {
// Generate an Ed25519 primary key on the rPGP channel with the requested key
// format version and return the imported key, or nullptr on failure.
auto GenerateRpgpKeyWithVersion(int version, const QString& email)
    -> GpgKeyPtr {
  auto info = QSharedPointer<KeyGenerateInfo>::create();
  info->SetName("version_probe");
  info->SetEmail(email);

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  if (!found) return nullptr;
  info->SetAlgo(algo);

  info->SetNonExpired(true);
  info->SetNonPassPhrase(true);
  info->SetKeyVersion(version);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kRpgpChannelForUnitTest)
          .GenerateKeySync(info);

  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return nullptr;
  if (data_object->GetObjectSize() < 1) return nullptr;

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  if (!result.IsGood() || result.GetFingerprint().isEmpty()) return nullptr;

  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
  return GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
      .GetKeyPtr(result.GetFingerprint());
}
}  // namespace

TEST_F(RpgpCoreTest, GenerateV4KeyTest) {
  auto key = GenerateRpgpKeyWithVersion(4, "v4@gpgfrontend.bktus.com");
  ASSERT_TRUE(key != nullptr);
  EXPECT_EQ(key->KeyVersion(), 4);

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
}

TEST_F(RpgpCoreTest, GenerateV6KeyTest) {
  auto key = GenerateRpgpKeyWithVersion(6, "v6@gpgfrontend.bktus.com");
  ASSERT_TRUE(key != nullptr);
  EXPECT_EQ(key->KeyVersion(), 6);

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
}

namespace {
// Re-fetch a key from the repository after an operation that rewrote it. The
// fingerprint is stable across a re-signing, so it round-trips by fpr.
auto ReloadKey(const QString& fpr) -> GpgKeyPtr {
  GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushKeyCache();
  return GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest).GetKeyPtr(fpr);
}
}  // namespace

// Setting an expiration on a V4 primary key must be reflected back through the
// metadata pipeline, and clearing it must return the key to "never expires".
TEST_F(RpgpCoreTest, CoreSetPrimaryKeyExpirationV4Test) {
  auto key = GenerateRpgpKeyWithVersion(4, "v4-expire@gpgfrontend.bktus.com");
  ASSERT_TRUE(key != nullptr);
  const auto fpr = key->Fingerprint();
  EXPECT_FALSE(key->IsExpired());
  EXPECT_EQ(key->ExpirationTime().toSecsSinceEpoch(), 0);

  const auto expires = QDateTime::currentDateTimeUtc().addYears(1);
  ASSERT_EQ(
      CheckGpgError(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                        .SetExpire(key, {}, expires)),
      GPG_ERR_NO_ERROR);

  key = ReloadKey(fpr);
  ASSERT_TRUE(key != nullptr);
  EXPECT_EQ(key->ExpirationTime().toSecsSinceEpoch(),
            expires.toSecsSinceEpoch());
  EXPECT_FALSE(key->IsExpired());

  // Clearing (a null expiry) returns it to never-expires.
  ASSERT_EQ(
      CheckGpgError(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                        .SetExpire(key, {}, std::nullopt)),
      GPG_ERR_NO_ERROR);

  key = ReloadKey(fpr);
  ASSERT_TRUE(key != nullptr);
  EXPECT_EQ(key->ExpirationTime().toSecsSinceEpoch(), 0);
  EXPECT_FALSE(key->IsExpired());

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
}

// A V6 primary key carries expiration in its direct-key signature; the pipeline
// must read it back the same way as for V4.
TEST_F(RpgpCoreTest, CoreSetPrimaryKeyExpirationV6Test) {
  auto key = GenerateRpgpKeyWithVersion(6, "v6-expire@gpgfrontend.bktus.com");
  ASSERT_TRUE(key != nullptr);
  const auto fpr = key->Fingerprint();

  const auto expires = QDateTime::currentDateTimeUtc().addYears(2);
  ASSERT_EQ(
      CheckGpgError(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                        .SetExpire(key, {}, expires)),
      GPG_ERR_NO_ERROR);

  key = ReloadKey(fpr);
  ASSERT_TRUE(key != nullptr);
  EXPECT_EQ(key->ExpirationTime().toSecsSinceEpoch(),
            expires.toSecsSinceEpoch());
  EXPECT_FALSE(key->IsExpired());

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
}

// Setting an expiration on a subkey rebuilds its binding signature. The binding
// must stay valid afterwards -- encrypt/decrypt through the encryption subkey
// is the proof -- and the new expiry must be visible on that subkey.
TEST_F(RpgpCoreTest, CoreSetSubkeyExpirationPreservesBindingTest) {
  auto info =
      KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
          .ImportKey(GFBuffer(QString::fromLatin1(test_private_key_data)));
  ASSERT_TRUE(info != nullptr);

  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr(kFixtureKeyFpr);
  ASSERT_TRUE(key != nullptr);

  QString encr_subkey_fpr;
  for (const auto& s_key : key->SubKeys()) {
    if (!s_key.IsHasCertCap() && s_key.IsHasEncrCap()) {
      encr_subkey_fpr = s_key.Fingerprint();
      break;
    }
  }
  ASSERT_FALSE(encr_subkey_fpr.isEmpty());

  const auto expires = QDateTime::currentDateTimeUtc().addYears(1);
  ASSERT_EQ(
      CheckGpgError(KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest)
                        .SetExpire(key, encr_subkey_fpr, expires)),
      GPG_ERR_NO_ERROR);

  key = ReloadKey(kFixtureKeyFpr);
  ASSERT_TRUE(key != nullptr);

  bool found = false;
  for (const auto& s_key : key->SubKeys()) {
    if (s_key.Fingerprint() == encr_subkey_fpr) {
      found = true;
      EXPECT_EQ(s_key.ExpirationTime().toSecsSinceEpoch(),
                expires.toSecsSinceEpoch());
      EXPECT_FALSE(s_key.IsExpired());
      break;
    }
  }
  EXPECT_TRUE(found) << "the encryption subkey vanished after re-signing";

  // The rebuilt binding must still validate: encrypting to the key and
  // decrypting through the encryption subkey has to keep working.
  EXPECT_EQ(CheckGpgError(EncryptThenDecryptRoundTrip()), GPG_ERR_NO_ERROR);
}

namespace {
// Build an Ed25519 primary key generation request for `email` on `version`
// that expires at `expires`.
auto MakeExpiringKeyInfo(int version, const QString& email,
                         const QDateTime& expires)
    -> QSharedPointer<KeyGenerateInfo> {
  auto info = QSharedPointer<KeyGenerateInfo>::create();
  info->SetName("expire_probe");
  info->SetEmail(email);

  auto [found, algo] = KeyGenerateInfo::SearchPrimaryKeyAlgo("ed25519");
  if (!found) return nullptr;
  info->SetAlgo(algo);

  // Order matters: SetNonExpired(false) resets the stored expiry, so the
  // expiration time must be set afterwards.
  info->SetNonExpired(false);
  info->SetExpireTime(expires);
  info->SetNonPassPhrase(true);
  info->SetKeyVersion(version);
  return info;
}
}  // namespace

// A generated key must honor the requested expiration -- the rPGP builder has
// no expiration setter, so the engine stamps it after generation. V4 carries it
// in the User ID self-certification.
TEST_F(RpgpCoreTest, GenerateV4KeyWithExpirationTest) {
  const auto expires = QDateTime::currentDateTimeUtc().addYears(1);
  auto info =
      MakeExpiringKeyInfo(4, "v4-gen-expire@gpgfrontend.bktus.com", expires);
  ASSERT_TRUE(info != nullptr);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kRpgpChannelForUnitTest)
          .GenerateKeySync(info);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  auto key = ReloadKey(result.GetFingerprint());
  ASSERT_TRUE(key != nullptr);
  EXPECT_EQ(key->KeyVersion(), 4);
  EXPECT_FALSE(key->IsExpired());
  EXPECT_EQ(key->ExpirationTime().toSecsSinceEpoch(),
            expires.toSecsSinceEpoch());

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
}

// The V6 counterpart: expiration lives in the direct-key signature but must
// round-trip identically.
TEST_F(RpgpCoreTest, GenerateV6KeyWithExpirationTest) {
  const auto expires = QDateTime::currentDateTimeUtc().addYears(2);
  auto info =
      MakeExpiringKeyInfo(6, "v6-gen-expire@gpgfrontend.bktus.com", expires);
  ASSERT_TRUE(info != nullptr);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kRpgpChannelForUnitTest)
          .GenerateKeySync(info);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  auto key = ReloadKey(result.GetFingerprint());
  ASSERT_TRUE(key != nullptr);
  EXPECT_EQ(key->KeyVersion(), 6);
  EXPECT_FALSE(key->IsExpired());
  EXPECT_EQ(key->ExpirationTime().toSecsSinceEpoch(),
            expires.toSecsSinceEpoch());

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
}

// A primary+subkey generation must stamp the expiration onto both components,
// each from its own config, and the subkey binding must stay valid.
TEST_F(RpgpCoreTest, GenerateKeyWithSubkeyExpirationTest) {
  const auto p_expires = QDateTime::currentDateTimeUtc().addYears(1);
  const auto s_expires = QDateTime::currentDateTimeUtc().addYears(2);

  auto p_params =
      MakeExpiringKeyInfo(4, "gen-sub-expire@gpgfrontend.bktus.com", p_expires);
  ASSERT_TRUE(p_params != nullptr);

  auto s_params = QSharedPointer<KeyGenerateInfo>::create();
  s_params->SetIsSubKey(true);
  auto [found, s_algo] = KeyGenerateInfo::SearchSubKeyAlgo("cv25519");
  ASSERT_TRUE(found);
  s_params->SetAlgo(s_algo);
  s_params->SetNonExpired(false);
  s_params->SetExpireTime(s_expires);
  s_params->SetNonPassPhrase(true);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(kRpgpChannelForUnitTest)
          .GenerateKeyWithSubkeySync(p_params, s_params);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());
  auto key = ReloadKey(result.GetFingerprint());
  ASSERT_TRUE(key != nullptr);
  EXPECT_FALSE(key->IsExpired());
  EXPECT_EQ(key->ExpirationTime().toSecsSinceEpoch(),
            p_expires.toSecsSinceEpoch());

  // The primary key is also surfaced as a subkey entry, so pick the real
  // encryption subkey (no certify capability) rather than the front element.
  bool found_encr_sub = false;
  for (const auto& sub : key->SubKeys()) {
    if (sub.IsHasCertCap() || !sub.IsHasEncrCap()) continue;
    found_encr_sub = true;
    EXPECT_FALSE(sub.IsExpired());
    EXPECT_EQ(sub.ExpirationTime().toSecsSinceEpoch(),
              s_expires.toSecsSinceEpoch());
    break;
  }
  EXPECT_TRUE(found_encr_sub)
      << "the generated encryption subkey was not found";

  KeyManagementOperation::GetInstance(kRpgpChannelForUnitTest).DeleteKey(key);
}

}  // namespace GpgFrontend::Test
