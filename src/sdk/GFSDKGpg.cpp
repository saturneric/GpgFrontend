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

#include "GFSDKBasic.h"
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/model/DataObject.h"
#include "core/model/GpgSignResult.h"
#include "core/typedef/GpgTypedef.h"

//
#include "core/utils/GpgUtils.h"
#include "private/GFSDKPrivat.h"

auto GPGFRONTEND_MODULE_SDK_EXPORT GFGpgSignData(int channel, char** key_ids,
                                                 int key_ids_size, char* data,
                                                 int sign_mode, int ascii,
                                                 GFGpgSignResult** ps) -> int {
  auto singer_ids = CharArrayToQList(key_ids, key_ids_size);

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

  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) return -1;

  auto result =
      GpgFrontend::ExtractParams<GpgFrontend::GpgSignResult>(data_object, 0);
  auto out_buffer =
      GpgFrontend::ExtractParams<GpgFrontend::GFBuffer>(data_object, 1);

  *ps =
      static_cast<GFGpgSignResult*>(GFAllocateMemory(sizeof(GFGpgSignResult)));
  auto* s = *ps;
  s->signature = GFStrDup(out_buffer.ConvertToQByteArray());
  s->hash_algo = GFStrDup(result.HashAlgo());
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
