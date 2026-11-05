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

#include "MessageCryptoOperation.h"

#include <gpg-error.h>

#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgData.h"
#include "core/function/gpg/GpgErrorMap.h"
#include "core/function/gpg/GpgModelConvert.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

auto SetSignersGnuPGImpl(OpenPGPContext& ctx_,
                         const GpgAbstractKeyPtrList& signers, bool ascii)
    -> GpgError {
  auto& g_ctx = GpgCtx(ctx_);
  auto* ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();

  gpgme_signers_clear(ctx);

  // gpgme signs with gpg's default key when the signer list is empty, so a
  // signer quietly left out here becomes a signature by someone else that is
  // still reported as a success. Every requested key has to make it in.
  auto keys = ConvertKey2GpgKeyList(ctx_.GetChannel(), signers);

  QSet<QString> converted;
  for (const auto& key : keys) converted.insert(key->ID());
  for (const auto& signer : signers) {
    if (signer == nullptr) return GPG_ERR_NO_SECKEY;
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
    auto error = CheckGpgError(gpgme_signers_add(ctx, GpgmeKeyOf(*key)));
    if (error != GPG_ERR_NO_ERROR) {
      gpgme_signers_clear(ctx);
      return error;
    }
  }

  if (gpgme_signers_count(ctx) == 0) return GPG_ERR_NO_SECKEY;
  return GPG_ERR_NO_ERROR;
}

auto EncryptGnuPGImpl(OpenPGPContext& ctx_, const GpgAbstractKeyPtrList& keys,
                      const GFBuffer& in_buffer, bool ascii,
                      const DataObjectPtr& data_object) -> GpgError {
  auto& g_ctx = GpgCtx(ctx_);

  auto recipients = Convert2GpgKeyList(ctx_.GetChannel(), keys);

  QList<gpgme_key_t> gpg_recipients;
  for (const auto& key : recipients) {
    LOG_D() << "recipient key fpr: " << key.Fingerprint();
    gpg_recipients.push_back(GpgmeKeyOf(key));
  }
  gpg_recipients.push_back(nullptr);

  GpgData data_in(in_buffer);
  GpgData data_out;

  auto* ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();
  auto err = CheckGpgError(
      gpgme_op_encrypt(ctx, keys.isEmpty() ? nullptr : gpg_recipients.data(),
                       GPGME_ENCRYPT_ALWAYS_TRUST, data_in, data_out));
  data_object->Swap({
      EncryptResultOf(gpgme_op_encrypt_result(ctx)),
      data_out.Read2GFBuffer(),
  });

  return err;
}

auto EncryptSymmetricGnuPGImpl(OpenPGPContext& ctx, const GFBuffer& in_buffer,
                               bool ascii, const DataObjectPtr& data_object)
    -> GpgError {
  return EncryptGnuPGImpl(ctx, {}, in_buffer, ascii, data_object);
}

auto DecryptGnuPGImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
                      const DataObjectPtr& data_object) -> GpgError {
  auto& g_ctx = GpgCtx(ctx_);
  GpgData data_in(in_buffer);
  GpgData data_out;

  auto err = CheckGpgError(
      gpgme_op_decrypt(g_ctx.DefaultContext(), data_in, data_out));
  data_object->Swap({
      DecryptResultOf(gpgme_op_decrypt_result(g_ctx.DefaultContext())),
      data_out.Read2GFBuffer(),
  });

  return err;
}

namespace {
auto ToGpgmeSigMode(GpgSignMode mode) -> gpgme_sig_mode_t {
  switch (mode) {
    case GpgSignMode::kDETACH:
      return GPGME_SIG_MODE_DETACH;
    case GpgSignMode::kCLEAR:
      return GPGME_SIG_MODE_CLEAR;
    case GpgSignMode::kNORMAL:
    default:
      return GPGME_SIG_MODE_NORMAL;
  }
}
}  // namespace

auto SignGnuPGImpl(OpenPGPContext& ctx_, const GpgAbstractKeyPtrList& signers,
                   const GFBuffer& in_buffer, GpgSignMode mode, bool ascii,
                   const DataObjectPtr& data_object) -> GpgError {
  if (signers.empty()) return GPG_ERR_CANCELED;

  GpgError err;
  auto& g_ctx = GpgCtx(ctx_);

  // Set Singers of this opera
  err = SetSignersGnuPGImpl(ctx_, signers, ascii);
  if (err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgSignResult(GFSignResult{}), GFBuffer()});
    return err;
  }

  GpgData data_in(in_buffer);
  GpgData data_out;

  auto* ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();
  err = CheckGpgError(
      gpgme_op_sign(ctx, data_in, data_out, ToGpgmeSigMode(mode)));

  data_object->Swap({
      SignResultOf(gpgme_op_sign_result(ctx)),
      data_out.Read2GFBuffer(),
  });
  return err;
}

