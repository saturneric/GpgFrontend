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

#include <QBuffer>
#include <QTemporaryDir>

#include "GpgFrontendTest.h"
#include "core/function/GFBufferFactory.h"
#include "core/utils/IOUtils.h"

/**
 * @file GpgCoreTestBufferFactoryHash.cpp
 * @brief The hashing contract every integrity check in this tree relies on.
 *
 * SHA-256 used to be computed five separate ways here, across two different
 * crypto libraries: a module's package was signed over a libsodium digest but
 * admitted on a QCryptographicHash one. They agreed, but nothing made them
 * agree, and consolidating them onto GFBufferFactory swapped the backend under
 * existing callers.
 *
 * So these are KNOWN-ANSWER tests, not round-trips. A round-trip would still
 * pass if the whole tree moved to a different hash function; these pin the
 * actual bytes of FIPS 180-4 SHA-256, so the digest a signature covers cannot
 * drift from the digest a load is gated on.
 */

namespace GpgFrontend::Test {

// FIPS 180-4 / RFC 6234 published vectors.
constexpr auto kSha256Empty =
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
constexpr auto kSha256Abc =
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

TEST(BufferFactoryHashTest, TheEmptyInputHashesToItsPublishedVector) {
  // Not a formality: the one-shot ToSha256() returns nothing for empty input,
  // so a hex helper written over it would report the hash of zero bytes as
  // failure. Zero bytes is a legitimate input with a defined answer.
  EXPECT_EQ(GFBufferFactory::Sha256Hex(QByteArray()), kSha256Empty);
  EXPECT_EQ(GFBufferFactory::Sha256Hex(QByteArray("")), kSha256Empty);
}

TEST(BufferFactoryHashTest, AbcHashesToItsPublishedVector) {
  EXPECT_EQ(GFBufferFactory::Sha256Hex(QByteArrayLiteral("abc")), kSha256Abc);
}

// A digest helper that stops at the first NUL is a classic way to make two
// different files hash the same. These two differ only after the NUL.
TEST(BufferFactoryHashTest, EmbeddedNulBytesAreHashed) {
  const QByteArray a("a\0b", 3);
  const QByteArray b("a\0c", 3);

  const auto ha = GFBufferFactory::Sha256Hex(a);
  const auto hb = GFBufferFactory::Sha256Hex(b);

  ASSERT_EQ(ha.size(), 64);
  EXPECT_NE(ha, hb) << "bytes after a NUL must reach the hash";
  EXPECT_NE(ha, QString::fromLatin1(kSha256Empty));

  // And the length is what is hashed, not the C string.
  EXPECT_NE(ha, GFBufferFactory::Sha256Hex(QByteArrayLiteral("a")));
}

TEST(BufferFactoryHashTest, EveryDigestIsLowerCaseHexOfFixedWidth) {
  for (const auto& in : {QByteArray(), QByteArray("abc"), QByteArray(5000, 'x'),
                         QByteArray("a\0b", 3)}) {
    const auto hex = GFBufferFactory::Sha256Hex(in);
    ASSERT_EQ(hex.size(), 64) << "a successful digest is always 64 characters";
    EXPECT_EQ(hex, hex.toLower());
    for (const auto c : hex) {
      EXPECT_TRUE(c.isDigit() || (c >= u'a' && c <= u'f'))
          << "non-hex character in digest: " << c.toLatin1();
    }
  }
}

// The three entry points must be interchangeable, because different stages of
// the module pipeline reach for different ones over the same content.
TEST(BufferFactoryHashTest, BytesDeviceAndFileAgreeOnTheSameContent) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  // Deliberately includes the empty case and a NUL-bearing case, plus enough
  // data to cross the 64 KiB streaming chunk boundary more than once.
  QByteArray big(200 * 1024, Qt::Uninitialized);
  for (int i = 0; i < big.size(); ++i) {
    big[i] = static_cast<char>((i * 31 + (i >> 8)) & 0xFF);
  }

  const QList<QByteArray> cases = {QByteArray(), QByteArrayLiteral("abc"),
                                   QByteArray("a\0b\0\0c", 6), big};

  int n = 0;
  for (const auto& content : cases) {
    const auto expected = GFBufferFactory::Sha256Hex(content);
    ASSERT_EQ(expected.size(), 64);

    QBuffer buffer;
    ASSERT_TRUE(buffer.open(QIODevice::ReadWrite));
    buffer.write(content);
    EXPECT_EQ(GFBufferFactory::Sha256HexOfDevice(buffer), expected)
        << "device disagreed with bytes, case " << n;

    const auto path = dir.filePath(QString("case-%1.bin").arg(n++));
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write(content);
    f.close();
    EXPECT_EQ(GFBufferFactory::Sha256HexOfFile(path), expected)
        << "file disagreed with bytes, case " << n;

    // CalculateBinaryChacksum() now delegates here. It is the accessor the
    // module loader hashes a loose library through, so its answer must be the
    // same answer -- this is what the backend swap could have broken.
    EXPECT_EQ(CalculateBinaryChacksum(path), expected);
  }
}

// A digest is compared against a recorded one, so "could not read it" must
// never come back as a value that could compare equal to anything.
TEST(BufferFactoryHashTest, AFailedHashIsEmptyRatherThanAValue) {
  EXPECT_TRUE(GFBufferFactory::Sha256HexOfFile("/nonexistent/gf/no-such-file")
                  .isEmpty());

  QBuffer closed;
  EXPECT_TRUE(GFBufferFactory::Sha256HexOfDevice(closed).isEmpty())
      << "a device that was never opened must not report a digest";
}

// The device overload hashes the whole stream even when the caller already
// consumed part of it -- the module loader reads an 8-byte image header first.
TEST(BufferFactoryHashTest, ADevicePartlyReadIsStillHashedWhole) {
  const QByteArray content = QByteArrayLiteral("abc");

  QBuffer buffer;
  ASSERT_TRUE(buffer.open(QIODevice::ReadWrite));
  buffer.write(content);
  buffer.seek(0);
  EXPECT_EQ(buffer.read(2).size(), 2);

  EXPECT_EQ(GFBufferFactory::Sha256HexOfDevice(buffer), kSha256Abc);
}

}  // namespace GpgFrontend::Test
