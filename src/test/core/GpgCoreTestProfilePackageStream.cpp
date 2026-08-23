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

#include <gtest/gtest.h>
#include <sodium.h>

#include "core/profile/ProfilePackageStream.h"

namespace GpgFrontend::Test {

namespace {

/// @param is_protected what the package header would declare. Defaulted from
/// the passphrase for the tests that do not care, and passed explicitly by the
/// one that does.
auto Seal(const QByteArray& payload, const GFBuffer& passphrase,
          bool is_protected = true) -> QByteArray {
  QByteArray sealed;
  ProfilePackageStreamWriter writer(
      [&sealed](const char* data, qint64 length) {
        sealed.append(data, static_cast<qsizetype>(length));
        return true;
      },
      passphrase, is_protected);

  EXPECT_TRUE(writer.Begin());
  EXPECT_TRUE(writer.Write(payload.constData(), payload.size()));
  EXPECT_TRUE(writer.Finish());
  return sealed;
}

struct OpenResult {
  bool ok = false;
  bool complete = false;
  QByteArray payload;
};

auto Open(const QByteArray& sealed, const GFBuffer& passphrase,
          bool is_protected = true) -> OpenResult {
  OpenResult result;
  qint64 offset = 0;

  ProfilePackageStreamReader reader(
      [&sealed, &offset](char* out, qint64 length) -> qint64 {
        const auto available = std::min(length, sealed.size() - offset);
        if (available <= 0) return 0;
        std::memcpy(out, sealed.constData() + offset,
                    static_cast<size_t>(available));
        offset += available;
        return available;
      },
      passphrase, is_protected);

  if (!reader.Begin()) return result;

  GFBuffer chunk;
  while (true) {
    if (!reader.Next(chunk)) return result;
    if (chunk.Empty()) break;
    result.payload.append(chunk.ConvertToQByteArray());
    if (reader.Complete()) break;
  }

  result.ok = true;
  result.complete = reader.Complete();
  return result;
}

auto Payload(qsizetype size) -> QByteArray {
  QByteArray bytes(size, Qt::Uninitialized);
  for (qsizetype i = 0; i < size; ++i) {
    bytes[i] = static_cast<char>((i * 31 + 7) & 0xFF);
  }
  return bytes;
}

}  // namespace

TEST(ProfilePackageStreamTest, APayloadComesBackByteForByte) {
  const GFBuffer passphrase(QString("correct horse battery staple"));
  const auto payload = Payload(4096);

  const auto opened = Open(Seal(payload, passphrase), passphrase);
  ASSERT_TRUE(opened.ok);
  EXPECT_TRUE(opened.complete);
  EXPECT_EQ(opened.payload, payload);
}

TEST(ProfilePackageStreamTest, APayloadLargerThanOneChunkStillRoundTrips) {
  // The case the one-shot container could not do at all past a couple of
  // hundred megabytes: several chunks, none of which is ever held together.
  const GFBuffer passphrase(QString("pass"));
  const auto payload = Payload(kProfilePackageChunkBytes * 2 + 12345);

  const auto opened = Open(Seal(payload, passphrase), passphrase);
  ASSERT_TRUE(opened.ok);
  EXPECT_TRUE(opened.complete);
  EXPECT_EQ(opened.payload.size(), payload.size());
  EXPECT_EQ(opened.payload, payload);
}

TEST(ProfilePackageStreamTest, AnEmptyPayloadStillCarriesItsFinalTag) {
  const GFBuffer passphrase(QString("pass"));
  const auto opened = Open(Seal({}, passphrase), passphrase);
  ASSERT_TRUE(opened.ok);
  EXPECT_TRUE(opened.complete);
  EXPECT_TRUE(opened.payload.isEmpty());
}

TEST(ProfilePackageStreamTest, TheWrongPassphraseFailsAtTheFirstChunk) {
  const auto payload = Payload(1024);
  const auto sealed = Seal(payload, GFBuffer(QString("right")));

  const auto opened = Open(sealed, GFBuffer(QString("wrong")));
  EXPECT_FALSE(opened.ok);
  EXPECT_TRUE(opened.payload.isEmpty());
}

TEST(ProfilePackageStreamTest, ATruncatedPackageIsRefusedNotReadShort) {
  // The failure a length-prefixed stream would otherwise get wrong: without the
  // final tag, a file cut in half looks exactly like a smaller profile.
  const GFBuffer passphrase(QString("pass"));
  const auto payload = Payload(kProfilePackageChunkBytes * 2);

  auto sealed = Seal(payload, passphrase);
  ASSERT_GT(sealed.size(), 512);
  sealed.chop(sealed.size() / 3);

  const auto opened = Open(sealed, passphrase);
  // `ok` alone, not `ok && complete`: the helper only ever sets `complete` when
  // `ok`, so the conjunction was satisfied by the weaker of the two and the
  // test passed for less than its name claims.
  EXPECT_FALSE(opened.ok) << "a truncated package was accepted as complete";
}

TEST(ProfilePackageStreamTest,
     AProtectedBodyWithNoPassphraseIsRefusedNotMisread) {
  // What framing the body is in comes from the package's own header, never from
  // whether a passphrase was handed over. Inferring it from the secret meant an
  // empty passphrase turned a protected body into "this package is not
  // protected", and the ciphertext went to the archive reader as if it were a
  // plain body -- so a user who left the box blank was told their package was
  // corrupt.
  const auto payload = Payload(4096);
  const auto sealed = Seal(payload, GFBuffer(QString("pass")));

  const auto opened = Open(sealed, GFBuffer(), true);
  EXPECT_FALSE(opened.ok);
  EXPECT_TRUE(opened.payload.isEmpty());
}

TEST(ProfilePackageStreamTest, ADamagedChunkIsCaughtAtThatChunk) {
  const GFBuffer passphrase(QString("pass"));
  const auto payload = Payload(kProfilePackageChunkBytes + 64);

  auto sealed = Seal(payload, passphrase);
  // Well past the salt and stream header, so this lands inside a chunk body.
  const auto target = sealed.size() - 64;
  sealed[target] = static_cast<char>(sealed[target] ^ 0x40);

  const auto opened = Open(sealed, passphrase);
  EXPECT_FALSE(opened.ok);
}

TEST(ProfilePackageStreamTest, AnUnprotectedBodyIsThePayloadItself) {
  // Nothing to authenticate with, so framing it would cost without protecting
  // anything -- and the file is plaintext on disk either way.
  const auto payload = Payload(2048);
  const auto sealed = Seal(payload, GFBuffer(), false);

  EXPECT_EQ(sealed, payload);

  const auto opened = Open(sealed, GFBuffer(), false);
  ASSERT_TRUE(opened.ok);
  EXPECT_EQ(opened.payload, payload);
}

TEST(ProfilePackageStreamTest, AChunkClaimingAnAbsurdSizeIsRefused) {
  // The length prefix is attacker-controlled before anything authenticates it,
  // so it is the one field that can make a reader allocate on trust.
  const GFBuffer passphrase(QString("pass"));
  auto sealed = Seal(Payload(64), passphrase);

  // The prefix sits directly after the salt and the stream header.
  const auto prefix_at = crypto_pwhash_SALTBYTES +
                         crypto_secretstream_xchacha20poly1305_HEADERBYTES;
  ASSERT_GT(sealed.size(), prefix_at + 4);
  sealed[prefix_at] = static_cast<char>(0x7F);

  const auto opened = Open(sealed, passphrase);
  EXPECT_FALSE(opened.ok);
}

}  // namespace GpgFrontend::Test
