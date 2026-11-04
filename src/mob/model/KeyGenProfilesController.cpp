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

#include "model/KeyGenProfilesController.h"

#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend::Mobile {

namespace {

auto AlgoRef(const QString& id, const QString& type) -> QVariantMap {
  return {{QStringLiteral("id"), id}, {QStringLiteral("type"), type}};
}

auto Describe(const KeyAlgo& algo) -> QVariantMap {
  return {
      {QStringLiteral("id"), algo.Id()},
      {QStringLiteral("type"), algo.Type()},
      {QStringLiteral("name"),
       DescribeKeyAlgo(algo, KeyGenerateInfo::kNoneAlgo)},
      {QStringLiteral("postQuantum"), algo.IsPostQuantum()},
      {QStringLiteral("canEncrypt"), algo.CanEncrypt()},
      {QStringLiteral("canSign"), algo.CanSign()},
      {QStringLiteral("canAuthenticate"), algo.CanAuth()},
  };
}

auto DescribeAll(const QList<KeyAlgo>& algos) -> QVariantList {
  QVariantList list;
  for (const auto& algo : algos) {
    if (algo != KeyGenerateInfo::kNoneAlgo) list.append(Describe(algo));
  }
  return list;
}

/// The algorithm @p ref ({id, type}) names among @p algos.
auto Find(const QVariantMap& ref, const QList<KeyAlgo>& algos)
    -> std::optional<KeyAlgo> {
  const auto [found, algo] =
      GetAlgoByIdType(ref.value(QStringLiteral("id")).toString(),
                      ref.value(QStringLiteral("type")).toString(), algos);
  if (!found) return std::nullopt;
  return algo;
}

auto Channel() -> int { return UsableDefaultChannelHandle(); }

/// @p p as this channel would make it, hidden or not; nullopt when it
/// cannot be made here.
auto Resolve(const KeyGenProfile& p) -> std::optional<ResolvedKeyGenProfile> {
  const auto channel = Channel();
  return ResolveKeyGenProfile(
      p, KeyGenerateInfo::GetSupportedKeyAlgo(channel),
      KeyGenerateInfo::GetSupportedSubkeyAlgo(channel),
      [channel](const KeyAlgo& algo) { return algo.SubAlgos(channel); });
}

}  // namespace

KeyGenProfilesController::KeyGenProfilesController(QObject* parent)
    : QObject(parent) {}

auto KeyGenProfilesController::Profiles() const -> QVariantList {
  const auto settings = LoadKeyGenSettings();
  // The one Quick Generate uses, as it picks it: the stored name, else the
  // first Ed25519 profile on offer, else the first.
  const auto offered = OfferedKeyGenProfiles(Channel(), settings.profiles);
  const auto picked =
      PickDefaultKeyGenProfile(offered, settings.default_profile);
  const auto default_name =
      picked >= 0 ? offered.at(picked).profile.name : QString();

  QVariantList list;
  for (int i = 0; i < settings.profiles.size(); ++i) {
    const auto& p = settings.profiles.at(i);
    const auto resolved = Resolve(p);
    list.append(QVariantMap{
        {QStringLiteral("index"), i},
        {QStringLiteral("name"), KeyGenProfileDisplayName(p)},
        {QStringLiteral("storedName"), p.name},
        {QStringLiteral("description"), KeyGenProfileDisplayDescription(p)},
        {QStringLiteral("algorithms"),
         resolved ? DescribeKeyGenProfileAlgos(*resolved) : QString()},
        {QStringLiteral("available"), resolved.has_value()},
        {QStringLiteral("hidden"), p.hidden},
        {QStringLiteral("isDefault"),
         !p.hidden && resolved &&
             p.name.compare(default_name, Qt::CaseInsensitive) == 0},
        {QStringLiteral("primary"), AlgoRef(p.key_algo, p.key_algo_type)},
        {QStringLiteral("primarySub"),
         AlgoRef(p.key_sub_algo, p.key_sub_algo_type)},
        {QStringLiteral("validity"), p.key_validity},
        {QStringLiteral("hasSubkey"), p.has_s_key},
        {QStringLiteral("subkey"), AlgoRef(p.s_key_algo, p.s_key_algo_type)},
        {QStringLiteral("subkeySub"),
         AlgoRef(p.s_key_sub_algo, p.s_key_sub_algo_type)},
        {QStringLiteral("subkeyValidity"), p.s_key_validity},
    });
  }
  return list;
}

auto KeyGenProfilesController::PrimaryAlgos() const -> QVariantList {
  return DescribeAll(KeyGenerateInfo::GetSupportedKeyAlgo(Channel()));
}

auto KeyGenProfilesController::SubkeyAlgos() const -> QVariantList {
  return DescribeAll(KeyGenerateInfo::GetSupportedSubkeyAlgo(Channel()));
}

