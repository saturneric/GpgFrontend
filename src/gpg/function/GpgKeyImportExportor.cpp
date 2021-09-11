#include "gpg/function/GpgKeyImportExportor.h"
#include "GpgConstants.h"
#include <gpg-error.h>

/**
 * Import key pair
 * @param inBuffer input byte array
 * @return Import information
 */
GpgFrontend::GpgImportInformation
GpgFrontend::GpgKeyImportExportor::ImportKey(StdBypeArrayPtr in_buffer) {
  LOG(INFO) << "ImportKey Called in_buffer Size " << in_buffer->size();
  GpgData data_in(in_buffer->data(), in_buffer->size());
  auto err = check_gpg_error(gpgme_op_import(ctx, data_in));
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);
  gpgme_import_result_t result;

  result = gpgme_op_import_result(ctx);
  gpgme_import_status_t status = result->imports;
  auto import_info = std::make_unique<GpgImportInformation>(result);
  LOG(INFO) << "ImportKey import_information " << result->not_imported << " "
            << result->imported << " " << result->considered;
  while (status != nullptr) {
    GpgImportedKey key;
    key.import_status = static_cast<int>(status->status);
    key.fpr = status->fpr;
    import_info->importedKeys.emplace_back(key);
    status = status->next;
    LOG(INFO) << "ImportKey Fpr " << key.fpr << " Status " << key.import_status;
  }
  return *import_info;
}

/**
 * Export Key
 * @param uid_list key ids
 * @param out_buffer output byte array
 * @return if success
 */
bool GpgFrontend::GpgKeyImportExportor::ExportKeys(
    KeyIdArgsListPtr &uid_list, BypeArrayPtr &out_buffer) const {
  if (uid_list->empty())
    return false;

  // Alleviate another crash problem caused by an unknown array out-of-bounds
  // access
  for (size_t i = 0; i < uid_list->size(); i++) {
    GpgData data_out;
    auto err = gpgme_op_export(ctx, (*uid_list)[i].c_str(), 0, data_out);
    assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

    LOG(INFO) << "exportKeys read_bytes"
              << gpgme_data_seek(data_out, 0, SEEK_END);

    auto temp_out_buffer = data_out.Read2Buffer();
    std::swap(out_buffer, temp_out_buffer);
  }

  return true;
}

/**
 * Export keys
 * @param keys keys used
 * @param outBuffer output byte array
 * @return if success
 */
bool GpgFrontend::GpgKeyImportExportor::ExportKeys(
    KeyArgsList &keys, BypeArrayPtr &out_buffer) const {
  KeyIdArgsListPtr key_ids = std::make_unique<std::vector<std::string>>();
  for (const auto &key : keys)
    key_ids->push_back(key.id());
  return ExportKeys(key_ids, out_buffer);
}

/**
 * Export the secret key of a key pair(including subkeys)
 * @param key target key pair
 * @param outBuffer output byte array
 * @return if successful
 */
bool GpgFrontend::GpgKeyImportExportor::ExportSecretKey(
    const GpgKey &key, BypeArrayPtr out_buffer) const {

  LOG(INFO) << "Export Secret Key" << key.id().c_str();

  gpgme_key_t target_key[2] = {gpgme_key_t(key), nullptr};

  GpgData data_out;

  // export private key to outBuffer
  gpgme_error_t err =
      gpgme_op_export_keys(ctx, target_key, GPGME_EXPORT_MODE_SECRET, data_out);

  auto temp_out_buffer = data_out.Read2Buffer();
  std::swap(out_buffer, temp_out_buffer);

  return check_gpg_error_2_err_code(err) == GPG_ERR_NO_ERROR;
}
