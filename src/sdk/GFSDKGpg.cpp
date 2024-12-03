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

#include "GFSDKGpg.h"

// std::memset
#include <cstring>

#include "GFSDKBasic.h"
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/model/DataObject.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/GpgUtils.h"
#include "ui/UIModuleManager.h"

//
#include "private/GFSDKPrivat.h"

auto GPGFRONTEND_MODULE_SDK_EXPORT GFGpgSignData(int channel, char** key_ids,
                                                 int key_ids_size, char* data,
                                                 int sign_mode, int ascii,
                                                 GFGpgSignResult** ps) -> int {
  void* mem = GFAllocateMemory(sizeof(GFGpgSignResult));
  if (mem == nullptr) {
    *ps = nullptr;
    return -1;
  }

  std::memset(mem, 0, sizeof(GFGpgSignResult));
  *ps = new (mem) GFGpgSignResult{};
  auto* s = *ps;

  auto singer_ids = CharArrayToQStringList(key_ids, key_ids_size);

  GpgFrontend::KeyArgsList signer_keys;
  for (const auto& signer_id : singer_ids) {
    auto key =
        GpgFrontend::GpgKeyGetter::GetInstance(channel).GetKey(signer_id);
    if (key.IsGood()) signer_keys.push_back(key);
  }

  if (signer_keys.empty()) return -1;

  auto in_buffer = GpgFrontend::GFBuffer(GFUnStrDup(data).toUtf8());

  auto gpg_sign_mode =
      sign_mode == 0 ? GPGME_SIG_MODE_NORMAL : GPGME_SIG_MODE_DETACH;

  auto [err, data_object] =
      GpgFrontend::GpgBasicOperator::GetInstance(channel).SignSync(
          signer_keys, in_buffer, gpg_sign_mode, ascii != 0);

  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    s->error_string = GFStrDup(GpgFrontend::DescribeGpgErrCode(err).second);
    return -1;
  }

  auto result =
      GpgFrontend::ExtractParams<GpgFrontend::GpgSignResult>(data_object, 0);
  auto out_buffer =
      GpgFrontend::ExtractParams<GpgFrontend::GFBuffer>(data_object, 1);

  auto capsule_id =
      GpgFrontend::UI::UIModuleManager::GetInstance().MakeCapsule(result);

  s->signature = GFStrDup(out_buffer.ConvertToQByteArray());
  s->hash_algo = GFStrDup(result.HashAlgo());
  s->capsule_id = GFStrDup(capsule_id);
  s->error_string = GFStrDup(GpgFrontend::DescribeGpgErrCode(err).second);
  return 0;
}

auto GPGFRONTEND_MODULE_SDK_EXPORT GFGpgPublicKey(int channel, char* key_id,
                                                  int ascii) -> char* {
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(channel).GetKey(
      GFUnStrDup(key_id));

  if (!key.IsGood()) return nullptr;

  auto [err, buffer] =
      GpgFrontend::GpgKeyImportExporter::GetInstance(channel).ExportKey(
          key, false, ascii != 0, true);

  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) return nullptr;

  return GFStrDup(buffer.ConvertToQByteArray());
}

auto GPGFRONTEND_MODULE_SDK_EXPORT GFGpgKeyPrimaryUID(int channel, char* key_id,
                                                      GFGpgKeyUID** ps) -> int {
  auto key = GpgFrontend::GpgKeyGetter::GetInstance(channel).GetKey(
      GFUnStrDup(key_id));

  if (!key.IsGood()) return -1;

  auto uids = key.GetUIDs();
  auto& primary_uid = uids->front();

  *ps = static_cast<GFGpgKeyUID*>(GFAllocateMemory(sizeof(GFGpgKeyUID)));

  auto* s = *ps;
  s->name = GFStrDup(primary_uid.GetName());
  s->email = GFStrDup(primary_uid.GetEmail());
  s->comment = GFStrDup(primary_uid.GetComment());
  return 0;
}

