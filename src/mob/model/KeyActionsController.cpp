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

#include "model/KeyActionsController.h"

#include <QFile>
#include <QPointer>
#include <algorithm>

#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/function/openpgp/UserIdOperation.h"
#include "core/function/openpgp/helper/OpSupport.h"
#include "core/function/openpgp/support/KeyGenerationOpSupport.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/function/openpgp/support/UserIdOperaOpSupport.h"
#include "core/model/DataObject.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKeyGenerateInfo.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"
#include "files/StagingArea.h"
#include "model/ChangeRelay.h"
#include "model/ErrorWording.h"
#include "model/KeyGenProfilesController.h"
#include "model/KeyRoleModel.h"
#include "shell/rules/KeyExportRules.h"
#include "shell/rules/KeyGenProfile.h"

namespace GpgFrontend::Mobile {

namespace {

auto Offered() -> QList<ResolvedKeyGenProfile> {
  // The profile's own, as the desktop made them, else the defaults.
  const auto stored = LoadStoredKeyGenProfiles();
  auto offered =
      OfferedKeyGenProfiles(UsableDefaultChannelHandle(), stored.profiles);
  // The one Quick Generate uses first, the rest in their usual order.
  const auto preferred =
      PickDefaultKeyGenProfile(offered, stored.default_profile);
  if (preferred > 0) offered.move(preferred, 0);
  return offered;
}

auto Failure(const GFError& err) -> QVariantMap {
  return DescribeActionError(err);
}

auto FindKey(int channel, const QString& fingerprint) -> GpgAbstractKeyPtr {
  return GpgKeyRepository::GetInstance(channel).GetKeyPtr(
      fingerprint.toUpper());
}

auto FindGpgKey(int channel, const QString& fingerprint) -> GpgKeyPtr {
  return qSharedPointerDynamicCast<GpgKey>(FindKey(channel, fingerprint));
}

/// Where @p subkey (a fingerprint) is among @p key's subkeys, the primary
/// being 0, as the engines number them; -1 when it is not there.
auto SubkeyIndex(const GpgKey& key, const QString& subkey) -> int {
  const auto subkeys = key.SubKeys();
  for (int i = 0; i < subkeys.size(); ++i) {
    if (subkeys.at(i).Fingerprint().compare(subkey, Qt::CaseInsensitive) == 0) {
      return i;
    }
  }
  return -1;
}

/// A date the engines give as 0 for "none".
auto DateOrNone(const QDateTime& time) -> QDateTime {
  return time.isValid() && time.toSecsSinceEpoch() > 0 ? time : QDateTime();
}

auto Usage(bool certify, bool encrypt, bool sign, bool authenticate)
    -> QVariantMap {
  return {{QStringLiteral("certify"), certify},
          {QStringLiteral("encrypt"), encrypt},
          {QStringLiteral("sign"), sign},
          {QStringLiteral("authenticate"), authenticate}};
}

/// After a change to a key: read again, so what is shown is what is there.
void Reread(int channel) {
  GpgKeyRepository::GetInstance(channel).FlushKeyCache();
}

/// How a change to a key on @p channel ended: read again when it was made,
/// else what the engine said went wrong.
auto Settled(int channel, const GFError& err) -> QVariantMap {
  if (!err.Ok()) return Failure(err);
  Reread(channel);
  return {};
}

/// The first thing wrong with a user ID, the name before the email before
/// the comment, as the desktop's wizard lists them.
auto FirstProblem(const KeyGenIdentityCheck& identity) -> QString {
  if (!identity.name.isEmpty()) return identity.name;
  if (!identity.email.isEmpty()) return identity.email;
  return identity.comment;
}

}  // namespace

KeyActionsController::KeyActionsController(QObject* parent) : QObject(parent) {
  // The profiles' and the validities' names are translated ones.
  connect(&LanguageRelay(), &ChangeRelay::SignalChanged, this,
          &KeyActionsController::SignalProfilesChanged);
}

auto KeyActionsController::Busy() const -> bool { return busy_; }

auto KeyActionsController::Profiles() const -> QVariantList {
  QVariantList list;
  for (const auto& resolved : Offered()) {
    list.append(QVariantMap{
        {QStringLiteral("name"), KeyGenProfileDisplayName(resolved.profile)},
        {QStringLiteral("description"),
         KeyGenProfileDisplayDescription(resolved.profile)},
    });
  }
  return list;
}

auto KeyActionsController::checkIdentity(const QString& name,
                                         const QString& email,
                                         const QString& comment) const
    -> QVariantMap {
  const auto identity = CheckKeyGenIdentity(name, email, comment);
  return {{QStringLiteral("ok"), identity.Ok()},
          {QStringLiteral("name"), identity.name},
          {QStringLiteral("email"), identity.email},
          {QStringLiteral("comment"), identity.comment}};
}

void KeyActionsController::generate(const QString& name, const QString& email,
                                    int profile, bool with_passphrase,
                                    const QString& comment) {
  const auto offered = Offered();
  if (profile < 0 || profile >= offered.size()) {
    emit SignalFinished(QStringLiteral("generate"), false,
                        Failure(GFError::Application(GFErrorCode::kINVALID_VALUE)));
    return;
  }
  const auto resolved = offered.at(profile);
  // The name and comment become part of a mail address: what would break it
  // is refused here, as the desktop's wizard refuses it.
  const auto identity = CheckKeyGenIdentity(name, email, comment);
  if (!identity.Ok()) {
    emit SignalFinished(QStringLiteral("generate"), false,
                        {{QStringLiteral("error"), FirstProblem(identity)}});
    return;
  }

  run(QStringLiteral("generate"), [=]() -> QVariantMap {
    auto key = SecureCreateSharedObject<KeyGenerateInfo>();
    QSharedPointer<KeyGenerateInfo> subkey;
    // Before the profile, which gives a subkey the same choice.
    key->SetNonPassPhrase(!with_passphrase);
    ApplyKeyGenProfile(resolved, QDateTime::currentDateTime(), *key, subkey);
    key->SetName(name.trimmed());
    key->SetEmail(email.trimmed());
    key->SetComment(comment.trimmed());
    ApplyDefaultKeyVersion(*key, subkey);

    const auto channel = UsableDefaultChannelHandle();
    auto& opera = KeyGenerationOperation::GetInstance(channel);
    auto [err, data] = subkey != nullptr
                           ? opera.GenerateKeyWithSubkeySync(key, subkey)
                           : opera.GenerateKeySync(key);
    if (!err.Ok() || data == nullptr ||
        data->GetObjectSize() == 0) {
      return Failure(err);
    }
    GpgKeyRepository::GetInstance(channel).FlushKeyCache();
    return {{QStringLiteral("fingerprint"),
             ExtractParams<GpgGenerateKeyResult>(data, 0).GetFingerprint()}};
  });
}

void KeyActionsController::generateCustom(
    const QString& name, const QString& email, const QVariantMap& primary,
    const QVariantMap& subkey, bool with_passphrase, const QString& comment) {
  const auto identity = CheckKeyGenIdentity(name, email, comment);
  if (!identity.Ok()) {
    emit SignalFinished(QStringLiteral("generate"), false,
                        {{QStringLiteral("error"), FirstProblem(identity)}});
    return;
  }
  const auto channel = UsableDefaultChannelHandle();
  auto key = KeyGenProfilesController::InfoFromChoices(
      primary, false, KeyGenerateInfo::GetSupportedKeyAlgo(channel), channel);
  QSharedPointer<KeyGenerateInfo> sub;
  if (!subkey.isEmpty()) {
    sub = KeyGenProfilesController::InfoFromChoices(
        subkey, true, KeyGenerateInfo::GetSupportedSubkeyAlgo(channel),
        channel);
  }
  if (key == nullptr || (!subkey.isEmpty() && sub == nullptr)) {
    emit SignalFinished(
        QStringLiteral("generate"), false,
        Failure(GFError::Application(GFErrorCode::kINVALID_VALUE)));
    return;
  }

  run(QStringLiteral("generate"), [=]() -> QVariantMap {
    key->SetName(name.trimmed());
    key->SetEmail(email.trimmed());
    key->SetComment(comment.trimmed());
    key->SetNonPassPhrase(!with_passphrase);
    if (sub != nullptr) sub->SetNonPassPhrase(!with_passphrase);
    ApplyDefaultKeyVersion(*key, sub);

    auto& opera = KeyGenerationOperation::GetInstance(channel);
    auto [err, data] = sub != nullptr
                           ? opera.GenerateKeyWithSubkeySync(key, sub)
                           : opera.GenerateKeySync(key);
    if (!err.Ok() || data == nullptr || data->GetObjectSize() == 0) {
      return Failure(err);
    }
    GpgKeyRepository::GetInstance(channel).FlushKeyCache();
    return {{QStringLiteral("fingerprint"),
             ExtractParams<GpgGenerateKeyResult>(data, 0).GetFingerprint()}};
  });
}

void KeyActionsController::reloadProfiles() { emit SignalProfilesChanged(); }

void KeyActionsController::addCustomSubkey(const QString& fingerprint,
                                           const QVariantMap& choices,
                                           bool with_passphrase) {
  run(QStringLiteral("add_subkey"), [=]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    auto info = KeyGenProfilesController::InfoFromChoices(
        choices, true, KeyGenerateInfo::GetSupportedSubkeyAlgo(channel, *key),
        channel);
    if (info == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kINVALID_VALUE));
    }
    info->SetNonPassPhrase(!with_passphrase);
    auto [err, data] =
        KeyGenerationOperation::GetInstance(channel).GenerateSubkeySync(key,
                                                                        info);
    if (!err.Ok()) return Failure(err);
    Reread(channel);
    return {};
  });
}

