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
      : saved_(GpgFrontend::InstantMessageOperator::BookPhrase()) {}
  ~BookPhraseGuard() { Set(saved_); }
  static void Set(const QString& v) {
    GpgFrontend::InstantMessageOperator::SetBookPhrase(v);
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

TEST(InstantMessageOperatorTest, DecodeNotToken) {
  EXPECT_FALSE(InstantMessageOperator::Decode("hello there, friend!").ok);
}

TEST(InstantMessageOperatorTest, DecodeRejectsArmoredMessage) {
  const QString armored =
      "-----BEGIN PGP MESSAGE-----\n\nhF4Dabc=\n=abcd\n"
      "-----END PGP MESSAGE-----";
  EXPECT_FALSE(InstantMessageOperator::Decode(armored).ok);
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

// Padding is at least 30% of the frame, so the token is never a thin wrapper
// around the payload whose size can be read off its length.
TEST(InstantMessageOperatorTest, PaddingIsAtLeastThirtyPercent) {
  for (const int payload : {200, 512, 1000}) {
    const auto token =
        InstantMessageOperator::Encode(GFBuffer(PgpLikeBlob(payload)));
    ASSERT_FALSE(token.isEmpty());

    // Base58 carries log2(58) ≈ 5.858 bits per character, so the wire is at
    // most size*5.858/8 bytes; even that upper bound must exceed the payload
    // by 30%.
    const auto wire_bytes = static_cast<double>(token.size()) * 5.858 / 8.0;
    EXPECT_GT(wire_bytes, payload * 1.3)
        << "payload " << payload << " token " << token.size();
  }
}

// Token lengths are quantized: many distinct payload sizes must collapse onto
// the same handful of lengths, or the length itself fingerprints the message.
TEST(InstantMessageOperatorTest, LengthsQuantizeOntoALadder) {
  QSet<qsizetype> lengths;
  for (int payload = 200; payload < 260; ++payload) {
    const auto token =
        InstantMessageOperator::Encode(GFBuffer(PgpLikeBlob(payload)));
    ASSERT_FALSE(token.isEmpty());
    lengths.insert(token.size());
  }

  // 60 consecutive payload sizes; unpadded these would be ~60 distinct token
  // lengths. The ladder (plus the ±1 char Base58 wobble) must fold them into
  // far fewer rungs.
  EXPECT_LT(lengths.size(), 20) << "distinct lengths: " << lengths.size();
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
}

TEST(InstantMessageOperatorTest, FormatVersionIsStable) {
  EXPECT_EQ(InstantMessageOperator::FormatVersion(), 3);
}

// ---------------------------------------------------------------------------
// Shape pre-filter. Decode() is called speculatively on whatever the user has
// in the editor, so anything that is not plausibly a token must be rejected on
// its public shape alone — before the O(n^2) decoder and the 128 MiB KDF.
// ---------------------------------------------------------------------------

// Too short to hold a seed and a frame, so it cannot be a token whatever the
// book is.
TEST(InstantMessageOperatorTest, ShortInputIsRejected) {
  EXPECT_FALSE(InstantMessageOperator::Decode(QStringLiteral("hi")).ok);
  EXPECT_FALSE(InstantMessageOperator::Decode(QString(37, 'z')).ok);
}

// One character outside the alphabet disqualifies the whole input. Pins that
// the look-alikes Base58 excludes really are excluded, and that the characters
// a messenger might inject are not silently tolerated.
TEST(InstantMessageOperatorTest, NonBase58InputIsRejected) {
  for (const QChar bad : {QChar('0'), QChar('O'), QChar('I'), QChar('l'),
                          QChar('+'), QChar('/'), QChar('='), QChar('.')}) {
    QString s(200, 'z');
    s[100] = bad;
    EXPECT_FALSE(InstantMessageOperator::Decode(s).ok)
        << "accepted a token containing '" << bad.toLatin1() << "'";
  }
}

// The regression guard for the whole point of the pre-filter: a big paste of
// ordinary text must not reach the decoder or the KDF. A single Argon2id at
// 128 MiB is ~100-200ms, and the old uncapped O(n^2) Base58 decode was far
// worse, so the bound is loose enough not to be flaky but far below either.
TEST(InstantMessageOperatorTest, PreFilterIsFastOnLargeInput) {
  const QString prose = QStringLiteral("the quick brown fox. ").repeated(50000);
  const QString base58_only = QString(qsizetype{1024} * 1024, 'z');

  QElapsedTimer timer;
  timer.start();
  EXPECT_FALSE(InstantMessageOperator::Decode(prose).ok);
  EXPECT_FALSE(InstantMessageOperator::Decode(base58_only).ok);
  EXPECT_LT(timer.elapsed(), 500) << "elapsed: " << timer.elapsed() << "ms";
}

// ---------------------------------------------------------------------------
// Encode/Decode length agreement. If Encode() could emit a token longer than
// Decode() accepts, two copies of GpgFrontend would silently fail to talk to
// each other — the worst possible failure for this feature.
// ---------------------------------------------------------------------------

TEST(InstantMessageOperatorTest, EncodeRejectsOversizedPayload) {
  const auto over = InstantMessageOperator::MaxPayloadBytes() + 1;
  EXPECT_TRUE(
      InstantMessageOperator::Encode(GFBuffer(PgpLikeBlob(over))).isEmpty());
}

// The padding fraction and the ladder rung are both drawn from the keystream,
// so the worst case only shows up across many messages. Every one of them must
// stay inside what Decode() accepts and round-trip exactly.
TEST(InstantMessageOperatorTest, MaximumPayloadAlwaysRoundTrips) {
  const QByteArray blob =
      PgpLikeBlob(InstantMessageOperator::MaxPayloadBytes());

  for (int i = 0; i < 10; ++i) {
    const auto token = InstantMessageOperator::Encode(GFBuffer(blob));
    ASSERT_FALSE(token.isEmpty()) << "iteration " << i;

    const auto r = InstantMessageOperator::Decode(token);
    EXPECT_TRUE(r.ok) << "iteration " << i << " token " << token.size();
    EXPECT_EQ(r.pgp_message.ConvertToQByteArray(), blob) << "iteration " << i;
  }
}

// The payload limit is part of the wire contract between versions: a token one
// build emits must decode on another. Pinned so a change is a deliberate,
// reviewed act rather than a silent interop break.
TEST(InstantMessageOperatorTest, MaxPayloadBytesIsPinned) {
  EXPECT_EQ(InstantMessageOperator::MaxPayloadBytes(), 7000);
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

// The phrase is a secret: it round-trips through the encrypted durable cache
// and must never be left sitting in the plaintext settings file.
TEST(InstantMessageOperatorTest, BookPhraseStoredOutsideSettings) {
  BookPhraseGuard guard;

  BookPhraseGuard::Set(QStringLiteral("correct horse battery staple"));
  EXPECT_EQ(InstantMessageOperator::BookPhrase(),
            QStringLiteral("correct horse battery staple"));
  EXPECT_FALSE(GetSettings().contains(kPhraseKey));

  // Whitespace is trimmed on the way in, and a blank phrase clears the secret.
  BookPhraseGuard::Set(QStringLiteral("  spaced out  "));
  EXPECT_EQ(InstantMessageOperator::BookPhrase(), QStringLiteral("spaced out"));

  BookPhraseGuard::Set(QStringLiteral("   "));
  EXPECT_TRUE(InstantMessageOperator::BookPhrase().isEmpty());
  EXPECT_FALSE(InstantMessageOperator::BookConfigured());
}

// The default book ships in every copy of GpgFrontend, so every copy must
// build byte-identically the same one, or two users with no phrase set
// could not read each other. Pinned so it cannot drift unnoticed.
TEST(InstantMessageOperatorTest, DefaultBookIsPinned) {
  BookPhraseGuard guard;
  BookPhraseGuard::Set(QString());
  EXPECT_EQ(InstantMessageOperator::BookFingerprint(),
            QStringLiteral("9138-57AC"));
}

// The generated phrase is long, alphanumeric enough to survive any chat app,
// and never repeats.
TEST(InstantMessageOperatorTest, GeneratePhraseIsLongAndUnique) {
  const auto a = InstantMessageOperator::GeneratePhrase();
  const auto b = InstantMessageOperator::GeneratePhrase();

  EXPECT_EQ(a.size(), 256);
  EXPECT_EQ(b.size(), 256);
  EXPECT_NE(a, b);

  // Base58: alphanumeric minus the look-alikes 0 O I l.
  EXPECT_TRUE(QRegularExpression(QRegularExpression::anchoredPattern(
                                     QStringLiteral("[1-9A-HJ-NP-Za-km-z]+")))
                  .match(a)
                  .hasMatch());

  // It is usable as a phrase: applying it produces its own distinct book.
  BookPhraseGuard guard;
  BookPhraseGuard::Set(a);
  EXPECT_EQ(InstantMessageOperator::BookPhrase(), a);
  EXPECT_EQ(InstantMessageOperator::BookFingerprint(),
            InstantMessageOperator::BookFingerprintOf(a));
}

// A phrase written to the settings file by an older version is moved into the
// secure store on first read, and erased from settings.
TEST(InstantMessageOperatorTest, LegacyPlaintextPhraseIsMigrated) {
  BookPhraseGuard guard;
  BookPhraseGuard::Set(QString());

  GetSettings().setValue(kPhraseKey, QStringLiteral("legacy phrase"));
  ASSERT_TRUE(GetSettings().contains(kPhraseKey));

  EXPECT_EQ(InstantMessageOperator::BookPhrase(),
            QStringLiteral("legacy phrase"));
  EXPECT_FALSE(GetSettings().contains(kPhraseKey));
  // And it survives once the settings copy is gone.
  EXPECT_EQ(InstantMessageOperator::BookPhrase(),
            QStringLiteral("legacy phrase"));
}

// The settings UI previews the fingerprint of a phrase before applying it, so
// BookFingerprintOf() must agree with BookFingerprint() without touching the
// configured phrase.
TEST(InstantMessageOperatorTest, BookFingerprintOfPreviewsWithoutApplying) {
  BookPhraseGuard guard;

  BookPhraseGuard::Set(QStringLiteral("correct horse battery staple"));
  const auto active = InstantMessageOperator::BookFingerprint();

  EXPECT_EQ(InstantMessageOperator::BookFingerprintOf(
                QStringLiteral("correct horse battery staple")),
            active);
  // Whitespace is trimmed the same way the setter trims it.
  EXPECT_EQ(InstantMessageOperator::BookFingerprintOf(
                QStringLiteral("  correct horse battery staple ")),
            active);

  const auto other =
      InstantMessageOperator::BookFingerprintOf(QStringLiteral("some other"));
  EXPECT_NE(other, active);

  // Previewing did not change what is configured.
  EXPECT_EQ(InstantMessageOperator::BookPhrase(),
            QStringLiteral("correct horse battery staple"));
  EXPECT_EQ(InstantMessageOperator::BookFingerprint(), active);
}

}  // namespace GpgFrontend::Test
