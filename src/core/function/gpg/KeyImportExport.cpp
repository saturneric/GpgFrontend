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

#include "KeyImportExport.h"

#include "core/function/rpgp/KeyImportExport.h"
#include "core/model/GpgData.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto ImportKeyGnuPGImpl(GpgContext& ctx, const GFBuffer& in_buffer)
    -> QSharedPointer<GpgImportInformation> {
  if (in_buffer.Empty()) return {};

  GpgData data_in(in_buffer);
  auto err = CheckGpgError(gpgme_op_import(ctx.BinaryContext(), data_in));
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {};

  gpgme_import_result_t result;
  result = gpgme_op_import_result(ctx.BinaryContext());
  gpgme_import_status_t status = result->imports;

  auto import_info = SecureCreateSharedObject<GpgImportInformation>(result);
  while (status != nullptr) {
    GpgImportInformation::GpgImportedKey key;
    key.import_status = static_cast<int>(status->status);
    key.fpr = status->fpr;
    import_info->imported_keys.push_back(key);
    status = status->next;
  }
  return import_info;
}

auto ExportKeysGnuPGImpl(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                         bool secret, bool ascii, bool shortest, bool ssh_mode)
    -> std::tuple<GpgError, GFBuffer> {
  if (keys.empty()) return {GPG_ERR_CANCELED, {}};

  int mode = 0;
  if (secret) mode |= GPGME_EXPORT_MODE_SECRET;
  if (shortest) mode |= GPGME_EXPORT_MODE_MINIMAL;
  if (ssh_mode) mode |= GPGME_EXPORT_MODE_SSH;

  auto keys_array = Convert2RawGpgMEKeyList(ctx.GetChannel(), keys);

  // Last entry data_in array has to be nullptr
  keys_array.push_back(nullptr);

  GpgData data_out;
  auto* r_ctx = ascii ? ctx.DefaultContext() : ctx.BinaryContext();
  auto err = gpgme_op_export_keys(r_ctx, keys_array.data(), mode, data_out);
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
    return {err, {}};
  }

  return {err, data_out.Read2GFBuffer()};
}

auto ExportKeysAsyncGnuPGImpl(GpgContext& ctx,
                              const GpgAbstractKeyPtrList& keys, bool secret,
                              bool ascii, bool shortest, bool ssh_mode,
                              const DataObjectPtr& data_object) -> GpgError {
  auto [err, buffer] =
      ExportKeysGnuPGImpl(ctx, keys, secret, ascii, shortest, ssh_mode);
  data_object->Swap({buffer});
  return err;
}

auto ExportAllKeysGnuPGImpl(GpgContext& ctx, const GpgAbstractKeyPtrList& keys,
                            bool secret, bool ascii,
                            const DataObjectPtr& data_object) -> GpgError {
  // 1. Export public keys first (secret=false, shortest=false,
  // ssh_mode=false)
  auto [err, buffer] =
      ExportKeysGnuPGImpl(ctx, keys, false, ascii, false, false);
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return err;

  // 2. If requested, export secret keys and append them to the public
  // keys buffer
  if (secret) {
    auto [sec_err, sec_buffer] =
        ExportKeysGnuPGImpl(ctx, keys, true, ascii, false, false);
    if (gpgme_err_code(sec_err) != GPG_ERR_NO_ERROR) return sec_err;

    buffer.Append(sec_buffer);
  }

  // 3. Swap the combined buffer into the output data object
  data_object->Swap({buffer});
  return GPG_ERR_NO_ERROR;
}

auto ExportSubkeyGnuPGImpl(GpgContext& ctx, const QString& fpr, bool ascii)
    -> std::tuple<GpgError, GFBuffer> {
  int mode = 0;
  mode |= GPGME_EXPORT_MODE_SECRET_SUBKEY;

  auto pattern = fpr;
  if (!fpr.endsWith("!")) pattern += "!";

  GpgData data_out;
  auto* g_ctx = ascii ? ctx.DefaultContext() : ctx.BinaryContext();
  auto err =
      gpgme_op_export(g_ctx, pattern.toLatin1().constData(), mode, data_out);
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {err, {}};

  return {err, data_out.Read2GFBuffer()};
}
}  // namespace GpgFrontend
