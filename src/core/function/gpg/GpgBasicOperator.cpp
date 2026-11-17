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

#include "core/model/GpgData.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

GpgBasicOperator::GpgBasicOperator(int channel)
    : SingletonFunctionObject<GpgBasicOperator>(channel) {}

auto SetSignersImpl(GpgContext& ctx_, const GpgAbstractKeyPtrList& signers,
                    bool ascii) -> GpgError {
  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();

  gpgme_signers_clear(ctx);

  // gpgme signs with gpg's default key when the signer list is empty, so a
  // signer quietly left out here becomes a signature by someone else that is
  // still reported as a success. Every requested key has to make it in.
  for (const auto& signer : signers) {
    if (signer == nullptr) return GPG_ERR_NO_SECKEY;
  }
  auto keys = ConvertKey2GpgKeyList(ctx_.GetChannel(), signers);

  QSet<QString> converted;
  for (const auto& key : keys) converted.insert(key->ID());
  for (const auto& signer : signers) {
    // a disabled key is dropped by the conversion above
    if (signer->KeyType() == GpgAbstractKeyType::kGPG_KEY &&
        !converted.contains(signer->ID())) {
      LOG_W() << "signer cannot be used:" << signer->ID();
      gpgme_signers_clear(ctx);
      return GPG_ERR_NO_SECKEY;
    }
  }

  for (const auto& key : keys) {
    LOG_D() << "signer's key fpr: " << key->Fingerprint();
    if (!key->IsHasSignCap()) {
      LOG_W() << "signer has no usable signing key:" << key->Fingerprint();
      gpgme_signers_clear(ctx);
      return GPG_ERR_NO_SECKEY;
    }
    auto error =
        CheckGpgError(gpgme_signers_add(ctx, static_cast<gpgme_key_t>(*key)));
    if (error != GPG_ERR_NO_ERROR) {
      gpgme_signers_clear(ctx);
      return error;
    }
  }

  if (gpgme_signers_count(ctx) == 0) return GPG_ERR_NO_SECKEY;
  return GPG_ERR_NO_ERROR;
}

auto EncryptImpl(GpgContext& ctx_, const GpgAbstractKeyPtrList& keys,
                 const GFBuffer& in_buffer, bool ascii,
                 const DataObjectPtr& data_object) -> GpgError {
  // Converting leaves disabled and missing keys out without a word, and the
  // message would go out without them.
  if (HasUnusableKey(ctx_.GetChannel(), keys)) {
    data_object->Swap({GpgEncryptResult(), GFBuffer()});
    return GPG_ERR_UNUSABLE_PUBKEY;
  }
  auto recipients = Convert2RawGpgMEKeyList(ctx_.GetChannel(), keys);

  GpgData data_in(in_buffer);
  GpgData data_out;

  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
  auto err = CheckGpgError(
      gpgme_op_encrypt(ctx, keys.isEmpty() ? nullptr : recipients.data(),
                       GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out));
  data_object->Swap({
      GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
      data_out.Read2GFBuffer(),
  });

  return err;
}

void GpgBasicOperator::Encrypt(const GpgAbstractKeyPtrList& keys,
                               const GFBuffer& in_buffer, bool ascii,
                               const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return EncryptImpl(ctx_, keys, in_buffer, ascii, data_object);
      },
      cb, "gpgme_op_encrypt", "2.2.0");
}

auto GpgBasicOperator::EncryptSync(const GpgAbstractKeyPtrList& keys,
                                   const GFBuffer& in_buffer, bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return EncryptImpl(ctx_, keys, in_buffer, ascii, data_object);
      },
      "gpgme_op_encrypt", "2.2.0");
}

void GpgBasicOperator::EncryptSymmetric(const GFBuffer& in_buffer, bool ascii,
                                        const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return EncryptImpl(ctx_, {}, in_buffer, ascii, data_object);
      },
      cb, "gpgme_op_encrypt_symmetric", "2.2.0");
}

