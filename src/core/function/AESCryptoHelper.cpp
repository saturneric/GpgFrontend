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

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>

#include "core/function/SecureRandomGenerator.h"

namespace {

auto EvpEncryptImpl(const GpgFrontend::GFBuffer& plaintext,
                    const GpgFrontend::GFBuffer& key,
                    GpgFrontend::GFBuffer& out_iv,
                    GpgFrontend::GFBuffer& out_tag, const EVP_CIPHER* cipher,
                    const GpgFrontend::GFBuffer& aad)
    -> GpgFrontend::GFBufferOrNone {
  const int key_len = EVP_CIPHER_key_length(cipher);
  Q_ASSERT(key.Size() == static_cast<size_t>(key_len));
  if (key.Size() != static_cast<size_t>(key_len)) {
    LOG_E() << "key size is mismatch: " << key.Size();
    return {};
  }

  const int iv_len = EVP_CIPHER_iv_length(cipher);

  auto iv = GpgFrontend::SecureRandomGenerator::OpenSSLGenerate(iv_len);
  if (!iv) return {};

  out_iv = iv_len > 0 ? *iv : GpgFrontend::GFBuffer{};

  auto* ctx = EVP_CIPHER_CTX_new();
  if (ctx == nullptr) {
    LOG_E() << "EVP_CIPHER_CTX_new failed";
    return {};
  }
  GpgFrontend::GFBuffer ciphertext(plaintext.Size() +
                                   EVP_CIPHER_block_size(cipher));
  int len = 0;
  int ciphertext_len = 0;

  auto ret = EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr);
  if (ret != 1) {
    LOG_E() << "EVP_EncryptInit_ex failed";
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  if (iv_len > 0) {
    if (EVP_CIPHER_mode(cipher) == EVP_CIPH_GCM_MODE) {
      ret = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, nullptr);
      if (ret != 1) {
        LOG_E() << "EVP_CIPHER_CTX_ctrl Set IV length failed";
        EVP_CIPHER_CTX_free(ctx);
        return {};
      }
    }
  }

