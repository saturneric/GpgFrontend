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
#include "GpgFileOpera.h"

#include "core/function/ArchiveFileOperator.h"
#include "core/function/gpg/GpgBasicOperator.h"
#include "core/model/GpgData.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto ExtractArchiveHelper(const QString& out_path)
    -> QSharedPointer<GFDataExchanger> {
  auto ex = CreateStandardGFDataExchanger();
  ArchiveFileOperator::ExtractArchiveFromDataExchanger(
      ex, out_path, [](GFError err, const DataObjectPtr&) {
        FLOG_D("extract archive from data exchanger operation, err: %d", err);
      });
  return ex;
}

void CreateArchiveHelper(const QString& in_path,
                         const QSharedPointer<GFDataExchanger>& ex) {
  auto w_ex = QWeakPointer<GFDataExchanger>(ex);

  ArchiveFileOperator::NewArchive2DataExchanger(
      in_path, ex, [=](GFError err, const DataObjectPtr&) {
        FLOG_D("new archive 2 data exchanger operation, err: %d", err);
        if (decltype(ex) p_ex = w_ex.lock(); err < 0 && p_ex != nullptr) {
          ex->CloseWrite();
        }
      });
}

GpgFileOpera::GpgFileOpera(int channel)
    : SingletonFunctionObject<GpgFileOpera>(channel) {}

auto EncryptFileGpgDataImpl(GpgContext& ctx_, const KeyArgsList& keys,
                            GpgData& data_in, bool ascii, GpgData& data_out,
                            const DataObjectPtr& data_object) -> GpgError {
  auto recipients = Convert2RawGpgMEKeyList(keys);
  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();

  auto err = CheckGpgError(
      gpgme_op_encrypt(ctx, keys.isEmpty() ? nullptr : recipients.data(),
                       GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out));
  data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx))});
  return err;
}

auto EncryptFileImpl(GpgContext& ctx_, const KeyArgsList& keys,
                     const QString& in_path, bool ascii,
                     const QString& out_path,
                     const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return EncryptFileGpgDataImpl(ctx_, keys, data_in, ascii, data_out,
                                data_object);
}

void GpgFileOpera::EncryptFile(const KeyArgsList& keys, const QString& in_path,
                               bool ascii, const QString& out_path,
                               const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) {
        return EncryptFileImpl(ctx_, keys, in_path, ascii, out_path,
                               data_object);
      },
      cb, "gpgme_op_encrypt", "2.1.0");
}

auto GpgFileOpera::EncryptFileSync(
    const KeyArgsList& keys, const QString& in_path, bool ascii,
    const QString& out_path) -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) {
        return EncryptFileImpl(ctx_, keys, in_path, ascii, out_path,
                               data_object);
      },
      "gpgme_op_encrypt", "2.1.0");
}

void GpgFileOpera::EncryptDirectory(const KeyArgsList& keys,
                                    const QString& in_path, bool ascii,
                                    const QString& out_path,
                                    const GpgOperationCallback& cb) {
  auto ex = CreateStandardGFDataExchanger();

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(ex);
        GpgData data_out(out_path, false);

        return EncryptFileGpgDataImpl(ctx_, keys, data_in, ascii, data_out,
                                      data_object);
      },
      cb, "gpgme_op_encrypt", "2.1.0");

  CreateArchiveHelper(in_path, ex);
}

auto DecryptFileGpgDataImpl(GpgContext& ctx_, GpgData& data_in,
                            GpgData& data_out,
                            const DataObjectPtr& data_object) -> GpgError {
  auto err =
      CheckGpgError(gpgme_op_decrypt(ctx_.DefaultContext(), data_in, data_out));
  data_object->Swap(
      {GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext()))});

  return err;
}

auto DecryptFileImpl(GpgContext& ctx_, const QString& in_path,
                     const QString& out_path,
                     const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return DecryptFileGpgDataImpl(ctx_, data_in, data_out, data_object);
}

void GpgFileOpera::DecryptFile(const QString& in_path, const QString& out_path,
                               const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) {
        return DecryptFileImpl(ctx_, in_path, out_path, data_object);
      },
      cb, "gpgme_op_decrypt", "2.1.0");
}

auto GpgFileOpera::DecryptFileSync(const QString& in_path,
                                   const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) {
        return DecryptFileImpl(ctx_, in_path, out_path, data_object);
      },
      "gpgme_op_decrypt", "2.1.0");
}

