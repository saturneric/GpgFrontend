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

#include <array>
#include <cstring>

#include "GpgCoreTest.h"
#include "core/function/AESCryptoHelper.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/SecureRandomGenerator.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Test {

TEST(SecureMemoryAllocatorTest, BasicMallocAndFree) {
  void* ptr = SMAMalloc(128);
  ASSERT_NE(ptr, nullptr);
  memset(ptr, 0xAA, 128);  // try writing
  SMAFree(ptr);
}

TEST(SecureMemoryAllocatorTest, MallocZero) {
  void* ptr = SMAMalloc(0);
  // malloc(0) may return nullptr or a valid pointer, but it must be safe to
  // free.
  SMAFree(ptr);
}

TEST(SecureMemoryAllocatorTest, ReallocAndDataIntegrity) {
  const char* data = "test";
  size_t len = strlen(data);
  char* ptr = static_cast<char*>(SMAMalloc(len));
  strcpy(ptr, data);

  ptr = static_cast<char*>(SMARealloc(ptr, 2 * len));
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(memcmp(ptr, data, len), 0);
  SMAFree(ptr);
}

TEST(SecureMemoryAllocatorTest, ReallocNullPointer) {
  void* ptr = SMARealloc(nullptr, 32);
  ASSERT_NE(ptr, nullptr);
  SMAFree(ptr);
}

TEST(SecureMemoryAllocatorTest, FreeNullPointer) {
  SMAFree(nullptr);  // Should not crash or leak
}

TEST(SecureMemoryAllocatorTest, DoubleFreeShouldWarn) {
  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  // do not test for normal free()
  if (secure_level < 1) return;

  void* ptr = SMAMalloc(64);
  ASSERT_NE(ptr, nullptr);
  SMAFree(ptr);

#ifdef DEBUG
  EXPECT_DEATH({ SMAFree(ptr); }, "");
#else
  // Second free: implementation should warn but not crash
  SMASecFree(ptr);
#endif
}

TEST(SecureMemoryAllocatorTest, SecMallocAndFree) {
  void* ptr = SMASecMalloc(256);
  ASSERT_NE(ptr, nullptr);
  memset(ptr, 0xBB, 256);
  SMASecFree(ptr);
}

TEST(SecureMemoryAllocatorTest, SecReallocAndDataIntegrity) {
  const char* data = "secure_data";
  size_t len = strlen(data);
  char* ptr = static_cast<char*>(SMASecMalloc(len));
  strcpy(ptr, data);

  ptr = static_cast<char*>(SMASecRealloc(ptr, len + 10));
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(memcmp(ptr, data, len), 0);
  SMASecFree(ptr);
}

TEST(SecureMemoryAllocatorTest, SecFreeNullPointer) { SMASecFree(nullptr); }

TEST(SecureMemoryAllocatorTest, SecDoubleFreeShouldWarn) {
  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  // do not test for normal free()
  if (secure_level < 1) return;

  void* ptr = SMASecMalloc(128);
  ASSERT_NE(ptr, nullptr);
  SMASecFree(ptr);
#ifdef DEBUG
  EXPECT_DEATH({ SMAFree(ptr); }, "");
#else
  SMASecFree(ptr);
#endif
}

TEST(SecureMemoryAllocatorTest, SecMallocReturnsZeroedMemory) {
  // Both tiers hand back cleared memory: OPENSSL_zalloc below level 2 and
  // OPENSSL_secure_zalloc above it. Callers rely on not inheriting whatever
  // the previous owner left there.
  auto* ptr = static_cast<unsigned char*>(SMASecMalloc(256));
  ASSERT_NE(ptr, nullptr);

  for (size_t i = 0; i < 256; ++i) EXPECT_EQ(ptr[i], 0);

  SMASecFree(ptr);
}

TEST(SecureMemoryAllocatorTest, SecReallocGrowKeepsTheContents) {
  auto* ptr = static_cast<char*>(SMASecMalloc(8));
  ASSERT_NE(ptr, nullptr);
  std::memcpy(ptr, "secret", 6);

  auto* grown = static_cast<char*>(SMASecRealloc(ptr, 64));
  ASSERT_NE(grown, nullptr);
  EXPECT_EQ(std::memcmp(grown, "secret", 6), 0);

  SMASecFree(grown);
}

