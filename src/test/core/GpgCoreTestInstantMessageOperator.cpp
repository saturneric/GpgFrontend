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

// A blob shaped like an encrypted OpenPGP message (old-format PKESK, tag 1).
auto PgpLikeBlob(uint8_t first = 0x84) -> QByteArray {
  QByteArray b;
  b.append(static_cast<char>(first));
  for (int i = 1; i < 200; ++i) {
    b.append(static_cast<char>(((i * 37) + 11) & 0xFF));
  }
  return b;
}

// NORMAL encode/decode need no OpenPGP engine when no bundle is attached, so a
// dummy channel is fine for these container-level tests.
constexpr int kCh = 0;

}  // namespace

namespace GpgFrontend::Test {

using DecodeStatus = InstantMessageOperator::DecodeStatus;

// A NORMAL token (no bundle) wraps and recovers the OpenPGP message verbatim.
TEST(InstantMessageOperatorTest, NormalRoundTrip) {
  const QByteArray blob = PgpLikeBlob();

  const auto token =
      InstantMessageOperator::EncodeNormal(kCh, {}, GFBuffer(blob), false);
  // Single Base58 word (alphanumeric minus 0 O I l), no markdown/link chars.
  EXPECT_TRUE(QRegularExpression(QRegularExpression::anchoredPattern(
                                     QStringLiteral("[1-9A-HJ-NP-Za-km-z]+")))
                  .match(token)
                  .hasMatch());

  const auto r = InstantMessageOperator::Decode(kCh, token);
  EXPECT_EQ(r.status, DecodeStatus::kNORMAL_OK);
  EXPECT_EQ(r.pgp_message.ConvertToQByteArray(), blob);
  EXPECT_FALSE(r.imported_bundle);
}

TEST(InstantMessageOperatorTest, EncodeEmptyIsEmpty) {
  EXPECT_TRUE(
      InstantMessageOperator::EncodeNormal(kCh, {}, GFBuffer(), false).isEmpty());
}

TEST(InstantMessageOperatorTest, ContainsAndDispatch) {
  const auto token =
      InstantMessageOperator::EncodeNormal(kCh, {}, GFBuffer(PgpLikeBlob()),
                                           false);
  EXPECT_TRUE(InstantMessageOperator::Contains(token));
  EXPECT_FALSE(InstantMessageOperator::Contains("just a normal chat line"));
}

TEST(InstantMessageOperatorTest, DecodeNotToken) {
  const auto r = InstantMessageOperator::Decode(kCh, "hello there, friend!");
  EXPECT_EQ(r.status, DecodeStatus::kNOT_TOKEN);
}

TEST(InstantMessageOperatorTest, DecodeRejectsArmoredMessage) {
  const QString armored =
      "-----BEGIN PGP MESSAGE-----\n\nhF4Dabc=\n=abcd\n"
      "-----END PGP MESSAGE-----";
  EXPECT_EQ(InstantMessageOperator::Decode(kCh, armored).status,
            DecodeStatus::kNOT_TOKEN);
  EXPECT_FALSE(InstantMessageOperator::Contains(armored));
}

// A messenger may wrap the token across lines / inject spaces; still decodes.
TEST(InstantMessageOperatorTest, DecodeToleratesWhitespace) {
  auto token =
      InstantMessageOperator::EncodeNormal(kCh, {}, GFBuffer(PgpLikeBlob()),
                                           false);
  token.insert(token.size() / 2, QStringLiteral("\n  "));

  const auto r = InstantMessageOperator::Decode(kCh, token);
  EXPECT_EQ(r.status, DecodeStatus::kNORMAL_OK);
  EXPECT_EQ(r.pgp_message.ConvertToQByteArray(), PgpLikeBlob());
}

TEST(InstantMessageOperatorTest, FormatVersionIsStable) {
  EXPECT_EQ(InstantMessageOperator::FormatVersion(), 1);
}

}  // namespace GpgFrontend::Test