  ret = EVP_EncryptInit_ex(
      ctx, nullptr, nullptr, reinterpret_cast<const unsigned char*>(key.Data()),
      iv_len > 0 ? reinterpret_cast<const unsigned char*>(out_iv.Data())
                 : nullptr);
  if (ret != 1) {
    LOG_E() << "EVP_EncryptInit_ex failed";
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  // GCM/CCM mode
  if (!aad.Empty() && EVP_CIPHER_mode(cipher) == EVP_CIPH_GCM_MODE) {
    ret = EVP_EncryptUpdate(ctx, nullptr, &len,
                            reinterpret_cast<const unsigned char*>(aad.Data()),
                            static_cast<int>(aad.Size()));
    if (ret != 1) {
      LOG_E() << "AAD EncryptUpdate failed";
      EVP_CIPHER_CTX_free(ctx);
      return {};
    }
  }

  ret = EVP_EncryptUpdate(
      ctx, reinterpret_cast<unsigned char*>(ciphertext.Data()), &len,
      reinterpret_cast<const unsigned char*>(plaintext.Data()),
      static_cast<int>(plaintext.Size()));
  if (ret != 1) {
    LOG_E() << "EVP_EncryptUpdate failed";
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  ciphertext_len = len;

  ret = EVP_EncryptFinal_ex(
      ctx, reinterpret_cast<unsigned char*>(ciphertext.Data()) + len, &len);
  if (ret != 1) {
    LOG_E() << "EVP_EncryptFinal_ex failed";
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  ciphertext_len += len;
  ciphertext.Resize(ciphertext_len);

  if (EVP_CIPHER_mode(cipher) == EVP_CIPH_GCM_MODE) {
    out_tag = GpgFrontend::GFBuffer(16);
    ret = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG,
                              static_cast<int>(out_tag.Size()), out_tag.Data());
    if (ret != 1) {
      LOG_E() << "GET_TAG failed";
      EVP_CIPHER_CTX_free(ctx);
      return {};
    }
  } else {
    out_tag = GpgFrontend::GFBuffer();
  }

  EVP_CIPHER_CTX_free(ctx);
  return ciphertext;
}

auto EvpDecryptImpl(const GpgFrontend::GFBuffer& ciphertext,
                    const GpgFrontend::GFBuffer& key,
                    const GpgFrontend::GFBuffer& iv,
                    const GpgFrontend::GFBuffer& tag, const EVP_CIPHER* cipher,
                    const GpgFrontend::GFBuffer& aad = {})
    -> GpgFrontend::GFBufferOrNone {
  const int key_len = EVP_CIPHER_key_length(cipher);
  Q_ASSERT(key.Size() == static_cast<size_t>(key_len));
  if (key.Size() != static_cast<size_t>(key_len)) {
    LOG_E() << "key size is mismatch: " << key.Size();
    return {};
  }

  const int iv_len = EVP_CIPHER_iv_length(cipher);
  Q_ASSERT(iv.Size() == static_cast<size_t>(iv_len));
  if (iv.Size() != static_cast<size_t>(iv_len)) {
    LOG_E() << "iv size is mismatch: " << iv.Size();
    return {};
  }

  auto* ctx = EVP_CIPHER_CTX_new();
  if (ctx == nullptr) {
    LOG_E() << "EVP_CIPHER_CTX_new failed";
    return {};
  }
  GpgFrontend::GFBuffer plaintext(ciphertext.Size());
  int len = 0;
  int plaintext_len = 0;

  auto ret = EVP_DecryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr);
  if (ret != 1) {
    LOG_E() << "EVP_DecryptInit_ex failed";
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  if (iv_len > 0) {
    if (EVP_CIPHER_mode(cipher) == EVP_CIPH_GCM_MODE) {
      ret = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, nullptr);
      if (ret != 1) {
        LOG_E() << "EVP_CIPHER_CTX_ctrl Set IV length failed";
        EVP_CIPHER_CTX_free(ctx);
        return {};
      }
    }
  }

  ret = EVP_DecryptInit_ex(
      ctx, nullptr, nullptr, reinterpret_cast<const unsigned char*>(key.Data()),
      iv_len > 0 ? reinterpret_cast<const unsigned char*>(iv.Data()) : nullptr);
  if (ret != 1) {
    LOG_E() << "EVP_DecryptInit_ex failed";
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }

  if (!aad.Empty() && EVP_CIPHER_mode(cipher) == EVP_CIPH_GCM_MODE) {
    ret = EVP_DecryptUpdate(ctx, nullptr, &len,
                            reinterpret_cast<const unsigned char*>(aad.Data()),
                            static_cast<int>(aad.Size()));
    if (ret != 1) {
      LOG_E() << "AAD DecryptUpdate failed";
      EVP_CIPHER_CTX_free(ctx);
      return {};
    }
  }

  ret = EVP_DecryptUpdate(
      ctx, reinterpret_cast<unsigned char*>(plaintext.Data()), &len,
      reinterpret_cast<const unsigned char*>(ciphertext.Data()),
      static_cast<int>(ciphertext.Size()));
  if (ret != 1) {
    LOG_E() << "EVP_DecryptUpdate failed";
    EVP_CIPHER_CTX_free(ctx);
    return {};
  }
  plaintext_len = len;

  if (EVP_CIPHER_mode(cipher) == EVP_CIPH_GCM_MODE) {
    if (tag.Size() != 16) {
      LOG_E() << "GCM tag size invalid: " << tag.Size();
      EVP_CIPHER_CTX_free(ctx);
      return {};
    }
    ret = EVP_CIPHER_CTX_ctrl(
        ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.Size()),
        const_cast<unsigned char*>(
            reinterpret_cast<const unsigned char*>(tag.Data())));
    if (ret != 1) {
      LOG_E() << "SET_TAG failed";
      EVP_CIPHER_CTX_free(ctx);
      return {};
    }
  }

  ret = EVP_DecryptFinal_ex(
      ctx, reinterpret_cast<unsigned char*>(plaintext.Data()) + plaintext_len,
      &len);
  EVP_CIPHER_CTX_free(ctx);

