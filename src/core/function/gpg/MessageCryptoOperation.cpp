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

#include "core/model/GpgData.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

void SetSignersGnuPGImpl(OpenPGPContext& ctx_,
                         const GpgAbstractKeyPtrList& signers, bool ascii) {
  auto* ctx = ascii ? ctx_.DefaultContext() : ctx_.BinaryContext();

  gpgme_signers_clear(ctx);

  auto keys = ConvertKey2GpgKeyList(ctx_.GetChannel(), signers);
  for (const auto& key : keys) {
    LOG_D() << "signer's key fpr: " << key->Fingerprint();
    if (key->IsHasSignCap()) {
      auto error = gpgme_signers_add(ctx, static_cast<gpgme_key_t>(*key));
      CheckGpgError(error);
    }
  }

  auto count = gpgme_signers_count(ctx_.DefaultContext());
  if (static_cast<unsigned int>(signers.size()) != count) {
    FLOG_D("not all signers added");
  }
}

auto EncryptGnuPGImpl(OpenPGPContext& ctx_, const GpgAbstractKeyPtrList& keys,
                      const GFBuffer& in_buffer, bool ascii,
                      const DataObjectPtr& data_object) -> GpgError {
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

auto EncryptSymmetricGnuPGImpl(OpenPGPContext& ctx, const GFBuffer& in_buffer,
                               bool ascii, const DataObjectPtr& data_object)
    -> GpgError {
  return EncryptGnuPGImpl(ctx, {}, in_buffer, ascii, data_object);
}

auto DecryptGnuPGImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
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

auto SignGnuPGImpl(OpenPGPContext& ctx_, const GpgAbstractKeyPtrList& signers,
                   const GFBuffer& in_buffer, GpgSignMode mode, bool ascii,
                   const DataObjectPtr& data_object) -> GpgError {
  if (signers.empty()) return GPG_ERR_CANCELED;

  GpgError err;

  // Set Singers of this opera
  SetSignersGnuPGImpl(ctx_, signers, ascii);

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

auto VerifyGnuPGImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
                     const GFBuffer& sig_buffer,
                     const DataObjectPtr& data_object) -> GpgError {
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

auto EncryptSignGnuPGImpl(OpenPGPContext& ctx_,
                          const GpgAbstractKeyPtrList& keys,
                          const GpgAbstractKeyPtrList& signers,
                          const GFBuffer& in_buffer, bool ascii,
                          const DataObjectPtr& data_object) -> GpgError {
  if (keys.empty() || signers.empty()) return GPG_ERR_CANCELED;

  GpgError err;
  auto recipients = Convert2RawGpgMEKeyList(ctx_.GetChannel(), keys);

  // Last entry data_in array has to be nullptr
  recipients.push_back(nullptr);

  SetSignersGnuPGImpl(ctx_, signers, ascii);

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

auto DecryptVerifyGnuPGImpl(OpenPGPContext& ctx_, const GFBuffer& in_buffer,
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

auto GetSignersGnuPGImpl(OpenPGPContext& ctx_, bool ascii) -> KeyArgsList {
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
