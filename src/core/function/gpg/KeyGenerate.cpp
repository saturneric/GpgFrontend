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

#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/model/DataObject.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {
auto GenerateKeyGnuPGImpl(OpenPGPContext& ctx,
                          const QSharedPointer<KeyGenerateInfo>& params,
                          const DataObjectPtr& data_object) -> GpgError {
  if (params == nullptr || params->GetAlgo() == KeyGenerateInfo::kNoneAlgo ||
      params->IsSubKey()) {
    return GPG_ERR_CANCELED;
  }

  auto& g_ctx = GpgCtx(ctx);
  const auto userid = params->GetUserid();
  auto algo = params->GetAlgo().Id();

  if (params->SubAlgo().Id() != KeyGenerateInfo::kNoneAlgo.Id() &&
      !params->SubAlgo().Id().isEmpty()) {
    algo += "_" + params->SubAlgo().Id();
    LOG_D() << "hybrid key algo: " << algo;
  }

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

  err = gpgme_op_createkey(g_ctx.DefaultContext(), userid.toUtf8(),
                           algo.toUtf8(), 0, expires, nullptr, flags);

  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    data_object->Swap(
        {GpgGenerateKeyResult{gpgme_op_genkey_result(g_ctx.DefaultContext())}});
  } else {
    data_object->Swap({GpgGenerateKeyResult{}});
  }

  return CheckGpgError(err);
}

auto GenerateSubKeyGnuPGImpl(OpenPGPContext& ctx, const GpgKeyPtr& key,
                             const QSharedPointer<KeyGenerateInfo>& params,
                             const DataObjectPtr& data_object) -> GpgError {
  if (params == nullptr || params->GetAlgo() == KeyGenerateInfo::kNoneAlgo ||
      !params->IsSubKey()) {
    return GPG_ERR_CANCELED;
  }

  auto& g_ctx = GpgCtx(ctx);
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

  auto err = gpgme_op_createsubkey(g_ctx.DefaultContext(),
                                   static_cast<gpgme_key_t>(*key),
                                   algo.toLatin1(), 0, expires, flags);
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
    data_object->Swap({GpgGenerateKeyResult{}});
    return err;
  }

  data_object->Swap(
      {GpgGenerateKeyResult{gpgme_op_genkey_result(g_ctx.DefaultContext())}});
  return CheckGpgError(err);
}

namespace {

/// The "Key-Type" (or "Subkey-Type") lines of a batch parameter block for one
/// algorithm, prefixed with `prefix`. Empty when the batch format cannot name
/// it, in which case the caller keeps the two-call path.
auto BatchAlgoLines(const KeyAlgo& algo, const QString& prefix) -> QString {
  if (algo.IsPostQuantum() || algo.Type().isEmpty()) return {};

  // The algorithm's type is already a name the batch format knows.
  if (!algo.IsEcc()) {
    if (algo.KeyLength() <= 0) return {};
    return u"%1-Type: %2\n%1-Length: %3\n"_s.arg(
        prefix, algo.Type(), QString::number(algo.KeyLength()));
  }

  // The curve is the algorithm id, except where the agent's curve lookup is
  // case-sensitive about a spelling the id does not carry.
  auto curve = algo.Id();
  if (curve == u"x448"_s) curve = u"X448"_s;
  if (curve.startsWith(u"brainpoolp"_s)) curve[9] = u'P';
  return u"%1-Type: %2\n%1-Curve: %3\n"_s.arg(prefix, algo.Type(), curve);
}

auto BatchUsage(const KeyGenerateInfo& params) -> QString {
  QStringList usage;
  if (!params.IsSubKey()) usage << u"cert"_s;
  if (params.IsAllowSign()) usage << u"sign"_s;
  if (params.IsAllowEncr()) usage << u"encrypt"_s;
  if (params.IsAllowAuth()) usage << u"auth"_s;
  return usage.join(u',');
}

auto BatchExpire(const KeyGenerateInfo& params) -> QString {
  if (params.IsNonExpired()) return u"0"_s;
  return u"seconds=%1"_s.arg(
      QDateTime::currentDateTime().secsTo(params.GetExpireTime()));
}

/**
 * @brief The parameter block that makes the primary key and its subkey in a
 * single gpg run.
 *
 * Two runs (createkey, then createsubkey) cost the user a third prompt: the
 * agent does not keep the passphrase it was given for the new key, so adding
 * the subkey has to unlock that key again. One run asks once, and commits both
 * keys or neither.
 *
 * @return std::nullopt when the block cannot describe this pair
 */
auto BuildGenKeyParams(const KeyGenerateInfo& p_params,
                       const KeyGenerateInfo& s_params)
    -> std::optional<QString> {
  // A hybrid key is named by two algorithms; the batch format has no way to.
  for (const auto* params : {&p_params, &s_params}) {
    if (!params->SubAlgo().Id().isEmpty() &&
        params->SubAlgo().Id() != KeyGenerateInfo::kNoneAlgo.Id()) {
      return std::nullopt;
    }
  }

  // One block protects both keys the same way.
  if (p_params.IsNoPassPhrase() != s_params.IsNoPassPhrase()) {
    return std::nullopt;
  }

  const auto key_type = BatchAlgoLines(p_params.GetAlgo(), u"Key"_s);
  const auto subkey_type = BatchAlgoLines(s_params.GetAlgo(), u"Subkey"_s);
  if (key_type.isEmpty() || subkey_type.isEmpty()) return std::nullopt;

  // The block is line-based: a line break in a value would start a new line.
  const auto name = p_params.GetName();
  const auto email = p_params.GetEmail();
  const auto comment = p_params.GetComment();
  for (const auto& value : {name, email, comment}) {
    if (value.contains(u'\n') || value.contains(u'\r')) return std::nullopt;
  }

  auto params = u"<GnupgKeyParms format=\"internal\">\n"_s;
  params += key_type;
  params += u"Key-Usage: %1\n"_s.arg(BatchUsage(p_params));
  params += subkey_type;
  params += u"Subkey-Usage: %1\n"_s.arg(BatchUsage(s_params));
  params += u"Name-Real: %1\n"_s.arg(name);
  if (!email.isEmpty()) params += u"Name-Email: %1\n"_s.arg(email);
  if (!comment.isEmpty()) params += u"Name-Comment: %1\n"_s.arg(comment);
  params += u"Expire-Date: %1\n"_s.arg(BatchExpire(p_params));
  params += u"Subkey-Expire-Date: %1\n"_s.arg(BatchExpire(s_params));
  if (p_params.IsNoPassPhrase()) params += u"%no-protection\n"_s;
  params += u"%commit\n</GnupgKeyParms>\n"_s;
  return params;
}

}  // namespace

