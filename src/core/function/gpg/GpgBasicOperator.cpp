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

#include "GpgBasicOperator.h"

#include <gpg-error.h>

#include "core/GpgModel.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgFrontend::GpgBasicOperator::GpgBasicOperator(int channel)
    : SingletonFunctionObject<GpgBasicOperator>(channel) {}

void GpgFrontend::GpgBasicOperator::Encrypt(KeyListPtr keys,
                                            ConstBypeArrayRef in_buffer,
                                            const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      [&](const DataObjectPtr& data_object) -> GpgError {
        SPDLOG_DEBUG("key size: {}", keys->size());
        std::vector<gpgme_key_t> recipients(keys->size() + 1);

        for (const auto& key : *keys) {
          recipients.emplace_back(key);
        }

        // Last entry data_in array has to be nullptr
        recipients.emplace_back(nullptr);

        GpgData data_in(in_buffer.data(), in_buffer.size());
        GpgData data_out;

        auto err = CheckGpgError(
            gpgme_op_encrypt(ctx_.DefaultContext(), recipients.data(),
                             GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out));
        data_object->Swap(
            {NewResult(gpgme_op_encrypt_result(ctx_.DefaultContext())),
             data_out.Read2Buffer()});

        return err;
      },
      cb, "gpgme_op_encrypt", "2.1.0");
}

auto GpgFrontend::GpgBasicOperator::Decrypt(
    BypeArrayRef in_buffer, GpgFrontend::ByteArrayPtr& out_buffer,
    GpgFrontend::GpgDecrResult& result) -> GpgFrontend::GpgError {
  GpgError err;

  GpgData data_in(in_buffer.data(), in_buffer.size());
  GpgData data_out;
  err =
      CheckGpgError(gpgme_op_decrypt(ctx_.DefaultContext(), data_in, data_out));

  auto temp_data_out = data_out.Read2Buffer();
  std::swap(temp_data_out, out_buffer);

  auto temp_result = NewResult(gpgme_op_decrypt_result(ctx_.DefaultContext()));
  std::swap(result, temp_result);

  return err;
}

auto GpgFrontend::GpgBasicOperator::Verify(BypeArrayRef& in_buffer,
                                           ByteArrayPtr& sig_buffer,
                                           GpgVerifyResult& result) const
    -> GpgFrontend::GpgError {
  GpgError err;

  GpgData data_in(in_buffer.data(), in_buffer.size());
  GpgData data_out;

  if (sig_buffer != nullptr && !sig_buffer->empty()) {
    GpgData sig_data(sig_buffer->data(), sig_buffer->size());
    err = CheckGpgError(
        gpgme_op_verify(ctx_.DefaultContext(), sig_data, data_in, nullptr));
  } else {
    err = CheckGpgError(
        gpgme_op_verify(ctx_.DefaultContext(), data_in, nullptr, data_out));
  }

  auto temp_result = NewResult(gpgme_op_verify_result(ctx_.DefaultContext()));
  std::swap(result, temp_result);

  return err;
}

auto GpgFrontend::GpgBasicOperator::Sign(
    KeyListPtr signers, BypeArrayRef in_buffer, ByteArrayPtr& out_buffer,
    gpgme_sig_mode_t mode, GpgSignResult& result) -> GpgFrontend::GpgError {
  GpgError err;

  // Set Singers of this opera
  SetSigners(*signers);

  GpgData data_in(in_buffer.data(), in_buffer.size());
  GpgData data_out;

  err = CheckGpgError(
      gpgme_op_sign(ctx_.DefaultContext(), data_in, data_out, mode));

  auto temp_data_out = data_out.Read2Buffer();
  std::swap(temp_data_out, out_buffer);

  auto temp_result = NewResult(gpgme_op_sign_result(ctx_.DefaultContext()));

  std::swap(result, temp_result);

  return err;
}