void KeyActionsController::exportPublic(const QString& fingerprint) {
  run(QStringLiteral("export"), [fingerprint]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindKey(channel, fingerprint);
    if (key == nullptr)
      return Failure(GFError::Application(GFErrorCode::kNO_PUBLIC_KEY));
    const auto options =
        Shell::KeyExportOptionsFor(Shell::KeyExportKind::kPublic);
    auto [err, armored] =
        KeyImportExportOperation::GetInstance(channel).ExportKey(
            key, options.secret, options.ascii, options.shortest);
    if (!err.Ok()) return Failure(err);
    return {{QStringLiteral("armored"),
             QString::fromUtf8(armored.ConvertToQByteArray())},
            {QStringLiteral("fileName"),
             Shell::ExportKeyFileName(key->Name(), key->Email(), key->ID(),
                                      options.type)}};
  });
}

auto KeyActionsController::secretKeyFileName(const QString& fingerprint) const
    -> QString {
  const auto key = FindKey(UsableDefaultChannelHandle(), fingerprint);
  if (key == nullptr) return {};
  return Shell::ExportKeyFileName(
      key->Name(), key->Email(), key->ID(),
      Shell::KeyExportOptionsFor(Shell::KeyExportKind::kFullSecret).type);
}

void KeyActionsController::exportSecret(const QString& fingerprint,
                                        const QUrl& url) {
  // A picked place is a content:// URI on Android, which QFile opens as is.
  const auto path = LocalOrContent(url);
  run(QStringLiteral("export_secret"), [fingerprint, path]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindKey(channel, fingerprint);
    if (key == nullptr || !key->IsPrivateKey()) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    // Straight from the engine's buffer to the file: the key never becomes
    // a string anywhere, and stays protected by its own passphrase.
    // The whole key, as the desktop's full secret export.
    const auto options =
        Shell::KeyExportOptionsFor(Shell::KeyExportKind::kFullSecret);
    auto [err, armored] =
        KeyImportExportOperation::GetInstance(channel).ExportKey(
            key, options.secret, options.ascii, options.shortest);
    if (!err.Ok()) return Failure(err);
    if (armored.Empty()) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        file.write(armored.Data(), static_cast<qint64>(armored.Size())) !=
            static_cast<qint64>(armored.Size()) ||
        !file.flush()) {
      file.close();
      return Failure(GFError::Application(GFErrorCode::kIO));
    }
    file.close();
    return {};
  });
}

