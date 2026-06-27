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

#include "RustUtils.h"

#include "core/function/GFKeyDatabase.h"
#include "core/function/rpgp/KeyStorage.h"

namespace GpgFrontend {

auto GF_CORE_EXPORT RustEngineVersion() -> QString {
  char* c_str = Rust::gfr_rust_engine_version();
  if (c_str == nullptr) {
    LOG_E() << "Failed to get Rust engine version.";
    return {};
  }

  auto version = QString::fromUtf8(c_str);
  Rust::gfr_crypto_free_string(c_str);
  return version;
}

void SetRpgpPasswordCacheTtl(uint64_t ttl_secs, uint64_t max_ttl_secs) {
#ifdef HAS_RUST_SUPPORT
  Rust::gfr_set_password_cache_ttl(ttl_secs, max_ttl_secs);
  LOG_I() << "applied rPGP password cache ttl:" << ttl_secs
          << "s, max ttl:" << max_ttl_secs << "s";
#else
  (void)ttl_secs;
  (void)max_ttl_secs;
#endif
}

auto GF_CORE_EXPORT RustEngineBuildInfo() -> RpgpEngineInfo {
  RpgpEngineInfo info;

#ifdef HAS_RUST_SUPPORT
  char* c_str = Rust::gfr_rust_engine_build_info();
  if (c_str == nullptr) {
    LOG_E() << "Failed to get Rust engine build info.";
    return info;
  }

  auto raw = QString::fromUtf8(c_str);
  Rust::gfr_crypto_free_string(c_str);

  const auto lines = raw.split('\n', Qt::SkipEmptyParts);
  for (const auto& line : lines) {
    const auto sep = line.indexOf('\t');
    if (sep < 0) continue;

    const auto key = line.left(sep);
    const auto value = line.mid(sep + 1);

    if (key == "engine") {
      info.engine_version = value;
    } else if (key == "rustc") {
      info.rustc_version = value;
    } else if (key == "target") {
      info.target = value;
    } else if (key == "profile") {
      info.profile = value;
    } else if (key.startsWith("dep:")) {
      info.dependencies.append({key.mid(4), value});
    }
  }
#endif

  return info;
}

auto KeyAlgoId2GfrKeyAlgo(const QString& algo_id) -> Rust::GfrKeyAlgo {
  if (algo_id == "ed25519") return Rust::GfrKeyAlgo::ED25519;
  if (algo_id == "cv25519") return Rust::GfrKeyAlgo::CV25519;
  if (algo_id == "nistp256") return Rust::GfrKeyAlgo::NISTP256;
  if (algo_id == "nistp384") return Rust::GfrKeyAlgo::NISTP384;
  if (algo_id == "nistp521") return Rust::GfrKeyAlgo::NISTP521;
  if (algo_id == "brainpoolp256r1") return Rust::GfrKeyAlgo::BRAINPOOLP256;
  if (algo_id == "brainpoolp384r1") return Rust::GfrKeyAlgo::BRAINPOOLP384;
  if (algo_id == "brainpoolp512r1") return Rust::GfrKeyAlgo::BRAINPOOLP512;
  if (algo_id == "rsa2048") return Rust::GfrKeyAlgo::RSA2048;
  if (algo_id == "rsa3072") return Rust::GfrKeyAlgo::RSA3072;
  if (algo_id == "rsa4096") return Rust::GfrKeyAlgo::RSA4096;
  if (algo_id == "secp256k1") return Rust::GfrKeyAlgo::SECP256K1;
  if (algo_id == "ed448") return Rust::GfrKeyAlgo::ED448;
  if (algo_id == "x448") return Rust::GfrKeyAlgo::X448;
  if (algo_id == "rsa") return Rust::GfrKeyAlgo::RSA2048;
  if (algo_id == "dsa1024") return Rust::GfrKeyAlgo::DSA1024;
  if (algo_id == "dsa2048") return Rust::GfrKeyAlgo::DSA2048;
  if (algo_id == "dsa3072") return Rust::GfrKeyAlgo::DSA3072;
  if (algo_id == "dsa") return Rust::GfrKeyAlgo::DSA2048;
  if (algo_id == "ed25519legacy") return Rust::GfrKeyAlgo::ED25519LEGACY;
  if (algo_id == "kyber768_cv25519") return Rust::GfrKeyAlgo::KYBER768X25519;
  if (algo_id == "kyber1024_x448") return Rust::GfrKeyAlgo::KYBER1024X448;
  if (algo_id == "ky768_cv25519") return Rust::GfrKeyAlgo::KYBER768X25519;
  if (algo_id == "ky1024_x448") return Rust::GfrKeyAlgo::KYBER1024X448;
  if (algo_id == "mldsa65_ed25519") return Rust::GfrKeyAlgo::MLDSA65ED25519;
  if (algo_id == "mldsa87_ed448") return Rust::GfrKeyAlgo::MLDSA87ED448;
  if (algo_id == "slhdsashake128s") return Rust::GfrKeyAlgo::SLHDSASHAKE128S;
  if (algo_id == "slhdsashake128f") return Rust::GfrKeyAlgo::SLHDSASHAKE128F;
  if (algo_id == "slhdsashake256s") return Rust::GfrKeyAlgo::SLHDSASHAKE256S;

  LOG_W() << "Unknown key algo id: " << algo_id;
  return Rust::GfrKeyAlgo::Unknown;
}

auto KeyVersion2GfrKeyVersion(int version) -> Rust::GfrOpenPGPKeyVersion {
  switch (version) {
    case 4:
      return Rust::GfrOpenPGPKeyVersion::V4;
    case 6:
      return Rust::GfrOpenPGPKeyVersion::V6;
    default:
      return Rust::GfrOpenPGPKeyVersion::Unknown;
  }
}

auto GF_CORE_EXPORT GfrKeyAlgo2KeyAlgoName(Rust::GfrKeyAlgo algo) -> QString {
  switch (algo) {
    case Rust::GfrKeyAlgo::ED25519:
      return "ED25519";
    case Rust::GfrKeyAlgo::CV25519:
      return "CV25519";
    case Rust::GfrKeyAlgo::NISTP256:
      return "NIST P-256";
    case Rust::GfrKeyAlgo::NISTP384:
      return "NIST P-384";
    case Rust::GfrKeyAlgo::NISTP521:
      return "NIST P-521";
    case Rust::GfrKeyAlgo::RSA2048:
      return "RSA 2048";
    case Rust::GfrKeyAlgo::RSA3072:
      return "RSA 3072";
    case Rust::GfrKeyAlgo::RSA4096:
      return "RSA 4096";
    case Rust::GfrKeyAlgo::BRAINPOOLP256:
      return "Brainpool P-256";
    case Rust::GfrKeyAlgo::BRAINPOOLP384:
      return "Brainpool P-384";
    case Rust::GfrKeyAlgo::BRAINPOOLP512:
      return "Brainpool P-512";
    case Rust::GfrKeyAlgo::ED448:
      return "ED448";
    case Rust::GfrKeyAlgo::X448:
      return "X448";
    case Rust::GfrKeyAlgo::SECP256K1:
      return "SECP256K1";
    case Rust::GfrKeyAlgo::DSA1024:
      return "DSA 1024";
    case Rust::GfrKeyAlgo::DSA2048:
      return "DSA 2048";
    case Rust::GfrKeyAlgo::DSA3072:
      return "DSA 3072";
    case Rust::GfrKeyAlgo::ED25519LEGACY:
      return "ED25519";
    case Rust::GfrKeyAlgo::KYBER768X25519:
      return "Kyber768_X25519";
    case Rust::GfrKeyAlgo::KYBER1024X448:
      return "Kyber1024_X448";
    case Rust::GfrKeyAlgo::MLDSA65ED25519:
      return "ML-DSA-65_ED25519";
    case Rust::GfrKeyAlgo::MLDSA87ED448:
      return "ML-DSA-87_ED448";
    case Rust::GfrKeyAlgo::SLHDSASHAKE128S:
      return "SLH-DSA-SHAKE-128S";
    case Rust::GfrKeyAlgo::SLHDSASHAKE128F:
      return "SLH-DSA-SHAKE-128F";
    case Rust::GfrKeyAlgo::SLHDSASHAKE256S:
      return "SLH-DSA-SHAKE-256S";
    default:
      return "Unknown";
  }
}

auto SniffRecipients(GFKeyDatabase& key_db, const GFBuffer& in_buffer)
    -> QContainer<GFRecipient> {
  Rust::GfrRecipientResultC* out_recipients = nullptr;
  size_t recipient_count = 0;
  auto err = Rust::gfr_crypto_get_recipients(
      reinterpret_cast<const uint8_t*>(in_buffer.Data()), in_buffer.Size(),
      &out_recipients, &recipient_count);

  if (err != Rust::GfrStatus::Success || out_recipients == nullptr) {
    LOG_E() << "Rust FFI get_recipients failed.";
    return {};
  }

  QContainer<GFRecipient> recipients;
  for (size_t i = 0; i < recipient_count; ++i) {
    const auto& rec = out_recipients[i];

    auto recipient = GFRecipient{
        QString::fromUtf8(rec.key_id).toUpper(),
        QString::fromUtf8(rec.pub_algo),
    };

    auto meta = key_db.GetKeyMetadata(recipient.key_id);
    auto gf_key = GetKeyByKeyIdsForDecryption(key_db, {recipient.key_id});
    recipient.status = !gf_key ? GPG_ERR_NO_KEY : GPG_ERR_NO_ERROR;

    recipients.push_back(recipient);
  }

  return recipients;
}

auto SniffIssuerKeyIds(const GFBuffer& in_buffer) -> QStringList {
  char* out_issuers = nullptr;
  auto err = Rust::gfr_crypto_get_signature_issuers(
      reinterpret_cast<const uint8_t*>(in_buffer.Data()), in_buffer.Size(),
      &out_issuers);

  if (err != Rust::GfrStatus::Success || out_issuers == nullptr) {
    LOG_E() << "Rust FFI get_signature_issuers failed.";
    return {};
  }

  auto issuers_str = QString::fromUtf8(out_issuers);
  Rust::gfr_crypto_free_string(out_issuers);

  return issuers_str.toUpper().split(",", Qt::SkipEmptyParts);
  ;
}
auto GetKeyBlocksForVerification(GFKeyDatabase& key_db,
                                 const QStringList& key_ids)
    -> QContainer<GFBuffer> {
  QContainer<GFBuffer> verified_keys_utf8;
  for (const auto& issuer_id : key_ids) {
    auto key = key_db.GetKeyBlocks(issuer_id);
    if (key && !key->public_key.Empty()) {
      verified_keys_utf8.push_back(key->public_key);
    }
  }
  return verified_keys_utf8;
}

auto GetArmoredKeyBlocksForKeys(GFKeyDatabase& key_db,
                                const QStringList& key_ids, bool secret)
    -> QContainer<GFBuffer> {
  QContainer<GFBuffer> key_blocks;

  for (const auto& key_id : key_ids) {
    auto key_block = key_db.GetKeyBlocks(key_id);
    if (!key_block) {
      LOG_E() << "failed to get key block for fpr: " << key_id;
      continue;
    }

    if (secret && key_block->secret_key.Empty()) {
      LOG_W() << "requested secret key export, but secret key block is empty "
                 "for fpr: "
              << key_id;
      continue;
    }

    if (!secret && key_block->public_key.Empty()) {
      if (key_block->secret_key.Empty()) {
        LOG_W() << "requested public key export, but public key block is empty "
                   "for fpr: "
                << key_id;
        continue;
      }

      auto secret_key_block = key_block->secret_key;
      char* public_key = nullptr;

      Rust::GfrBuffer key_block_buffer = {
          reinterpret_cast<const uint8_t*>(secret_key_block.Data()),
          secret_key_block.Size()};

      auto err =
          Rust::gfr_crypto_extract_public_key(key_block_buffer, &public_key);

      if (err != Rust::GfrStatus::Success) {
        LOG_E() << "gfr_crypto_extract_public_key error, code: "
                << static_cast<int>(err) << ", fpr: " << key_id;
        continue;
      }

      auto public_key_qs = GFBuffer(public_key);
      key_blocks.push_back(public_key_qs);
      Rust::gfr_crypto_free_string(public_key);
      continue;
    }

    key_blocks.push_back(
        (secret ? key_block->secret_key : key_block->public_key));
  }

  return key_blocks;
}

}  // namespace GpgFrontend
