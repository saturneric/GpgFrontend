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

#include "GpgBasicOperator.h"

#include <gpg-error.h>

#include "core/GpgModel.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgBasicOperator::GpgBasicOperator(int channel)
    : SingletonFunctionObject<GpgBasicOperator>(channel) {}

void GpgBasicOperator::Encrypt(const KeyArgsList& keys,
                               const GFBuffer& in_buffer, bool ascii,
                               const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        if (keys.empty()) return GPG_ERR_CANCELED;

        QContainer<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        GpgData data_in(in_buffer);
        GpgData data_out;

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));
        data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
                           data_out.Read2GFBuffer()});

        return err;
      },
      cb, "gpgme_op_encrypt", "2.1.0");
}

auto GpgBasicOperator::EncryptSync(const KeyArgsList& keys,
                                   const GFBuffer& in_buffer, bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        if (keys.empty()) return GPG_ERR_CANCELED;

        QContainer<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        GpgData data_in(in_buffer);
        GpgData data_out;

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));
        data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
                           data_out.Read2GFBuffer()});

        return err;
      },
      "gpgme_op_encrypt", "2.1.0");
}

void GpgBasicOperator::EncryptSymmetric(const GFBuffer& in_buffer, bool ascii,
                                        const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_buffer);
        GpgData data_out;

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(
            ctx, nullptr, GPGME_ENCRYPT_SYMMETRIC, data_in, data_out));
        data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
                           data_out.Read2GFBuffer()});

        return err;
      },
      cb, "gpgme_op_encrypt_symmetric", "2.1.0");
}

auto GpgBasicOperator::EncryptSymmetricSync(const GFBuffer& in_buffer,
                                            bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_buffer);
        GpgData data_out;

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        auto err = CheckGpgError(gpgme_op_encrypt(
            ctx, nullptr, GPGME_ENCRYPT_SYMMETRIC, data_in, data_out));
        data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
                           data_out.Read2GFBuffer()});

        return err;
      },
      "gpgme_op_encrypt_symmetric", "2.1.0");
}

void GpgBasicOperator::Decrypt(const GFBuffer& in_buffer,
                               const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_buffer);
        GpgData data_out;

        auto err = CheckGpgError(
            gpgme_op_decrypt(ctx_.DefaultContext(), data_in, data_out));
        data_object->Swap(
            {GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
             data_out.Read2GFBuffer()});

        return err;
      },
      cb, "gpgme_op_decrypt", "2.1.0");
}

auto GpgBasicOperator::DecryptSync(const GFBuffer& in_buffer)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgData data_in(in_buffer);
        GpgData data_out;

        auto err = CheckGpgError(
            gpgme_op_decrypt(ctx_.DefaultContext(), data_in, data_out));
        data_object->Swap(
            {GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
             data_out.Read2GFBuffer()});

        return err;
      },
      "gpgme_op_decrypt", "2.1.0");
}

void GpgBasicOperator::Verify(const GFBuffer& in_buffer,
                              const GFBuffer& sig_buffer,
                              const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        GpgData data_in(in_buffer);
        GpgData data_out;

        if (!sig_buffer.Empty()) {
          GpgData sig_data(sig_buffer);
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

auto GpgBasicOperator::VerifySync(const GFBuffer& in_buffer,
                                  const GFBuffer& sig_buffer)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        GpgData data_in(in_buffer);
        GpgData data_out;

        if (!sig_buffer.Empty()) {
          GpgData sig_data(sig_buffer);
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

void GpgBasicOperator::Sign(const KeyArgsList& signers,
                            const GFBuffer& in_buffer, GpgSignMode mode,
                            bool ascii, const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        if (signers.empty()) return GPG_ERR_CANCELED;

        GpgError err;

        // Set Singers of this opera
        SetSigners(signers, ascii);

        GpgData data_in(in_buffer);
        GpgData data_out;

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        err = CheckGpgError(gpgme_op_sign(ctx, data_in, data_out, mode));

        data_object->Swap({GpgSignResult(gpgme_op_sign_result(ctx)),
                           data_out.Read2GFBuffer()});
        return err;
      },
      cb, "gpgme_op_sign", "2.1.0");
}

auto GpgBasicOperator::SignSync(
    const KeyArgsList& signers, const GFBuffer& in_buffer, GpgSignMode mode,
    bool ascii) -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        if (signers.empty()) return GPG_ERR_CANCELED;

        GpgError err;

        // Set Singers of this opera
        SetSigners(signers, ascii);

        GpgData data_in(in_buffer);
        GpgData data_out;

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        err = CheckGpgError(gpgme_op_sign(ctx, data_in, data_out, mode));

        data_object->Swap({GpgSignResult(gpgme_op_sign_result(ctx)),
                           data_out.Read2GFBuffer()});
        return err;
      },
      "gpgme_op_sign", "2.1.0");
}