void KeyActionsController::remove(const QString& fingerprint) {
  run(QStringLiteral("delete"), [fingerprint]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindKey(channel, fingerprint);
    if (key == nullptr) return Failure(GFError::Application(GFErrorCode::kNO_PUBLIC_KEY));
    const auto deleted =
        KeyManagementOperation::GetInstance(channel).DeleteKey(key);
    auto& repository = GpgKeyRepository::GetInstance(channel);
    repository.FlushKeyCache();
    if (!deleted || FindKey(channel, fingerprint) != nullptr) {
      return Failure(GFError::Application(GFErrorCode::kGENERAL));
    }
    return {};
  });
}

auto KeyActionsController::Validities() const -> QVariantList {
  QVariantList list;
  for (const auto& id : KeyGenValidityPresets()) {
    list.append(
        QVariantMap{{QStringLiteral("id"), id},
                    {QStringLiteral("name"), KeyGenValidityDisplayName(id)}});
  }
  return list;
}

auto KeyActionsController::describe(const QString& fingerprint) const
    -> QVariantMap {
  const auto channel = UsableDefaultChannelHandle();
  const auto key = FindGpgKey(channel, fingerprint);
  if (key == nullptr || !key->IsGood()) return {};

  QVariantList uids;
  const auto key_uids = key->UIDs();
  for (int i = 0; i < key_uids.size(); ++i) {
    const auto& uid = key_uids.at(i);
    uids.append(QVariantMap{
        {QStringLiteral("uid"), uid.GetUID()},
        {QStringLiteral("name"), uid.GetName()},
        {QStringLiteral("email"), uid.GetEmail()},
        {QStringLiteral("comment"), uid.GetComment()},
        {QStringLiteral("primary"), i == 0},
        {QStringLiteral("revoked"), uid.GetRevoked()},
        {QStringLiteral("invalid"), uid.GetInvalid()},
    });
  }

  QVariantList subkeys;
  const auto key_subkeys = key->SubKeys();
  for (int i = 0; i < key_subkeys.size(); ++i) {
    const auto& sub = key_subkeys.at(i);
    subkeys.append(QVariantMap{
        {QStringLiteral("index"), i},
        {QStringLiteral("primary"), i == 0},
        {QStringLiteral("keyId"), sub.ID()},
        {QStringLiteral("fingerprint"), sub.Fingerprint()},
        {QStringLiteral("algorithm"), sub.PublicKeyAlgo()},
        {QStringLiteral("algorithmDetail"), sub.Algo()},
        {QStringLiteral("keySize"), sub.KeyLength()},
        {QStringLiteral("usage"),
         Usage(sub.IsHasCertCap(), sub.IsHasEncrCap(), sub.IsHasSignCap(),
               sub.IsHasAuthCap())},
        // This part's own, in GnuPG's letters, whether or not it is usable.
        {QStringLiteral("capabilities"), GetUsagesByAbstractKey(&sub)},
        {QStringLiteral("created"), sub.CreationTime()},
        {QStringLiteral("expires"), DateOrNone(sub.ExpirationTime())},
        {QStringLiteral("secret"), sub.IsSecretKey()},
        {QStringLiteral("card"), sub.IsCardKey()},
        {QStringLiteral("expired"), sub.IsExpired()},
        {QStringLiteral("revoked"), sub.IsRevoked()},
        {QStringLiteral("disabled"), sub.IsDisabled()},
        {QStringLiteral("adsk"), sub.IsADSK()},
    });
  }

  // What changes the key needs its secret primary key, as on the desktop.
  const auto own = key->IsPrivateKey() && key->IsHasMasterKey();
  const auto can = QVariantMap{
      {QStringLiteral("addSubkey"),
       own && IsOpSupported<GenerateSubKeyTag>(channel)},
      {QStringLiteral("setExpiration"),
       own && IsOpSupported<SetExpireOpTag>(channel)},
      {QStringLiteral("revokeSubkey"),
       own && IsOpSupported<RevokeSubKeyOpTag>(channel)},
      {QStringLiteral("deleteSubkey"),
       own && IsOpSupported<DeleteSubKeyOpTag>(channel)},
      {QStringLiteral("changePassphrase"),
       key->IsPrivateKey() && IsOpSupported<ModifyKeyPassphraseOpTag>(channel)},
      {QStringLiteral("addUid"), own && IsOpSupported<AddUserIdOpTag>(channel)},
      {QStringLiteral("setPrimaryUid"),
       own && IsOpSupported<SetPrimaryUserIdOpTag>(channel)},
      {QStringLiteral("revokeUid"),
       own && IsOpSupported<RevokeUserIdOpTag>(channel)},
      {QStringLiteral("deleteUid"),
       own && IsOpSupported<DeleteUserIdOpTag>(channel)},
  };

  return {
      {QStringLiteral("name"), key->Name()},
      {QStringLiteral("email"), key->Email()},
      {QStringLiteral("comment"), key->Comment()},
      {QStringLiteral("keyId"), key->ID()},
      {QStringLiteral("fingerprint"), key->Fingerprint()},
      {QStringLiteral("version"), key->KeyVersion()},
      {QStringLiteral("algorithm"), key->PublicKeyAlgo()},
      {QStringLiteral("algorithmDetail"), key->Algo()},
      {QStringLiteral("keySize"), key->PrimaryKeyLength()},
      {QStringLiteral("usage"),
       Usage(key->IsHasActualCertCap(), key->IsHasActualEncrCap(),
             key->IsHasActualSignCap(), key->IsHasActualAuthCap())},
      {QStringLiteral("created"), key->CreationTime()},
      {QStringLiteral("expires"), DateOrNone(key->ExpirationTime())},
      {QStringLiteral("lastUpdate"), DateOrNone(key->LastUpdateTime())},
      {QStringLiteral("hasSecret"), key->IsPrivateKey()},
      {QStringLiteral("primarySecret"), key->IsHasMasterKey()},
      // GnuPG's mark, as the key lists show it (sec, sec#, sec>, pub).
      {QStringLiteral("secretState"), KeyRoleModel::SecretState(key.get())},
      // What the key as a whole can do, as the key lists show it.
      {QStringLiteral("capabilities"), KeyRoleModel::Capabilities(key.get())},
      {QStringLiteral("cardKey"), key->IsHasCardKey()},
      {QStringLiteral("expired"), key->IsExpired()},
      {QStringLiteral("revoked"), key->IsRevoked()},
      {QStringLiteral("disabled"), key->IsDisabled()},
      {QStringLiteral("uids"), uids},
      {QStringLiteral("subkeys"), subkeys},
      {QStringLiteral("can"), can},
  };
}

