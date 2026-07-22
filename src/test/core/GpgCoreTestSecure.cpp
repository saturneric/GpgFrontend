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

#include "GFCoreTest.h"
#include "core/function/AESCryptoHelper.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/PassphraseGenerator.h"
#include "core/function/SecureRandomGenerator.h"

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
  // +1 for the terminating NUL; a strlen-sized buffer would overflow by a byte.
  char* ptr = static_cast<char*>(SMAMalloc(len + 1));
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
  // +1 for the terminating NUL. sodium_malloc places the region flush against
  // a guard page, so a strlen-sized buffer segfaults on the NUL write.
  char* ptr = static_cast<char*>(SMASecMalloc(len + 1));
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

TEST_F(GFCoreTest, CoreSecureTestC) {
  auto buffer = SecureRandomGenerator::Generate(256);
  ASSERT_TRUE(buffer.has_value());
  ASSERT_EQ(buffer->Size(), 256);
}

TEST_F(GFCoreTest, CoreSecureTestD) {
  auto key = SecureRandomGenerator::Generate(256);
  ASSERT_TRUE(key.has_value());
  ASSERT_EQ(key->Size(), 256);

  GFBuffer plaintext(QString::fromUtf8("HELLO WORLD!"));
  auto encrypted = AESCryptoHelper::Encrypt(*key, plaintext);
  ASSERT_TRUE(encrypted.has_value());

  auto decrypted = AESCryptoHelper::Decrypt(*key, *encrypted);
  ASSERT_TRUE(decrypted.has_value());
  ASSERT_EQ(decrypted, plaintext);
}

TEST_F(GFCoreTest, CoreSecureTestE) {
  GFBuffer plaintext(QString::fromUtf8("HELLO WORLD! HELLO!!!"));

  auto encoded = GFBufferFactory::ToBase64(plaintext);
  ASSERT_TRUE(encoded.has_value());

  auto decoded = GFBufferFactory::FromBase64(*encoded);
  ASSERT_TRUE(decoded.has_value());
  ASSERT_EQ(*decoded, plaintext);
}

TEST_F(GFCoreTest, CoreSecureTestF) {
  GFBuffer plaintext(QString::fromUtf8("LOL!!! HELLO WORLD! HELLO!!!"));
  ASSERT_TRUE(!plaintext.Empty());

  auto sha256 = GFBufferFactory::ToSha256(plaintext);
  ASSERT_TRUE(sha256.has_value());
  ASSERT_EQ(sha256->ConvertToQByteArray().toHex(),
            "b30649a855a5bbca09084a0b351aace5cfb850e583dea1b3e1782c3123c36113");
}

TEST_F(GFCoreTest, CoreSecureTestG) {
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

namespace {

auto IsAlphanum(char c) -> bool {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z');
}

}  // namespace

TEST(PassphraseAlphabetTest, MappingCoversAllByteValues) {
  // every one of the 256 possible input bytes must either be rejected or map
  // to a character inside the alphabet. a signed-char cast breaks this for
  // everything from 0x80 up.
  std::array<char, 256> src{};
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = static_cast<char>(i);
  }

  std::array<char, 256> dst{};
  auto written =
      MapRandomBytesToAlphanum(src.data(), src.size(), dst.data(), dst.size());

  ASSERT_GT(written, 0U);
  for (size_t i = 0; i < written; ++i) {
    EXPECT_TRUE(IsAlphanum(dst[i]))
        << "index " << i << " produced non-alphanumeric byte "
        << static_cast<int>(static_cast<unsigned char>(dst[i]));
  }
}

TEST(PassphraseAlphabetTest, MappingRejectsBiasInducingBytes) {
  // 248..255 are the eight bytes that would fold onto '0'..'7' a fifth time.
  // they must be discarded, not mapped.
  std::array<char, 8> src{};
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = static_cast<char>(248 + i);
  }

  std::array<char, 8> dst{};
  EXPECT_EQ(
      MapRandomBytesToAlphanum(src.data(), src.size(), dst.data(), dst.size()),
      0U);
}

TEST(PassphraseAlphabetTest, MappingIsUnbiased) {
  // deterministic input: every accepted byte value exactly once. each of the
  // 62 characters must therefore appear exactly 248/62 == 4 times. no RNG is
  // involved, so this cannot flake.
  std::array<char, 248> src{};
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = static_cast<char>(i);
  }

  std::array<char, 248> dst{};
  auto written =
      MapRandomBytesToAlphanum(src.data(), src.size(), dst.data(), dst.size());
  ASSERT_EQ(written, 248U);

  QMap<char, int> histogram;
  for (size_t i = 0; i < written; ++i) {
    histogram[dst[i]]++;
  }

  EXPECT_EQ(histogram.size(), 62);
  for (auto it = histogram.begin(); it != histogram.end(); ++it) {
    EXPECT_EQ(it.value(), 4) << "character '" << it.key() << "' appeared "
                             << it.value() << " times, expected 4";
  }
}

TEST(PassphraseAlphabetTest, MappingRespectsDestinationCapacity) {
  std::array<char, 64> src{};
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = static_cast<char>(i);
  }

  std::array<char, 8> dst{};
  dst.fill('\0');

  EXPECT_EQ(MapRandomBytesToAlphanum(src.data(), src.size(), dst.data(), 4),
            4U);
  // must not have written past the requested capacity.
  EXPECT_EQ(dst[4], '\0');
}

TEST(PassphraseAlphabetTest, MappingHandlesEmptyAndNullInput) {
  std::array<char, 4> dst{};

  EXPECT_EQ(MapRandomBytesToAlphanum(nullptr, 16, dst.data(), dst.size()), 0U);
  EXPECT_EQ(MapRandomBytesToAlphanum(dst.data(), 0, dst.data(), dst.size()),
            0U);
}

TEST(PassphraseAlphabetTest, GenerateReturnsRequestedLength) {
  for (int len : {1, 8, 32, 256}) {
    auto passphrase = PassphraseGenerator::GetInstance().Generate(len);
    ASSERT_TRUE(passphrase.has_value()) << "failed at len " << len;
    EXPECT_EQ(passphrase->Size(), static_cast<size_t>(len));
  }
}

TEST(PassphraseAlphabetTest, GenerateProducesOnlyAlphanum) {
  constexpr int kLen = 512;

  auto passphrase = PassphraseGenerator::GetInstance().Generate(kLen);
  ASSERT_TRUE(passphrase.has_value());

  const auto bytes = passphrase->ConvertToQByteArray();
  ASSERT_EQ(bytes.size(), kLen);
  for (int i = 0; i < bytes.size(); ++i) {
    EXPECT_TRUE(IsAlphanum(bytes[i]))
        << "index " << i << " produced non-alphanumeric byte "
        << static_cast<int>(static_cast<unsigned char>(bytes[i]));
  }
}

TEST(PassphraseAlphabetTest, GenerateRejectsNonPositiveLength) {
  EXPECT_FALSE(PassphraseGenerator::GetInstance().Generate(0).has_value());
  EXPECT_FALSE(PassphraseGenerator::GetInstance().Generate(-1).has_value());
}

}  // namespace GpgFrontend::Test