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
    GpgContext& ctx, const QSharedPointer<KeyGenerateInfo>& p_params,
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
    s_key_configs[0].algo = KeyAlgoId2GfrKeyAlgo(s_params->GetAlgo().Id());

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

auto GenerateKeyRpgpImpl(GpgContext& ctx,
                         const QSharedPointer<KeyGenerateInfo>& params,
                         const DataObjectPtr& data_object) -> GpgError {
  return GenerateKeyWithSubkeyRpgpImpl(ctx, params, nullptr, data_object);
}

auto GenerateSubKeyRpgpImpl(GpgContext& ctx, const GpgKeyPtr& key,
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

  Rust::GfrKeyConfig key_config;
  key_config.algo = KeyAlgoId2GfrKeyAlgo(params->GetAlgo().Id());
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

  data_object->Swap({
      GpgGenerateKeyResult{QString::fromUtf8(kg_result.fingerprint)},
      GpgGenerateKeyResult{QString::fromUtf8(kg_result.fingerprint)},
  });
  return GPG_ERR_NO_ERROR;
}

}  // namespace GpgFrontend