void KeyActionsController::addSubkey(const QString& fingerprint,
                                     const QString& purpose,
                                     const QString& validity,
                                     bool with_passphrase) {
  const auto parsed = ParseSubkeyPurpose(purpose);
  if (!parsed) {
    emit SignalFinished(
        QStringLiteral("add_subkey"), false,
        Failure(GFError::Application(GFErrorCode::kINVALID_VALUE)));
    return;
  }
  run(QStringLiteral("add_subkey"), [=]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    const auto subkey_purpose = *parsed;
    const auto algo = SuggestSubkeyAlgo(
        subkey_purpose, NormalizeKeyAlgoId(key->Algo()),
        KeyGenerateInfo::GetSupportedSubkeyAlgo(channel, *key));
    if (algo == KeyGenerateInfo::kNoneAlgo) {
      return Failure(GFError::Application(GFErrorCode::kNOT_SUPPORTED));
    }

    auto info = SecureCreateSharedObject<KeyGenerateInfo>(true);
    ApplySubkeyPurpose(*info, subkey_purpose, algo);
    const auto never = IsNeverExpiringValidity(validity);
    info->SetNonExpired(never);
    if (!never) {
      info->SetExpireTime(
          ParseKeyGenValidity(validity, QDateTime::currentDateTime()));
    }
    info->SetNonPassPhrase(!with_passphrase);

    auto [err, data] =
        KeyGenerationOperation::GetInstance(channel).GenerateSubkeySync(key,
                                                                        info);
    if (!err.Ok()) return Failure(err);
    Reread(channel);
    return {};
  });
}

