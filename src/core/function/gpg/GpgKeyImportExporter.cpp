/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "GpgKeyImportExporter.h"

#include "core/GpgModel.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgKeyImportExporter::GpgKeyImportExporter(int channel)
    : SingletonFunctionObject<GpgKeyImportExporter>(channel),
      ctx_(GpgContext::GetInstance(SingletonFunctionObject::GetChannel())) {}

/**
 * Import key pair
 * @param inBuffer input byte array
 * @return Import information
 */
auto GpgKeyImportExporter::ImportKey(StdBypeArrayPtr in_buffer)
    -> GpgImportInformation {
  if (in_buffer->empty()) return {};

  GpgData data_in(in_buffer->data(), in_buffer->size());
  auto err = CheckGpgError(gpgme_op_import(ctx_.DefaultContext(), data_in));
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {};

  gpgme_import_result_t result;
  result = gpgme_op_import_result(ctx_.DefaultContext());
  gpgme_import_status_t status = result->imports;
  auto import_info = std::make_unique<GpgImportInformation>(result);
  while (status != nullptr) {
    GpgImportedKey key;
    key.import_status = static_cast<int>(status->status);
    key.fpr = status->fpr;
    import_info->imported_keys.emplace_back(key);
    status = status->next;
  }

  return *import_info;
}

/**
 * Export Key
 * @param uid_list key ids
 * @param out_buffer output byte array
 * @return if success
 */
auto GpgKeyImportExporter::ExportKeys(KeyIdArgsListPtr& uid_list,
                                      ByteArrayPtr& out_buffer,
                                      bool secret) const -> bool {
  if (uid_list->empty()) return false;

  int mode = 0;
  if (secret) mode |= GPGME_EXPORT_MODE_SECRET;

  auto keys = GpgKeyGetter::GetInstance().GetKeys(uid_list);
  auto* keys_array = new gpgme_key_t[keys->size() + 1];

  int index = 0;
  for (const auto& key : *keys) {
    keys_array[index++] = gpgme_key_t(key);
  }
  keys_array[index] = nullptr;

  GpgData data_out;
  auto err =
      gpgme_op_export_keys(ctx_.DefaultContext(), keys_array, mode, data_out);
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return false;

  delete[] keys_array;

  SPDLOG_DEBUG("export keys read_bytes: {}",
               gpgme_data_seek(data_out, 0, SEEK_END));

  auto temp_out_buffer = data_out.Read2Buffer();

  swap(temp_out_buffer, out_buffer);

  return true;
}

/**
 * Export keys
 * @param keys keys used
 * @param outBuffer output byte array
 * @return if success
 */
auto GpgKeyImportExporter::ExportKeys(const KeyArgsList& keys,
                                      ByteArrayPtr& out_buffer,
                                      bool secret) const -> bool {
  KeyIdArgsListPtr key_ids = std::make_unique<std::vector<std::string>>();
  for (const auto& key : keys) key_ids->push_back(key.GetId());
  return ExportKeys(key_ids, out_buffer, secret);
}

/**
 * Export all the keys both private and public keys
 * @param uid_list key ids
 * @param out_buffer output byte array
 * @return if success
 */
auto GpgKeyImportExporter::ExportAllKeys(KeyIdArgsListPtr& uid_list,
                                         ByteArrayPtr& out_buffer,
                                         bool secret) const -> bool {
  bool result = true;
  result = ((static_cast<int>(ExportKeys(uid_list, out_buffer, false)) &
             static_cast<int>(result)) != 0);

  ByteArrayPtr temp_buffer;
  if (secret) {
    result = ((static_cast<int>(ExportKeys(uid_list, temp_buffer, true)) &
               static_cast<int>(result)) != 0);
  }
  out_buffer->append(*temp_buffer);
  return result;
}

/**
 * Export the secret key of a key pair(including subkeys)
 * @param key target key pair
 * @param outBuffer output byte array
 * @return if successful
 */
auto GpgKeyImportExporter::ExportSecretKey(const GpgKey& key,
                                           ByteArrayPtr& out_buffer) const
    -> bool {
  SPDLOG_DEBUG("export secret key: {}", key.GetId().c_str());

  gpgme_key_t target_key[2] = {static_cast<gpgme_key_t>(key), nullptr};

  GpgData data_out;
  // export private key to outBuffer
  gpgme_error_t err = gpgme_op_export_keys(ctx_.DefaultContext(), target_key,
                                           GPGME_EXPORT_MODE_SECRET, data_out);

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);

  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgKeyImportExporter::ExportKey(const GpgKey& key,
                                     ByteArrayPtr& out_buffer) const -> bool {
  GpgData data_out;
  auto err =
      gpgme_op_export(ctx_.DefaultContext(), key.GetId().c_str(), 0, data_out);

  SPDLOG_DEBUG("export keys read_bytes: {}",
               gpgme_data_seek(data_out, 0, SEEK_END));

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);
  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgKeyImportExporter::ExportKeyOpenSSH(const GpgKey& key,
                                            ByteArrayPtr& out_buffer) const
    -> bool {
  GpgData data_out;
  auto err = gpgme_op_export(ctx_.DefaultContext(), key.GetId().c_str(),
                             GPGME_EXPORT_MODE_SSH, data_out);

  SPDLOG_DEBUG("read_bytes: {}", gpgme_data_seek(data_out, 0, SEEK_END));

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);
  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

auto GpgKeyImportExporter::ExportSecretKeyShortest(
    const GpgKey& key, ByteArrayPtr& out_buffer) const -> bool {
  GpgData data_out;
  auto err = gpgme_op_export(ctx_.DefaultContext(), key.GetId().c_str(),
                             GPGME_EXPORT_MODE_MINIMAL, data_out);

  SPDLOG_DEBUG("read_bytes: {}", gpgme_data_seek(data_out, 0, SEEK_END));

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);
  return CheckGpgError(err) == GPG_ERR_NO_ERROR;
}

GpgImportInformation::GpgImportInformation() = default;

GpgImportInformation::GpgImportInformation(gpgme_import_result_t result) {
  if (result->unchanged != 0) unchanged = result->unchanged;
  if (result->considered != 0) considered = result->considered;
  if (result->no_user_id != 0) no_user_id = result->no_user_id;
  if (result->imported != 0) imported = result->imported;
  if (result->imported_rsa != 0) imported_rsa = result->imported_rsa;
  if (result->unchanged != 0) unchanged = result->unchanged;
  if (result->new_user_ids != 0) new_user_ids = result->new_user_ids;
  if (result->new_sub_keys != 0) new_sub_keys = result->new_sub_keys;
  if (result->new_signatures != 0) new_signatures = result->new_signatures;
  if (result->new_revocations != 0) new_revocations = result->new_revocations;
  if (result->secret_read != 0) secret_read = result->secret_read;
  if (result->secret_imported != 0) secret_imported = result->secret_imported;
  if (result->secret_unchanged != 0) {
    secret_unchanged = result->secret_unchanged;
  }
  if (result->not_imported != 0) not_imported = result->not_imported;
}

}  // namespace GpgFrontend
