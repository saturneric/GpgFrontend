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

#include "GpgCoreTest.h"
#include "core/function/AESCryptoHelper.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/SecureRandomGenerator.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreSecureTestA) {
  auto buffer = SecureRandomGenerator::GetInstance().GnuPGGenerateZBase32();
  ASSERT_TRUE(buffer.has_value());
  ASSERT_EQ(buffer->Size(), 31);
}

TEST_F(GpgCoreTest, CoreSecureTestB) {
  auto buffer = SecureRandomGenerator::GetInstance().GnuPGGenerate(16);
  ASSERT_TRUE(buffer.has_value());
  ASSERT_EQ(buffer->Size(), 31);

  buffer = SecureRandomGenerator::GetInstance().GnuPGGenerate(512);
  ASSERT_TRUE(buffer.has_value());
  ASSERT_EQ(buffer->Size(), 512);
}

TEST_F(GpgCoreTest, CoreSecureTestC) {
  auto buffer = SecureRandomGenerator::OpenSSLGenerate(256);
  ASSERT_TRUE(buffer.has_value());
  ASSERT_EQ(buffer->Size(), 256);
}

TEST_F(GpgCoreTest, CoreSecureTestD) {
  auto key = SecureRandomGenerator::OpenSSLGenerate(256);
  ASSERT_TRUE(key.has_value());
  ASSERT_EQ(key->Size(), 256);

  GFBuffer plaintext(QString::fromUtf8("HELLO WORLD!"));
  auto encrypted = AESCryptoHelper::GCMEncrypt(*key, plaintext);
  ASSERT_TRUE(encrypted.has_value());

  auto decrypted = AESCryptoHelper::GCMDecrypt(*key, *encrypted);
  ASSERT_TRUE(decrypted.has_value());
  ASSERT_EQ(decrypted, plaintext);
}

TEST_F(GpgCoreTest, CoreSecureTestE) {
  GFBuffer plaintext(QString::fromUtf8("HELLO WORLD! HELLO!!!"));

  auto encoded = GFBufferFactory::ToBase64(plaintext);
  ASSERT_TRUE(encoded.has_value());

  auto decoded = GFBufferFactory::FromBase64(*encoded);
  ASSERT_TRUE(decoded.has_value());
  ASSERT_EQ(*decoded, plaintext);
}

TEST_F(GpgCoreTest, CoreSecureTestF) {
  GFBuffer plaintext(QString::fromUtf8("LOL!!! HELLO WORLD! HELLO!!!"));
  ASSERT_TRUE(!plaintext.Empty());

  auto sha256 = GFBufferFactory::ToSha256(plaintext);
  ASSERT_TRUE(sha256.has_value());
  ASSERT_EQ(sha256->ConvertToQByteArray().toHex(),
            "b30649a855a5bbca09084a0b351aace5cfb850e583dea1b3e1782c3123c36113");
}

TEST_F(GpgCoreTest, CoreSecureTestG) {
  GFBuffer plaintext(QString::fromUtf8("LOL!!! HELLO WORLD! HELLO!!!"));
  ASSERT_TRUE(!plaintext.Empty());

  auto sha256 = GFBufferFactory::ToHMACSha256(GFBuffer("123456"), plaintext);
  ASSERT_TRUE(sha256.has_value());
  ASSERT_EQ(sha256->ConvertToQByteArray().toHex(),
            "5651ab54d616162a71a1e04937105ac2e731a9025f9b8735a9bc4ca975bbaeb1");
}

}  // namespace GpgFrontend::Test