TEST(SecureMemoryAllocatorTest, SecReallocShrinkKeepsTheLeadingBytes) {
  auto* ptr = static_cast<char*>(SMASecMalloc(64));
  ASSERT_NE(ptr, nullptr);
  std::memcpy(ptr, "secret", 6);

  auto* shrunk = static_cast<char*>(SMASecRealloc(ptr, 6));
  ASSERT_NE(shrunk, nullptr);
  EXPECT_EQ(std::memcmp(shrunk, "secret", 6), 0);

  SMASecFree(shrunk);
}

TEST(SecureMemoryAllocatorTest, SecReallocToZeroReleasesTheBlock) {
  auto* ptr = SMASecMalloc(32);
  ASSERT_NE(ptr, nullptr);

  EXPECT_EQ(SMASecRealloc(ptr, 0), nullptr);
}

TEST(SecureMemoryAllocatorTest, SecureMemoryCanBeReleasedThroughSMAFree) {
  // The secure tier registers its allocations at every level, so the plain
  // free path recognises the pointer and still wipes it rather than handing a
  // secret straight back to the heap.
  auto* ptr = SMASecMalloc(128);
  ASSERT_NE(ptr, nullptr);

  SMAFree(ptr);
}

TEST(SecureMemoryAllocatorTest, ParallelAllocAndFree) {
  constexpr int kThreads = 8;
  constexpr int kIters = 100;
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < kIters; ++j) {
        void* ptr = SMAMalloc(32 + j);
        memset(ptr, 0xCD, 32 + j);
        SMAFree(ptr);
      }
    });
  }
  for (auto& t : threads) t.join();
}

TEST(SecureMemoryAllocatorTest, ParallelSecAllocAndFree) {
  constexpr int kThreads = 8;
  constexpr int kIters = 100;
  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < kIters; ++j) {
        void* ptr = SMASecMalloc(32 + j);
        memset(ptr, 0xCD, 32 + j);
        SMASecFree(ptr);
      }
    });
  }
  for (auto& t : threads) t.join();
}

TEST_F(GpgCoreTest, CoreSecureTestA) {
  auto buffer = SecureRandomGenerator::GetInstance().GnuPGGenerateZBase32();
  ASSERT_TRUE(buffer.has_value());
  ASSERT_EQ(buffer->Size(), 31);
}

