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

#include "model/KeyDescription.h"

#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/helper/OpSupport.h"
#include "core/function/openpgp/support/KeyGenerationOpSupport.h"
#include "core/function/openpgp/support/KeyImportExportOpSupport.h"
#include "core/function/openpgp/support/KeyManagementOpSupport.h"
#include "core/function/openpgp/support/UserIdOperaOpSupport.h"
#include "core/utils/GpgUtils.h"
#include "model/KeyRoleModel.h"

namespace GpgFrontend::Mobile {

namespace {

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

}  // namespace

auto DescribeKey(int channel, const QString& fingerprint) -> QVariantMap {
  const auto key =
      GpgKeyRepository::GetInstance(channel).GetKeyPtr(fingerprint.toUpper());
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

  // What changes the key needs its secret primary key, as on the desktop;
  // a revoked key is changed no more (the desktop hides those tabs): it is
  // shared, backed up or deleted.
  const auto own =
      key->IsPrivateKey() && key->IsHasMasterKey() && !key->IsRevoked();
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
       own && IsOpSupported<ModifyKeyPassphraseOpTag>(channel)},
      {QStringLiteral("revokeKey"),
       own && IsOpSupported<GenerateRevCertOpTag>(channel) &&
           IsOpSupported<ImportRevCertOpTag>(channel)},
      {QStringLiteral("saveRevocation"),
       own && IsOpSupported<GenerateRevCertOpTag>(channel)},
      {QStringLiteral("addUid"), own && IsOpSupported<AddUserIdOpTag>(channel)},
      {QStringLiteral("setPrimaryUid"),
       own && IsOpSupported<SetPrimaryUserIdOpTag>(channel)},
      {QStringLiteral("revokeUid"),
       own && IsOpSupported<RevokeUserIdOpTag>(channel)},
      {QStringLiteral("deleteUid"),
       own && IsOpSupported<DeleteUserIdOpTag>(channel)},
  };

  // Its state as the key list says it, read now: the page shows it after a
  // change rather than what the list said when it was opened.
  auto described = KeyRoleModel::StatusFacts(key.get());
  described.insert({
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
  });
  return described;
}

}  // namespace GpgFrontend::Mobile
