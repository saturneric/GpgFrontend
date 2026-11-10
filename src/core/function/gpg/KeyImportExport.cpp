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

#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgData.h"
#include "core/function/gpg/GpgErrorMap.h"
#include "core/function/gpg/GpgModelConvert.h"
#include "core/function/rpgp/KeyImportExport.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

namespace {

/**
 * @brief Catches the secret keys gpg skipped during one export.
 *
 * When the agent cannot hand a secret subkey to gpg (its pinentry was
 * cancelled, the passphrase was wrong), gpg leaves the subkey out, says so in
 * an "ERROR export_keys.secret <code>" status line and still writes the rest:
 * the primary key's stub and the user IDs. gpgme (1.24 at least) does not
 * turn that line into the operation's error, so a subkey export reported
 * success and the user saved a file without the subkey they asked for.
 *
 * While alive, it makes @p ctx pass every status line to it ("full-status")
 * and keeps the first such error; the destructor restores the context.
 */
class SkippedSecretKeyWatch {
 public:
  explicit SkippedSecretKeyWatch(gpgme_ctx_t ctx) : ctx_(ctx) {
    gpgme_get_status_cb(ctx_, &prev_cb_, &prev_value_);
    const auto* full_status = gpgme_get_ctx_flag(ctx_, "full-status");
    prev_full_status_ = full_status != nullptr ? full_status : "";

    gpgme_set_status_cb(ctx_, &SkippedSecretKeyWatch::on_status, this);
    gpgme_set_ctx_flag(ctx_, "full-status", "1");
  }

  ~SkippedSecretKeyWatch() {
    gpgme_set_ctx_flag(ctx_, "full-status", prev_full_status_.constData());
    gpgme_set_status_cb(ctx_, prev_cb_, prev_value_);
  }

  SkippedSecretKeyWatch(const SkippedSecretKeyWatch&) = delete;
  auto operator=(const SkippedSecretKeyWatch&)
      -> SkippedSecretKeyWatch& = delete;

  /**
   * @brief The error of the first secret key gpg skipped, 0 when none was.
   */
  [[nodiscard]] auto Error() const -> gpgme_error_t { return error_; }

 private:
  gpgme_ctx_t ctx_;
  gpgme_status_cb_t prev_cb_ = nullptr;
  void* prev_value_ = nullptr;
  QByteArray prev_full_status_;
  gpgme_error_t error_ = GPG_ERR_NO_ERROR;

  static auto on_status(void* opaque, const char* keyword, const char* args)
      -> gpgme_error_t {
    auto* self = static_cast<SkippedSecretKeyWatch*>(opaque);
    if (self->error_ != GPG_ERR_NO_ERROR || keyword == nullptr ||
        args == nullptr || qstrcmp(keyword, "ERROR") != 0) {
      return GPG_ERR_NO_ERROR;
    }

    // "<location> <error code>"
    const auto fields = QByteArray(args).split(' ');
    if (fields.size() < 2 || fields[0] != "export_keys.secret") {
      return GPG_ERR_NO_ERROR;
    }

    bool ok = false;
    const auto code = fields[1].toUInt(&ok);
    // A line without a usable code still means a key was left out.
    self->error_ =
        ok && gpgme_err_code(code) != GPG_ERR_NO_ERROR ? code : GPG_ERR_GENERAL;

    // gpg has already moved on to the next key; let it finish.
    return GPG_ERR_NO_ERROR;
  }
};

}  // namespace

auto ImportKeyGnuPGImpl(OpenPGPContext& ctx, const GFBuffer& in_buffer)
    -> QSharedPointer<GpgImportInformation> {
  if (in_buffer.Empty()) return {};

  auto& g_ctx = GpgCtx(ctx);
  GpgData data_in(in_buffer);
  auto err = CheckGpgError(gpgme_op_import(g_ctx.BinaryContext(), data_in));
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {};

  return SecureCreateSharedObject<GpgImportInformation>(
      ImportInformationOf(gpgme_op_import_result(g_ctx.BinaryContext())));
}

auto ExportKeysGnuPGImpl(OpenPGPContext& ctx, const GpgAbstractKeyPtrList& keys,
                         bool secret, bool ascii, bool shortest, bool ssh_mode)
    -> std::tuple<GpgError, GFBuffer> {
  if (keys.empty()) return {GPG_ERR_CANCELED, {}};

  int mode = 0;
  if (secret) mode |= GPGME_EXPORT_MODE_SECRET;
  if (shortest) mode |= GPGME_EXPORT_MODE_MINIMAL;
  if (ssh_mode) mode |= GPGME_EXPORT_MODE_SSH;

  auto recipients = Convert2GpgKeyList(ctx.GetChannel(), keys);
  QList<gpgme_key_t> gpg_recipients;
  for (const auto& key : recipients) {
    LOG_D() << "recipient key fpr: " << key.Fingerprint();
    gpg_recipients.push_back(GpgmeKeyOf(key));
  }
  gpg_recipients.push_back(nullptr);

  auto& g_ctx = GpgCtx(ctx);
  GpgData data_out;
  auto* r_ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();
  auto err = gpgme_op_export_keys(r_ctx, gpg_recipients.data(), mode, data_out);
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
    return {err, {}};
  }

  return {err, data_out.Read2GFBuffer()};
}

auto ExportKeysAsyncGnuPGImpl(OpenPGPContext& ctx,
                              const GpgAbstractKeyPtrList& keys, bool secret,
                              bool ascii, bool shortest, bool ssh_mode,
                              const DataObjectPtr& data_object) -> GpgError {
  auto [err, buffer] =
      ExportKeysGnuPGImpl(ctx, keys, secret, ascii, shortest, ssh_mode);
  data_object->Swap({buffer});
  return err;
}

auto ExportAllKeysGnuPGImpl(OpenPGPContext& ctx,
                            const GpgAbstractKeyPtrList& keys, bool secret,
                            bool ascii, const DataObjectPtr& data_object)
    -> GpgError {
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

auto ExportSubkeyGnuPGImpl(OpenPGPContext& ctx, const QString& fpr, bool ascii)
    -> std::tuple<GpgError, GFBuffer> {
  int mode = 0;
  mode |= GPGME_EXPORT_MODE_SECRET_SUBKEY;

  auto pattern = fpr;
  if (!fpr.endsWith("!")) pattern += "!";

  auto& g_ctx = GpgCtx(ctx);
  GpgData data_out;
  auto* r_ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();
  SkippedSecretKeyWatch skipped(r_ctx);
  auto err =
      gpgme_op_export(r_ctx, pattern.toLatin1().constData(), mode, data_out);
  if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) err = skipped.Error();
  if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) return {err, {}};

  return {err, data_out.Read2GFBuffer()};
}
}  // namespace GpgFrontend
