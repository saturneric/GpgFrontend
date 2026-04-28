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

#include "KeyGenerate.h"

#include "core/GpgCoreRust.h"
#include "core/function/rpgp/RustEngineCallback.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend {
auto GenerateKeyWithSubkeyRpgpImpl(
    OpenPGPContext& ctx, const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params,
    const DataObjectPtr& data_object) -> GpgError {
  auto& kie = KeyImportExportOperation::GetInstance(ctx.GetChannel());

  Rust::GfrKeyConfig key_config;
  key_config.algo = KeyAlgoId2GfrKeyAlgo(p_params->GetAlgo().Id());
  key_config.can_sign = p_params->IsAllowSign();
  key_config.can_encrypt = p_params->IsAllowEncr();
  key_config.can_auth = p_params->IsAllowAuth();
  key_config.has_passphrase = !p_params->IsNoPassPhrase();

  if (key_config.algo == Rust::GfrKeyAlgo::Unknown) {
    LOG_E() << "Unsupported key algorithm: " << p_params->GetAlgo().Id();
    return GPG_ERR_UNSUPPORTED_ALGORITHM;
  }

  Rust::GfrStatus err = Rust::GfrStatus::Success;
  Rust::GfrKeyGenerateResult kg_result;

  if (s_params != nullptr) {
    std::array<Rust::GfrKeyConfig, 1> s_key_configs;

    auto algo = s_params->GetAlgo().Id();

    // For hybrid key generation, the primary key algorithm string is appended
    // with the subkey algorithm string, separated by an underscore.
    if (s_params->SubAlgo().Id() != KeyGenerateInfo::kNoneAlgo.Id() &&
        !s_params->SubAlgo().Id().isEmpty()) {
      algo += "_" + s_params->SubAlgo().Id();
      LOG_D() << "hybrid subkey algo detected: " << algo;
    }

    s_key_configs[0].algo = KeyAlgoId2GfrKeyAlgo(algo);

    if (s_key_configs[0].algo == Rust::GfrKeyAlgo::Unknown) {
      LOG_E() << "Unsupported subkey algorithm: " << s_params->GetAlgo().Id();
      return GPG_ERR_UNSUPPORTED_ALGORITHM;
    }

    s_key_configs[0].can_sign = s_params->IsAllowSign();
    s_key_configs[0].can_encrypt = s_params->IsAllowEncr();
    s_key_configs[0].can_auth = s_params->IsAllowAuth();
    s_key_configs[0].has_passphrase = !s_params->IsNoPassPhrase();

    err = Rust::gfr_crypto_generate_key(
        p_params->GetUserid().toUtf8().constData(), key_config,
        s_key_configs.data(), s_key_configs.size(), FetchPasswordCallback,
        FreeCallback, &kg_result);
  } else {
    err = Rust::gfr_crypto_generate_key(
        p_params->GetUserid().toUtf8().constData(), key_config, nullptr, 0,
        FetchPasswordCallback, FreeCallback, &kg_result);
  }

  if (err != Rust::GfrStatus::Success) {
    if (err == Rust::GfrStatus::ErrorFetchPasswordFailed) {
      LOG_E() << "Key generation failed: Failed to fetch password.";
      return GPG_ERR_NO_PASSPHRASE;
    }

    data_object->Swap({GpgGenerateKeyResult{}});
    LOG_D() << "gfr_crypto_create_v6_key error, code: "
            << static_cast<int>(err);

    return GPG_ERR_GENERAL;
  }

  auto armored_s_key = QString::fromUtf8(kg_result.secret_key);
  auto armored_p_key = QString::fromUtf8(kg_result.public_key);

  Rust::gfr_crypto_free_key_generate_result(&kg_result);

  auto import_info = kie.ImportKey(GFBuffer(armored_s_key));

  data_object->Swap({
      GpgGenerateKeyResult{QString::fromUtf8(kg_result.fingerprint)},
      GpgGenerateKeyResult{QString::fromUtf8(kg_result.fingerprint)},
  });
  return GPG_ERR_NO_ERROR;
}

auto GenerateKeyRpgpImpl(OpenPGPContext& ctx,
                         const QSharedPointer<KeyGenerateInfo>& params,
                         const DataObjectPtr& data_object) -> GpgError {
  return GenerateKeyWithSubkeyRpgpImpl(ctx, params, nullptr, data_object);
}

auto GenerateSubKeyRpgpImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                            const QSharedPointer<KeyGenerateInfo>& params,
                            const DataObjectPtr& data_object) -> GpgError {
  auto& kie = KeyImportExportOperation::GetInstance(ctx.GetChannel());

  if (key == nullptr) {
    LOG_E() << "primary key is null";
    return GPG_ERR_GENERAL;
  }

  auto key_db = ctx.KeyDatabase();
  if (key_db == nullptr) {
    LOG_E() << "key database is not initialized";
    return GPG_ERR_GENERAL;
  }

  auto gf_key = key_db->GetKeyByIdentifier(key->Fingerprint());
  if (!gf_key) {
    LOG_E() << "failed to find key in database for fpr: " << key->Fingerprint();
    return GPG_ERR_GENERAL;
  }

  if (!gf_key->metadata.has_secret) {
    LOG_E() << "cannot add subkey to a public key, primary key fpr: "
            << key->Fingerprint();
    return GPG_ERR_UNSUPPORTED_OPERATION;
  }

  auto algo = params->GetAlgo().Id();

  // For hybrid key generation, the primary key algorithm string is appended
  // with the subkey algorithm string, separated by an underscore.
  if (params->SubAlgo().Id() != KeyGenerateInfo::kNoneAlgo.Id() &&
      !params->SubAlgo().Id().isEmpty()) {
    algo += "_" + params->SubAlgo().Id();
    LOG_D() << "hybrid subkey algo detected: " << algo;
  }

  Rust::GfrKeyConfig key_config;
  key_config.algo = KeyAlgoId2GfrKeyAlgo(algo);
  key_config.can_sign = params->IsAllowSign();
  key_config.can_encrypt = params->IsAllowEncr();
  key_config.can_auth = params->IsAllowAuth();
  key_config.has_passphrase = !params->IsNoPassPhrase();

  if (key_config.algo == Rust::GfrKeyAlgo::Unknown) {
    LOG_E() << "Unsupported subkey algorithm: " << params->GetAlgo().Id();
    return GPG_ERR_UNSUPPORTED_ALGORITHM;
  }

  Rust::GfrKeyGenerateResult kg_result;

  auto key_block_data = gf_key->blocks.secret_key.toUtf8();
  if (key_block_data.isEmpty()) {
    LOG_E() << "primary key block data is empty, primary key fpr: "
            << key->Fingerprint();
    return GPG_ERR_GENERAL;
  }

  auto err = Rust::gfr_crypto_add_subkey(
      ctx.GetChannel(), key_block_data.constData(), key_config,
      FetchPasswordCallback, FreeCallback, &kg_result);
  if (err != Rust::GfrStatus::Success) {
    data_object->Swap({GpgGenerateKeyResult{}});
    LOG_D() << "gfr_crypto_add_subkey error, code: " << static_cast<int>(err);
    return GPG_ERR_GENERAL;
  }

  auto armored_s_key = QString::fromUtf8(kg_result.secret_key);
  auto armored_p_key = QString::fromUtf8(kg_result.public_key);

  LOG_D() << "generated subkey, armored public key: " << armored_p_key;

  auto import_info = kie.ImportKey(GFBuffer(armored_s_key));
  if (import_info->imported == 0) {
    LOG_E() << "failed to import generated subkey into database";
    data_object->Swap({GpgGenerateKeyResult{}});
    return GPG_ERR_GENERAL;
  }

  data_object->Swap({
      GpgGenerateKeyResult{QString::fromUtf8(kg_result.fingerprint)},
  });
  return GPG_ERR_NO_ERROR;
}

auto FilterKeyAlgoByKeyRpgpImpl(OpenPGPContext& ctx, const GpgKey& key,
                                const QContainer<KeyAlgo>& algos)
    -> QContainer<KeyAlgo> {
  auto key_ver = key.KeyVersion();
  QContainer<KeyAlgo> filtered_algos;

  if (key_ver == 0) {
    LOG_E() << "invalid primary key version: " << key_ver;
    return {};
  }

  for (const auto& algo : algos) {
    auto algo_id = algo.Id();

    if (algo_id == "none") {
      // "none" algorithm is always allowed, skip the version check.
      filtered_algos.prepend(algo);
      continue;
    }

    if (key_ver == 6) {
      // For primary keys of version 4, any subkey algorithm is allowed.
      filtered_algos.append(algo);
      continue;
    }

    if (algo_id == "ky768" || algo_id == "kyber768" || algo_id == "ky024" ||
        algo_id == "kyber1024") {
      // Kyber subkeys are only allowed for primary keys of version 6.
      continue;
    }

    filtered_algos.append(algo);
  }

  return filtered_algos;
}

}  // namespace GpgFrontend