void KeyActionsController::setExpiration(const QString& fingerprint,
                                         const QString& subkey,
                                         const QString& validity) {
  run(QStringLiteral("expiration"), [=]() -> QVariantMap {
    const auto never = IsNeverExpiringValidity(validity);
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    const auto err = KeyManagementOperation::GetInstance(channel).SetExpire(
        key, subkey,
        never ? std::optional<QDateTime>()
              : std::optional<QDateTime>(ParseKeyGenValidity(
                    validity, QDateTime::currentDateTime())));
    if (!err.Ok()) return Failure(err);
    Reread(channel);
    return {};
  });
}

void KeyActionsController::revokeSubkey(const QString& fingerprint,
                                        const QString& subkey, int reason,
                                        const QString& text) {
  run(QStringLiteral("revoke_subkey"), [=]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    // The primary key is not a subkey to revoke.
    const auto index = SubkeyIndex(*key, subkey);
    if (index <= 0) {
      return Failure(GFError::Application(GFErrorCode::kINVALID_VALUE));
    }
    const auto err =
        KeyManagementOperation::GetInstance(channel).RevokeSubkeySync(
            key, index, std::clamp(reason, 0, 3), text);
    return Settled(channel, err);
  });
}

void KeyActionsController::deleteSubkey(const QString& fingerprint,
                                        const QString& subkey) {
  run(QStringLiteral("delete_subkey"), [=]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    const auto index = SubkeyIndex(*key, subkey);
    if (index <= 0) {
      return Failure(GFError::Application(GFErrorCode::kINVALID_VALUE));
    }
    const auto err =
        KeyManagementOperation::GetInstance(channel).DeleteSubkeySync(key,
                                                                      index);
    return Settled(channel, err);
  });
}

