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

#include <QJsonDocument>

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
constexpr auto kEmptyPassphrase = "empty message";
constexpr auto kEmptyMessage =
    "-----BEGIN PGP MESSAGE-----\n\n"
    "jA0ECQMK3eIjRMa4iolg0jUBKuEMDoPt/DYIZHp6kjDY7qGNLdPPXdVJO+rR61S/\n"
    "CgLfFhNMgaW3uBpxnFz5kampRYtqZA==\n"
    "=Ce8i\n"
    "-----END PGP MESSAGE-----\n";

struct Finished {
  QString output;
  QVariantMap result;
  bool sensitive = false;
  bool replaces = false;
  int id = 0;
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
      done.replaces = crypto_->ReplacesText();
      done.id = crypto_->FinishedId();
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

// Refused before it reaches the engine, an operation ends inside the
// queue's Submit(): its end is still told after encrypt() returned, when the
// page knows which operation is its own and waits for it. Told at once, it
// was told to nobody, and the page showed nothing at all.
TEST_F(TextCryptoTest, AnOperationRefusedAtOnceIsToldAfterItStarted) {
  const auto gone = QStringLiteral("0123456789ABCDEF0123456789ABCDEF01234567");
  bool returned = false;
  int told = 0;
  bool told_after = false;
  int started_id = 0;
  int told_id = 0;
  QMetaObject::Connection connection;
  RunOnMainThread([&]() {
    connection = QObject::connect(
        crypto_, &TextCryptoController::SignalFinished, [&](bool) {
          ++told;
          told_after = returned;
          told_id = crypto_->FinishedId();
        });
    EXPECT_TRUE(crypto_->encrypt(kMessage, {gone}));
    returned = true;
    started_id = crypto_->OperationId();
  });
  const auto refused = Finish();
  RunOnMainThread([&]() { QObject::disconnect(connection); });
  EXPECT_LT(refused.result.value("status").toInt(), 0);
  EXPECT_EQ(told, 1);
  EXPECT_TRUE(told_after);
  EXPECT_NE(started_id, 0);
  EXPECT_EQ(told_id, started_id);
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
  // Three facts apart: it holds, the key is fine, and nothing vouched for
  // who bob is -- rPGP weighs no one's identity.
  EXPECT_EQ(signature.value("signature").toString(), "valid");
  EXPECT_EQ(signature.value("keyStatus").toString(), "ok");
  EXPECT_EQ(signature.value("identityTrust").toString(), "not_assessed");

  // One changed word, and it no longer holds.
  auto tampered = signed_text.output;
  tampered.replace("usual", "other");
  Start([&](auto* c) { c->verify(tampered); });
  const auto rejected = Finish();
  const auto bad = rejected.result.value("signatures").toList();
  ASSERT_EQ(bad.size(), 1);
  EXPECT_EQ(bad.front().toMap().value("signature").toString(), "invalid");
  EXPECT_FALSE(bad.front().toMap().value("good").toBool());
}

TEST_F(TextCryptoTest, EncryptToAPassphraseAndDecryptIt) {
  Start([](auto* c) { c->encrypt(kMessage, {}, {}, true); });
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

  // Decrypt alone, as the desktop's Decrypt: the same text, and the
  // signature inside not looked at.
  Start([&](auto* c) { c->decrypt(encrypted.output, false); });
  const auto alone = Finish();
  EXPECT_EQ(alone.result.value("status").toInt(), 1);
  EXPECT_EQ(alone.output, QString(kMessage));
  EXPECT_TRUE(alone.result.value("signatures").toList().isEmpty());
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
  // A failure makes no text to put in place of the message.
  EXPECT_FALSE(failed.replaces);

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
  EXPECT_FALSE(declined.replaces);

  // Cancelled, then asked again: it runs, and this time it decrypts.
  Start([&](auto* c) { c->decrypt(encrypted.output); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Answer(kAlicePassphrase);
  const auto again = Finish();
  EXPECT_EQ(again.output, QString(kMessage));
  EXPECT_TRUE(again.replaces);
}

// Whether the text is replaced is said by how the operation ended, not
// guessed from the text it made.
TEST_F(TextCryptoTest, WhatReplacesTheTextIsSaidNotGuessed) {
  // An encryption and a signature make text to put in place of the text.
  Start([](auto* c) { c->encrypt(kMessage, {bob_}, bob_); });
  const auto encrypted = Finish();
  ASSERT_EQ(encrypted.result.value("status").toInt(), 1);
  EXPECT_TRUE(encrypted.replaces);

  Start([](auto* c) { c->sign(kMessage, bob_); });
  const auto signed_text = Finish();
  EXPECT_TRUE(signed_text.replaces);

  // A verification only says whether the signature holds.
  Start([&](auto* c) { c->verify(signed_text.output); });
  const auto verified = Finish();
  EXPECT_EQ(verified.result.value("status").toInt(), 1);
  EXPECT_FALSE(verified.replaces);

  // An empty message decrypts to nothing: that is its text, and it is put
  // in all the same. (The engine refuses to encrypt nothing; GnuPG made
  // this one: `gpg --symmetric --armor` of empty input.)
  Start([](auto* c) { c->decrypt(kEmptyMessage); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Answer(kEmptyPassphrase);
  const auto nothing = Finish();
  EXPECT_GE(nothing.result.value("status").toInt(), 0)
      << nothing.result.value("description").toString().toStdString();
  EXPECT_TRUE(nothing.output.isEmpty());
  EXPECT_TRUE(nothing.replaces);
}

// A result kept back while the person decides is that operation's, and
// goes once taken, released or cleared.
TEST_F(TextCryptoTest, AKeptResultIsThatOperationsOnly) {
  Start([](auto* c) { c->encrypt(kMessage, {bob_}); });
  const auto first = Finish();
  ASSERT_TRUE(first.replaces);
  RunOnMainThread([&]() {
    EXPECT_TRUE(crypto_->hold(first.id));
    EXPECT_FALSE(crypto_->hold(first.id + 1000));
  });

  Start([](auto* c) { c->sign(kMessage, bob_); });
  const auto second = Finish();
  ASSERT_NE(second.id, first.id);
  RunOnMainThread([&]() {
    // The first one's, not the latest one's.
    EXPECT_EQ(crypto_->take(first.id), first.output);
    EXPECT_FALSE(crypto_->isHeld(first.id));
    EXPECT_TRUE(crypto_->take(first.id).isEmpty());
    EXPECT_TRUE(crypto_->hold(second.id));
    crypto_->clear();
    EXPECT_FALSE(crypto_->isHeld(second.id));
  });
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
  Start([](auto* c) { c->encrypt(kMessage, {}, {}, true); });
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
  Start([](auto* c) { c->encrypt(kMessage, {}, {}, true); });
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

TEST_F(TextCryptoTest, TheLogKeepsNoText) {
  Start([](auto* c) { c->encrypt(kMessage, {bob_}); });
  const auto encrypted = Finish();
  ASSERT_EQ(encrypted.result.value("status").toInt(), 1);
  Start([&](auto* c) { c->decrypt(encrypted.output); });
  const auto decrypted = Finish();
  ASSERT_EQ(decrypted.output, kMessage);

  const auto record = FindRecord([](const QVariantMap& r) {
    return r.value("operation").toString() == "decrypt";
  });
  ASSERT_FALSE(record.isEmpty());
  EXPECT_EQ(record.value("phase").toString(), "done");
  EXPECT_EQ(record.value("report").toMap().value("status").toInt(), 1);
  // The text it read is known by its version alone, and nothing it wrote
  // is anywhere in the record.
  EXPECT_EQ(record.value("targets").toList().first().toMap().value("fileId"),
            -1);
  const auto dumped = QJsonDocument::fromVariant(record).toJson();
  EXPECT_FALSE(dumped.contains("Meet at the usual place"));
  EXPECT_FALSE(dumped.contains("BEGIN PGP MESSAGE"));
}

TEST_F(TextCryptoTest, ADeclinedPromptIsACancelledRecord) {
  Start([](auto* c) { c->encrypt(kMessage, {}, {}, true); });
  const auto request = WaitForRequest();
  ASSERT_FALSE(request.isEmpty());
  EXPECT_EQ(request.value("operation").toString(), "encrypt");
  Decline();
  Finish();
  const auto record = FindRecord([&request](const QVariantMap& r) {
    return r.value("operationId") == request.value("operationId");
  });
  EXPECT_EQ(record.value("phase").toString(), "cancelled");
  EXPECT_TRUE(record.value("symmetric").toBool());
}

TEST_F(TextCryptoTest, NoRecipientsIsNeverAPassphrase) {
  bool started = true;
  Start([&started](auto* c) { started = c->encrypt(kMessage, {}); });
  EXPECT_FALSE(started);
  EXPECT_FALSE(Busy());
  EXPECT_TRUE(CurrentRequest().isEmpty());

  // A passphrase only when asked for, and then no recipient is used.
  Start([&started](auto* c) {
    started = c->encrypt(kMessage, {bob_}, alice_, true);
  });
  EXPECT_TRUE(started);
  const auto request = WaitForRequest();
  ASSERT_FALSE(request.isEmpty());
  const auto record = FindRecord([&request](const QVariantMap& r) {
    return r.value("operationId") == request.value("operationId");
  });
  EXPECT_TRUE(record.value("symmetric").toBool());
  EXPECT_TRUE(record.value("recipients").toStringList().isEmpty());
  EXPECT_TRUE(record.value("signer").toString().isEmpty());
  Decline();
  Finish();
}

TEST_F(TextCryptoTest, AResultKnowsWhichTextIsItsOwn) {
  Start([](auto* c) { c->encrypt(kMessage, {bob_}); });
  const auto encrypted = Finish();
  ASSERT_FALSE(encrypted.output.isEmpty());
  int id = 0;
  RunOnMainThread([this, &id]() { id = crypto_->OperationId(); });
  ASSERT_NE(id, 0);

  QString input;
  QString output;
  QString edited;
  QString other;
  RunOnMainThread([&]() {
    auto* log = GpgFrontend::Mobile::OperationQueue::Instance()->Log();
    input = log->textRelation(id, kMessage);
    output = log->textRelation(id, encrypted.output);
    edited = log->textRelation(id, encrypted.output + "x");
    other = log->textRelation(id + 1000, kMessage);
  });
  EXPECT_EQ(input, "input");
  EXPECT_EQ(output, "output");
  EXPECT_TRUE(edited.isEmpty());
  EXPECT_TRUE(other.isEmpty());
}

TEST_F(TextCryptoTest, ARetryIsAsItWasAsked) {
  Start([](auto* c) { c->encrypt(kMessage, {}, {}, true); });
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Decline();
  ASSERT_TRUE(Finish().result.value("cancelled").toBool());
  int first = 0;
  RunOnMainThread([&]() { first = crypto_->OperationId(); });

  bool retried = false;
  Start([&](auto* c) { retried = c->retry(first); });
  ASSERT_TRUE(retried);
  ASSERT_FALSE(WaitForRequest().isEmpty());
  Answer("a retried passphrase");
  const auto done = Finish();
  EXPECT_EQ(done.result.value("status").toInt(), 1);
  EXPECT_TRUE(done.output.startsWith("-----BEGIN PGP MESSAGE-----"));

  int second = 0;
  RunOnMainThread([&]() { second = crypto_->OperationId(); });
  EXPECT_NE(second, first);
  const auto record = FindRecord([second](const QVariantMap& r) {
    return r.value("operationId") == second;
  });
  EXPECT_TRUE(record.value("symmetric").toBool());
  EXPECT_EQ(record.value("targets").toList().first().toMap().value("fileId"),
            -1);

  // The earlier one's text is no longer held; once cleared, none is.
  Start([&](auto* c) { retried = c->retry(first); });
  EXPECT_FALSE(retried);
  RunOnMainThread([this]() { crypto_->clear(); });
  Start([&](auto* c) { retried = c->retry(second); });
  EXPECT_FALSE(retried);
}

}  // namespace GpgFrontend::Test::Mobile
