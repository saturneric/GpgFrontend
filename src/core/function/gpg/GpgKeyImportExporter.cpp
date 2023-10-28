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

#include "GpgConstants.h"
#include "GpgKeyGetter.h"

GpgFrontend::GpgKeyImportExporter::GpgKeyImportExporter(int channel)
    : SingletonFunctionObject<GpgKeyImportExporter>(channel) {}

/**
 * Import key pair
 * @param inBuffer input byte array
 * @return Import information
 */
GpgFrontend::GpgImportInformation GpgFrontend::GpgKeyImportExporter::ImportKey(
    StdBypeArrayPtr in_buffer) {
  if (in_buffer->empty()) return {};

  GpgData data_in(in_buffer->data(), in_buffer->size());
  auto err = check_gpg_error(gpgme_op_import(ctx_, data_in));
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {};

  gpgme_import_result_t result;
  result = gpgme_op_import_result(ctx_);
  gpgme_import_status_t status = result->imports;
  auto import_info = std::make_unique<GpgImportInformation>(result);
  while (status != nullptr) {
    GpgImportedKey key;
    key.import_status = static_cast<int>(status->status);
    key.fpr = status->fpr;
    import_info->importedKeys.emplace_back(key);
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
bool GpgFrontend::GpgKeyImportExporter::ExportKeys(KeyIdArgsListPtr& uid_list,
                                                   ByteArrayPtr& out_buffer,
                                                   bool secret) const {
  if (uid_list->empty()) return false;

  int _mode = 0;
  if (secret) _mode |= GPGME_EXPORT_MODE_SECRET;

  auto keys = GpgKeyGetter::GetInstance().GetKeys(uid_list);
  auto keys_array = new gpgme_key_t[keys->size() + 1];

  int index = 0;
  for (const auto& key : *keys) {
    keys_array[index++] = gpgme_key_t(key);
  }
  keys_array[index] = nullptr;

  GpgData data_out;
  auto err = gpgme_op_export_keys(ctx_, keys_array, _mode, data_out);
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
bool GpgFrontend::GpgKeyImportExporter::ExportKeys(const KeyArgsList& keys,
                                                   ByteArrayPtr& out_buffer,
                                                   bool secret) const {
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
bool GpgFrontend::GpgKeyImportExporter::ExportAllKeys(
    KeyIdArgsListPtr& uid_list, ByteArrayPtr& out_buffer, bool secret) const {
  bool result = true;
  result = ExportKeys(uid_list, out_buffer, false) & result;

  ByteArrayPtr temp_buffer;
  if (secret) {
    result = ExportKeys(uid_list, temp_buffer, true) & result;
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
bool GpgFrontend::GpgKeyImportExporter::ExportSecretKey(
    const GpgKey& key, ByteArrayPtr& out_buffer) const {
  SPDLOG_DEBUG("export secret key: {}", key.GetId().c_str());

  gpgme_key_t target_key[2] = {gpgme_key_t(key), nullptr};

  GpgData data_out;
  // export private key to outBuffer
  gpgme_error_t err = gpgme_op_export_keys(ctx_, target_key,
                                           GPGME_EXPORT_MODE_SECRET, data_out);

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);

  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}

bool GpgFrontend::GpgKeyImportExporter::ExportKey(
    const GpgFrontend::GpgKey& key,
    GpgFrontend::ByteArrayPtr& out_buffer) const {
  GpgData data_out;
  auto err = gpgme_op_export(ctx_, key.GetId().c_str(), 0, data_out);

  SPDLOG_DEBUG("export keys read_bytes: {}",
               gpgme_data_seek(data_out, 0, SEEK_END));

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);
  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}

bool GpgFrontend::GpgKeyImportExporter::ExportKeyOpenSSH(
    const GpgFrontend::GpgKey& key,
    GpgFrontend::ByteArrayPtr& out_buffer) const {
  GpgData data_out;
  auto err = gpgme_op_export(ctx_, key.GetId().c_str(), GPGME_EXPORT_MODE_SSH,
                             data_out);

  SPDLOG_DEBUG("read_bytes: {}", gpgme_data_seek(data_out, 0, SEEK_END));

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);
  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}

bool GpgFrontend::GpgKeyImportExporter::ExportSecretKeyShortest(
    const GpgFrontend::GpgKey& key,
    GpgFrontend::ByteArrayPtr& out_buffer) const {
  GpgData data_out;
  auto err = gpgme_op_export(ctx_, key.GetId().c_str(),
                             GPGME_EXPORT_MODE_MINIMAL, data_out);

  SPDLOG_DEBUG("read_bytes: {}", gpgme_data_seek(data_out, 0, SEEK_END));

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);
  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}

GpgFrontend::GpgImportInformation::GpgImportInformation() = default;

GpgFrontend::GpgImportInformation::GpgImportInformation(
    gpgme_import_result_t result) {
  if (result->unchanged) unchanged = result->unchanged;
  if (result->considered) considered = result->considered;
  if (result->no_user_id) no_user_id = result->no_user_id;
  if (result->imported) imported = result->imported;
  if (result->imported_rsa) imported_rsa = result->imported_rsa;
  if (result->unchanged) unchanged = result->unchanged;
  if (result->new_user_ids) new_user_ids = result->new_user_ids;
  if (result->new_sub_keys) new_sub_keys = result->new_sub_keys;
  if (result->new_signatures) new_signatures = result->new_signatures;
  if (result->new_revocations) new_revocations = result->new_revocations;
  if (result->secret_read) secret_read = result->secret_read;
  if (result->secret_imported) secret_imported = result->secret_imported;
  if (result->secret_unchanged) secret_unchanged = result->secret_unchanged;
  if (result->not_imported) not_imported = result->not_imported;
}