auto KeyGenProfilesController::subkeyAlgosFor(const QString& fingerprint) const
    -> QVariantList {
  const auto channel = Channel();
  const auto key = qSharedPointerDynamicCast<GpgKey>(
      GpgKeyRepository::GetInstance(channel).GetKeyPtr(fingerprint.toUpper()));
  if (key == nullptr) return {};
  return DescribeAll(KeyGenerateInfo::GetSupportedSubkeyAlgo(channel, *key));
}

auto KeyGenProfilesController::pairings(const QVariantMap& algo) const
    -> QVariantList {
  const auto channel = Channel();
  auto all = KeyGenerateInfo::GetSupportedKeyAlgo(channel);
  all += KeyGenerateInfo::GetSupportedSubkeyAlgo(channel);
  const auto found = Find(algo, all);
  if (!found) return {};
  return DescribeAll(found->SubAlgos(channel));
}

auto KeyGenProfilesController::usage(const QVariantMap& algo, bool subkey) const
    -> QVariantMap {
  const auto channel = Channel();
  const auto found =
      Find(algo, subkey ? KeyGenerateInfo::GetSupportedSubkeyAlgo(channel)
                        : KeyGenerateInfo::GetSupportedKeyAlgo(channel));
  if (!found) return {};
  KeyGenerateInfo info(subkey);
  info.SetAlgo(*found);
  return {
      {QStringLiteral("encrypt"), info.IsAllowEncr()},
      {QStringLiteral("sign"), info.IsAllowSign()},
      {QStringLiteral("authenticate"), info.IsAllowAuth()},
      {QStringLiteral("changeEncrypt"), info.IsAllowModifyEncr()},
      {QStringLiteral("changeSign"), info.IsAllowModifySign()},
      {QStringLiteral("changeAuthenticate"), info.IsAllowModifyAuth()},
  };
}

auto KeyGenProfilesController::suggestSubkey(const QVariantMap& primary,
                                             const QString& purpose) const
    -> QVariantMap {
  const auto algo = SuggestSubkeyAlgo(
      ParseSubkeyPurpose(purpose).value_or(SubkeyPurpose::kEncrypt),
      primary.value(QStringLiteral("id")).toString(),
      KeyGenerateInfo::GetSupportedSubkeyAlgo(Channel()));
  if (algo == KeyGenerateInfo::kNoneAlgo) return {};
  return AlgoRef(algo.Id(), algo.Type());
}

auto KeyGenProfilesController::anonymousUid() const -> QVariantMap {
  // The default pattern where no profile is open (and in tests).
  const auto pattern = ProfileSession::Loaded()
                           ? LoadKeyGenSettings().uid_pattern
                           : UidPattern{};
  const auto uid = ExpandUidPattern(pattern, SystemUidPatternRandom(),
                                    QDateTime::currentDateTime());
  return {{QStringLiteral("name"), uid.name},
          {QStringLiteral("email"), uid.email},
          {QStringLiteral("comment"), uid.comment}};
}

auto KeyGenProfilesController::save(int index, const QVariantMap& profile)
    -> QString {
  // Read afresh: another page may have changed them since.
  auto settings = LoadKeyGenSettings();
  const auto name = profile.value(QStringLiteral("storedName")).toString();
  const auto problem =
      CheckKeyGenProfileName(name.trimmed(), settings.profiles, index);
  if (problem != KeyGenProfileNameProblem::kNone) {
    return DescribeKeyGenProfileNameProblem(problem);
  }
  const auto description =
      profile.value(QStringLiteral("description")).toString().trimmed();
  if (description.size() > kKeyGenProfileDescriptionMaxLen) {
    return tr("The description is too long.");
  }

  const auto ref = [&profile](const char* key) {
    return profile.value(QString::fromLatin1(key)).toMap();
  };
  KeyGenProfile p;
  p.name = name.trimmed();
  p.description = description;
  p.key_algo = ref("primary").value(QStringLiteral("id")).toString();
  p.key_algo_type = ref("primary").value(QStringLiteral("type")).toString();
  p.key_sub_algo = ref("primarySub").value(QStringLiteral("id")).toString();
  p.key_sub_algo_type =
      ref("primarySub").value(QStringLiteral("type")).toString();
  p.key_validity = profile.value(QStringLiteral("validity")).toString();
  p.has_s_key = profile.value(QStringLiteral("hasSubkey")).toBool();
  if (p.has_s_key) {
    p.s_key_algo = ref("subkey").value(QStringLiteral("id")).toString();
    p.s_key_algo_type = ref("subkey").value(QStringLiteral("type")).toString();
    p.s_key_sub_algo = ref("subkeySub").value(QStringLiteral("id")).toString();
    p.s_key_sub_algo_type =
        ref("subkeySub").value(QStringLiteral("type")).toString();
    p.s_key_validity =
        profile.value(QStringLiteral("subkeyValidity")).toString();
  }
  p.hidden = profile.value(QStringLiteral("hidden")).toBool();

  // What it makes has to be makeable here, as the desktop's editor insists.
  if (!Resolve(p)) {
    return tr("Choose the algorithms in full: this one cannot be made here.");
  }

  if (index >= 0 && index < settings.profiles.size()) {
    // The one Quick Generate uses follows a renamed profile.
    const auto old = settings.profiles.at(index).name;
    if (old.compare(settings.default_profile, Qt::CaseInsensitive) == 0) {
      settings.default_profile = p.name;
    }
    settings.profiles[index] = p;
  } else {
    settings.profiles.append(p);
  }
  store(settings);
  return {};
}

