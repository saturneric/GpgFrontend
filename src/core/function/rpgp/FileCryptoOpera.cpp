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

#include "FileCryptoOpera.h"

#include "core/GpgCoreRust.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/function/rpgp/RustEngineCallback.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend {

auto EncryptFileRpgpImpl(GpgContext& ctx_, const GpgAbstractKeyPtrList& keys,
                         const QString& in_path, bool ascii,
                         const QString& out_path,
                         const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  // 1. Vector to hold the actual memory of the UTF-8 strings
  QContainer<QByteArray> key_blocks_utf8 =
      GetPublicKeysByKeyIdsForEncryption(*key_db, keys);
  if (key_blocks_utf8.empty()) {
    LOG_E() << "No valid recipients found for encryption.";
    return GPG_ERR_GENERAL;  // Or appropriate error code
  }

  // 2. Vector to hold the pointers to pass to Rust FFI
  std::vector<const char*> recipient_cstrs;
  recipient_cstrs.reserve(key_blocks_utf8.size());
  for (const auto& ba : key_blocks_utf8) {
    recipient_cstrs.push_back(ba.constData());
  }

  auto in_file_path_utf8 = in_path.toUtf8();
  auto out_file_path_utf8 = out_path.toUtf8();
  Rust::GfrEncryptResultC encrypt_result;

  // Call Rust FFI. Ensure in_buffer is a null-terminated C-string if Rust
  // expects it.
  auto status = Rust::gfr_crypto_encrypt_file(
      in_file_path_utf8.constData(), out_file_path_utf8.constData(),
      recipient_cstrs.data(), recipient_cstrs.size(), ascii, &encrypt_result);

  if (status != Rust::GfrStatus::Success) {
    LOG_E() << "Rust FFI encryption failed.";
    return GPG_ERR_GENERAL;
  }

  GFEncryptResult result = GfrEncryptResultC2GFEncryptResult(encrypt_result);
  Rust::gfr_crypto_free_encrypt_result(&encrypt_result);

  data_object->Swap({GpgEncryptResult(result)});
  return GPG_ERR_NO_ERROR;
}

auto DecryptFileRpgpImpl(GpgContext& ctx_, const QString& in_path,
                         const QString& out_path,
                         const DataObjectPtr& data_object) -> GpgError {
  auto key_db = ctx_.KeyDatabase();
  if (!key_db) {
    LOG_E() << "Failed to get key database from context";
    return GPG_ERR_GENERAL;
  }

  Rust::GfrDecryptResultC decrypt_result;
  auto in_file_path_utf8 = in_path.toUtf8();
  auto out_file_path_utf8 = out_path.toUtf8();

  auto err = Rust::gfr_crypto_decrypt_file(
      ctx_.GetChannel(), in_file_path_utf8.constData(),
      out_file_path_utf8.constData(), false, FetchSecretKeyCallback,
      FetchPasswordCallback, FreeCallback, key_db.data(), &decrypt_result);

  if (err != Rust::GfrStatus::Success) {
    if (err == Rust::GfrStatus::ErrorDecryptionFailed) {
      data_object->Swap({GpgDecryptResult{}, GFBuffer()});
      return GPG_ERR_BAD_PASSPHRASE;
    }

    LOG_E() << "Rust FFI decryption failed."
            << " Status: " << static_cast<int>(err);
    return GPG_ERR_GENERAL;
  }

  GFDecryptResult result = GfrDecryptResultC2GFDecryptResult(decrypt_result);
  Rust::gfr_crypto_free_decrypt_result(&decrypt_result);

  data_object->Swap({
      GpgDecryptResult(result),
  });

  return GPG_ERR_NO_ERROR;
}

}  // namespace GpgFrontend