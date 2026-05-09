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

#include "AESCryptoHelper.h"

#include <sodium.h>

#include "core/function/SecureRandomGenerator.h"

namespace {

constexpr std::string_view kMagic = "GFSEC2";
constexpr size_t kMagicLen = kMagic.size();

constexpr size_t kSaltLen = crypto_pwhash_SALTBYTES;
constexpr size_t kNonceLen = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
constexpr size_t kTagLen = crypto_aead_xchacha20poly1305_ietf_ABYTES;
constexpr size_t kKeyLen = crypto_aead_xchacha20poly1305_ietf_KEYBYTES;

constexpr unsigned long long kArgon2OpsLimit = 3;
constexpr size_t kArgon2MemLimit = 65536ULL * 1024ULL;  // 64 MiB

auto EnsureSodiumInit() -> bool {
  static const int kRc = sodium_init();
  if (kRc < 0) {
    LOG_E() << "sodium_init failed";
    return false;
  }
  return true;
}

auto MakeMagicBuffer() -> GpgFrontend::GFBuffer {
  GpgFrontend::GFBuffer magic(kMagicLen);
  std::memcpy(magic.Data(), kMagic.data(), kMagicLen);
  return magic;
}

auto HasMagic(const GpgFrontend::GFBuffer& buffer) -> bool {
  if (buffer.Size() < kMagicLen) return false;
  return std::memcmp(buffer.Data(), kMagic.data(), kMagicLen) == 0;
}

auto DeriveKeySodium(const GpgFrontend::GFBuffer& passphrase,
                     const GpgFrontend::GFBuffer& salt)
    -> GpgFrontend::GFBufferOrNone {
  if (!EnsureSodiumInit()) return {};

  if (salt.Size() != kSaltLen) {
    LOG_E() << "invalid salt size:" << salt.Size() << "expected:" << kSaltLen;
    return {};
  }

  GpgFrontend::GFBuffer key(kKeyLen);

  const int rc = crypto_pwhash(
      reinterpret_cast<unsigned char*>(key.Data()),
      static_cast<unsigned long long>(key.Size()), passphrase.Data(),
      static_cast<unsigned long long>(passphrase.Size()),
      reinterpret_cast<const unsigned char*>(salt.Data()), kArgon2OpsLimit,
      kArgon2MemLimit, crypto_pwhash_ALG_ARGON2ID13);

  if (rc != 0) {
    LOG_E() << "crypto_pwhash failed";
    return {};
  }

  return key;
}

auto SodiumEncryptImpl(const GpgFrontend::GFBuffer& raw_key,
                       const GpgFrontend::GFBuffer& plaintext)
    -> GpgFrontend::GFBufferOrNone {
  if (!EnsureSodiumInit()) return {};

  auto salt = GpgFrontend::SecureRandomGenerator::Generate(kSaltLen);
  if (!salt) return {};

  auto nonce = GpgFrontend::SecureRandomGenerator::Generate(kNonceLen);
  if (!nonce) return {};

  auto key = DeriveKeySodium(raw_key, *salt);
  if (!key) return {};

  GpgFrontend::GFBuffer ciphertext(plaintext.Size());
  GpgFrontend::GFBuffer tag(kTagLen);

  const int rc = crypto_aead_xchacha20poly1305_ietf_encrypt_detached(
      reinterpret_cast<unsigned char*>(ciphertext.Data()),
      reinterpret_cast<unsigned char*>(tag.Data()), nullptr,
      reinterpret_cast<const unsigned char*>(plaintext.Data()),
      static_cast<unsigned long long>(plaintext.Size()), nullptr, 0, nullptr,
      reinterpret_cast<const unsigned char*>(nonce->Data()),
      reinterpret_cast<const unsigned char*>(key->Data()));

  sodium_memzero(key->Data(), key->Size());

  if (rc != 0) {
    LOG_E() << "crypto_aead_xchacha20poly1305_ietf_encrypt_detached failed";
    return {};
  }

  auto magic = MakeMagicBuffer();

  GpgFrontend::GFBuffer encrypted;
  encrypted.Combine({magic, *salt, *nonce, ciphertext, tag});

  return encrypted;
}

auto SodiumDecryptImpl(const GpgFrontend::GFBuffer& raw_key,
                       const GpgFrontend::GFBuffer& encrypted)
    -> GpgFrontend::GFBufferOrNone {
  if (!EnsureSodiumInit()) return {};

  constexpr size_t kMinLen = kMagicLen + kSaltLen + kNonceLen + kTagLen;

  if (encrypted.Size() < kMinLen) {
    LOG_E() << "encrypted buffer too short:" << encrypted.Size();
    return {};
  }

  if (!HasMagic(encrypted)) {
    LOG_E() << "invalid encrypted buffer magic";
    return {};
  }

  const size_t salt_offset = kMagicLen;
  const size_t nonce_offset = salt_offset + kSaltLen;
  const size_t ciphertext_offset = nonce_offset + kNonceLen;
  const size_t ciphertext_len =
      encrypted.Size() - kMagicLen - kSaltLen - kNonceLen - kTagLen;
  const size_t tag_offset = encrypted.Size() - kTagLen;

  auto salt = encrypted.Mid(static_cast<ssize_t>(salt_offset), kSaltLen);
  auto nonce = encrypted.Mid(static_cast<ssize_t>(nonce_offset), kNonceLen);
  auto ciphertext = encrypted.Mid(static_cast<ssize_t>(ciphertext_offset),
                                  static_cast<ssize_t>(ciphertext_len));
  auto tag = encrypted.Mid(static_cast<ssize_t>(tag_offset), kTagLen);

  auto key = DeriveKeySodium(raw_key, salt);
  if (!key) return {};

  GpgFrontend::GFBuffer plaintext(ciphertext.Size());

  const int rc = crypto_aead_xchacha20poly1305_ietf_decrypt_detached(
      reinterpret_cast<unsigned char*>(plaintext.Data()), nullptr,
      reinterpret_cast<const unsigned char*>(ciphertext.Data()),
      static_cast<unsigned long long>(ciphertext.Size()),
      reinterpret_cast<const unsigned char*>(tag.Data()), nullptr, 0,
      reinterpret_cast<const unsigned char*>(nonce.Data()),
      reinterpret_cast<const unsigned char*>(key->Data()));

  sodium_memzero(key->Data(), key->Size());

  if (rc != 0) {
    LOG_E() << "crypto_aead_xchacha20poly1305_ietf_decrypt_detached failed:"
            << "tag mismatch or corrupted data";
    return {};
  }

  return plaintext;
}

}  // namespace

namespace GpgFrontend {

auto AESCryptoHelper::GCMEncrypt(const GpgFrontend::GFBuffer& raw_key,
                                 const GpgFrontend::GFBuffer& plaintext)
    -> GFBufferOrNone {
  return SodiumEncryptImpl(raw_key, plaintext);
}

auto AESCryptoHelper::GCMDecrypt(const GpgFrontend::GFBuffer& raw_key,
                                 const GpgFrontend::GFBuffer& encrypted)
    -> GFBufferOrNone {
  return SodiumDecryptImpl(raw_key, encrypted);
}

auto AESCryptoHelper::GCMEncryptLite(const GpgFrontend::GFBuffer& raw_key,
                                     const GpgFrontend::GFBuffer& plaintext)
    -> GFBufferOrNone {
  return SodiumEncryptImpl(raw_key, plaintext);
}

auto AESCryptoHelper::GCMDecryptLite(const GpgFrontend::GFBuffer& raw_key,
                                     const GpgFrontend::GFBuffer& encrypted)
    -> GFBufferOrNone {
  return SodiumDecryptImpl(raw_key, encrypted);
}

}  // namespace GpgFrontend