auto GPGFRONTEND_MODULE_SDK_EXPORT
GFGpgEncryptData(int channel, char** key_ids, int key_ids_size, char* data,
                 int ascii, GFGpgEncryptionResult** ps) -> int {
  void* mem = GFAllocateMemory(sizeof(GFGpgEncryptionResult));
  if (mem == nullptr) {
    *ps = nullptr;
    return -1;
  }

  std::memset(mem, 0, sizeof(GFGpgEncryptionResult));
  *ps = new (mem) GFGpgEncryptionResult{};
  auto* s = *ps;

  auto encrypt_key_ids = CharArrayToQStringList(key_ids, key_ids_size);

  GpgFrontend::KeyArgsList encrypt_keys;
  for (const auto& encrypt_key_id : encrypt_key_ids) {
    auto key =
        GpgFrontend::GpgKeyGetter::GetInstance(channel).GetKey(encrypt_key_id);
    if (key.IsGood()) encrypt_keys.push_back(key);
  }

  if (encrypt_keys.empty()) return -1;

  auto in_buffer = GpgFrontend::GFBuffer(GFUnStrDup(data).toUtf8());

  auto [err, data_object] =
      GpgFrontend::GpgBasicOperator::GetInstance(channel).EncryptSync(
          encrypt_keys, in_buffer, ascii != 0);

  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    s->error_string = GFStrDup(GpgFrontend::DescribeGpgErrCode(err).second);
    return -1;
  }

  auto result =
      GpgFrontend::ExtractParams<GpgFrontend::GpgEncryptResult>(data_object, 0);
  auto out_buffer =
      GpgFrontend::ExtractParams<GpgFrontend::GFBuffer>(data_object, 1);

  auto capsule_id =
      GpgFrontend::UI::UIModuleManager::GetInstance().MakeCapsule(result);

  s->encrypted_data = GFStrDup(out_buffer.ConvertToQByteArray());
  s->capsule_id = GFStrDup(capsule_id);
  s->error_string = GFStrDup(GpgFrontend::DescribeGpgErrCode(err).second);
  return 0;
}

auto GPGFRONTEND_MODULE_SDK_EXPORT
GFGpgDecryptData(int channel, char* data, GFGpgDecryptResult** ps) -> int {
  void* mem = GFAllocateMemory(sizeof(GFGpgDecryptResult));
  if (mem == nullptr) {
    *ps = nullptr;
    return -1;
  }

  std::memset(mem, 0, sizeof(GFGpgDecryptResult));
  *ps = new (mem) GFGpgDecryptResult{};
  auto* s = *ps;

  auto in_buffer = GpgFrontend::GFBuffer(GFUnStrDup(data).toUtf8());

  auto [err, data_object] =
      GpgFrontend::GpgBasicOperator::GetInstance(channel).DecryptSync(
          in_buffer);

  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    s->error_string = GFStrDup(GpgFrontend::DescribeGpgErrCode(err).second);
    return -1;
  }

  auto result =
      GpgFrontend::ExtractParams<GpgFrontend::GpgDecryptResult>(data_object, 0);
  auto out_buffer =
      GpgFrontend::ExtractParams<GpgFrontend::GFBuffer>(data_object, 1);

  auto capsule_id =
      GpgFrontend::UI::UIModuleManager::GetInstance().MakeCapsule(result);

  s->decrypted_data = GFStrDup(out_buffer.ConvertToQByteArray());
  s->capsule_id = GFStrDup(capsule_id);
  s->error_string = GFStrDup(GpgFrontend::DescribeGpgErrCode(err).second);
  return 0;
}

auto GPGFRONTEND_MODULE_SDK_EXPORT GFGpgVerifyData(
    int channel, char* data, char* signature, GFGpgVerifyResult** ps) -> int {
  void* mem = GFAllocateMemory(sizeof(GFGpgVerifyResult));
  if (mem == nullptr) {
    *ps = nullptr;
    return -1;
  }

  std::memset(mem, 0, sizeof(GFGpgVerifyResult));
  *ps = new (mem) GFGpgVerifyResult{};
  auto* s = *ps;

  auto in_buffer = GpgFrontend::GFBuffer(GFUnStrDup(data).toUtf8());
  auto sig_buffer = GpgFrontend::GFBuffer(GFUnStrDup(signature).toUtf8());

  auto [err, data_object] =
      GpgFrontend::GpgBasicOperator::GetInstance(channel).VerifySync(
          in_buffer, sig_buffer);

  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    s->error_string = GFStrDup(GpgFrontend::DescribeGpgErrCode(err).second);
    return -1;
  }

  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) return -1;

  auto result =
      GpgFrontend::ExtractParams<GpgFrontend::GpgVerifyResult>(data_object, 0);

  auto capsule_id =
      GpgFrontend::UI::UIModuleManager::GetInstance().MakeCapsule(result);

  s->capsule_id = GFStrDup(capsule_id);
  s->error_string = GFStrDup(GpgFrontend::DescribeGpgErrCode(err).second);
  return 0;
}
