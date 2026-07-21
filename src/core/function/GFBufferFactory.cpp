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

#include "GFBufferFactory.h"

#include <sodium.h>

#include <cstring>

#include "core/function/AESCryptoHelper.h"
#include "core/function/SecureRandomGenerator.h"
#include "core/utils/IOUtils.h"

namespace {

auto EnsureSodiumInit() -> bool {
  static const int kRc = sodium_init();
  if (kRc < 0) {
    LOG_E() << "sodium_init failed";
    return false;
  }
  return true;
}

}  // namespace

namespace GpgFrontend {

GFBufferFactory::GFBufferFactory(int channel)
    : SingletonFunctionObject<GFBufferFactory>(channel) {}

auto GFBufferFactory::Compress(const GFBuffer& buffer) -> GFBufferOrNone {
  Q_UNUSED(buffer);
  return {};
}

auto GFBufferFactory::Encrypt(const GFBuffer& passphase, const GFBuffer& buffer)
    -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  return AESCryptoHelper::Encrypt(passphase, buffer);
}

auto GFBufferFactory::Decrypt(const GFBuffer& passphase, const GFBuffer& buffer)
    -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  return AESCryptoHelper::Decrypt(passphase, buffer);
}

auto GFBufferFactory::FromFile(const class QString& path) -> GFBufferOrNone {
  auto [succ, buffer] = ReadFileGFBuffer(path);
  Q_ASSERT(succ);
  if (!succ) {
    LOG_E() << "read gf buffer from file failed: " << path;
    return {};
  }
  return buffer;
}

auto GFBufferFactory::ToFile(const class QString& path, const GFBuffer& buffer)
    -> bool {
  auto succ = WriteFileGFBuffer(path, buffer);
  Q_ASSERT(succ);
  if (!succ) LOG_E() << "write gf buffer to file failed: " << path;
  return succ;
}

auto GFBufferFactory::ToFileAtomic(const class QString& path,
                                   const GFBuffer& buffer) -> bool {
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    LOG_E() << "open file for atomic write failed:" << path;
    return false;
  }

  const auto written =
      file.write(buffer.Data(), static_cast<qint64>(buffer.Size()));
  if (written != static_cast<qint64>(buffer.Size())) {
    LOG_E() << "short write during atomic write:" << path;
    file.cancelWriting();
    return false;
  }

  // commit() flushes and renames; without it QSaveFile discards the temporary.
  if (!file.commit()) {
    LOG_E() << "commit atomic write failed:" << path;
    return false;
  }

  return true;
}

auto GFBufferFactory::ToBase64(const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  if (!EnsureSodiumInit()) return {};

  const auto out_len =
      sodium_base64_encoded_len(buffer.Size(), sodium_base64_VARIANT_ORIGINAL);

  GFBuffer ret(out_len);

  char* encoded =
      sodium_bin2base64(ret.Data(), ret.Size(),
                        reinterpret_cast<const unsigned char*>(buffer.Data()),
                        buffer.Size(), sodium_base64_VARIANT_ORIGINAL);

  if (encoded == nullptr) {
    LOG_E() << "sodium_bin2base64 failed";
    return {};
  }

  const auto real_len = std::strlen(ret.Data());
  ret.Resize(static_cast<ssize_t>(real_len));

  return ret;
}

auto GFBufferFactory::FromBase64(const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  if (!EnsureSodiumInit()) return {};

  GFBuffer ret(((buffer.Size() / 4) * 3) + 3);

  size_t decoded_len = 0;

  const int rc =
      sodium_base642bin(reinterpret_cast<unsigned char*>(ret.Data()),
                        ret.Size(), buffer.Data(), buffer.Size(), nullptr,
                        &decoded_len, nullptr, sodium_base64_VARIANT_ORIGINAL);

  if (rc != 0) {
    LOG_E() << "sodium_base642bin failed";
    return {};
  }

  ret.Resize(static_cast<ssize_t>(decoded_len));
  return ret;
}

auto GFBufferFactory::ToSha256(const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  if (!EnsureSodiumInit()) return {};

  GFBuffer ret(crypto_hash_sha256_BYTES);

  const int rc =
      crypto_hash_sha256(reinterpret_cast<unsigned char*>(ret.Data()),
                         reinterpret_cast<const unsigned char*>(buffer.Data()),
                         static_cast<unsigned long long>(buffer.Size()));

  if (rc != 0) {
    LOG_E() << "crypto_hash_sha256 failed";
    return {};
  }

  return ret;
}

auto GFBufferFactory::ToSha256(
    const std::function<void(const GFBufferFactory::Sha256Chunk&)>& feeder)
    -> GFBufferOrNone {
  if (!EnsureSodiumInit()) return {};

  crypto_hash_sha256_state state;
  if (crypto_hash_sha256_init(&state) != 0) return {};

  feeder([&state](const void* data, const size_t len) {
    crypto_hash_sha256_update(&state, static_cast<const unsigned char*>(data),
                              static_cast<unsigned long long>(len));
  });

  GFBuffer ret(crypto_hash_sha256_BYTES);
  if (crypto_hash_sha256_final(
          &state, reinterpret_cast<unsigned char*>(ret.Data())) != 0) {
    return {};
  }

  return ret;
}

auto GFBufferFactory::ToHMACSha256(const GFBuffer& key, const GFBuffer& data)
    -> GFBufferOrNone {
  if (key.Empty() || data.Empty()) return {};
  if (!EnsureSodiumInit()) return {};

  // The one-shot crypto_auth_hmacsha256() reads a fixed 32-byte key. To produce
  // RFC 2104 compliant HMAC for keys of any length (short keys are zero-padded
  // to the block size, long keys are hashed) use the streaming API, which does
  // the standard key processing internally.
  crypto_auth_hmacsha256_state state;
  if (crypto_auth_hmacsha256_init(
          &state, reinterpret_cast<const unsigned char*>(key.Data()),
          key.Size()) != 0) {
    LOG_E() << "crypto_auth_hmacsha256_init failed";
    return {};
  }

  crypto_auth_hmacsha256_update(
      &state, reinterpret_cast<const unsigned char*>(data.Data()),
      static_cast<unsigned long long>(data.Size()));

  GFBuffer ret(crypto_auth_hmacsha256_BYTES);
  crypto_auth_hmacsha256_final(&state,
                               reinterpret_cast<unsigned char*>(ret.Data()));

  sodium_memzero(&state, sizeof state);

  return ret;
}

auto GFBufferFactory::RandomOpenSSLPassphase(int len) -> GFBufferOrNone {
  len = std::max(len, 16);
  len = std::min(len, 512);
  return SecureRandomGenerator::Generate(static_cast<size_t>(len));
}

auto GFBufferFactory::RandomGpgPassphase(int len) -> GFBufferOrNone {
  len = std::max(len, 16);
  len = std::min(len, 512);
  return ph_gen_.GenerateBytes(len);
}

auto GFBufferFactory::RandomGpgZBasePassphase(int len) -> GFBufferOrNone {
  Q_UNUSED(len);
  return SecureRandomGenerator::GenerateZBase32();
}

auto GFBufferFactory::EncryptLite(const GFBuffer& passphase,
                                  const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  return AESCryptoHelper::EncryptLite(passphase, buffer);
}

auto GFBufferFactory::DecryptLite(const GFBuffer& passphase,
                                  const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  return AESCryptoHelper::DecryptLite(passphase, buffer);
}

}  // namespace GpgFrontend