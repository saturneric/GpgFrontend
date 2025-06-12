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

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "core/function/AESCryptoHelper.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

GFBufferFactory::GFBufferFactory(int channel)
    : SingletonFunctionObject<GFBufferFactory>(channel) {}

auto GFBufferFactory::Compress(const GFBuffer& buffer) -> GFBufferOrNone {
  return {};
}

auto GFBufferFactory::Encrypt(const GFBuffer& passphase, const GFBuffer& buffer)
    -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  return AESCryptoHelper::GCMEncrypt(passphase, buffer);
}

auto GFBufferFactory::Decrypt(const GFBuffer& passphase, const GFBuffer& buffer)
    -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  return AESCryptoHelper::GCMDecrypt(passphase, buffer);
}

auto GFBufferFactory::FromFile(const class QString& path) -> GFBufferOrNone {
  auto [succ, buffer] = ReadFileGFBuffer(path);
  Q_ASSERT(succ);
  if (!succ) LOG_E() << "read gf buffer from file failed: " << path;
  return buffer;
}

auto GFBufferFactory::ToFile(const class QString& path, const GFBuffer& buffer)
    -> bool {
  auto succ = WriteFileGFBuffer(path, buffer);
  Q_ASSERT(succ);
  if (!succ) LOG_E() << "write gf buffer to file failed: " << path;
  return succ;
}

auto GFBufferFactory::ToBase64(const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  GFBuffer ret((4 * ((buffer.Size() + 2) / 3)) + 1);
  int out_len =
      EVP_EncodeBlock(reinterpret_cast<unsigned char*>(ret.Data()),
                      reinterpret_cast<const unsigned char*>(buffer.Data()),
                      static_cast<int>(buffer.Size()));
  if (out_len < 0) return {};
  ret.Resize(out_len);
  return ret;
}

auto GFBufferFactory::ToSha256(const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  GFBuffer ret(32);

  auto* res =
      SHA256(reinterpret_cast<const unsigned char*>(buffer.Data()),
             buffer.Size(), reinterpret_cast<unsigned char*>(ret.Data()));
  if (res != nullptr) return ret;

  return {};
}

auto GFBufferFactory::ToHMACSha256(const GFBuffer& key, const GFBuffer& data)
    -> GFBufferOrNone {
  unsigned int out_len = 0;
  std::array<unsigned char, EVP_MAX_MD_SIZE> out;

  auto* result = HMAC(EVP_sha256(), key.Data(), static_cast<int>(key.Size()),
                      reinterpret_cast<const unsigned char*>(data.Data()),
                      data.Size(), out.data(), &out_len);
  if (result == nullptr) return {};

  return GFBuffer(reinterpret_cast<const char*>(out.data()), out_len);
}

auto GFBufferFactory::FromBase64(const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};

  GFBuffer ret(((buffer.Size() + 1) / 4) * 3);

  int decoded_len =
      EVP_DecodeBlock(reinterpret_cast<unsigned char*>(ret.Data()),
                      reinterpret_cast<const unsigned char*>(buffer.Data()),
                      static_cast<int>(buffer.Size()));

  if (decoded_len < 0) return {};

  int pad = 0;
  if (buffer.Size() >= 2) {
    const char* d = buffer.Data();
    if (d[buffer.Size() - 1] == '=') pad++;
    if (d[buffer.Size() - 2] == '=') pad++;
  }

  int real_len = decoded_len - pad;
  if (real_len < 0) return {};
  ret.Resize(real_len);

  return ret;
}

auto GFBufferFactory::RandomOpenSSLPassphase(int len) -> GFBufferOrNone {
  len = std::max(len, 16);
  len = std::min(len, 512);
  return PassphraseGenerator::GenerateBytesByOpenSSL(len);
}

auto GFBufferFactory::RandomGpgPassphase(int len) -> GFBufferOrNone {
  len = std::max(len, 16);
  len = std::min(len, 512);
  return ph_gen_.GenerateBytes(len);
}

auto GFBufferFactory::RandomGpgZBasePassphase(int len) -> GFBufferOrNone {
  len = std::max(len, 31);
  len = std::min(len, 31);
  return ph_gen_.Generate(len);
}

auto GFBufferFactory::EncryptLite(const GFBuffer& passphase,
                                  const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  return AESCryptoHelper::GCMEncryptLite(passphase, buffer);
}

auto GFBufferFactory::DecryptLite(const GFBuffer& passphase,
                                  const GFBuffer& buffer) -> GFBufferOrNone {
  if (buffer.Empty()) return {};
  return AESCryptoHelper::GCMDecryptLite(passphase, buffer);
}
}  // namespace GpgFrontend
