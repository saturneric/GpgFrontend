#include "gpg/function/GpgKeyImportExportor.h"

/**
 * Import key pair
 * @param inBuffer input byte array
 * @return Import information
 */
GpgFrontend::GpgImportInformation
GpgFrontend::GpgKeyImportExportor::ImportKey(StdBypeArrayPtr in_buffer) {
  auto import_information = std::make_unique<GpgImportInformation>();
  GpgData data_in(in_buffer->data(), in_buffer->size());
  auto err = gpgme_op_import(ctx, data_in);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);
  gpgme_import_result_t result;

  result = gpgme_op_import_result(ctx);

  if (result->unchanged)
    import_information->unchanged = result->unchanged;
  if (result->considered)
    import_information->considered = result->considered;
  if (result->no_user_id)
    import_information->no_user_id = result->no_user_id;
  if (result->imported)
    import_information->imported = result->imported;
  if (result->imported_rsa)
    import_information->imported_rsa = result->imported_rsa;
  if (result->unchanged)
    import_information->unchanged = result->unchanged;
  if (result->new_user_ids)
    import_information->new_user_ids = result->new_user_ids;
  if (result->new_sub_keys)
    import_information->new_sub_keys = result->new_sub_keys;
  if (result->new_signatures)
    import_information->new_signatures = result->new_signatures;
  if (result->new_revocations)
    import_information->new_revocations = result->new_revocations;
  if (result->secret_read)
    import_information->secret_read = result->secret_read;
  if (result->secret_imported)
    import_information->secret_imported = result->secret_imported;
  if (result->secret_unchanged)
    import_information->secret_unchanged = result->secret_unchanged;
  if (result->not_imported)
    import_information->not_imported = result->not_imported;

  gpgme_result_unref(result);

  gpgme_import_status_t status = result->imports;
  while (status != nullptr) {
    GpgImportedKey key;
    key.importStatus = static_cast<int>(status->status);
    key.fpr = status->fpr;
    import_information->importedKeys.emplace_back(key);
    status = status->next;
  }
  return *import_information;
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
  for (int i = 0; i < uid_list->size(); i++) {
    GpgData data_out;
    auto err = gpgme_op_export(ctx, (*uid_list)[i].c_str(), 0, data_out);
    assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

    qDebug() << "exportKeys read_bytes"
             << gpgme_data_seek(data_out, 0, SEEK_END);

    auto temp_out_buffer = std::move(data_out.Read2Buffer());
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

  qDebug() << "Export Secret Key" << key.id().c_str();

  gpgme_key_t target_key[2] = {gpgme_key_t(key), nullptr};

  GpgData data_out;

  // export private key to outBuffer
  gpgme_error_t error =
      gpgme_op_export_keys(ctx, target_key, GPGME_EXPORT_MODE_SECRET, data_out);

  auto temp_out_buffer = std::move(data_out.Read2Buffer());
  std::swap(out_buffer, temp_out_buffer);
  return true;
}
