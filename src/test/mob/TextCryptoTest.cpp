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

#include "MobileCryptoTest.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
#include "core/function/rpgp/PasswordFetcher.h"
#include "crypto/TextCryptoController.h"
#include "lifecycle/AppLifecycle.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::AppLifecycle;
using GpgFrontend::Mobile::TextCryptoController;

namespace {

constexpr auto kAlicePassphrase = "alice correct horse";
constexpr auto kMessage = "Meet at the usual place.\nBring the key.";

struct Finished {
  QString output;
  QVariantMap result;
  bool sensitive = false;
};

}  // namespace

class TextCryptoTest : public ::testing::Test {
 protected:
  static inline QString alice_;  ///< has a passphrase
  static inline QString bob_;    ///< has none

  static void SetUpTestSuite() {
    alice_ = GenerateKey("Alice", kAlicePassphrase);
    bob_ = GenerateKey("Bob", {});
  }

  void SetUp() override {
    ASSERT_FALSE(alice_.isEmpty());
    ASSERT_FALSE(bob_.isEmpty());
    ClearRpgpPasswordCache();
    RunOnMainThread([this]() { crypto_ = new TextCryptoController(); });
  }

  void TearDown() override {
    // Nothing left asking for the next test to trip over.
    RunOnMainThread([]() { Broker()->CancelAll(); });
    RunOnMainThread([this]() { delete crypto_; });
  }

  void Start(const std::function<void(TextCryptoController*)>& op) {
    RunOnMainThread([this, &op]() { op(crypto_); });
  }

  auto Busy() -> bool {
    bool busy = false;
    RunOnMainThread([this, &busy]() { busy = crypto_->Busy(); });
    return busy;
  }

  /// Wait for the operation, answering nothing.
  auto Finish(int timeout_ms = 20000) -> Finished {
    EXPECT_TRUE(WaitFor([this]() { return !Busy(); }, timeout_ms));
    Finished done;
    RunOnMainThread([this, &done]() {
      done.output = crypto_->Output();
      done.result = crypto_->Result();
      done.sensitive = crypto_->Sensitive();
    });
    return done;
  }

  TextCryptoController* crypto_ = nullptr;
};

TEST_F(TextCryptoTest, AnEncryptionNamesItsRecipients) {
  Start([](auto* c) { c->encrypt(kMessage, {bob_}); });
  const auto encrypted = Finish();
  ASSERT_EQ(encrypted.result.value("status").toInt(), 1);
  EXPECT_EQ(encrypted.result.value("recipients").toList().size(), 1);
}

TEST_F(TextCryptoTest, DecryptedTextStaysProtectedUntilCleared) {
  Start([](auto* c) { c->encrypt(kMessage, {bob_}); });
  const auto encrypted = Finish();
  ASSERT_EQ(encrypted.result.value("status").toInt(), 1);
  Start([&](auto* c) { c->decrypt(encrypted.output); });
  ASSERT_TRUE(Finish().sensitive);

  // The page shows each result in its one editor: the plaintext is still
  // there, or one Undo away, after the next operation.
  Start([](auto* c) { c->verify(kMessage); });
  EXPECT_TRUE(Finish().sensitive);

  RunOnMainThread([this]() { crypto_->clear(); });
  EXPECT_FALSE(Finish().sensitive);
}

TEST_F(TextCryptoTest, AChosenKeyThatIsGoneIsNotQuietlyLeftOut) {
  // Deleted between choosing it and pressing Encrypt: the message would go to
  // fewer people than chosen, and look no different.
  const auto gone = QStringLiteral("0123456789ABCDEF0123456789ABCDEF01234567");
  Start([&](auto* c) { c->encrypt(kMessage, {bob_, gone}); });
  const auto refused = Finish();
  EXPECT_LT(refused.result.value("status").toInt(), 0);
  EXPECT_TRUE(refused.output.isEmpty());

  Start([&](auto* c) { c->sign(kMessage, gone); });
  EXPECT_LT(Finish().result.value("status").toInt(), 0);
}