void GpgFileOpera::DecryptArchive(const QString& in_path,
                                  const QString& out_path,
                                  const GpgOperationCallback& cb) {
  auto ex = ExtractArchiveHelper(out_path);

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_path, true);
        GpgData data_out(ex);

        return DecryptFileGpgDataImpl(ctx_, data_in, data_out, data_object);
      },
      cb, "gpgme_op_decrypt", "2.1.0");
}

auto SignFileGpgDataImpl(GpgContext& ctx_, GpgBasicOperator& basic_opera_,
                         const KeyArgsList& keys, GpgData& data_in, bool ascii,
                         GpgData& data_out,
                         const DataObjectPtr& data_object) -> GpgError {
  GpgError err;

  // Set Singers of this opera
  basic_opera_.SetSigners(keys, ascii);

  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
  err = CheckGpgError(
      gpgme_op_sign(ctx, data_in, data_out, GPGME_SIG_MODE_DETACH));

  data_object->Swap({
      GpgSignResult(gpgme_op_sign_result(ctx)),
  });
  return err;
}

auto SignFileImpl(GpgContext& ctx_, GpgBasicOperator& basic_opera_,
                  const KeyArgsList& keys, const QString& in_path, bool ascii,
                  const QString& out_path,
                  const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return SignFileGpgDataImpl(ctx_, basic_opera_, keys, data_in, ascii, data_out,
                             data_object);
}

void GpgFileOpera::SignFile(const KeyArgsList& keys, const QString& in_path,
                            bool ascii, const QString& out_path,
                            const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) {
        return SignFileImpl(ctx_, basic_opera_, keys, in_path, ascii, out_path,
                            data_object);
      },
      cb, "gpgme_op_sign", "2.1.0");
}

auto GpgFileOpera::SignFileSync(const KeyArgsList& keys, const QString& in_path,
                                bool ascii, const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) {
        return SignFileImpl(ctx_, basic_opera_, keys, in_path, ascii, out_path,
                            data_object);
      },
      "gpgme_op_sign", "2.1.0");
}

auto VerifyFileImpl(GpgContext& ctx_, const QString& data_path,
                    const QString& sign_path,
                    const DataObjectPtr& data_object) -> GpgError {
  GpgError err;

  GpgData data_in(data_path, true);
  GpgData data_out;
  if (!sign_path.isEmpty()) {
    GpgData sig_data(sign_path, true);
    err = CheckGpgError(
        gpgme_op_verify(ctx_.DefaultContext(), sig_data, data_in, nullptr));
  } else {
    err = CheckGpgError(
        gpgme_op_verify(ctx_.DefaultContext(), data_in, nullptr, data_out));
  }

  data_object->Swap({
      GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
  });

  return err;
}

void GpgFileOpera::VerifyFile(const QString& data_path,
                              const QString& sign_path,
                              const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return VerifyFileImpl(ctx_, data_path, sign_path, data_object);
      },
      cb, "gpgme_op_verify", "2.1.0");
}

auto GpgFileOpera::VerifyFileSync(const QString& data_path,
                                  const QString& sign_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return VerifyFileImpl(ctx_, data_path, sign_path, data_object);
      },
      "gpgme_op_verify", "2.1.0");
}

auto EncryptSignFileGpgDataImpl(GpgContext& ctx_,
                                GpgBasicOperator& basic_opera_,
                                const KeyArgsList& keys,
                                const KeyArgsList& signer_keys,
                                GpgData& data_in, bool ascii, GpgData& data_out,
                                const DataObjectPtr& data_object) -> GpgError {
  GpgError err;
  auto recipients = Convert2RawGpgMEKeyList(keys);

  basic_opera_.SetSigners(signer_keys, ascii);

  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
  err = CheckGpgError(gpgme_op_encrypt_sign(
      ctx, recipients.data(), GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out));

  data_object->Swap({
      GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
      GpgSignResult(gpgme_op_sign_result(ctx)),
  });

  return err;
}

auto EncryptSignFileImpl(GpgContext& ctx_, GpgBasicOperator& basic_opera_,
                         const KeyArgsList& keys,
                         const KeyArgsList& signer_keys, const QString& in_path,
                         bool ascii, const QString& out_path,
                         const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return EncryptSignFileGpgDataImpl(ctx_, basic_opera_, keys, signer_keys,
                                    data_in, ascii, data_out, data_object);
}

void GpgFileOpera::EncryptSignFile(const KeyArgsList& keys,
                                   const KeyArgsList& signer_keys,
                                   const QString& in_path, bool ascii,
                                   const QString& out_path,
                                   const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) {
        return EncryptSignFileImpl(ctx_, basic_opera_, keys, signer_keys,
                                   in_path, ascii, out_path, data_object);
      },
      cb, "gpgme_op_encrypt_sign", "2.1.0");
}

