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

#include <QSet>

#include "private/GFSDKGpgInternal.h"

// std::memset
#include <cstring>

// std::any_cast
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <any>

#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/function/result_analyse/GpgDecryptResultAnalyse.h"
#include "core/function/result_analyse/GpgEncryptResultAnalyse.h"
#include "core/function/result_analyse/GpgSignResultAnalyse.h"
#include "core/function/result_analyse/GpgVerifyResultAnalyse.h"
#include "core/model/DataObject.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgUID.h"
#include "core/model/GpgVerifyResult.h"
#include "core/typedef/GpgTypedef.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/RustUtils.h"
#include "ui/UIModuleManager.h"
#include "ui/function/ImportKey.h"
#include "ui/function/InfoBoardCardConverter.h"

//
#include "GFHostImpl.h"
#include "private/GFHostContext.h"
#include "private/GFSDKPrivate.h"

namespace {

// Convert an analysed result into the Info Board card JSON modules forward to
// the UI, reusing the same converter the native (non-module) operations use so
// emailed results render identically.
void EmitResultCards(const GpgFrontend::GpgOpResultInfo& info,
                     const char** cards) {
  if (cards == nullptr) return;
  const auto card_list = GpgFrontend::UI::convert_op_info_to_cards(info);
  *cards = GFStrDup(
      QString::fromUtf8(GpgFrontend::UI::encode_info_board_cards(card_list)));
}

}  // namespace

namespace gf_host {

auto GFGpgPublicKey(int channel, const char* key_id, int ascii) -> GFBufferRef {
  auto key = GpgFrontend::GpgKeyRepository::GetInstance(channel).GetKeyPtr(
      GFStrView(key_id));
  if (key == nullptr) return nullptr;

  auto [err, buffer] =
      GpgFrontend::KeyImportExportOperation::GetInstance(channel).ExportKey(
          key, false, ascii != 0, true);

  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) return nullptr;

  // Octets, not text: a binary export is not UTF-8, and a round trip through
  // QString would replace every invalid sequence.
  return GFBufferNewFromBytes(buffer.Data(), buffer.Size());
}

auto GFGpgKeyPrimaryUidParts(int channel, const char* key_id, char** name,
                             char** email, char** comment) -> int {
  auto key = GpgFrontend::GpgKeyRepository::GetInstance(channel).GetKey(
      GFStrView(key_id));

  if (!key.IsGood()) return -1;

  auto uids = key.UIDs();
  if (uids.empty()) return -1;
  const auto& primary_uid = uids.front();

  // Three independent out-parameters rather than one struct whose three
  // char* members the caller had to free one by one. A caller that wants
  // only the address asks for only the address.
  if (name != nullptr) *name = GFStrDup(primary_uid.GetName());
  if (email != nullptr) *email = GFStrDup(primary_uid.GetEmail());
  if (comment != nullptr) *comment = GFStrDup(primary_uid.GetComment());
  return 0;
}

auto GFGpgImportKeys(int channel, const char* data, int size) -> int {
  auto in_buffer = GpgFrontend::GFBuffer(QByteArray::fromRawData(data, size));
  // The Host parents its own import dialog; nothing from a module is ever
  // taken for a widget.
  GpgFrontend::UI::ImportKeys(nullptr, channel, in_buffer);
  return 0;
}

auto GFGpgCurrentGpgContextChannel() -> int {
  // Read on the GUI thread, where the main window lives.
  return GpgFrontend::UI::CurrentGpgContextChannel();
}

auto GFGpgExportKey(int channel, const char* key_id, int ascii,
                    GFBufferRef* out) -> int {
  if (out == nullptr) return -1;
  *out = nullptr;

  auto key = GpgFrontend::GpgKeyRepository::GetInstance(channel).GetKeyPtr(
      GFStrView(key_id));
  if (key == nullptr) return -1;

  auto [err, buffer] =
      GpgFrontend::KeyImportExportOperation::GetInstance(channel).ExportKey(
          key, false, ascii != 0, false);
  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) return -1;

  // Octets, not text; see GFGpgPublicKey.
  *out = GFBufferNewFromBytes(buffer.Data(), buffer.Size());
  return *out == nullptr ? -1 : 0;
}