TEST_F(TextCryptoTest, EncryptToAKeyAndDecryptIt) {
  Start([](auto* c) { c->encrypt(kMessage, {bob_}); });
  const auto encrypted = Finish();
  ASSERT_EQ(encrypted.result.value("status").toInt(), 1)
      << encrypted.result.value("errors").toStringList().join("; ")
             .toStdString();
  EXPECT_TRUE(encrypted.output.startsWith("-----BEGIN PGP MESSAGE-----"));
  EXPECT_FALSE(encrypted.sensitive);

  Start([&](auto* c) { c->decrypt(encrypted.output); });
  const auto decrypted = Finish();
  EXPECT_EQ(decrypted.result.value("status").toInt(), 1)
      << decrypted.result.value("report").toString().toStdString();
  EXPECT_EQ(decrypted.result.value("recipients").toList().size(), 1);
  EXPECT_EQ(decrypted.output, QString(kMessage));
  // Plaintext on screen.
  EXPECT_TRUE(decrypted.sensitive);

  RunOnMainThread([this]() { crypto_->clear(); });
  EXPECT_TRUE(Finish().output.isEmpty());
  EXPECT_FALSE(Finish().sensitive);
}

TEST_F(TextCryptoTest, SignAndVerifyInTheClear) {
  Start([](auto* c) { c->sign(kMessage, bob_); });
  const auto signed_text = Finish();
  ASSERT_EQ(signed_text.result.value("status").toInt(), 1);
  EXPECT_TRUE(
      signed_text.output.startsWith("-----BEGIN PGP SIGNED MESSAGE-----"));

  Start([&](auto* c) { c->verify(signed_text.output); });
  const auto verified = Finish();
  const auto signatures = verified.result.value("signatures").toList();
  ASSERT_EQ(signatures.size(), 1);
  const auto signature = signatures.front().toMap();
  EXPECT_TRUE(signature.value("good").toBool());
  EXPECT_EQ(signature.value("fingerprint").toString().toUpper(),
            bob_.toUpper());

  // One changed word, and it no longer holds.
  auto tampered = signed_text.output;
  tampered.replace("usual", "other");
  Start([&](auto* c) { c->verify(tampered); });
  const auto rejected = Finish();
  const auto bad = rejected.result.value("signatures").toList();
  ASSERT_EQ(bad.size(), 1);
  EXPECT_FALSE(bad.front().toMap().value("good").toBool());
}