TEST_F(GpgCoreTest, CoreSecureTestB) {
  auto buffer = SecureRandomGenerator::GetInstance().GnuPGGenerate(1);
  ASSERT_TRUE(buffer.has_value());
  ASSERT_EQ(buffer->Size(), 8);

  buffer = SecureRandomGenerator::GetInstance().GnuPGGenerate(8);
  ASSERT_TRUE(buffer.has_value());
  ASSERT_EQ(buffer->Size(), 8);

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

TEST(GFBufferFactoryTest, RandomGpgPassphase) {
  GFBufferFactory factory;
  for (int len : {16, 32, 64}) {
    auto pass = factory.RandomGpgPassphase(len);
    ASSERT_TRUE(pass.has_value());
    EXPECT_EQ(pass->Size(), len);
  }
}

TEST(GFBufferFactoryTest, RandomGpgZBasePassphase) {
  GFBufferFactory factory;
  auto pass = factory.RandomGpgZBasePassphase(31);
  ASSERT_TRUE(pass.has_value());
  EXPECT_EQ(pass->Size(), 31);
}

TEST(GFBufferFactoryTest, EncryptAndDecrypt) {
  GFBufferFactory factory;
  GFBuffer pass("testpassword");
  GFBuffer plain("supersecretdata");

  auto enc = GpgFrontend::GFBufferFactory::Encrypt(pass, plain);
  ASSERT_TRUE(enc.has_value());
  auto dec = GpgFrontend::GFBufferFactory::Decrypt(pass, *enc);
  ASSERT_TRUE(dec.has_value());
  EXPECT_EQ(*dec, plain);

  // Test empty
  GFBuffer empty;
  EXPECT_FALSE(factory.Encrypt(pass, empty).has_value());
  EXPECT_FALSE(factory.Decrypt(pass, empty).has_value());
}

TEST(GFBufferFactoryTest, EncryptLiteAndDecryptLite) {
  GFBufferFactory factory;
  GFBuffer pass("litepw");
  GFBuffer plain("hellolite");

  auto enc = factory.EncryptLite(pass, plain);
  ASSERT_TRUE(enc.has_value());
  auto dec = factory.DecryptLite(pass, *enc);
  ASSERT_TRUE(dec.has_value());
  EXPECT_EQ(*dec, plain);
}

TEST(GFBufferFactoryTest, ToFileAndFromFile) {
  GFBufferFactory factory;
  GFBuffer buf("write and read");
  QString path = "tmp_gfbuffertest_file";

  bool succ = GpgFrontend::GFBufferFactory::ToFile(path, buf);
  ASSERT_TRUE(succ);

  auto loaded = GpgFrontend::GFBufferFactory::FromFile(path);
  ASSERT_TRUE(loaded.has_value());
  EXPECT_EQ(*loaded, buf);

  // Cleanup
  QFile::remove(path);
}
// --- MemoryWipeTest ------------------------------------------------------
//
// These pin the wipe primitives themselves. Note what is deliberately NOT
// tested: GFBuffer::Impl's wipe-on-release. Observing it would mean reading
// memory after it was freed, which is undefined behaviour and would trip ASan.
// The primitive below is tested, the safety argument (impl_ is a shared_ptr,
// so the destructor runs only after the last copy-on-write share is gone)
// lives in a comment at the call site, and the wiring is a review fact.

TEST(MemoryWipeTest, SecureWipeMemoryZeroesEveryByte) {
  std::array<unsigned char, 64> buf{};
  buf.fill(0xAB);

  SecureWipeMemory(buf.data(), buf.size());

  for (const auto byte : buf) EXPECT_EQ(byte, 0);
}

TEST(MemoryWipeTest, SecureWipeMemoryToleratesNullAndZeroLength) {
  SecureWipeMemory(nullptr, 16);

  std::array<unsigned char, 4> buf{};
  buf.fill(0xCD);
  SecureWipeMemory(buf.data(), 0);

  // A zero length must leave the region untouched rather than guess.
  for (const auto byte : buf) EXPECT_EQ(byte, 0xCD);
}

TEST(MemoryWipeTest, WipeStringOverwritesAnUnsharedStringWhereItLies) {
  QString secret = QStringLiteral("correct horse battery staple");
  secret.detach();

  const auto* storage = secret.constData();
  const auto size = secret.size();

  EXPECT_TRUE(WipeString(secret));

  // The wipe happened in the original storage, so the characters that were
  // there are gone rather than merely unreferenced.
  for (qsizetype i = 0; i < size; ++i) EXPECT_EQ(storage[i].unicode(), 0);
  EXPECT_TRUE(secret.isEmpty());
}

TEST(MemoryWipeTest, WipeStringReportsThatASharedStringCouldNotBeOverwritten) {
  QString secret = QStringLiteral("shared secret");
  QString other = secret;  // implicit sharing: same storage

  // data() has to detach, so the wipe lands on a private copy and the original
  // characters survive in `other`. Saying so is the whole point of the bool.
  EXPECT_FALSE(WipeString(secret));
  EXPECT_EQ(other, QStringLiteral("shared secret"));
  EXPECT_TRUE(secret.isEmpty());
}

TEST(MemoryWipeTest, WipeStringOnAnEmptyStringIsANoOp) {
  QString empty;
  EXPECT_TRUE(WipeString(empty));
  EXPECT_TRUE(empty.isEmpty());
}

TEST(MemoryWipeTest, WipeByteArrayOverwritesAnUnsharedArrayWhereItLies) {
  QByteArray secret("super secret bytes");
  secret.detach();

  const auto* storage = secret.constData();
  const auto size = secret.size();

  EXPECT_TRUE(WipeByteArray(secret));

  for (qsizetype i = 0; i < size; ++i) EXPECT_EQ(storage[i], 0);
  EXPECT_TRUE(secret.isEmpty());
}

TEST(MemoryWipeTest,
     WipeByteArrayReportsThatASharedArrayCouldNotBeOverwritten) {
  QByteArray secret("shared bytes");
  QByteArray other = secret;

  EXPECT_FALSE(WipeByteArray(secret));
  EXPECT_EQ(other, QByteArray("shared bytes"));
  EXPECT_TRUE(secret.isEmpty());
}

TEST(MemoryWipeTest, WipeByteArrayOnAnEmptyArrayIsANoOp) {
  QByteArray empty;
  EXPECT_TRUE(WipeByteArray(empty));
  EXPECT_TRUE(empty.isEmpty());
}

}  // namespace GpgFrontend::Test
