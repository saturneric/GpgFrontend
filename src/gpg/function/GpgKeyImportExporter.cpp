/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "gpg/function/GpgKeyImportExporter.h"

#include "GpgConstants.h"
#include "gpg/function/GpgKeyGetter.h"

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

  DLOG(INFO) << "exportKeys read_bytes"
             << gpgme_data_seek(data_out, 0, SEEK_END);

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
 * Export the secret key of a key pair(including subkeys)
 * @param key target key pair
 * @param outBuffer output byte array
 * @return if successful
 */
bool GpgFrontend::GpgKeyImportExporter::ExportSecretKey(
    const GpgKey& key, ByteArrayPtr& out_buffer) const {
  DLOG(INFO) << "Export Secret Key" << key.GetId().c_str();

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

  DLOG(INFO) << "exportKeys read_bytes"
             << gpgme_data_seek(data_out, 0, SEEK_END);

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

  DLOG(INFO) << "read_bytes" << gpgme_data_seek(data_out, 0, SEEK_END);

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

  DLOG(INFO) << "read_bytes" << gpgme_data_seek(data_out, 0, SEEK_END);

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);
  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}