auto GpgFileOpera::EncryptSignFileSync(
    const KeyArgsList& keys, const KeyArgsList& signer_keys,
    const QString& in_path, bool ascii,
    const QString& out_path) -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) {
        return EncryptSignFileImpl(ctx_, basic_opera_, keys, signer_keys,
                                   in_path, ascii, out_path, data_object);
      },
      "gpgme_op_encrypt_sign", "2.1.0");
}

void GpgFileOpera::EncryptSignDirectory(const KeyArgsList& keys,
                                        const KeyArgsList& signer_keys,
                                        const QString& in_path, bool ascii,
                                        const QString& out_path,
                                        const GpgOperationCallback& cb) {
  auto ex = CreateStandardGFDataExchanger();

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(ex);
        GpgData data_out(out_path, false);

        return EncryptSignFileGpgDataImpl(ctx_, basic_opera_, keys, signer_keys,
                                          data_in, ascii, data_out,
                                          data_object);
      },
      cb, "gpgme_op_encrypt_sign", "2.1.0");

  CreateArchiveHelper(in_path, ex);
}

auto DecryptVerifyFileGpgDataImpl(
    GpgContext& ctx_, GpgData& data_in, GpgData& data_out,
    const DataObjectPtr& data_object) -> GpgError {
  auto err = CheckGpgError(
      gpgme_op_decrypt_verify(ctx_.DefaultContext(), data_in, data_out));

  data_object->Swap({
      GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
      GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
  });
  return err;
}

auto DecryptVerifyFileImpl(GpgContext& ctx_, const QString& in_path,
                           const QString& out_path,
                           const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_path, true);
  GpgData data_out(out_path, false);

  return DecryptVerifyFileGpgDataImpl(ctx_, data_in, data_out, data_object);
}

void GpgFileOpera::DecryptVerifyFile(const QString& in_path,
                                     const QString& out_path,
                                     const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return DecryptVerifyFileImpl(ctx_, in_path, out_path, data_object);
      },
      cb, "gpgme_op_decrypt_verify", "2.1.0");
}

auto GpgFileOpera::DecryptVerifyFileSync(const QString& in_path,
                                         const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return DecryptVerifyFileImpl(ctx_, in_path, out_path, data_object);
      },
      "gpgme_op_decrypt_verify", "2.1.0");
}

void GpgFileOpera::DecryptVerifyArchive(const QString& in_path,
                                        const QString& out_path,
                                        const GpgOperationCallback& cb) {
  auto ex = ExtractArchiveHelper(out_path);

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_path, true);
        GpgData data_out(ex);
        return DecryptVerifyFileGpgDataImpl(ctx_, data_in, data_out,
                                            data_object);
      },
      cb, "gpgme_op_decrypt_verify", "2.1.0");
}

void GpgFileOpera::EncryptFileSymmetric(const QString& in_path, bool ascii,
                                        const QString& out_path,
                                        const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return EncryptFileImpl(ctx_, {}, in_path, ascii, out_path, data_object);
      },
      cb, "gpgme_op_encrypt_symmetric", "2.1.0");
}

auto GpgFileOpera::EncryptFileSymmetricSync(const QString& in_path, bool ascii,
                                            const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        return EncryptFileImpl(ctx_, {}, in_path, ascii, out_path, data_object);
      },
      "gpgme_op_encrypt_symmetric", "2.1.0");
}

void GpgFileOpera::EncryptDirectorySymmetric(const QString& in_path, bool ascii,
                                             const QString& out_path,
                                             const GpgOperationCallback& cb) {
  auto ex = CreateStandardGFDataExchanger();

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) {
        GpgData data_in(ex);
        GpgData data_out(out_path, false);

        return EncryptFileGpgDataImpl(ctx_, {}, data_in, ascii, data_out,
                                      data_object);
      },
      cb, "gpgme_op_encrypt_symmetric", "2.1.0");

  CreateArchiveHelper(in_path, ex);
}

auto GpgFileOpera::EncryptDirectorySymmetricSync(
    const QString& in_path, bool ascii,
    const QString& out_path) -> std::tuple<GpgError, DataObjectPtr> {
  auto ex = CreateStandardGFDataExchanger();

  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(ex);
        GpgData data_out(out_path, false);

        return EncryptFileGpgDataImpl(ctx_, {}, data_in, ascii, data_out,
                                      data_object);
      },
      "gpgme_op_encrypt_symmetric", "2.1.0");

  CreateArchiveHelper(in_path, ex);
}

}  // namespace GpgFrontend