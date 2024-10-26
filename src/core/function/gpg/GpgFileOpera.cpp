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

constexpr ssize_t kDataExchangerSize = 8192;

GpgFileOpera::GpgFileOpera(int channel)
    : SingletonFunctionObject<GpgFileOpera>(channel) {}

void GpgFileOpera::EncryptFile(const KeyArgsList& keys, const QString& in_path,
                               bool ascii, const QString& out_path,
                               const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        std::vector<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));
        data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx))});

        return err;
      },
      cb, "gpgme_op_encrypt", "2.1.0");
}

auto GpgFileOpera::EncryptFileSync(
    const KeyArgsList& keys, const QString& in_path, bool ascii,
    const QString& out_path) -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        std::vector<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));
        data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx))});

        return err;
      },
      "gpgme_op_encrypt", "2.1.0");
}

void GpgFileOpera::EncryptDirectory(const KeyArgsList& keys,
                                    const QString& in_path, bool ascii,
                                    const QString& out_path,
                                    const GpgOperationCallback& cb) {
  auto ex = std::make_shared<GFDataExchanger>(kDataExchangerSize);
  auto w_ex = std::weak_ptr<GFDataExchanger>(ex);

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        std::vector<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        GpgData data_in(ex);
        GpgData data_out(out_path, false);

        FLOG_D("encrypt directory start");

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));
        data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx))});

        FLOG_D("encrypt directory finished, err: %d", err);
        return err;
      },
      cb, "gpgme_op_encrypt", "2.1.0");

  ArchiveFileOperator::NewArchive2DataExchanger(
      in_path, ex, [=](GFError err, const DataObjectPtr&) {
        FLOG_D("new archive 2 data exchanger operation, err: %d", err);
        if (decltype(ex) p_ex = w_ex.lock(); err < 0 && p_ex != nullptr) {
          ex->CloseWrite();
        }
      });
}

void GpgFileOpera::DecryptFile(const QString& in_path, const QString& out_path,
                               const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto err = CheckGpgError(
            gpgme_op_decrypt(ctx_.DefaultContext(), data_in, data_out));
        data_object->Swap(
            {GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext()))});

        return err;
      },
      cb, "gpgme_op_decrypt", "2.1.0");
}

auto GpgFileOpera::DecryptFileSync(const QString& in_path,
                                   const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto err = CheckGpgError(
            gpgme_op_decrypt(ctx_.DefaultContext(), data_in, data_out));
        data_object->Swap(
            {GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext()))});

        return err;
      },
      "gpgme_op_decrypt", "2.1.0");
}

void GpgFileOpera::DecryptArchive(const QString& in_path,
                                  const QString& out_path,
                                  const GpgOperationCallback& cb) {
  auto ex = std::make_shared<GFDataExchanger>(kDataExchangerSize);

  ArchiveFileOperator::ExtractArchiveFromDataExchanger(
      ex, out_path, [](GFError err, const DataObjectPtr&) {
        FLOG_D("extract archive from data exchanger operation, err: %d", err);
      });

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_path, true);
        GpgData data_out(ex);

        auto err = CheckGpgError(
            gpgme_op_decrypt(ctx_.DefaultContext(), data_in, data_out));

        data_object->Swap(
            {GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext()))});
        return err;
      },
      cb, "gpgme_op_decrypt", "2.1.0");
}

void GpgFileOpera::SignFile(const KeyArgsList& keys, const QString& in_path,
                            bool ascii, const QString& out_path,
                            const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        // Set Singers of this opera
        GpgBasicOperator::GetInstance(GetChannel()).SetSigners(keys, ascii);

        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        err = CheckGpgError(
            gpgme_op_sign(ctx, data_in, data_out, GPGME_SIG_MODE_DETACH));

        data_object->Swap({
            GpgSignResult(gpgme_op_sign_result(ctx)),
        });
        return err;
      },
      cb, "gpgme_op_sign", "2.1.0");
}