  if (ret != 1) {
    LOG_E() << "EVP_DecryptFinal_ex failed: tag/PKCS7 mismatch or corrupted";
    return {};
  }
  plaintext_len += len;
  plaintext.Resize(plaintext_len);

  return plaintext;
}

auto DeriveKeyArgon2(const GpgFrontend::GFBuffer& passphrase,
                     const GpgFrontend::GFBuffer& salt, int t_cost = 3,
                     int m_cost = 65536, int parallelism = 4)
    -> GpgFrontend::GFBufferOrNone {
  GpgFrontend::GFBuffer key(32);

  auto* kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
  if (kdf == nullptr) {
    LOG_E() << "EVP_KDF_fetch failed";
    return {};
  }

  EVP_KDF_CTX* kctx = EVP_KDF_CTX_new(kdf);
  if (kctx == nullptr) {
    LOG_E() << "EVP_KDF_CTX_new failed";
    EVP_KDF_free(kdf);
    return {};
  }

  std::array<OSSL_PARAM, 6> params = {
      {OSSL_PARAM_octet_string("pass", const_cast<char*>(passphrase.Data()),
                               passphrase.Size()),
       OSSL_PARAM_octet_string("salt", const_cast<char*>(salt.Data()),
                               salt.Size()),
       OSSL_PARAM_int("t", &t_cost), OSSL_PARAM_int("m", &m_cost),
       OSSL_PARAM_int("p", &parallelism), OSSL_PARAM_END}};

  int rc = EVP_KDF_derive(kctx, reinterpret_cast<unsigned char*>(key.Data()),
                          key.Size(), params.data());

  EVP_KDF_CTX_free(kctx);
  EVP_KDF_free(kdf);

  if (rc != 1) {
    LOG_E() << "EVP_KDF_derive failed";
    return {};
  }

  return key;
}

}  // namespace

namespace GpgFrontend {

auto AESCryptoHelper::GCMEncrypt(const GpgFrontend::GFBuffer& raw_key,
                                 const GpgFrontend::GFBuffer& plaintext)
    -> GFBufferOrNone {
  GpgFrontend::GFBuffer iv;
  GpgFrontend::GFBuffer tag;

  auto salt = SecureRandomGenerator::OpenSSLGenerate(16);
  if (!salt) return {};

  auto key = DeriveKeyArgon2(raw_key, *salt);
  if (!key) return {};

  auto ciphertext =
      EvpEncryptImpl(plaintext, *key, iv, tag, EVP_aes_256_gcm(), {});
  if (!ciphertext) return {};

  GFBuffer encrypted;
  encrypted.Append(*salt);
  encrypted.Append(iv);
  encrypted.Append(*ciphertext);
  encrypted.Append(tag);
  return encrypted;
}

auto AESCryptoHelper::GCMDecrypt(const GpgFrontend::GFBuffer& raw_key,
                                 const GpgFrontend::GFBuffer& encrypted)
    -> GFBufferOrNone {
  constexpr size_t kSaltLen = 16;
  constexpr size_t kIvLen = 12;
  constexpr size_t kTagLen = 16;

  if (encrypted.Size() < kSaltLen + kIvLen + kTagLen) {
    LOG_E() << "ciphertext too short";
    return {};
  }

  auto salt = encrypted.Left(kSaltLen);
  auto iv = encrypted.Mid(kSaltLen, kIvLen);
  auto tag =
      encrypted.Mid(static_cast<ssize_t>(encrypted.Size() - kTagLen), kTagLen);
  auto ciphertext = encrypted.Mid(
      kSaltLen + kIvLen,
      static_cast<ssize_t>(encrypted.Size() - kSaltLen - kIvLen - kTagLen));

  auto key = DeriveKeyArgon2(raw_key, salt);
  if (!key) return {};

  auto plaintext = EvpDecryptImpl(ciphertext, *key, iv, tag, EVP_aes_256_gcm());
  if (!plaintext) {
    LOG_E() << "decrypt failed: tag mismatch or ciphertext damaged";
    return {};
  }
  return plaintext;
}
}  // namespace GpgFrontend
