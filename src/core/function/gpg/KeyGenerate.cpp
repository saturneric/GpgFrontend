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

#include "KeyGenerate.h"

#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/model/DataObject.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {
auto GenerateKeyGnuPGImpl(GpgContext& ctx,
                          const QSharedPointer<KeyGenerateInfo>& params,
                          const DataObjectPtr& data_object) -> GpgError {
  if (params == nullptr || params->GetAlgo() == KeyGenerateInfo::kNoneAlgo ||
      params->IsSubKey()) {
    return GPG_ERR_CANCELED;
  }

  const auto userid = params->GetUserid();
  const auto algo = params->GetAlgo().Id();

  unsigned long expires =
      QDateTime::currentDateTime().secsTo(params->GetExpireTime());

  GpgError err;
  unsigned int flags = 0;

  if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
  if (params->IsAllowEncr()) flags |= GPGME_CREATE_ENCR;
  if (params->IsAllowSign()) flags |= GPGME_CREATE_SIGN;
  if (params->IsAllowAuth()) flags |= GPGME_CREATE_AUTH;
  if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
  if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

  LOG_D() << "key generation args: " << userid << algo << expires << flags;
  LOG_D() << "key generation flags" << params->IsAllowEncr()
          << params->IsAllowSign() << params->IsAllowAuth()
          << !params->IsSubKey();

  err = gpgme_op_createkey(ctx.DefaultContext(), userid.toUtf8(), algo.toUtf8(),
                           0, expires, nullptr, flags);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    data_object->Swap(
        {GpgGenerateKeyResult{gpgme_op_genkey_result(ctx.DefaultContext())}});
  } else {
    data_object->Swap({GpgGenerateKeyResult{}});
  }

  return CheckGpgError(err);
}

auto GenerateSubKeyGnuPGImpl(GpgContext& ctx, const GpgKeyPtr& key,
                             const QSharedPointer<KeyGenerateInfo>& params,
                             const DataObjectPtr& data_object) -> GpgError {
  if (params == nullptr || params->GetAlgo() == KeyGenerateInfo::kNoneAlgo ||
      !params->IsSubKey()) {
    return GPG_ERR_CANCELED;
  }

  auto algo = params->GetAlgo().Id();
  LOG_D() << "primary subkey algo: " << algo
          << ", sub algo: " << params->SubAlgo().Id();

  if (params->SubAlgo().Id() != KeyGenerateInfo::kNoneAlgo.Id() &&
      !params->SubAlgo().Id().isEmpty()) {
    algo += "_" + params->SubAlgo().Id();
    LOG_D() << "hybrid subkey algo: " << algo;
  }

  unsigned long expires =
      QDateTime::currentDateTime().secsTo(params->GetExpireTime());
  unsigned int flags = 0;

  if (!params->IsSubKey()) flags |= GPGME_CREATE_CERT;
  if (params->IsAllowEncr()) flags |= GPGME_CREATE_ENCR;
  if (params->IsAllowSign()) flags |= GPGME_CREATE_SIGN;
  if (params->IsAllowAuth()) flags |= GPGME_CREATE_AUTH;
  if (params->IsNonExpired()) flags |= GPGME_CREATE_NOEXPIRE;
  if (params->IsNoPassPhrase()) flags |= GPGME_CREATE_NOPASSWD;

  LOG_D() << "subkey generation args: " << key->ID() << algo << expires
          << flags;

  auto err = gpgme_op_createsubkey(ctx.DefaultContext(),
                                   static_cast<gpgme_key_t>(*key),
                                   algo.toLatin1(), 0, expires, flags);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgGenerateKeyResult{}});
    return err;
  }

  data_object->Swap(
      {GpgGenerateKeyResult{gpgme_op_genkey_result(ctx.DefaultContext())}});
  return CheckGpgError(err);
}

auto GenerateKeyWithSubkeyGnuPGImpl(
    GpgContext& ctx, const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params,
    const DataObjectPtr& data_object) -> GpgError {
  auto& key_getter = GpgKeyRepository::GetInstance(ctx.GetChannel());

  auto err = GenerateKeyGnuPGImpl(ctx, p_params, data_object);

  if (err != GPG_ERR_NO_ERROR) return err;
  if (!data_object->Check<GpgGenerateKeyResult>()) return GPG_ERR_CANCELED;

  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  auto key = key_getter.GetKeyPtr(result.GetFingerprint());
  if (key == nullptr) {
    LOG_W() << "cannot get key which has been generated, fpr: "
            << result.GetFingerprint();
    return GPG_ERR_CANCELED;
  }

  if (s_params == nullptr) return err;

  data_object->Swap({});
  err = GenerateSubKeyGnuPGImpl(ctx, key, s_params, data_object);
  auto s_result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);

  data_object->Swap({result, s_result});
  return err;
}

}  // namespace GpgFrontend