auto GpgFileOpera::SignFileSync(const KeyArgsList& keys, const QString& in_path,
                                bool ascii, const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        // Set Singers of this opera
        GpgBasicOperator::GetInstance(GetChannel()).SetSigners(keys, ascii);

        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        err = CheckGpgError(
            gpgme_op_sign(ctx, data_in, data_out, GPGME_SIG_MODE_DETACH));

        data_object->Swap({
            GpgSignResult(gpgme_op_sign_result(ctx)),
        });
        return err;
      },
      "gpgme_op_sign", "2.1.0");
}

void GpgFileOpera::VerifyFile(const QString& data_path,
                              const QString& sign_path,
                              const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        GpgData data_in(data_path, true);
        GpgData data_out;
        if (!sign_path.isEmpty()) {
          GpgData sig_data(sign_path, true);
          err = CheckGpgError(gpgme_op_verify(ctx_.DefaultContext(), sig_data,
                                              data_in, nullptr));
        } else {
          err = CheckGpgError(gpgme_op_verify(ctx_.DefaultContext(), data_in,
                                              nullptr, data_out));
        }

        data_object->Swap({
            GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
        });

        return err;
      },
      cb, "gpgme_op_verify", "2.1.0");
}

auto GpgFileOpera::VerifyFileSync(const QString& data_path,
                                  const QString& sign_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        GpgData data_in(data_path, true);
        GpgData data_out;
        if (!sign_path.isEmpty()) {
          GpgData sig_data(sign_path, true);
          err = CheckGpgError(gpgme_op_verify(ctx_.DefaultContext(), sig_data,
                                              data_in, nullptr));
        } else {
          err = CheckGpgError(gpgme_op_verify(ctx_.DefaultContext(), data_in,
                                              nullptr, data_out));
        }

        data_object->Swap({
            GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
        });

        return err;
      },
      "gpgme_op_verify", "2.1.0");
}

void GpgFileOpera::EncryptSignFile(const KeyArgsList& keys,
                                   const KeyArgsList& signer_keys,
                                   const QString& in_path, bool ascii,
                                   const QString& out_path,
                                   const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;
        std::vector<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        GpgBasicOperator::GetInstance(GetChannel())
            .SetSigners(signer_keys, ascii);

        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        err = CheckGpgError(gpgme_op_encrypt_sign(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));

        data_object->Swap({
            GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
            GpgSignResult(gpgme_op_sign_result(ctx)),
        });
        return err;
      },
      cb, "gpgme_op_encrypt_sign", "2.1.0");
}

auto GpgFileOpera::EncryptSignFileSync(
    const KeyArgsList& keys, const KeyArgsList& signer_keys,
    const QString& in_path, bool ascii,
    const QString& out_path) -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;
        std::vector<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        GpgBasicOperator::GetInstance(GetChannel())
            .SetSigners(signer_keys, ascii);

        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        err = CheckGpgError(gpgme_op_encrypt_sign(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));

        data_object->Swap({
            GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
            GpgSignResult(gpgme_op_sign_result(ctx)),
        });
        return err;
      },
      "gpgme_op_encrypt_sign", "2.1.0");
}

void GpgFileOpera::EncryptSignDirectory(const KeyArgsList& keys,
                                        const KeyArgsList& signer_keys,
                                        const QString& in_path, bool ascii,
                                        const QString& out_path,
                                        const GpgOperationCallback& cb) {
  auto ex = std::make_shared<GFDataExchanger>(kDataExchangerSize);
  auto w_ex = std::weak_ptr<GFDataExchanger>(ex);

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;
        std::vector<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        GpgBasicOperator::GetInstance(GetChannel())
            .SetSigners(signer_keys, ascii);

        GpgData data_in(ex);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        err = CheckGpgError(gpgme_op_encrypt_sign(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));

        data_object->Swap({
            GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
            GpgSignResult(gpgme_op_sign_result(ctx)),
        });
        return err;
      },
      cb, "gpgme_op_encrypt_sign", "2.1.0");

  ArchiveFileOperator::NewArchive2DataExchanger(
      in_path, ex, [=](GFError err, const DataObjectPtr&) {
        FLOG_D("new archive 2 fd operation, err: %d", err);
        if (decltype(ex) p_ex = w_ex.lock(); err < 0 && p_ex != nullptr) {
          ex->CloseWrite();
        }
      });
}

