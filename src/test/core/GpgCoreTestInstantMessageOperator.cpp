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

#include "core/function/InstantMessageOperator.h"

namespace {

// A blob shaped like an encrypted OpenPGP message: first byte is an old-format
// PKESK (public-key encrypted session key, tag 1) packet header, as produced by
// GnuPG (this is exactly why real armored messages begin "hF4D...").
auto PgpLikeBlob(uint8_t first = 0x84) -> QByteArray {
  QByteArray b;
  b.append(static_cast<char>(first));
  for (int i = 1; i < 200; ++i) {
    b.append(static_cast<char>(((i * 37) + 11) & 0xFF));
  }
  return b;
}

}  // namespace

namespace GpgFrontend::Test {

TEST(InstantMessageOperatorTest, EncodeDetectRoundTrip) {
  const QByteArray blob = PgpLikeBlob();

  const auto token = InstantMessageOperator::Encode(GFBuffer(blob));
  // No marker/prefix, single-line, and Base58 (alphanumeric minus 0 O I l): no
  // '-', '_', '+', '/', '=' that a messenger could format, link, or wrap on.
  EXPECT_TRUE(QRegularExpression(QRegularExpression::anchoredPattern(
                                     QStringLiteral("[1-9A-HJ-NP-Za-km-z]+")))
                  .match(token)
                  .hasMatch());

  GFBuffer out;
  ASSERT_TRUE(InstantMessageOperator::Detect(token, out));
  EXPECT_EQ(out.ConvertToQByteArray(), blob);
}

TEST(InstantMessageOperatorTest, EncodeEmptyIsEmpty) {
  EXPECT_TRUE(InstantMessageOperator::Encode(GFBuffer()).isEmpty());
}

// The random layer: the same input encodes differently each time (per-message
// salt), yet both tokens recover the identical raw message.
TEST(InstantMessageOperatorTest, RandomLayerVariesButRoundTrips) {
  const QByteArray blob = PgpLikeBlob();

  const auto a = InstantMessageOperator::Encode(GFBuffer(blob));
  const auto b = InstantMessageOperator::Encode(GFBuffer(blob));
  EXPECT_NE(a, b);

  GFBuffer out_a;
  GFBuffer out_b;
  ASSERT_TRUE(InstantMessageOperator::Detect(a, out_a));
  ASSERT_TRUE(InstantMessageOperator::Detect(b, out_b));
  EXPECT_EQ(out_a.ConvertToQByteArray(), blob);
  EXPECT_EQ(out_b.ConvertToQByteArray(), blob);
}

// The versioned envelope and password-book hash reference are stable, so a
// message encoded on one machine decodes on another.
TEST(InstantMessageOperatorTest, VersionAndBookIdAreStable) {
  EXPECT_EQ(InstantMessageOperator::FormatVersion(), 1);

  const auto id = InstantMessageOperator::ActiveBookId();
  EXPECT_EQ(id.size(), 4);
  EXPECT_EQ(id, InstantMessageOperator::ActiveBookId());
}

TEST(InstantMessageOperatorTest, DetectNone) {
  GFBuffer out;
  EXPECT_FALSE(InstantMessageOperator::Detect("just a normal chat line", out));
  EXPECT_FALSE(InstantMessageOperator::Contains("nothing to see here"));
}

TEST(InstantMessageOperatorTest, DetectToleratesWhitespace) {
  auto token = InstantMessageOperator::Encode(GFBuffer(PgpLikeBlob()));
  // Simulate a messenger wrapping the token across lines / adding spaces.
  token.insert(token.size() / 2, QStringLiteral("\n  "));

  GFBuffer out;
  ASSERT_TRUE(InstantMessageOperator::Detect(token, out));
  EXPECT_EQ(out.ConvertToQByteArray(), PgpLikeBlob());
}

// A token whose book matches (default) but whose payload is not an OpenPGP
// message is reported as malformed, not as a generic non-token.
TEST(InstantMessageOperatorTest, InspectMalformedWhenPayloadNotPgp) {
  QByteArray not_pgp;
  not_pgp.append('\x00');  // no packet-header high bit
  for (int i = 1; i < 200; ++i) not_pgp.append(static_cast<char>(i));

  const auto token = InstantMessageOperator::Encode(GFBuffer(not_pgp));
  GFBuffer out;
  EXPECT_FALSE(InstantMessageOperator::Detect(token, out));
  EXPECT_EQ(InstantMessageOperator::Inspect(token, out),
            InstantMessageOperator::DetectStatus::kMALFORMED);
}

// Ordinary text and a valid token classify as kNotToken / kOk respectively.
TEST(InstantMessageOperatorTest, InspectStatuses) {
  GFBuffer out;
  EXPECT_EQ(InstantMessageOperator::Inspect("hello there, friend!", out),
            InstantMessageOperator::DetectStatus::kNOT_TOKEN);

  const auto token = InstantMessageOperator::Encode(GFBuffer(PgpLikeBlob()));
  EXPECT_EQ(InstantMessageOperator::Inspect(token, out),
            InstantMessageOperator::DetectStatus::kOK);
}

TEST(InstantMessageOperatorTest, DetectRejectsArmoredMessage) {
  const QString armored =
      "-----BEGIN PGP MESSAGE-----\n\nhF4Dabc=\n=abcd\n"
      "-----END PGP MESSAGE-----";
  GFBuffer out;
  EXPECT_FALSE(InstantMessageOperator::Detect(armored, out));
}

TEST(InstantMessageOperatorTest, DetectRejectsShortLookAlike) {
  const auto token =
      InstantMessageOperator::Encode(GFBuffer(PgpLikeBlob(0x84)));
  // Truncate to a handful of chars: too short to be a real message.
  GFBuffer out;
  EXPECT_FALSE(InstantMessageOperator::Detect(token.left(8), out));
}

// The symmetric (SKESK, tag 3) leading byte is also accepted.
TEST(InstantMessageOperatorTest, DetectAcceptsSymmetricLeadingByte) {
  const auto token =
      InstantMessageOperator::Encode(GFBuffer(PgpLikeBlob(0xC3)));
  GFBuffer out;
  EXPECT_TRUE(InstantMessageOperator::Detect(token, out));
}

}  // namespace GpgFrontend::Test
