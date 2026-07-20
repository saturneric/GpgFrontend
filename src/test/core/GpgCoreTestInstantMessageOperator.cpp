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

#include <QByteArray>
#include <QString>

#include "core/function/GlobalSettingStation.h"
#include "core/function/InstantMessageOperator.h"

namespace {

// A blob shaped like an encrypted OpenPGP message (old-format PKESK, tag 1).
auto PgpLikeBlob(int len = 200, uint8_t first = 0x84) -> QByteArray {
  QByteArray b;
  b.append(static_cast<char>(first));
  for (int i = 1; i < len; ++i) {
    b.append(static_cast<char>(((i * 37) + 11) & 0xFF));
  }
  return b;
}

constexpr auto kPhraseKey = "im/password_book_phrase";

// Save/restore the Message Book phrase so tests that change it stay isolated.
class BookPhraseGuard {
 public:
  BookPhraseGuard()
      : saved_(GpgFrontend::GetSettings().value(kPhraseKey).toString()) {}
  ~BookPhraseGuard() {
    GpgFrontend::GetSettings().setValue(kPhraseKey, saved_);
  }
  static void Set(const QString& v) {
    GpgFrontend::GetSettings().setValue(kPhraseKey, v);
  }

 private:
  QString saved_;
};

}  // namespace