void KeyActionsController::addUid(const QString& fingerprint,
                                  const QString& name, const QString& email,
                                  const QString& comment) {
  // As a new key's user ID is checked: its parts become a mail address.
  const auto identity = CheckKeyGenIdentity(name, email, comment);
  if (!identity.Ok()) {
    emit SignalFinished(QStringLiteral("add_uid"), false,
                        {{QStringLiteral("error"), FirstProblem(identity)}});
    return;
  }
  run(QStringLiteral("add_uid"), [=]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    const auto err = UserIdOperation::GetInstance(channel).AddUIDSync(
        key, name.trimmed(), comment.trimmed(), email.trimmed());
    return Settled(channel, err);
  });
}

void KeyActionsController::setPrimaryUid(const QString& fingerprint,
                                         const QString& uid) {
  run(QStringLiteral("primary_uid"), [=]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    const auto err =
        UserIdOperation::GetInstance(channel).SetPrimaryUIDSync(key, uid);
    return Settled(channel, err);
  });
}

void KeyActionsController::revokeUid(const QString& fingerprint,
                                     const QString& uid, int reason,
                                     const QString& text) {
  run(QStringLiteral("revoke_uid"), [=]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    // The primary user ID is the key's name: it is not revoked on its own.
    if (!key->UIDs().isEmpty() && key->UIDs().front().GetUID() == uid) {
      return Failure(GFError::Application(GFErrorCode::kINVALID_VALUE));
    }
    // The reason as given: a user ID takes only 0 (none) or 4 (no longer
    // valid), and the core refuses any other.
    const auto err = UserIdOperation::GetInstance(channel).RevokeUIDSync(
        key, uid, reason, text);
    return Settled(channel, err);
  });
}

void KeyActionsController::deleteUid(const QString& fingerprint,
                                     const QString& uid) {
  run(QStringLiteral("delete_uid"), [=]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    if (!key->UIDs().isEmpty() && key->UIDs().front().GetUID() == uid) {
      return Failure(GFError::Application(GFErrorCode::kINVALID_VALUE));
    }
    const auto err =
        UserIdOperation::GetInstance(channel).DeleteUIDSync(key, uid);
    return Settled(channel, err);
  });
}

void KeyActionsController::changePassphrase(const QString& fingerprint) {
  run(QStringLiteral("passphrase"), [=]() -> QVariantMap {
    const auto channel = UsableDefaultChannelHandle();
    const auto key = FindGpgKey(channel, fingerprint);
    if (key == nullptr) {
      return Failure(GFError::Application(GFErrorCode::kNO_SECRET_KEY));
    }
    auto [err, data] =
        KeyManagementOperation::GetInstance(channel).ModifyPasswordSync(key);
    if (!err.Ok()) return Failure(err);
    Reread(channel);
    return {};
  });
}

auto KeyActionsController::begin() -> bool {
  if (busy_) return false;
  busy_ = true;
  emit SignalBusyChanged();
  return true;
}

void KeyActionsController::run(const QString& action,
                               std::function<QVariantMap()> work) {
  if (!begin()) return;

  auto detail = QSharedPointer<QVariantMap>::create();
  QPointer<KeyActionsController> self(this);
  auto* task = new Thread::Task(
      [work = std::move(work), detail](const DataObjectPtr&) -> int {
        *detail = work();
        return 0;
      },
      "mobile_key_" + action, TransferParams(),
      [self, action, detail](int, const DataObjectPtr&) {
        if (self == nullptr) return;
        self->busy_ = false;
        emit self->SignalBusyChanged();
        emit self->SignalFinished(
            action, !detail->contains(QStringLiteral("error")), *detail);
      });

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(task);
}

}  // namespace GpgFrontend::Mobile
