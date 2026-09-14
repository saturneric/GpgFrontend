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

#include "GFSDKGpg.h"

#include "private/GFSDKGpgInternal.h"

#include <QSet>

// std::memset
#include <cstring>

// std::any_cast
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <any>

#include "GFSDKBasic.h"
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
#include "private/GFSDKPrivat.h"

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

auto GF_SDK_EXPORT GFGpgPublicKey(int channel, const char* key_id, int ascii)
    -> char* {
  auto key = GpgFrontend::GpgKeyRepository::GetInstance(channel).GetKeyPtr(
      GFStrView(key_id));
  if (key == nullptr) return nullptr;

  auto [err, buffer] =
      GpgFrontend::KeyImportExportOperation::GetInstance(channel).ExportKey(
          key, false, ascii != 0, true);

  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) return nullptr;

  return GFStrDup(buffer.ConvertToQByteArray());
}

auto GF_SDK_EXPORT GFGpgKeyPrimaryUID(int channel, const char* key_id,
                                      GFGpgKeyUID** ps) -> int {
  auto key = GpgFrontend::GpgKeyRepository::GetInstance(channel).GetKey(
      GFStrView(key_id));

  if (!key.IsGood()) return -1;

  auto uids = key.UIDs();
  auto& primary_uid = uids.front();

  *ps = static_cast<GFGpgKeyUID*>(GFAllocateMemory(sizeof(GFGpgKeyUID)));

  auto* s = *ps;
  s->name = GFStrDup(primary_uid.GetName());
  s->email = GFStrDup(primary_uid.GetEmail());
  s->comment = GFStrDup(primary_uid.GetComment());
  return 0;
}

auto GFGpgImportKeys(int channel, void* parent, const char* data, int size)
    -> int {
  auto in_buffer = GpgFrontend::GFBuffer(QByteArray::fromRawData(data, size));

  QObject* p = nullptr;
  if (parent != nullptr) {
    p = static_cast<QObject*>(parent);
  }

  GpgFrontend::UI::ImportKeys(qobject_cast<QWidget*>(p), channel, in_buffer);
  return 0;
}

auto GFGpgCurrentGpgContextChannel() -> int {
  auto* object =
      GpgFrontend::UI::UIModuleManager::GetInstance().GetQObject("main_window");

  auto* main_window = qobject_cast<GpgFrontend::UI::MainWindow*>(object);
  if (main_window != nullptr) {
    return main_window->GetCurrentGpgContextChannel();
  }

  return -1;
}

auto GFGpgExportKey(int channel, const char* key_id, int ascii, char** data,
                    int* size) -> int {
  auto key = GpgFrontend::GpgKeyRepository::GetInstance(channel).GetKeyPtr(
      GFStrView(key_id));
  if (key == nullptr) return -1;

  auto [err, buffer] =
      GpgFrontend::KeyImportExportOperation::GetInstance(channel).ExportKey(
          key, false, ascii != 0, false);
  if (GpgFrontend::CheckGpgError(err) != GPG_ERR_NO_ERROR) return -1;
  auto byte_array = buffer.ConvertToQByteArray();
  *data = GFStrDup(byte_array);
  *size = static_cast<int>(byte_array.size());
  return 0;
}

namespace {

// Engine-neutral analysis. The raw gpgme_*_result handles only exist for the
// native (GnuPG) engine; the rPGP engine stores its result in the Gpg*Result
// model object instead, which the SDK stashed in a UI capsule. Recover that
// model from the capsule and run the same analyser used by the native path, so
// both engines produce identical reports and status codes. Returns -1 when the
// capsule is missing or holds an unexpected type.
template <typename ResultT, typename AnalyseT>
auto AnalyseResultByCapsule(int channel, gpgme_error_t err, const char* capsule_id,
                            const char** analyse, const char** cards) -> int {
  if (analyse == nullptr) return -1;

  auto capsule = GpgFrontend::UI::UIModuleManager::GetInstance().GetCapsule(
      GFStrView(capsule_id));

  auto* result = std::any_cast<ResultT>(&capsule);
  if (result == nullptr) return -1;

  AnalyseT ra(channel, err, *result);
  ra.Analyse();
  *analyse = GFStrDup(ra.GetResultReport());
  EmitResultCards(ra.GetOpInfo(), cards);
  return ra.GetStatus();
}

}  // namespace

