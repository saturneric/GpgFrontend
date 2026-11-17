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

#include "GpgKeyImportExporter.h"

#include "core/function/gpg/GpgCommandExecutor.h"
#include "core/model/GpgData.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/AsyncUtils.h"
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
auto RevocationTargetFromPacketList(const QString& packets) -> QString {
  static const QRegularExpression kKeyId(
      R"(^:signature packet: algo \d+, keyid ([0-9A-Fa-f]{16})\b)");
  static const QRegularExpression kIssuerFpr(
      R"(\(issuer fpr v\d+ ([0-9A-Fa-f]{40,64})\))");

  int packet_count = 0;
  bool key_revocation = false;
  QString key_id;
  QString issuer_fpr;
  for (const auto& line : packets.split('\n')) {
    if (line.startsWith(':')) {
      ++packet_count;
      const auto m = kKeyId.match(line);
      if (m.hasMatch()) key_id = m.captured(1);
      continue;
    }
    if (line.contains("sigclass 0x20")) key_revocation = true;
    const auto m = kIssuerFpr.match(line);
    if (m.hasMatch() && issuer_fpr.isEmpty()) issuer_fpr = m.captured(1);
  }

  // A certificate is that one signature: anything beside it (a key, a
  // second signature) is not one, whatever it would do on import.
  if (packet_count != 1 || !key_revocation || key_id.isEmpty()) return {};
  return (issuer_fpr.isEmpty() ? key_id : issuer_fpr).toUpper();
}

auto GpgKeyImportExporter::RevocationCertificateTarget(
    const GFBuffer& rev_cert) const -> QString {
  if (rev_cert.Empty()) return {};

  QTemporaryFile file;
  if (!file.open()) return {};
  const auto written = file.write(rev_cert.ConvertToQByteArray());
  file.close();
  if (written != static_cast<qint64>(rev_cert.Size())) return {};

  auto [exit_code, out, err] =
      GpgCommandExecutor::GetInstance(GetChannel())
          .GpgExecuteSync({{},
                           {"--batch", "--no-tty", "--list-packets",
                            file.fileName()},
                           {}});
  if (exit_code != 0) return {};
  return RevocationTargetFromPacketList(QString::fromUtf8(out));
}

auto GpgKeyImportExporter::ImportKey(const GFBuffer& in_buffer)
    -> QSharedPointer<GpgImportInformation> {
  if (in_buffer.Empty()) return {};

  GpgData data_in(in_buffer);
  auto err = CheckGpgError(gpgme_op_import(ctx_.BinaryContext(), data_in));
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {};

  gpgme_import_result_t result;
  result = gpgme_op_import_result(ctx_.BinaryContext());
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

/**
 * Export keys
 * @param keys keys used
 * @param outBuffer output byte array
 * @return if success
 */
auto GpgKeyImportExporter::ExportKey(const GpgAbstractKeyPtr& key, bool secret,
                                     bool ascii, bool shortest,
                                     bool ssh_mode) const
    -> std::tuple<GpgError, GFBuffer> {
  if (key == nullptr) return {GPG_ERR_CANCELED, {}};

  int mode = 0;
  if (secret) mode |= GPGME_EXPORT_MODE_SECRET;
  if (shortest) mode |= GPGME_EXPORT_MODE_MINIMAL;
  if (ssh_mode) mode |= GPGME_EXPORT_MODE_SSH;

  QContainer<gpgme_key_t> keys_array =
      Convert2RawGpgMEKeyList(GetChannel(), {key});

  GpgData data_out;
  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
  auto err = gpgme_op_export_keys(ctx, keys_array.data(), mode, data_out);
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {err, {}};

  return {err, data_out.Read2GFBuffer()};
}

/**
 * Export keys
 * @param keys keys used
 * @param outBuffer output byte array
 * @return if success
 */
void GpgKeyImportExporter::ExportKeys(const GpgAbstractKeyPtrList& keys,
                                      bool secret, bool ascii, bool shortest,
                                      bool ssh_mode,
                                      const GpgOperationCallback& cb) const {
  RunGpgOperaAsync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) -> GpgError {
        if (keys.empty()) return GPG_ERR_CANCELED;

        int mode = 0;
        if (secret) mode |= GPGME_EXPORT_MODE_SECRET;
        if (shortest) mode |= GPGME_EXPORT_MODE_MINIMAL;
        if (ssh_mode) mode |= GPGME_EXPORT_MODE_SSH;

        auto keys_array = Convert2RawGpgMEKeyList(GetChannel(), keys);

        // Last entry data_in array has to be nullptr
        keys_array.push_back(nullptr);

        GpgData data_out;
        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = gpgme_op_export_keys(ctx, keys_array.data(), mode, data_out);
        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return err;

        data_object->Swap({data_out.Read2GFBuffer()});
        return err;
      },
      cb, "gpgme_op_export_keys", "2.1.0");
}

/**
 * Export keys
 * @param keys keys used
 * @param outBuffer output byte array
 * @return if success
 */
void GpgKeyImportExporter::ExportAllKeys(const GpgAbstractKeyPtrList& keys,
                                         bool secret, bool ascii,
                                         const GpgOperationCallback& cb) const {
  RunGpgOperaAsync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) -> GpgError {
        if (keys.empty()) return GPG_ERR_CANCELED;

        int mode = 0;
        auto keys_array = Convert2RawGpgMEKeyList(GetChannel(), keys);

        // Last entry data_in array has to be nullptr
        keys_array.push_back(nullptr);

        GpgData data_out;
        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = gpgme_op_export_keys(ctx, keys_array.data(), mode, data_out);
        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return err;

        auto buffer = data_out.Read2GFBuffer();

        if (secret) {
          int mode = 0;
          mode |= GPGME_EXPORT_MODE_SECRET;

          GpgData data_out_secret;
          auto err = gpgme_op_export_keys(ctx, keys_array.data(), mode,
                                          data_out_secret);
          if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return err;

          buffer.Append(data_out_secret.Read2GFBuffer());
        }

        data_object->Swap({buffer});
        return err;
      },
      cb, "gpgme_op_export_keys", "2.1.0");
}

auto GpgKeyImportExporter::ExportSubkey(const QString& fpr, bool ascii) const
    -> std::tuple<GpgError, GFBuffer> {
  int mode = 0;
  mode |= GPGME_EXPORT_MODE_SECRET_SUBKEY;

  auto pattern = fpr;
  if (!fpr.endsWith("!")) pattern += "!";

  GpgData data_out;
  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
  auto err =
      gpgme_op_export(ctx, pattern.toLatin1().constData(), mode, data_out);
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {err, {}};

  return {err, data_out.Read2GFBuffer()};
}
}  // namespace GpgFrontend