void KeyGenProfilesController::remove(int index) {
  // Read afresh: another page may have changed them since.
  auto settings = LoadKeyGenSettings();
  if (index < 0 || index >= settings.profiles.size()) return;
  const auto removed = settings.profiles.takeAt(index);
  if (removed.name.compare(settings.default_profile, Qt::CaseInsensitive) ==
      0) {
    settings.default_profile.clear();
  }
  store(settings);
}

auto KeyGenProfilesController::ExpertMode() const -> bool {
  // A setting of the profile's: before one is open, the plain mode.
  return ProfileSession::Loaded() && LoadKeyGenSettings().expert_mode;
}

void KeyGenProfilesController::SetExpertMode(bool on) {
  if (!ProfileSession::Loaded() || on == ExpertMode()) return;
  StoreKeyGenExpertMode(on);
  emit SignalExpertModeChanged();
}

void KeyGenProfilesController::move(int from, int to) {
  // Read afresh: another page may have changed them since.
  auto settings = LoadKeyGenSettings();
  auto& profiles = settings.profiles;
  if (from < 0 || from >= profiles.size() || to < 0 || to >= profiles.size() ||
      from == to) {
    return;
  }
  profiles.move(from, to);
  store(settings);
}

void KeyGenProfilesController::setDefault(int index) {
  // Read afresh: another page may have changed them since.
  auto settings = LoadKeyGenSettings();
  if (index < 0 || index >= settings.profiles.size()) return;
  settings.default_profile = settings.profiles.at(index).name;
  store(settings);
}

void KeyGenProfilesController::restoreDefaults() {
  // Read afresh: another page may have changed them since.
  auto settings = LoadKeyGenSettings();
  settings.profiles = RestoreDefaultKeyGenProfiles(settings.profiles);
  store(settings);
}

void KeyGenProfilesController::reload() { emit SignalProfilesChanged(); }

void KeyGenProfilesController::store(const KeyGenSO& settings) {
  StoreKeyGenSettings(settings);
  emit SignalProfilesChanged();
}

auto KeyGenProfilesController::InfoFromChoices(const QVariantMap& choices,
                                               bool subkey,
                                               const QList<KeyAlgo>& algos,
                                               int channel)
    -> QSharedPointer<KeyGenerateInfo> {
  const auto algo = Find(choices.value(QStringLiteral("algo")).toMap(), algos);
  if (!algo) return nullptr;

  auto info = SecureCreateSharedObject<KeyGenerateInfo>(subkey);
  info->SetAlgo(*algo);
  info->SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
  const auto pairs = algo->SubAlgos(channel);
  if (!pairs.isEmpty()) {
    // A hybrid is made of two halves: without its second, it is not one.
    const auto pair =
        Find(choices.value(QStringLiteral("pair")).toMap(), pairs);
    if (!pair) return nullptr;
    info->SetSubAlgo(*pair);
  }

  const auto usage = choices.value(QStringLiteral("usage")).toMap();
  const auto want = [&usage](const char* key, bool fallback) {
    const auto v = usage.value(QString::fromLatin1(key));
    return v.isValid() ? v.toBool() : fallback;
  };
  if (info->IsAllowModifyEncr()) {
    info->SetAllowEncr(want("encrypt", info->IsAllowEncr()));
  }
  if (info->IsAllowModifySign()) {
    info->SetAllowSign(want("sign", info->IsAllowSign()));
  }
  if (info->IsAllowModifyAuth()) {
    info->SetAllowAuth(want("authenticate", info->IsAllowAuth()));
  }

  const auto validity = choices.value(QStringLiteral("validity")).toString();
  const auto never = IsNeverExpiringValidity(validity);
  info->SetNonExpired(never);
  if (!never) {
    info->SetExpireTime(
        ParseKeyGenValidity(validity, QDateTime::currentDateTime()));
  }
  return info;
}

}  // namespace GpgFrontend::Mobile