auto GenerateKeyWithSubkeyGnuPGImpl(
    OpenPGPContext& ctx, const QSharedPointer<KeyGenerateInfo>& p_params,
    const QSharedPointer<KeyGenerateInfo>& s_params,
    const DataObjectPtr& data_object) -> GpgError {
  auto& key_getter = GpgKeyRepository::GetInstance(ctx.GetChannel());

  const auto batch_params =
      p_params != nullptr && s_params != nullptr && !p_params->IsSubKey() &&
              s_params->IsSubKey() &&
              p_params->GetAlgo() != KeyGenerateInfo::kNoneAlgo &&
              s_params->GetAlgo() != KeyGenerateInfo::kNoneAlgo
          ? BuildGenKeyParams(*p_params, *s_params)
          : std::nullopt;

  if (batch_params) {
    auto& g_ctx = GpgCtx(ctx);
    LOG_D() << "generating key with subkey in one run, algos:"
            << p_params->GetAlgo().Id() << s_params->GetAlgo().Id();

    auto err = CheckGpgError(gpgme_op_genkey(
        g_ctx.DefaultContext(), batch_params->toUtf8(), nullptr, nullptr));
    if (err != GPG_ERR_NO_ERROR) {
      data_object->Swap({GpgGenerateKeyResult{}});
      return err;
    }

    auto result =
        GpgGenerateKeyResult{gpgme_op_genkey_result(g_ctx.DefaultContext())};
    auto key = key_getter.GetKeyPtr(result.GetFingerprint());
    if (key == nullptr) {
      LOG_W() << "cannot get key which has been generated, fpr: "
              << result.GetFingerprint();
      return GPG_ERR_CANCELED;
    }

    // SubKeys() lists the primary first; the one subkey made here is last.
    const auto s_keys = key->SubKeys();
    auto s_result = s_keys.size() > 1
                        ? GpgGenerateKeyResult{s_keys.back().Fingerprint()}
                        : GpgGenerateKeyResult{};

    data_object->Swap({result, s_result});
    return err;
  }

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

auto FilterKeyAlgoByKeyGnuPGImpl(OpenPGPContext&, const GpgKey&,
                                 const QList<KeyAlgo>& algos)
    -> QList<KeyAlgo> {
  // GnuPG does not have the limitation on subkey algorithms based on primary
  // key version, return all algos directly

  return algos;
}

}  // namespace GpgFrontend