auto GF_SDK_EXPORT GFAnalyseEncryptResultByCapsule(int channel,
                                                   gpgme_error_t err,
                                                   const char* capsule_id,
                                                   const char** analyse,
                                                   const char** cards) -> int {
  return AnalyseResultByCapsule<GpgFrontend::GpgEncryptResult,
                                GpgFrontend::GpgEncryptResultAnalyse>(
      channel, err, capsule_id, analyse, cards);
}

auto GF_SDK_EXPORT GFAnalyseSignResultByCapsule(int channel, gpgme_error_t err,
                                                const char* capsule_id,
                                                const char** analyse,
                                                const char** cards) -> int {
  return AnalyseResultByCapsule<GpgFrontend::GpgSignResult,
                                GpgFrontend::GpgSignResultAnalyse>(
      channel, err, capsule_id, analyse, cards);
}

auto GF_SDK_EXPORT GFAnalyseDecryptResultByCapsule(int channel,
                                                   gpgme_error_t err,
                                                   const char* capsule_id,
                                                   const char** analyse,
                                                   const char** cards) -> int {
  return AnalyseResultByCapsule<GpgFrontend::GpgDecryptResult,
                                GpgFrontend::GpgDecryptResultAnalyse>(
      channel, err, capsule_id, analyse, cards);
}

