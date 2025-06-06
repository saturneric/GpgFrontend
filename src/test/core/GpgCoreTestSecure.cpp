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

}  // namespace GpgFrontend::Test