void GpgFileOpera::DecryptVerifyFile(const QString& in_path,
                                     const QString& out_path,
                                     const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        err = CheckGpgError(
            gpgme_op_decrypt_verify(ctx_.DefaultContext(), data_in, data_out));

        data_object->Swap({
            GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
            GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
        });

        return err;
      },
      cb, "gpgme_op_decrypt_verify", "2.1.0");
}

auto GpgFileOpera::DecryptVerifyFileSync(const QString& in_path,
                                         const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        err = CheckGpgError(
            gpgme_op_decrypt_verify(ctx_.DefaultContext(), data_in, data_out));

        data_object->Swap({
            GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
            GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
        });

        return err;
      },
      "gpgme_op_decrypt_verify", "2.1.0");
}

void GpgFileOpera::DecryptVerifyArchive(const QString& in_path,
                                        const QString& out_path,
                                        const GpgOperationCallback& cb) {
  auto ex = std::make_shared<GFDataExchanger>(kDataExchangerSize);

  ArchiveFileOperator::ExtractArchiveFromDataExchanger(
      ex, out_path, [](GFError err, const DataObjectPtr&) {
        FLOG_D("extract archive from ex operation, err: %d", err);
      });

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        GpgData data_in(in_path, true);
        GpgData data_out(ex);

        err = CheckGpgError(
            gpgme_op_decrypt_verify(ctx_.DefaultContext(), data_in, data_out));

        data_object->Swap({
            GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
            GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
        });

        return err;
      },
      cb, "gpgme_op_decrypt_verify", "2.1.0");
}

void GpgFileOpera::EncryptFileSymmetric(const QString& in_path, bool ascii,
                                        const QString& out_path,
                                        const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(
            ctx, nullptr, GPGME_ENCRYPT_SYMMETRIC, data_in, data_out));
        data_object->Swap({
            GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
        });

        return err;
      },
      cb, "gpgme_op_encrypt_symmetric", "2.1.0");
}

auto GpgFileOpera::EncryptFileSymmetricSync(const QString& in_path, bool ascii,
                                            const QString& out_path)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_path, true);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(
            ctx, nullptr, GPGME_ENCRYPT_SYMMETRIC, data_in, data_out));
        data_object->Swap({
            GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
        });

        return err;
      },
      "gpgme_op_encrypt_symmetric", "2.1.0");
}

void GpgFileOpera::EncryptDerectorySymmetric(const QString& in_path, bool ascii,
                                             const QString& out_path,
                                             const GpgOperationCallback& cb) {
  auto ex = std::make_shared<GFDataExchanger>(kDataExchangerSize);

  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(ex);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(
            ctx, nullptr, GPGME_ENCRYPT_SYMMETRIC, data_in, data_out));
        data_object->Swap({
            GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
        });

        return err;
      },
      cb, "gpgme_op_encrypt_symmetric", "2.1.0");

  ArchiveFileOperator::NewArchive2DataExchanger(
      in_path, ex, [=](GFError err, const DataObjectPtr&) {
        FLOG_D("new archive 2 fd operation, err: %d", err);
      });
}

auto GpgFileOpera::EncryptDerectorySymmetricSync(
    const QString& in_path, bool ascii,
    const QString& out_path) -> std::tuple<GpgError, DataObjectPtr> {
  auto ex = std::make_shared<GFDataExchanger>(kDataExchangerSize);

  ArchiveFileOperator::NewArchive2DataExchanger(
      in_path, ex, [=](GFError err, const DataObjectPtr&) {
        FLOG_D("new archive 2 fd operation, err: %d", err);
      });

  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(ex);
        GpgData data_out(out_path, false);

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(
            ctx, nullptr, GPGME_ENCRYPT_SYMMETRIC, data_in, data_out));
        data_object->Swap({
            GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
        });

        return err;
      },
      "gpgme_op_encrypt_symmetric", "2.1.0");
}

}  // namespace GpgFrontend