auto VerifyGnuPGImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
                     const GFBuffer& sig_buffer,
                     const DataObjectPtr& data_object) -> GpgError {
  GpgError err;

  auto& g_ctx = GpgCtx(ctx_);
  GpgData data_in(in_buffer);
  GpgData data_out;

  // gpg verifies the armored block and skips anything around it, and the
  // verdict would then stand next to text nobody signed.
  if (sig_buffer.Empty() &&
      HasTextOutsideSignedArmor(in_buffer.ConvertToQByteArray())) {
    LOG_W() << "refusing to verify: the input has text outside its signature";
    data_object->Swap({GpgVerifyResult(GFVerifyResult{}), GFBuffer()});
    return GPG_ERR_BAD_DATA;
  }

  if (!sig_buffer.Empty()) {
    GpgData sig_data(sig_buffer);
    err = CheckGpgError(
        gpgme_op_verify(g_ctx.DefaultContext(), sig_data, data_in, nullptr));
  } else {
    err = CheckGpgError(
        gpgme_op_verify(g_ctx.DefaultContext(), data_in, nullptr, data_out));
  }

  data_object->Swap({
      VerifyResultOf(gpgme_op_verify_result(g_ctx.DefaultContext())),
      GFBuffer(),
  });

  return err;
}

auto EncryptSignGnuPGImpl(OpenPGPContext& ctx_,
                          const GpgAbstractKeyPtrList& keys,
                          const GpgAbstractKeyPtrList& signers,
                          const GFBuffer& in_buffer, bool ascii,
                          const DataObjectPtr& data_object) -> GpgError {
  if (keys.empty() || signers.empty()) return GPG_ERR_CANCELED;

  GpgError err;
  auto& g_ctx = GpgCtx(ctx_);

  auto recipients = Convert2GpgKeyList(ctx_.GetChannel(), keys);
  QList<gpgme_key_t> gpg_recipients;
  for (const auto& key : recipients) {
    LOG_D() << "recipient key fpr: " << key.Fingerprint();
    gpg_recipients.push_back(GpgmeKeyOf(key));
  }
  gpg_recipients.push_back(nullptr);

  err = SetSignersGnuPGImpl(ctx_, signers, ascii);
  if (err != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgEncryptResult(GFEncryptResult{}),
                       GpgSignResult(GFSignResult{}), GFBuffer()});
    return err;
  }

  GpgData data_in(in_buffer);
  GpgData data_out;

  auto* ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();
  err = CheckGpgError(gpgme_op_encrypt_sign(ctx, gpg_recipients.data(),
                                            GPGME_ENCRYPT_ALWAYS_TRUST, data_in,
                                            data_out));

  data_object->Swap({
      EncryptResultOf(gpgme_op_encrypt_result(ctx)),
      SignResultOf(gpgme_op_sign_result(ctx)),
      data_out.Read2GFBuffer(),
  });
  return err;
}

auto DecryptVerifyGnuPGImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
                            const DataObjectPtr& data_object) -> GpgError {
  GpgError err;

  GpgData data_in(in_buffer);
  GpgData data_out;

  auto& g_ctx = GpgCtx(ctx_);
  err = CheckGpgError(
      gpgme_op_decrypt_verify(g_ctx.DefaultContext(), data_in, data_out));

  data_object->Swap({
      DecryptResultOf(gpgme_op_decrypt_result(g_ctx.DefaultContext())),
      VerifyResultOf(gpgme_op_verify_result(g_ctx.DefaultContext())),
      data_out.Read2GFBuffer(),
  });

  return err;
}

auto GetSignersGnuPGImpl(OpenPGPContext& ctx_, bool ascii) -> KeyArgsList {
  auto& g_ctx = GpgCtx(ctx_);
  auto* ctx = ascii ? g_ctx.DefaultContext() : g_ctx.BinaryContext();

  auto count = gpgme_signers_count(ctx);
  auto signers = KeyArgsList{};
  for (auto i = 0U; i < count; i++) {
    auto key = GpgKey(AdoptGpgmeKey(gpgme_signers_enum(ctx, i)));
    signers.push_back(GpgKey(key));
  }
  return signers;
}

}  // namespace GpgFrontend