auto GpgFrontend::GpgBasicOperator::DecryptVerify(
    BypeArrayRef in_buffer, ByteArrayPtr& out_buffer,
    GpgDecrResult& decrypt_result, GpgVerifyResult& verify_result) -> GpgError {
  GpgError err;

  GpgData data_in(in_buffer.data(), in_buffer.size());
  GpgData data_out;

  err = CheckGpgError(
      gpgme_op_decrypt_verify(ctx_.DefaultContext(), data_in, data_out));

  auto temp_data_out = data_out.Read2Buffer();
  std::swap(temp_data_out, out_buffer);

  auto temp_decr_result =
      NewResult(gpgme_op_decrypt_result(ctx_.DefaultContext()));
  std::swap(decrypt_result, temp_decr_result);

  auto temp_verify_result =
      NewResult(gpgme_op_verify_result(ctx_.DefaultContext()));
  std::swap(verify_result, temp_verify_result);

  return err;
}

auto GpgFrontend::GpgBasicOperator::EncryptSign(
    KeyListPtr keys, KeyListPtr signers, BypeArrayRef in_buffer,
    ByteArrayPtr& out_buffer, GpgEncrResult& encr_result,
    GpgSignResult& sign_result) -> GpgError {
  GpgError err;
  SetSigners(*signers);

  // gpgme_encrypt_result_t e_result;
  gpgme_key_t recipients[keys->size() + 1];

  // set key for user
  int index = 0;
  for (const auto& key : *keys) {
    recipients[index++] = static_cast<gpgme_key_t>(key);
  }

  // Last entry dataIn array has to be nullptr
  recipients[keys->size()] = nullptr;

  GpgData data_in(in_buffer.data(), in_buffer.size());
  GpgData data_out;

  // If the last parameter isnt 0, a private copy of data is made
  err = CheckGpgError(gpgme_op_encrypt_sign(ctx_.DefaultContext(), recipients,
                                            GPGME_ENCRYPT_ALWAYS_TRUST, data_in,
                                            data_out));

  auto temp_data_out = data_out.Read2Buffer();
  std::swap(temp_data_out, out_buffer);

  auto temp_encr_result =
      NewResult(gpgme_op_encrypt_result(ctx_.DefaultContext()));
  swap(encr_result, temp_encr_result);
  auto temp_sign_result =
      NewResult(gpgme_op_sign_result(ctx_.DefaultContext()));
  swap(sign_result, temp_sign_result);

  return err;
}

void GpgFrontend::GpgBasicOperator::SetSigners(KeyArgsList& signers) {
  gpgme_signers_clear(ctx_.DefaultContext());
  for (const GpgKey& key : signers) {
    SPDLOG_DEBUG("key fpr: {}", key.GetFingerprint());
    if (key.IsHasActualSigningCapability()) {
      SPDLOG_DEBUG("signer");
      auto error = gpgme_signers_add(ctx_.DefaultContext(), gpgme_key_t(key));
      CheckGpgError(error);
    }
  }
  if (signers.size() != gpgme_signers_count(ctx_.DefaultContext()))
    SPDLOG_DEBUG("not all signers added");
}

auto GpgFrontend::GpgBasicOperator::GetSigners()
    -> std::unique_ptr<GpgFrontend::KeyArgsList> {
  auto count = gpgme_signers_count(ctx_.DefaultContext());
  auto signers = std::make_unique<std::vector<GpgKey>>();
  for (auto i = 0U; i < count; i++) {
    auto key = GpgKey(gpgme_signers_enum(ctx_.DefaultContext(), i));
    signers->push_back(GpgKey(std::move(key)));
  }
  return signers;
}

auto GpgFrontend::GpgBasicOperator::EncryptSymmetric(
    GpgFrontend::ByteArray& in_buffer, GpgFrontend::ByteArrayPtr& out_buffer,
    GpgFrontend::GpgEncrResult& result) -> gpg_error_t {
  // deepcopy from ByteArray to GpgData
  GpgData data_in(in_buffer.data(), in_buffer.size());
  GpgData data_out;

  GpgError err = CheckGpgError(gpgme_op_encrypt(ctx_.DefaultContext(), nullptr,
                                                GPGME_ENCRYPT_SYMMETRIC,
                                                data_in, data_out));

  auto temp_data_out = data_out.Read2Buffer();
  std::swap(temp_data_out, out_buffer);

  // TODO(Saturneric): maybe a bug of gpgme
  if (gpgme_err_code(err) == GPG_ERR_NO_ERROR) {
    auto temp_result =
        NewResult(gpgme_op_encrypt_result(ctx_.DefaultContext()));
    std::swap(result, temp_result);
  }

  return err;
}
}  // namespace GpgFrontend