TEST_F(TextCryptoTest, EncryptToAPassphraseAndDecryptIt) {
  Start([](auto* c) { c->encrypt(kMessage, {}); });
  const auto asked = WaitForRequest();
  ASSERT_FALSE(asked.isEmpty());
  EXPECT_TRUE(asked.value("key").toString().isEmpty());
  Answer("a passphrase for the message");
  const auto encrypted = Finish();
  ASSERT_EQ(encrypted.result.value("status").toInt(), 1);

  ClearRpgpPasswordCache();
  Start([&](auto* c) { c->decrypt(encrypted.output); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Answer("a passphrase for the message");
  const auto decrypted = Finish();
  EXPECT_EQ(decrypted.output, QString(kMessage));
}

TEST_F(TextCryptoTest, EncryptAndSignThenDecryptAndVerify) {
  Start([](auto* c) { c->encrypt(kMessage, {bob_}, alice_); });
  // Alice's key signs, so her passphrase is asked.
  const auto asked = WaitForRequest();
  ASSERT_FALSE(asked.isEmpty());
  EXPECT_TRUE(asked.value("key").toString().contains("Alice"));
  Answer(kAlicePassphrase);
  const auto encrypted = Finish();
  ASSERT_GE(encrypted.result.value("status").toInt(), 0);

  Start([&](auto* c) { c->decrypt(encrypted.output); });
  const auto decrypted = Finish();
  EXPECT_EQ(decrypted.output, QString(kMessage));
  const auto signatures = decrypted.result.value("signatures").toList();
  ASSERT_EQ(signatures.size(), 1);
  EXPECT_TRUE(signatures.front().toMap().value("good").toBool());
}

// rPGP does not ask again after a wrong passphrase, as GnuPG does: the
// operation fails, and the wrong passphrase is not kept for the next try.
TEST_F(TextCryptoTest, AWrongPassphraseFailsAndIsNotKept) {
  Start([](auto* c) { c->encrypt(kMessage, {alice_}); });
  const auto encrypted = Finish();
  ASSERT_EQ(encrypted.result.value("status").toInt(), 1);

  Start([&](auto* c) { c->decrypt(encrypted.output); });
  const auto first = WaitForRequest();
  ASSERT_FALSE(first.isEmpty());
  EXPECT_FALSE(first.value("retry").toBool());
  Answer("not the passphrase");

  const auto failed = Finish();
  EXPECT_LT(failed.result.value("status").toInt(), 0);
  EXPECT_FALSE(failed.result.value("cancelled").toBool());
  EXPECT_TRUE(failed.output.isEmpty());

  Start([&](auto* c) { c->decrypt(encrypted.output); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Answer(kAlicePassphrase);
  EXPECT_EQ(Finish().output, QString(kMessage));
}

TEST_F(TextCryptoTest, DecliningThePassphraseCancels) {
  Start([](auto* c) { c->encrypt(kMessage, {alice_}); });
  const auto encrypted = Finish();
  ASSERT_EQ(encrypted.result.value("status").toInt(), 1);

  Start([&](auto* c) { c->decrypt(encrypted.output); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Decline();

  const auto declined = Finish();
  EXPECT_LT(declined.result.value("status").toInt(), 0);
  EXPECT_TRUE(declined.result.value("cancelled").toBool());
  EXPECT_TRUE(declined.output.isEmpty());
  EXPECT_FALSE(declined.sensitive);
}

TEST_F(TextCryptoTest, GoingToTheBackgroundCancelsAPromptAndForgets) {
  Start([](auto* c) { c->encrypt(kMessage, {alice_}); });
  const auto encrypted = Finish();
  ASSERT_EQ(encrypted.result.value("status").toInt(), 1);

  // Unlocked once, so the passphrase is cached...
  Start([&](auto* c) { c->decrypt(encrypted.output); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Answer(kAlicePassphrase);
  ASSERT_EQ(Finish().output, QString(kMessage));

  auto* lifecycle = AppLifecycle::create(nullptr, nullptr);
  ASSERT_NE(lifecycle, nullptr);
  const auto leave_and_return = [lifecycle]() {
    RunOnMainThread([lifecycle]() {
      lifecycle->OnApplicationStateChanged(Qt::ApplicationSuspended);
      lifecycle->OnApplicationStateChanged(Qt::ApplicationActive);
    });
  };

  // ...and forgotten across the background: asked once more, with nothing
  // here clearing the cache.
  leave_and_return();
  Start([&](auto* c) { c->decrypt(encrypted.output); });
  ASSERT_FALSE(WaitForRequest().isEmpty())
      << "the passphrase outlived going to the background";
  Answer(kAlicePassphrase);
  ASSERT_EQ(Finish().output, QString(kMessage));

  // A prompt on screen when the application leaves is cancelled.
  ClearRpgpPasswordCache();
  Start([&](auto* c) { c->decrypt(encrypted.output); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  RunOnMainThread([lifecycle]() {
    lifecycle->OnApplicationStateChanged(Qt::ApplicationSuspended);
  });
  const auto cancelled = Finish();
  EXPECT_TRUE(cancelled.result.value("cancelled").toBool());
  EXPECT_TRUE(CurrentRequest().isEmpty());
  RunOnMainThread([lifecycle]() {
    lifecycle->OnApplicationStateChanged(Qt::ApplicationActive);
  });
}

TEST_F(TextCryptoTest, AnUnansweredPromptEndsAsCancelledAtItsTimeout) {
  {
    auto settings = GetSettings();
    SettingItems::kPassphrasePromptTimeout.Write(settings, 1);
  }
  Start([](auto* c) { c->encrypt(kMessage, {}); });
  const auto request = WaitForRequest();
  ASSERT_FALSE(request.isEmpty());
  EXPECT_EQ(request.value("timeout").toInt(), 1);

  // Nobody answers: cancelled by the prompt's own clock, not failed by the
  // core giving up on it.
  const auto done = Finish(10000);
  EXPECT_TRUE(done.result.value("cancelled").toBool());
  EXPECT_TRUE(CurrentRequest().isEmpty());

  auto settings = GetSettings();
  SettingItems::kPassphrasePromptTimeout.Write(
      settings, SettingItems::kPassphrasePromptTimeout.Default());
}

TEST_F(TextCryptoTest, OneOperationAtATime) {
  Start([](auto* c) { c->encrypt(kMessage, {}); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  ASSERT_TRUE(Busy());

  // Ignored while the first waits at its prompt.
  Start([](auto* c) { c->encrypt("another", {bob_}); });
  Decline();
  const auto first = Finish();
  EXPECT_TRUE(first.result.value("cancelled").toBool());

  // And never run afterwards either: it needs no prompt, so it would have
  // left a message to bob by now.
  const auto ran_later = WaitFor(
      [this]() {
        QString output;
        RunOnMainThread([this, &output]() { output = crypto_->Output(); });
        return !output.isEmpty();
      },
      1000);
  EXPECT_FALSE(ran_later);
  EXPECT_FALSE(Busy());
}

}  // namespace GpgFrontend::Test::Mobile