void GpgBasicOperator::DecryptVerify(const GFBuffer& in_buffer,
                                     const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        GpgData data_in(in_buffer);
        GpgData data_out;

        err = CheckGpgError(
            gpgme_op_decrypt_verify(ctx_.DefaultContext(), data_in, data_out));

        data_object->Swap(
            {GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
             GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
             data_out.Read2GFBuffer()});

        return err;
      },
      cb, "gpgme_op_decrypt_verify", "2.1.0");
}

auto GpgBasicOperator::DecryptVerifySync(const GFBuffer& in_buffer)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        GpgError err;

        GpgData data_in(in_buffer);
        GpgData data_out;

        err = CheckGpgError(
            gpgme_op_decrypt_verify(ctx_.DefaultContext(), data_in, data_out));

        data_object->Swap(
            {GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
             GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
             data_out.Read2GFBuffer()});

        return err;
      },
      "gpgme_op_decrypt_verify", "2.1.0");
}

void GpgBasicOperator::EncryptSign(const KeyArgsList& keys,
                                   const KeyArgsList& signers,
                                   const GFBuffer& in_buffer, bool ascii,
                                   const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        if (keys.empty() || signers.empty()) return GPG_ERR_CANCELED;

        GpgError err;
        QContainer<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        SetSigners(signers, ascii);

        GpgData data_in(in_buffer);
        GpgData data_out;

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        err = CheckGpgError(gpgme_op_encrypt_sign(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));

        data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
                           GpgSignResult(gpgme_op_sign_result(ctx)),
                           data_out.Read2GFBuffer()});
        return err;
      },
      cb, "gpgme_op_encrypt_sign", "2.1.0");
}

auto GpgBasicOperator::EncryptSignSync(const KeyArgsList& keys,
                                       const KeyArgsList& signers,
                                       const GFBuffer& in_buffer, bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      [=](const DataObjectPtr& data_object) -> GpgError {
        if (keys.empty() || signers.empty()) return GPG_ERR_CANCELED;

        GpgError err;
        QContainer<gpgme_key_t> recipients(keys.begin(), keys.end());

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        SetSigners(signers, ascii);

        GpgData data_in(in_buffer);
        GpgData data_out;

        auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
        err = CheckGpgError(gpgme_op_encrypt_sign(ctx, recipients.data(),
                                                  GPGME_ENCRYPT_ALWAYS_TRUST,
                                                  data_in, data_out));

        data_object->Swap({GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
                           GpgSignResult(gpgme_op_sign_result(ctx)),
                           data_out.Read2GFBuffer()});
        return err;
      },
      "gpgme_op_encrypt_sign", "2.1.0");
}

void GpgBasicOperator::SetSigners(const KeyArgsList& signers, bool ascii) {
  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();

  gpgme_signers_clear(ctx);

  for (const GpgKey& key : signers) {
    LOG_D() << "signer's key fpr: " << key.GetFingerprint();
    if (key.IsHasActualSigningCapability()) {
      auto error = gpgme_signers_add(ctx, gpgme_key_t(key));
      CheckGpgError(error);
    }
  }
  if (signers.size() != gpgme_signers_count(ctx_.DefaultContext())) {
    FLOG_D("not all signers added");
  }
}

auto GpgBasicOperator::GetSigners(bool ascii) -> KeyArgsList {
  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();

  auto count = gpgme_signers_count(ctx);
  auto signers = KeyArgsList{};
  for (auto i = 0U; i < count; i++) {
    auto key = GpgKey(gpgme_signers_enum(ctx, i));
    signers.push_back(GpgKey(std::move(key)));
  }
  return signers;
}
}  // namespace GpgFrontend