namespace {

// Serialize the structured result so a module can reason about individual
// signatures and recipients. Cards and the report are for display; this is
// for decisions, and re-parsing prose to recover a fingerprint is not a
// decision path worth having.
auto SignerToJson(const GpgFrontend::GpgSignerInfo& signer) -> QJsonObject {
  return QJsonObject{
      {"uid", signer.uid},
      {"fingerprint", signer.fingerprint},
      {"keyId", signer.keyId},
      {"pubkeyAlgo", signer.pubkeyAlgo},
      {"hashAlgo", signer.hashAlgo},
      {"signTime", signer.signTime.isValid()
                       ? signer.signTime.toUTC().toString(Qt::ISODate)
                       : QString()}};
}

auto OpInfoToJson(const GpgFrontend::GpgOpResultInfo& info) -> QByteArray {
  QJsonObject root{
      {"status", info.status},
      {"operation", info.operation},
      {"engine", info.engine},
      {"description", info.description},
      {"inputHash", info.inputHash},
      {"details", QJsonArray::fromStringList(info.details)},
  };

  if (!info.signatures.isEmpty()) {
    QJsonArray sigs;
    for (const auto& sig : info.signatures) {
      auto obj = SignerToJson(sig.signer);
      obj.insert("validity", static_cast<int>(sig.validity));
      obj.insert("warnings", QJsonArray::fromStringList(sig.warnings));
      sigs.append(obj);
    }
    root.insert("signatures", sigs);
  }

  if (!info.newSignatures.isEmpty()) {
    QJsonArray sigs;
    for (const auto& sig : info.newSignatures) {
      auto obj = SignerToJson(sig.signer);
      obj.insert("sigMode", sig.sigMode);
      sigs.append(obj);
    }
    root.insert("newSignatures", sigs);
  }

  if (!info.invalidSigners.isEmpty()) {
    QJsonArray invalid;
    for (const auto& pair : info.invalidSigners) {
      invalid.append(
          QJsonObject{{"fingerprint", pair.first}, {"error", pair.second}});
    }
    root.insert("invalidSigners", invalid);
  }

  if (!info.recipients.isEmpty()) {
    QJsonArray recipients;
    for (const auto& reci : info.recipients) {
      recipients.append(
          QJsonObject{{"uid", reci.uid},
                      {"fingerprint", reci.fingerprint},
                      {"keyId", reci.keyId},
                      {"pubkeyAlgo", reci.pubkeyAlgo},
                      {"keyFound", reci.keyFound},
                      {"algoIsPrimaryKey", reci.algoIsPrimaryKey}});
    }
    root.insert("recipients", recipients);
  }

  if (!info.filename.isEmpty()) root.insert("filename", info.filename);
  if (!info.symmetricAlgo.isEmpty()) {
    root.insert("symmetricAlgo", info.symmetricAlgo);
  }
  root.insert("mimeEncoded", info.mimeEncoded);
  root.insert("messageIntegrityProtected", info.messageIntegrityProtected);

  return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void EmitResultInfo(const GpgFrontend::GpgOpResultInfo& info,
                    const char** info_json) {
  if (info_json == nullptr) return;
  *info_json = GFStrDup(QString::fromUtf8(OpInfoToJson(info)));
}

// Engine-neutral analysis. The raw gpgme_*_result handles only exist for the
// native (GnuPG) engine; the rPGP engine stores its result in the Gpg*Result
// model object instead, which the SDK keeps in the result handle. Recover that
// model and run the same analyser used by the native path, so
// both engines produce identical reports and status codes. The report, the
// cards and the structured form come from one call because the capsule is
// consumed on first use. Returns -1 when the capsule is missing or holds an
// unexpected type.
template <typename ResultT, typename AnalyseT>
auto AnalyseResultInfoByCapsule(int channel, gpgme_error_t err,
                                const char* capsule_id, const char** analyse,
                                const char** cards, const char** info_json)
    -> int {
  if (analyse == nullptr) return -1;

  auto capsule = TakeResultModel(capsule_id);

  auto* result = std::any_cast<ResultT>(&capsule);
  if (result == nullptr) return -1;

  AnalyseT ra(channel, err, *result);
  ra.Analyse();
  *analyse = GFStrDup(ra.GetResultReport());
  EmitResultCards(ra.GetOpInfo(), cards);
  EmitResultInfo(ra.GetOpInfo(), info_json);
  return ra.GetStatus();
}

}  // namespace

auto GFAnalyseVerifyResultInfoByCapsule(int channel, gpgme_error_t err,
                                        const char* capsule_id,
                                        const char** analyse,
                                        const char** cards,
                                        const char** info_json) -> int {
  return AnalyseResultInfoByCapsule<GpgFrontend::GpgVerifyResult,
                                    GpgFrontend::GpgVerifyResultAnalyse>(
      channel, err, capsule_id, analyse, cards, info_json);
}

auto GFAnalyseSignResultInfoByCapsule(int channel, gpgme_error_t err,
                                      const char* capsule_id,
                                      const char** analyse, const char** cards,
                                      const char** info_json) -> int {
  return AnalyseResultInfoByCapsule<GpgFrontend::GpgSignResult,
                                    GpgFrontend::GpgSignResultAnalyse>(
      channel, err, capsule_id, analyse, cards, info_json);
}

auto GFAnalyseEncryptResultInfoByCapsule(int channel, gpgme_error_t err,
                                         const char* capsule_id,
                                         const char** analyse,
                                         const char** cards,
                                         const char** info_json) -> int {
  return AnalyseResultInfoByCapsule<GpgFrontend::GpgEncryptResult,
                                    GpgFrontend::GpgEncryptResultAnalyse>(
      channel, err, capsule_id, analyse, cards, info_json);
}

auto GFAnalyseDecryptResultInfoByCapsule(int channel, gpgme_error_t err,
                                         const char* capsule_id,
                                         const char** analyse,
                                         const char** cards,
                                         const char** info_json) -> int {
  return AnalyseResultInfoByCapsule<GpgFrontend::GpgDecryptResult,
                                    GpgFrontend::GpgDecryptResultAnalyse>(
      channel, err, capsule_id, analyse, cards, info_json);
}

auto FindKeyBriefRows(int channel, const QString& email)
    -> std::optional<QList<KeyBriefRow>> {
  const auto wanted = email.trimmed();
  if (wanted.isEmpty()) return std::nullopt;

  QList<KeyBriefRow> rows;
  auto all = GpgFrontend::AbstractKeyRepository::GetInstance(channel).Fetch();
  for (const auto& abstract_key : all) {
    if (abstract_key == nullptr) continue;
    if (abstract_key->KeyType() != GpgFrontend::GpgAbstractKeyType::kGPG_KEY) {
      continue;
    }

    auto key = qSharedPointerDynamicCast<GpgFrontend::GpgKey>(abstract_key);
    if (key == nullptr) continue;

    // Every UID, not just the primary one: one key legitimately carries
    // several addresses, and matching only the primary reports "no key" for a
    // key that is sitting right there.
    const auto uids = key->UIDs();
    for (qsizetype i = 0; i < uids.size(); ++i) {
      const auto& uid = uids[i];
      if (uid.GetEmail().compare(wanted, Qt::CaseInsensitive) != 0) continue;

      KeyBriefRow row;
      row.fingerprint = key->Fingerprint().toUtf8();
      row.key_id = key->ID().toUtf8();
      row.uid = key->UID().toUtf8();
      row.matched_email = uid.GetEmail().toUtf8();
      const auto expires = key->ExpirationTime();
      row.expires_at = expires.isValid()
                           ? static_cast<int64_t>(expires.toSecsSinceEpoch())
                           : 0;
      // Usability only. Whether this key belongs to whoever claimed the
      // address is a separate question, answered by the matched_uid_* fields
      // and by the caller -- never folded into this number.
      row.usability = static_cast<int>(GpgFrontend::GetKeyStatus(key.get()));
      row.can_encrypt = key->IsHasActualEncrCap() ? 1 : 0;
      row.can_sign = key->IsHasActualSignCap() ? 1 : 0;
      row.matched_uid_is_primary = i == 0 ? 1 : 0;
      row.matched_uid_revoked = uid.GetRevoked() ? 1 : 0;
      rows.append(row);
      break;
    }
  }
  return rows;
}

auto ListKeyAddressRows(int channel, bool secret_only) -> QList<QByteArray> {
  QStringList entries;
  QSet<QString> seen;

  auto all = GpgFrontend::AbstractKeyRepository::GetInstance(channel).Fetch();
  for (const auto& abstract_key : all) {
    if (abstract_key == nullptr) continue;
    if (abstract_key->KeyType() != GpgFrontend::GpgAbstractKeyType::kGPG_KEY) {
      continue;
    }

    auto key = qSharedPointerDynamicCast<GpgFrontend::GpgKey>(abstract_key);
    if (key == nullptr) continue;

    // The identities this user can send AS are exactly the ones they hold a
    // secret half for. A public key in the keyring is someone else's address.
    if (secret_only && !key->IsPrivateKey()) continue;

    // Every UID, not only the primary: one key legitimately carries several
    // addresses, and offering only the first hides the rest of them.
    for (const auto& uid : key->UIDs()) {
      // A revoked UID is an address its owner has withdrawn. Offering it as a
      // suggestion would invite the user to send to an identity that has been
      // retired, which is worse than not suggesting anything.
      if (uid.GetRevoked()) continue;

      const auto email = uid.GetEmail().trimmed();
      if (email.isEmpty()) continue;

      // Deduplicated on the address alone: a correspondent who appears on
      // three keys is one entry, and the name attached to the first is as good
      // as any. This list is a hint, not an identity claim.
      const auto seen_key = email.toLower();
      if (seen.contains(seen_key)) continue;
      seen.insert(seen_key);

      const auto name = uid.GetName().trimmed();
      entries.append(name.isEmpty() ? email
                                    : QString("%1 <%2>").arg(name, email));
    }
  }

  entries.sort(Qt::CaseInsensitive);
  QList<QByteArray> rows;
  rows.reserve(entries.size());
  for (const auto& e : entries) rows.append(e.toUtf8());
  return rows;
}

auto SniffRecipientRows(int channel, const GpgFrontend::GFBuffer& data)
    -> QList<RecipientRow> {
  QList<RecipientRow> rows;
  const auto key_ids = GpgFrontend::SniffRecipientKeyIds(data);
  // Before the repository is touched: bytes that name no recipient need no
  // key lookup, and reaching for a channel's repository creates its context.
  if (key_ids.isEmpty()) return rows;
  auto& repository = GpgFrontend::AbstractKeyRepository::GetInstance(channel);

  for (const auto& key_id : key_ids) {
    RecipientRow row;
    row.key_id = key_id.toUtf8();

    // An all-zero identifier is the wildcard key id: the sender asked for the
    // recipient to be withheld. Looking it up would report "no such key" for
    // what is really "no answer given", and the user may well be that hidden
    // recipient themselves.
    if (key_id.count('0') == key_id.size()) {
      row.hidden = 1;
      rows.append(row);
      continue;
    }

    // Resolves a key id OR a fingerprint, and a subkey as readily as a
    // primary -- which is what this needs, since a PKESK names the encryption
    // subkey. Engine-neutral, so it is equally right on a GnuPG channel and
    // an rPGP one.
    auto key = repository.GetKey(key_id);
    if (key != nullptr) {
      row.key_found = 1;
      // The secret half, and nothing else, is what decides decryptability.
      row.has_secret = key->IsPrivateKey() ? 1 : 0;
      row.pub_algo = key->PublicKeyAlgo().toUtf8();
      row.fingerprint = key->Fingerprint().toUtf8();
      row.uid = key->UID().toUtf8();
    }
    rows.append(row);
  }
  return rows;
}

}  // namespace gf_host