auto GF_SDK_EXPORT GFAnalyseVerifyResultByCapsule(int channel,
                                                  gpgme_error_t err,
                                                  const char* capsule_id,
                                                  const char** analyse,
                                                  const char** cards) -> int {
  return AnalyseResultByCapsule<GpgFrontend::GpgVerifyResult,
                                GpgFrontend::GpgVerifyResultAnalyse>(
      channel, err, capsule_id, analyse, cards);
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

// Same recovery path as AnalyseResultByCapsule, with the structured form
// emitted alongside. They are one function rather than two because the capsule
// is consumed on first use: a caller cannot ask for cards now and JSON later.
template <typename ResultT, typename AnalyseT>
auto AnalyseResultInfoByCapsule(int channel, gpgme_error_t err,
                                const char* capsule_id, const char** analyse,
                                const char** cards, const char** info_json)
    -> int {
  if (analyse == nullptr) return -1;

  auto capsule = GpgFrontend::UI::UIModuleManager::GetInstance().GetCapsule(
      GFStrView(capsule_id));

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

auto GF_SDK_EXPORT GFAnalyseVerifyResultInfoByCapsule(
    int channel, gpgme_error_t err, const char* capsule_id, const char** analyse,
    const char** cards, const char** info_json) -> int {
  return AnalyseResultInfoByCapsule<GpgFrontend::GpgVerifyResult,
                                    GpgFrontend::GpgVerifyResultAnalyse>(
      channel, err, capsule_id, analyse, cards, info_json);
}

auto GF_SDK_EXPORT GFAnalyseSignResultInfoByCapsule(
    int channel, gpgme_error_t err, const char* capsule_id, const char** analyse,
    const char** cards, const char** info_json) -> int {
  return AnalyseResultInfoByCapsule<GpgFrontend::GpgSignResult,
                                    GpgFrontend::GpgSignResultAnalyse>(
      channel, err, capsule_id, analyse, cards, info_json);
}

auto GF_SDK_EXPORT GFAnalyseEncryptResultInfoByCapsule(
    int channel, gpgme_error_t err, const char* capsule_id, const char** analyse,
    const char** cards, const char** info_json) -> int {
  return AnalyseResultInfoByCapsule<GpgFrontend::GpgEncryptResult,
                                    GpgFrontend::GpgEncryptResultAnalyse>(
      channel, err, capsule_id, analyse, cards, info_json);
}

auto GF_SDK_EXPORT GFAnalyseDecryptResultInfoByCapsule(
    int channel, gpgme_error_t err, const char* capsule_id, const char** analyse,
    const char** cards, const char** info_json) -> int {
  return AnalyseResultInfoByCapsule<GpgFrontend::GpgDecryptResult,
                                    GpgFrontend::GpgDecryptResultAnalyse>(
      channel, err, capsule_id, analyse, cards, info_json);
}

auto GFGpgFindKeysByEmail(int channel, const char* email,
                                        GFGpgKeyBrief** keys, int* count)
    -> int {
  if (keys == nullptr || count == nullptr) return -1;
  *keys = nullptr;
  *count = 0;

  // Not GFUnStrDup: that helper FREES what it is handed, which is right for
  // the char* parameters the SDK takes ownership of, but `email` is a plain
  // const input the caller still owns and will free itself.
  if (email == nullptr) return -1;
  const auto wanted = QString::fromUtf8(email).trimmed();
  if (wanted.isEmpty()) return -1;

  struct Match {
    QSharedPointer<GpgFrontend::GpgKey> key;
    GpgFrontend::GpgUID uid;
    bool primary;
  };
  QList<Match> matches;

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
    bool first = true;
    for (const auto& uid : uids) {
      if (uid.GetEmail().compare(wanted, Qt::CaseInsensitive) == 0) {
        matches.append({key, uid, first});
        break;
      }
      first = false;
    }
  }

  if (matches.isEmpty()) return 0;

  auto* array = static_cast<GFGpgKeyBrief*>(
      GFAllocateMemory(sizeof(GFGpgKeyBrief) * matches.size()));
  if (array == nullptr) return -1;
  std::memset(array, 0, sizeof(GFGpgKeyBrief) * matches.size());

  for (int i = 0; i < matches.size(); ++i) {
    const auto& match = matches[i];
    auto& brief = array[i];

    brief.fingerprint = GFStrDup(match.key->Fingerprint());
    brief.key_id = GFStrDup(match.key->ID());
    brief.uid = GFStrDup(match.key->UID());
    brief.matched_email = GFStrDup(match.uid.GetEmail());

    const auto expires = match.key->ExpirationTime();
    brief.expires_at = expires.isValid()
                           ? static_cast<int64_t>(expires.toSecsSinceEpoch())
                           : 0;

    // Usability only. Whether this key belongs to whoever claimed the address
    // is a separate question, answered by the matched_uid_* fields and by the
    // caller -- never folded into this number.
    brief.usability =
        static_cast<int>(GpgFrontend::GetKeyStatus(match.key.get()));

    brief.can_encrypt = match.key->IsHasActualEncrCap() ? 1 : 0;
    brief.can_sign = match.key->IsHasActualSignCap() ? 1 : 0;
    brief.matched_uid_is_primary = match.primary ? 1 : 0;
    brief.matched_uid_revoked = match.uid.GetRevoked() ? 1 : 0;
  }

  *keys = array;
  *count = static_cast<int>(matches.size());
  return 0;
}

auto GFGpgListKeyAddresses(int channel, int secret_only,
                                         char*** addresses, int* count) -> int {
  if (addresses == nullptr || count == nullptr) return -1;
  *addresses = nullptr;
  *count = 0;

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
    if (secret_only != 0 && !key->IsPrivateKey()) continue;

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

  if (entries.isEmpty()) return 0;

  entries.sort(Qt::CaseInsensitive);

  auto* array =
      static_cast<char**>(GFAllocateMemory(sizeof(char*) * entries.size()));
  if (array == nullptr) return -1;

  for (int i = 0; i < entries.size(); ++i) {
    array[i] = GFStrDup(entries.at(i));
  }

  *addresses = array;
  *count = static_cast<int>(entries.size());
  return 0;
}

auto GFGpgFreeStringArray(char** strings, int count) -> void {
  if (strings == nullptr) return;
  for (int i = 0; i < count; ++i) GFFreeMemory(strings[i]);
  GFFreeMemory(strings);
}

auto GFGpgFreeKeyBriefs(GFGpgKeyBrief* keys, int count) -> void {
  if (keys == nullptr) return;
  for (int i = 0; i < count; ++i) {
    GFFreeMemory(keys[i].fingerprint);
    GFFreeMemory(keys[i].key_id);
    GFFreeMemory(keys[i].uid);
    GFFreeMemory(keys[i].matched_email);
  }
  GFFreeMemory(keys);
}

auto GFGpgSniffEncryptedRecipients(int channel, const char* data,
                                                 int size,
                                                 GFGpgEncRecipient** out,
                                                 int* count) -> int {
  if (out == nullptr || count == nullptr) return -1;
  *out = nullptr;
  *count = 0;

  // Like `email` in GFGpgFindKeysByEmail, `data` is a plain const input the
  // caller still owns: not GFUnStrDup, which would free it.
  if (data == nullptr || size <= 0) return -1;

  const auto key_ids = GpgFrontend::SniffRecipientKeyIds(
      GpgFrontend::GFBuffer(data, static_cast<size_t>(size)));
  if (key_ids.isEmpty()) return 0;

  auto* array = static_cast<GFGpgEncRecipient*>(
      GFAllocateMemory(sizeof(GFGpgEncRecipient) * key_ids.size()));
  if (array == nullptr) return -1;
  std::memset(array, 0, sizeof(GFGpgEncRecipient) * key_ids.size());

  auto& repository = GpgFrontend::AbstractKeyRepository::GetInstance(channel);

  for (int i = 0; i < key_ids.size(); ++i) {
    const auto& key_id = key_ids[i];
    auto& entry = array[i];

    entry.key_id = GFStrDup(key_id);
    entry.pub_algo = GFStrDup(QString{});
    entry.fingerprint = GFStrDup(QString{});
    entry.uid = GFStrDup(QString{});

    // An all-zero identifier is the wildcard key id: the sender asked for the
    // recipient to be withheld. Looking it up would report "no such key" for
    // what is really "no answer given", and the user may well be that hidden
    // recipient themselves.
    if (key_id.count('0') == key_id.size()) {
      entry.hidden = 1;
      continue;
    }

    // Resolves a key id OR a fingerprint, and a subkey as readily as a
    // primary -- which is what this needs, since a PKESK names the encryption
    // subkey. Engine-neutral, so it is equally right on a GnuPG channel and
    // an rPGP one.
    auto key = repository.GetKey(key_id);
    if (key == nullptr) continue;

    entry.key_found = 1;
    // The secret half, and nothing else, is what decides decryptability.
    entry.has_secret = key->IsPrivateKey() ? 1 : 0;

    GFFreeMemory(entry.pub_algo);
    GFFreeMemory(entry.fingerprint);
    GFFreeMemory(entry.uid);
    entry.pub_algo = GFStrDup(key->PublicKeyAlgo());
    entry.fingerprint = GFStrDup(key->Fingerprint());
    entry.uid = GFStrDup(key->UID());
  }

  *out = array;
  *count = static_cast<int>(key_ids.size());
  return 0;
}

auto GFGpgFreeEncRecipients(GFGpgEncRecipient* out, int count)
    -> void {
  if (out == nullptr) return;
  for (int i = 0; i < count; ++i) {
    GFFreeMemory(out[i].key_id);
    GFFreeMemory(out[i].pub_algo);
    GFFreeMemory(out[i].fingerprint);
    GFFreeMemory(out[i].uid);
  }
  GFFreeMemory(out);
}