auto GpgBasicOperator::EncryptSymmetricSync(const GFBuffer& in_buffer,
                                            bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return EncryptImpl(ctx_, {}, in_buffer, ascii, data_object);
      },
      "gpgme_op_encrypt_symmetric", "2.2.0");
}

auto DecryptImpl(GpgContext& ctx_, const GFBuffer& in_buffer,
                 const DataObjectPtr& data_object) -> GpgError {
  GpgData data_in(in_buffer);
  GpgData data_out;

  auto err =
      CheckGpgError(gpgme_op_decrypt(ctx_.DefaultContext(), data_in, data_out));
  data_object->Swap({
      GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
      data_out.Read2GFBuffer(),
  });

  return err;
}

void GpgBasicOperator::Decrypt(const GFBuffer& in_buffer,
                               const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return DecryptImpl(ctx_, in_buffer, data_object);
      },
      cb, "gpgme_op_decrypt", "2.2.0");
}

auto GpgBasicOperator::DecryptSync(const GFBuffer& in_buffer)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return DecryptImpl(ctx_, in_buffer, data_object);
      },
      "gpgme_op_decrypt", "2.2.0");
}

auto VerifyImpl(GpgContext& ctx_, const GFBuffer& in_buffer,
                const GFBuffer& sig_buffer, const DataObjectPtr& data_object)
    -> GpgError {
  GpgError err;

  GpgData data_in(in_buffer);
  GpgData data_out;

  if (!sig_buffer.Empty()) {
    GpgData sig_data(sig_buffer);
    err = CheckGpgError(
        gpgme_op_verify(ctx_.DefaultContext(), sig_data, data_in, nullptr));
  } else {
    err = CheckGpgError(
        gpgme_op_verify(ctx_.DefaultContext(), data_in, nullptr, data_out));
  }

  data_object->Swap({
      GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
      GFBuffer(),
  });

  return err;
}

void GpgBasicOperator::Verify(const GFBuffer& in_buffer,
                              const GFBuffer& sig_buffer,
                              const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) -> GpgError {
        return VerifyImpl(ctx_, in_buffer, sig_buffer, data_object);
      },
      cb, "gpgme_op_verify", "2.2.0");
}

auto GpgBasicOperator::VerifySync(const GFBuffer& in_buffer,
                                  const GFBuffer& sig_buffer)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return VerifyImpl(ctx_, in_buffer, sig_buffer, data_object);
      },
      "gpgme_op_verify", "2.2.0");
}

auto SignImpl(GpgContext& ctx_, const GpgAbstractKeyPtrList& signers,
              const GFBuffer& in_buffer, GpgSignMode mode, bool ascii,
              const DataObjectPtr& data_object) -> GpgError {
  if (signers.empty()) return GPG_ERR_CANCELED;

  GpgError err;

  // Set Singers of this opera
  err = SetSignersImpl(ctx_, signers, ascii);
  if (err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgSignResult(), GFBuffer()});
    return err;
  }

  GpgData data_in(in_buffer);
  GpgData data_out;

  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
  err = CheckGpgError(gpgme_op_sign(ctx, data_in, data_out, mode));

  data_object->Swap({
      GpgSignResult(gpgme_op_sign_result(ctx)),
      data_out.Read2GFBuffer(),
  });
  return err;
}

void GpgBasicOperator::Sign(const GpgAbstractKeyPtrList& signers,
                            const GFBuffer& in_buffer, GpgSignMode mode,
                            bool ascii, const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return SignImpl(ctx_, signers, in_buffer, mode, ascii, data_object);
      },
      cb, "gpgme_op_sign", "2.2.0");
}

auto GpgBasicOperator::SignSync(const GpgAbstractKeyPtrList& signers,
                                const GFBuffer& in_buffer, GpgSignMode mode,
                                bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return SignImpl(ctx_, signers, in_buffer, mode, ascii, data_object);
      },
      "gpgme_op_sign", "2.2.0");
}