namespace GpgFrontend::Test {

// A token wraps and recovers the OpenPGP message verbatim.
TEST(InstantMessageOperatorTest, NormalRoundTrip) {
  const QByteArray blob = PgpLikeBlob();

  const auto token = InstantMessageOperator::Encode(GFBuffer(blob));
  ASSERT_FALSE(token.isEmpty());
  // Single Base58 word (alphanumeric minus 0 O I l), no markdown/link chars.
  EXPECT_TRUE(QRegularExpression(QRegularExpression::anchoredPattern(
                                     QStringLiteral("[1-9A-HJ-NP-Za-km-z]+")))
                  .match(token)
                  .hasMatch());

  const auto r = InstantMessageOperator::Decode(token);
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(r.pgp_message.ConvertToQByteArray(), blob);
}

TEST(InstantMessageOperatorTest, EncodeEmptyIsEmpty) {
  EXPECT_TRUE(InstantMessageOperator::Encode(GFBuffer()).isEmpty());
}

TEST(InstantMessageOperatorTest, ContainsAndDispatch) {
  const auto token = InstantMessageOperator::Encode(GFBuffer(PgpLikeBlob()));
  EXPECT_TRUE(InstantMessageOperator::Contains(token));
  EXPECT_FALSE(InstantMessageOperator::Contains("just a normal chat line"));
}

TEST(InstantMessageOperatorTest, DecodeNotToken) {
  EXPECT_FALSE(InstantMessageOperator::Decode("hello there, friend!").ok);
}

TEST(InstantMessageOperatorTest, DecodeRejectsArmoredMessage) {
  const QString armored =
      "-----BEGIN PGP MESSAGE-----\n\nhF4Dabc=\n=abcd\n"
      "-----END PGP MESSAGE-----";
  EXPECT_FALSE(InstantMessageOperator::Decode(armored).ok);
  EXPECT_FALSE(InstantMessageOperator::Contains(armored));
}

// A messenger may wrap the token across lines / inject spaces; still decodes.
TEST(InstantMessageOperatorTest, DecodeToleratesWhitespace) {
  const QByteArray blob = PgpLikeBlob();
  auto token = InstantMessageOperator::Encode(GFBuffer(blob));
  token.insert(token.size() / 2, QStringLiteral("\n  "));

  const auto r = InstantMessageOperator::Decode(token);
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(r.pgp_message.ConvertToQByteArray(), blob);
}

// No stable wire marker: the same message encodes differently every time (fresh
// random seed), so two tokens are never byte-identical.
TEST(InstantMessageOperatorTest, WhitenedTokenVariesPerMessage) {
  const QByteArray blob = PgpLikeBlob();
  const auto a = InstantMessageOperator::Encode(GFBuffer(blob));
  const auto b = InstantMessageOperator::Encode(GFBuffer(blob));
  ASSERT_FALSE(a.isEmpty());
  ASSERT_FALSE(b.isEmpty());
  EXPECT_NE(a, b);
  // Both still decode to the same payload.
  EXPECT_EQ(InstantMessageOperator::Decode(a).pgp_message.ConvertToQByteArray(),
            blob);
  EXPECT_EQ(InstantMessageOperator::Decode(b).pgp_message.ConvertToQByteArray(),
            blob);
}

// Random padding hides the true length: even a 1-byte payload produces a token
// far larger than its content.
TEST(InstantMessageOperatorTest, LengthIsPadded) {
  const auto token =
      InstantMessageOperator::Encode(GFBuffer(QByteArray(1, '\x84')));
  ASSERT_FALSE(token.isEmpty());
  EXPECT_GT(token.size(), 60);
}

// A token whitened under one Message Book phrase is indistinguishable from
// random under a different phrase — it neither decodes nor is recognised.
TEST(InstantMessageOperatorTest, WrongBookDoesNotDecode) {
  BookPhraseGuard guard;
  const QByteArray blob = PgpLikeBlob();

  BookPhraseGuard::Set(QStringLiteral("correct horse battery staple"));
  const auto token = InstantMessageOperator::Encode(GFBuffer(blob));
  ASSERT_FALSE(token.isEmpty());
  EXPECT_TRUE(InstantMessageOperator::Decode(token).ok);

  BookPhraseGuard::Set(QStringLiteral("a completely different phrase"));
  EXPECT_FALSE(InstantMessageOperator::Decode(token).ok);
  EXPECT_FALSE(InstantMessageOperator::Contains(token));
}

TEST(InstantMessageOperatorTest, FormatVersionIsStable) {
  EXPECT_EQ(InstantMessageOperator::FormatVersion(), 2);
}

// The fingerprint identifies the book: stable for one phrase, different across
// phrases, and short enough to compare by eye.
TEST(InstantMessageOperatorTest, BookFingerprintIdentifiesBook) {
  BookPhraseGuard guard;

  BookPhraseGuard::Set(QStringLiteral("correct horse battery staple"));
  const auto fpr = InstantMessageOperator::BookFingerprint();

  // "XXXX-XXXX" of uppercase hex.
  EXPECT_EQ(fpr.size(), 9);
  EXPECT_EQ(fpr.at(4), QLatin1Char('-'));
  EXPECT_TRUE(QRegularExpression(QStringLiteral("^[0-9A-F]{4}-[0-9A-F]{4}$"))
                  .match(fpr)
                  .hasMatch());

  // Stable for the same phrase.
  EXPECT_EQ(InstantMessageOperator::BookFingerprint(), fpr);

  // A different phrase is a different book.
  BookPhraseGuard::Set(QStringLiteral("a completely different phrase"));
  const auto other = InstantMessageOperator::BookFingerprint();
  EXPECT_NE(other, fpr);

  // The default book is its own distinct, constant book.
  BookPhraseGuard::Set(QString());
  const auto def = InstantMessageOperator::BookFingerprint();
  EXPECT_NE(def, fpr);
  EXPECT_NE(def, other);

  // Returning to the first phrase reproduces the first fingerprint — this is
  // what makes it usable as a shared check value between peers.
  BookPhraseGuard::Set(QStringLiteral("correct horse battery staple"));
  EXPECT_EQ(InstantMessageOperator::BookFingerprint(), fpr);
}

// The result card reports whether a shared phrase backs the book; a blank or
// whitespace-only phrase means the default book, which is only obfuscation.
TEST(InstantMessageOperatorTest, BookConfiguredTracksPhrase) {
  BookPhraseGuard guard;

  BookPhraseGuard::Set(QString());
  EXPECT_FALSE(InstantMessageOperator::BookConfigured());

  BookPhraseGuard::Set(QStringLiteral("   \t "));
  EXPECT_FALSE(InstantMessageOperator::BookConfigured());

  BookPhraseGuard::Set(QStringLiteral("correct horse battery staple"));
  EXPECT_TRUE(InstantMessageOperator::BookConfigured());
}

}  // namespace GpgFrontend::Test