auto DecryptVerifyImpl(GpgContext& ctx_, const GFBuffer& in_buffer,
                       const DataObjectPtr& data_object) -> GpgError {
  GpgError err;

  GpgData data_in(in_buffer);
  GpgData data_out;

  err = CheckGpgError(
      gpgme_op_decrypt_verify(ctx_.DefaultContext(), data_in, data_out));

  data_object->Swap({
      GpgDecryptResult(gpgme_op_decrypt_result(ctx_.DefaultContext())),
      GpgVerifyResult(gpgme_op_verify_result(ctx_.DefaultContext())),
      data_out.Read2GFBuffer(),
  });

  return err;
}

void GpgBasicOperator::DecryptVerify(const GFBuffer& in_buffer,
                                     const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) {
        return DecryptVerifyImpl(ctx_, in_buffer, data_object);
      },
      cb, "gpgme_op_decrypt_verify", "2.2.0");
}

auto GpgBasicOperator::DecryptVerifySync(const GFBuffer& in_buffer)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) -> GpgError {
        return DecryptVerifyImpl(ctx_, in_buffer, data_object);
      },
      "gpgme_op_decrypt_verify", "2.2.0");
}

auto EncryptSignImpl(GpgContext& ctx_, const GpgAbstractKeyPtrList& keys,
                     const GpgAbstractKeyPtrList& signers,
                     const GFBuffer& in_buffer, bool ascii,
                     const DataObjectPtr& data_object) -> GpgError {
  if (keys.empty() || signers.empty()) return GPG_ERR_CANCELED;

  GpgError err;
  if (HasUnusableKey(ctx_.GetChannel(), keys)) {
    data_object->Swap({GpgEncryptResult(), GpgSignResult(), GFBuffer()});
    return GPG_ERR_UNUSABLE_PUBKEY;
  }
  auto recipients = Convert2RawGpgMEKeyList(ctx_.GetChannel(), keys);

  // Last entry data_in array has to be nullptr
  recipients.push_back(nullptr);

  err = SetSignersImpl(ctx_, signers, ascii);
  if (err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgEncryptResult(), GpgSignResult(), GFBuffer()});
    return err;
  }

  GpgData data_in(in_buffer);
  GpgData data_out;

  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();
  err = CheckGpgError(gpgme_op_encrypt_sign(
      ctx, recipients.data(), GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out));

  data_object->Swap({
      GpgEncryptResult(gpgme_op_encrypt_result(ctx)),
      GpgSignResult(gpgme_op_sign_result(ctx)),
      data_out.Read2GFBuffer(),
  });
  return err;
}

void GpgBasicOperator::EncryptSign(const GpgAbstractKeyPtrList& keys,
                                   const GpgAbstractKeyPtrList& signers,
                                   const GFBuffer& in_buffer, bool ascii,
                                   const GpgOperationCallback& cb) {
  RunGpgOperaAsync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) -> GpgError {
        return EncryptSignImpl(ctx_, keys, signers, in_buffer, ascii,
                               data_object);
      },
      cb, "gpgme_op_encrypt_sign", "2.2.0");
}

auto GpgBasicOperator::EncryptSignSync(const GpgAbstractKeyPtrList& keys,
                                       const GpgAbstractKeyPtrList& signers,
                                       const GFBuffer& in_buffer, bool ascii)
    -> std::tuple<GpgError, DataObjectPtr> {
  return RunGpgOperaSync(
      GetChannel(),
      [=](const DataObjectPtr& data_object) -> GpgError {
        return EncryptSignImpl(ctx_, keys, signers, in_buffer, ascii,
                               data_object);
      },
      "gpgme_op_encrypt_sign", "2.2.0");
}

auto GpgBasicOperator::SetSigners(const GpgAbstractKeyPtrList& signers,
                                  bool ascii) -> GpgError {
  return SetSignersImpl(ctx_, signers, ascii);
}

auto GpgBasicOperator::GetSigners(bool ascii) -> KeyArgsList {
  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();

  auto count = gpgme_signers_count(ctx);
  auto signers = KeyArgsList{};
  for (auto i = 0U; i < count; i++) {
    auto key = GpgKey(gpgme_signers_enum(ctx, i));
    signers.push_back(GpgKey(key));
  }
  return signers;
}
}  // namespace GpgFrontend
