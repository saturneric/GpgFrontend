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

#include "core/function/KeyGenProfile.h"

#include "core/model/CacheObject.h"
#include "core/model/SettingsObject.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

namespace {

/// A type left out by an old profile is looked up from the id alone.
auto TypeOrLookup(const QString& id, const QString& type) -> QString {
  if (!type.isEmpty()) return type;
  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo(id);
  return found ? algo.Type() : QString{};
}

auto MakeProfile(const char* name, const char* description, const QString& algo,
                 const QString& type, const QString& s_algo = {},
                 const QString& s_type = {}, const QString& ss_algo = {},
                 const QString& ss_type = {}) -> KeyGenProfile {
  KeyGenProfile p;
  p.name = QString::fromUtf8(name);
  p.description = QString::fromUtf8(description);
  p.key_algo = algo;
  p.key_algo_type = type;
  p.has_s_key = !s_algo.isEmpty();
  p.s_key_algo = s_algo;
  p.s_key_algo_type = s_type;
  p.s_key_sub_algo = ss_algo;
  p.s_key_sub_algo_type = ss_type;
  return p;
}

/// The pairing a profile names for a hybrid, or kNoneAlgo when it names none.
/// Nothing when it names one the engine cannot pair.
auto ResolveSub(const KeyAlgo& algo, const QString& id, const QString& type,
                const SubAlgosOf& sub_algos_of) -> std::optional<KeyAlgo> {
  if (id.isEmpty()) return KeyGenerateInfo::kNoneAlgo;
  auto [found, sub] = GetAlgoByIdType(id, type, sub_algos_of(algo));
  if (!found) return std::nullopt;
  return sub;
}

/// A hybrid without its pairing is half an algorithm: fill in the first one.
auto DefaultSub(const KeyAlgo& algo, const KeyAlgo& chosen,
                const SubAlgosOf& sub_algos_of) -> KeyAlgo {
  if (chosen != KeyGenerateInfo::kNoneAlgo) return chosen;
  const auto subs = sub_algos_of(algo);
  return subs.isEmpty() ? KeyGenerateInfo::kNoneAlgo : subs.front();
}

void ApplyValidity(KeyGenerateInfo& info, const QString& validity,
                   const QDateTime& now) {
  // Expiry before the flag: turning the flag off keeps a date that is ahead.
  info.SetExpireTime(ParseKeyGenValidity(validity, now));
  info.SetNonExpired(IsNeverExpiringValidity(validity));
}

}  // namespace

auto KeyGenProfile::operator==(const KeyGenProfile& o) const -> bool {
  return std::tie(name, description, key_algo, key_algo_type, key_validity,
                  key_sub_algo, key_sub_algo_type, has_s_key, s_key_algo,
                  s_key_algo_type, s_key_validity, s_key_sub_algo,
                  s_key_sub_algo_type, hidden) ==
         std::tie(o.name, o.description, o.key_algo, o.key_algo_type,
                  o.key_validity, o.key_sub_algo, o.key_sub_algo_type,
                  o.has_s_key, o.s_key_algo, o.s_key_algo_type,
                  o.s_key_validity, o.s_key_sub_algo, o.s_key_sub_algo_type,
                  o.hidden);
}

auto KeyGenProfileFromJson(const QJsonObject& obj)
    -> std::optional<KeyGenProfile> {
  if (obj.isEmpty() || !obj.contains("name") || !obj.contains("primary")) {
    return std::nullopt;
  }

  const auto primary = obj.value("primary").toObject();
  const auto subkey = obj.value("subkey").toObject();

  KeyGenProfile p;
  p.name = obj.value("name").toString().trimmed();
  p.key_algo = primary.value("algo").toString().trimmed().toLower();
  if (p.name.isEmpty() || p.key_algo.isEmpty()) return std::nullopt;

  p.description = obj.value("description").toString().trimmed();
  p.key_algo_type = primary.value("type").toString().trimmed();
  p.key_validity = primary.value("validity").toString(u"2y"_s);
  p.hidden = obj.value("hidden").toBool(false);

  p.key_sub_algo = primary.value("sub_algo").toString().trimmed().toLower();
  if (!p.key_sub_algo.isEmpty()) {
    p.key_sub_algo_type = TypeOrLookup(
        p.key_sub_algo, primary.value("sub_algo_type").toString().trimmed());
  }

  p.s_key_algo = subkey.value("algo").toString().trimmed().toLower();
  p.has_s_key = !p.s_key_algo.isEmpty();
  if (p.has_s_key) {
    p.s_key_algo_type =
        TypeOrLookup(p.s_key_algo, subkey.value("type").toString().trimmed());
    p.s_key_validity = subkey.value("validity").toString(p.key_validity);

    p.s_key_sub_algo = subkey.value("sub_algo").toString().trimmed().toLower();
    if (!p.s_key_sub_algo.isEmpty()) {
      p.s_key_sub_algo_type = TypeOrLookup(
          p.s_key_sub_algo, subkey.value("sub_algo_type").toString().trimmed());
    }
  } else {
    p.s_key_validity = p.key_validity;
  }

  return p;
}

auto KeyGenProfileToJson(const KeyGenProfile& p) -> QJsonObject {
  QJsonObject primary{
      {"algo", p.key_algo},
      {"type", p.key_algo_type},
      {"validity", p.key_validity},
  };
  if (!p.key_sub_algo.isEmpty()) {
    primary["sub_algo"] = p.key_sub_algo;
    primary["sub_algo_type"] = p.key_sub_algo_type;
  }

  QJsonObject obj{
      {"name", p.name},
      {"hidden", p.hidden},
      {"primary", primary},
  };
  if (!p.description.isEmpty()) obj["description"] = p.description;

  if (p.has_s_key) {
    QJsonObject subkey{
        {"algo", p.s_key_algo},
        {"type", p.s_key_algo_type},
        {"validity", p.s_key_validity},
    };
    if (!p.s_key_sub_algo.isEmpty()) {
      subkey["sub_algo"] = p.s_key_sub_algo;
      subkey["sub_algo_type"] = p.s_key_sub_algo_type;
    }
    obj["subkey"] = subkey;
  }

  return obj;
}

auto KeyGenProfilesFromJson(const QJsonArray& array) -> QList<KeyGenProfile> {
  QList<KeyGenProfile> profiles;
  for (const auto& item : array) {
    if (auto p = KeyGenProfileFromJson(item.toObject())) {
      profiles.append(*p);
    } else {
      LOG_W() << "skipping an unreadable key generation profile:"
              << item.toObject().toVariantMap();
    }
  }
  return profiles;
}

auto KeyGenProfilesToJson(const QList<KeyGenProfile>& profiles) -> QJsonArray {
  QJsonArray array;
  for (const auto& p : profiles) array.append(KeyGenProfileToJson(p));
  return array;
}

auto MakeDefaultKeyGenProfiles() -> QList<KeyGenProfile> {
  // Stored in English and translated only when shown, so a stored name stays
  // the same whatever the language; see KeyGenProfileDisplayName().
  auto full_pqc = [](const char* name, const char* description,
                     const QString& sign, const QString& sign_pair,
                     const QString& kem, const QString& kem_pair) {
    auto p = MakeProfile(name, description, sign, u"HYBRID-SIGN"_s, kem,
                         u"HYBRID-KEM"_s, kem_pair, u"ECDH"_s);
    p.key_sub_algo = sign_pair;
    p.key_sub_algo_type = u"EdDSA"_s;
    return p;
  };

  auto deprecated = MakeProfile(
      QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                        "DSA-2048 + ELG-2048 (Deprecated)"),
      QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                        "Old algorithms kept only to read very old keys. Do "
                        "not use for new keys."),
      u"dsa2048"_s, u"DSA"_s, u"elg2048"_s, u"ELG-E"_s);
  deprecated.hidden = true;

  return {
      // The recommended modern default.
      MakeProfile(QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "Ed25519 / X25519 (Modern & Fast)"),
                  QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "Small, fast keys that every current "
                                    "OpenPGP program understands. The best "
                                    "choice for most people."),
                  u"ed25519"_s, u"EdDSA"_s, u"cv25519"_s, u"ECDH"_s),
      MakeProfile(QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "Ed448 / X448 (High Security)"),
                  QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "A larger curve with a higher security "
                                    "margin. Some older programs cannot use "
                                    "it."),
                  u"ed448"_s, u"EdDSA"_s, u"x448"_s, u"ECDH"_s),
      MakeProfile(QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "RSA-3072 (Balanced Compatibility)"),
                  QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "Works with almost every OpenPGP program, "
                                    "including old ones. Slower and larger "
                                    "than elliptic curves."),
                  u"rsa3072"_s, u"RSA"_s),
      MakeProfile(QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "RSA-4096 (High Compatibility)"),
                  QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "The widest compatibility with a larger "
                                    "safety margin. Slow to generate and use."),
                  u"rsa4096"_s, u"RSA"_s),
      MakeProfile(QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "NIST P-384 (Standard ECC)"),
                  QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "Elliptic curves from the NIST standard, "
                                    "for places that require them."),
                  u"nistp384"_s, u"ECDSA"_s, u"nistp384"_s, u"ECDH"_s),
      MakeProfile(QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "Brainpool P-256 (EU Standard ECC)"),
                  QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "Elliptic curves required by some European "
                                    "standards and authorities."),
                  u"brainpoolp256r1"_s, u"ECDSA"_s, u"brainpoolp256r1"_s,
                  u"ECDH"_s),
      // Post-quantum encryption under a classical signing primary.
      MakeProfile(
          QT_TRANSLATE_NOOP(
              "GpgFrontend::KeyGenProfile",
              "Ed25519 / ML-KEM-768 + X25519 (PQC Encryption Hybrid)"),
          QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                            "Messages to you stay safe even against future "
                            "quantum computers. Needs a recent OpenPGP "
                            "program on the sender's side."),
          u"ed25519"_s, u"EdDSA"_s, u"ky768"_s, u"HYBRID-KEM"_s, u"cv25519"_s,
          u"ECDH"_s),
      MakeProfile(
          QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                            "Ed448 / ML-KEM-1024 + X448 (PQC Encryption "
                            "Hybrid)"),
          QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                            "Quantum-resistant encryption at the highest "
                            "level. Needs a recent OpenPGP program on the "
                            "sender's side."),
          u"ed448"_s, u"EdDSA"_s, u"kyber1024"_s, u"HYBRID-KEM"_s, u"x448"_s,
          u"ECDH"_s),
      // Post-quantum for both; offered only where the engine has them.
      full_pqc(
          QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                            "ML-DSA-65 + Ed25519 / ML-KEM-768 + X25519 (Full "
                            "PQC Hybrid)"),
          QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                            "Quantum-resistant signatures and encryption. "
                            "Uses the new v6 key format, which few programs "
                            "read yet."),
          u"mldsa65"_s, u"ed25519"_s, u"ky768"_s, u"cv25519"_s),
      full_pqc(
          QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                            "ML-DSA-87 + Ed448 / ML-KEM-1024 + X448 (Full PQC "
                            "Hybrid)"),
          QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                            "Quantum-resistant at the highest level. Uses the "
                            "new v6 key format, which few programs read yet."),
          u"mldsa87"_s, u"ed448"_s, u"kyber1024"_s, u"x448"_s),
      MakeProfile(QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "RSA-2048 (Legacy Compatibility)"),
                  QT_TRANSLATE_NOOP("GpgFrontend::KeyGenProfile",
                                    "Only for systems that accept nothing "
                                    "newer. Its safety margin is shrinking."),
                  u"rsa2048"_s, u"RSA"_s),
      // Kept so it can be restored, never offered.
      deprecated,
  };
}

auto KeyGenProfileDisplayName(const KeyGenProfile& profile) -> QString {
  return QCoreApplication::translate(kKeyGenProfileTrContext,
                                     profile.name.toUtf8().constData());
}

auto KeyGenProfileDisplayDescription(const KeyGenProfile& profile) -> QString {
  if (profile.description.isEmpty()) return {};
  return QCoreApplication::translate(kKeyGenProfileTrContext,
                                     profile.description.toUtf8().constData());
}

auto KeyGenProfileFingerprint(const KeyGenProfile& p) -> QString {
  const auto part = [](const QString& id, const QString& type) {
    return id.toLower() + u'/' + type.toLower();
  };
  QStringList parts{
      part(p.key_algo, p.key_algo_type),
      part(p.key_sub_algo, p.key_sub_algo_type),
      p.key_validity,
  };
  if (p.has_s_key) {
    parts << part(p.s_key_algo, p.s_key_algo_type)
          << part(p.s_key_sub_algo, p.s_key_sub_algo_type) << p.s_key_validity;
  }
  return QString::fromLatin1(
      QCryptographicHash::hash(parts.join(u'|').toUtf8(),
                               QCryptographicHash::Sha256)
          .toHex());
}

auto RestoreDefaultKeyGenProfiles(const QList<KeyGenProfile>& current)
    -> QList<KeyGenProfile> {
  QSet<QString> present;
  for (const auto& p : current) present.insert(KeyGenProfileFingerprint(p));

  auto restored = current;
  for (auto p : MakeDefaultKeyGenProfiles()) {
    if (present.contains(KeyGenProfileFingerprint(p))) continue;

    const auto base = p.name;
    for (int n = 2; CheckKeyGenProfileName(p.name, restored) ==
                    KeyGenProfileNameProblem::kTaken;
         ++n) {
      const auto suffix = u" (%1)"_s.arg(n);
      p.name = base.left(kKeyGenProfileNameMaxLen - suffix.size()) + suffix;
    }
    restored.append(p);
  }
  return restored;
}

auto CheckKeyGenProfileName(const QString& name,
                            const QList<KeyGenProfile>& profiles,
                            int ignore_index) -> KeyGenProfileNameProblem {
  const auto trimmed = name.trimmed();
  if (trimmed.isEmpty()) return KeyGenProfileNameProblem::kEmpty;
  if (trimmed.size() > kKeyGenProfileNameMaxLen) {
    return KeyGenProfileNameProblem::kTooLong;
  }
  if (trimmed.compare(u"Custom"_s, Qt::CaseInsensitive) == 0 ||
      trimmed.compare(QObject::tr("Custom"), Qt::CaseInsensitive) == 0) {
    return KeyGenProfileNameProblem::kReserved;
  }
  for (qsizetype i = 0; i < profiles.size(); ++i) {
    if (i == ignore_index) continue;
    if (profiles.at(i).name.trimmed().compare(trimmed, Qt::CaseInsensitive) ==
        0) {
      return KeyGenProfileNameProblem::kTaken;
    }
  }
  return KeyGenProfileNameProblem::kNone;
}

auto DescribeKeyGenProfileNameProblem(KeyGenProfileNameProblem problem)
    -> QString {
  switch (problem) {
    case KeyGenProfileNameProblem::kNone:
      return {};
    case KeyGenProfileNameProblem::kEmpty:
      return QObject::tr("Give the profile a name.");
    case KeyGenProfileNameProblem::kTooLong:
      return QObject::tr("The name can be at most %1 characters long.")
          .arg(kKeyGenProfileNameMaxLen);
    case KeyGenProfileNameProblem::kReserved:
      return QObject::tr(
          "\"Custom\" names the choice you have not saved; choose another "
          "name.");
    case KeyGenProfileNameProblem::kTaken:
      return QObject::tr("Another profile already has this name.");
  }
  return {};
}

auto KeyGenValidityPresets() -> QStringList {
  return {u"3m"_s, u"6m"_s, u"1y"_s, u"2y"_s, u"5y"_s, u"10y"_s, u"forever"_s};
}

auto IsNeverExpiringValidity(const QString& validity) -> bool {
  return validity == u"forever"_s || validity == u"none"_s;
}

auto ParseKeyGenValidity(const QString& validity, const QDateTime& now)
    -> QDateTime {
  if (IsNeverExpiringValidity(validity)) return now.addYears(100);
  if (validity.size() >= 2) {
    bool ok = false;
    const auto n = validity.left(validity.size() - 1).toLongLong(&ok);
    if (ok && n > 0) {
      switch (validity.back().unicode()) {
        case 'y':
          return now.addYears(static_cast<int>(n));
        case 'm':
          return now.addMonths(static_cast<int>(n));
        case 'd':
          return now.addDays(n);
        case 't':
          return now.addSecs(n);
        default:
          break;
      }
    }
  }
  return now.addYears(2);
}

auto KeyGenValidityFor(bool non_expired, const QDateTime& expire,
                       const QDateTime& now) -> QString {
  if (non_expired) return u"forever"_s;

  constexpr qint64 kDay = 24LL * 3600;
  for (const auto& preset : KeyGenValidityPresets()) {
    if (IsNeverExpiringValidity(preset)) continue;
    if (std::abs(ParseKeyGenValidity(preset, now).secsTo(expire)) < kDay) {
      return preset;
    }
  }

  const auto days = std::max<qint64>(1, (now.secsTo(expire) + kDay / 2) / kDay);
  return u"%1d"_s.arg(days);
}

auto KeyGenValidityDisplayName(const QString& validity) -> QString {
  if (IsNeverExpiringValidity(validity)) return QObject::tr("Never");

  // Spelled out rather than "%n year(s)": a plural form only reads right
  // through a translation, and the source language has none loaded.
  bool ok = false;
  const auto n = validity.left(validity.size() - 1).toInt(&ok);
  if (!ok || n <= 0) return validity;

  switch (validity.back().unicode()) {
    case 'y':
      return n == 1 ? QObject::tr("1 year") : QObject::tr("%1 years").arg(n);
    case 'm':
      return n == 1 ? QObject::tr("1 month") : QObject::tr("%1 months").arg(n);
    case 'd':
      return n == 1 ? QObject::tr("1 day") : QObject::tr("%1 days").arg(n);
    case 't': {
      const int days = n < 24 * 3600 ? 1 : n / (24 * 3600);
      return days == 1 ? QObject::tr("1 day")
                       : QObject::tr("%1 days").arg(days);
    }
    default:
      return validity;
  }
}

auto ResolveKeyGenProfile(const KeyGenProfile& profile,
                          const QList<KeyAlgo>& primary_algos,
                          const QList<KeyAlgo>& subkey_algos,
                          const SubAlgosOf& sub_algos_of)
    -> std::optional<ResolvedKeyGenProfile> {
  ResolvedKeyGenProfile r;
  r.profile = profile;

  auto [found, primary] =
      GetAlgoByIdType(profile.key_algo, profile.key_algo_type, primary_algos);
  if (!found) return std::nullopt;
  r.primary = primary;

  const auto primary_sub = ResolveSub(primary, profile.key_sub_algo,
                                      profile.key_sub_algo_type, sub_algos_of);
  if (!primary_sub) return std::nullopt;
  r.primary_sub = DefaultSub(primary, *primary_sub, sub_algos_of);

  r.subkey = KeyGenerateInfo::kNoneAlgo;
  r.subkey_sub = KeyGenerateInfo::kNoneAlgo;
  if (profile.has_s_key) {
    auto [s_found, subkey] = GetAlgoByIdType(
        profile.s_key_algo, profile.s_key_algo_type, subkey_algos);
    if (!s_found) return std::nullopt;
    r.subkey = subkey;

    const auto subkey_sub =
        ResolveSub(subkey, profile.s_key_sub_algo, profile.s_key_sub_algo_type,
                   sub_algos_of);
    if (!subkey_sub) return std::nullopt;
    r.subkey_sub = DefaultSub(subkey, *subkey_sub, sub_algos_of);
  }

  r.post_quantum = r.primary.IsPostQuantum() || r.subkey.IsPostQuantum();
  r.ecc = !r.post_quantum && r.primary.IsEcc();
  return r;
}

auto ResolveKeyGenProfiles(const QList<KeyGenProfile>& profiles,
                           const QList<KeyAlgo>& primary_algos,
                           const QList<KeyAlgo>& subkey_algos,
                           const SubAlgosOf& sub_algos_of)
    -> QList<ResolvedKeyGenProfile> {
  QList<ResolvedKeyGenProfile> offered;
  for (const auto& p : profiles) {
    if (p.hidden || p.name.isEmpty()) continue;
    if (auto r = ResolveKeyGenProfile(p, primary_algos, subkey_algos,
                                      sub_algos_of)) {
      offered.append(*r);
    }
  }
  return offered;
}

auto PickDefaultKeyGenProfile(const QList<ResolvedKeyGenProfile>& offered,
                              const QString& default_name) -> int {
  if (offered.isEmpty()) return -1;

  const auto name = default_name.trimmed();
  if (!name.isEmpty()) {
    for (qsizetype i = 0; i < offered.size(); ++i) {
      if (offered.at(i).profile.name.compare(name, Qt::CaseInsensitive) == 0) {
        return static_cast<int>(i);
      }
    }
  }

  for (qsizetype i = 0; i < offered.size(); ++i) {
    if (offered.at(i).primary.Id() == u"ed25519"_s) return static_cast<int>(i);
  }
  return 0;
}

void ApplyKeyGenProfile(const ResolvedKeyGenProfile& resolved,
                        const QDateTime& now, KeyGenerateInfo& key,
                        QSharedPointer<KeyGenerateInfo>& subkey) {
  key.SetAlgo(resolved.primary);
  key.SetSubAlgo(resolved.primary_sub);
  ApplyValidity(key, resolved.profile.key_validity, now);

  if (!resolved.profile.has_s_key) {
    subkey = nullptr;
    return;
  }

  if (subkey == nullptr) {
    subkey = SecureCreateSharedObject<KeyGenerateInfo>(true);
  }
  subkey->SetNonPassPhrase(key.IsNoPassPhrase());
  subkey->SetAlgo(resolved.subkey);
  subkey->SetSubAlgo(resolved.subkey_sub);
  ApplyValidity(*subkey, resolved.profile.s_key_validity, now);
}

auto KeyGenProfileFromInfo(const KeyGenerateInfo& key,
                           const QSharedPointer<KeyGenerateInfo>& subkey,
                           const QDateTime& now) -> KeyGenProfile {
  const auto has_sub = [](const KeyAlgo& a) {
    return a != KeyGenerateInfo::kNoneAlgo && !a.Id().isEmpty();
  };

  KeyGenProfile p;
  p.key_algo = key.GetAlgo().Id();
  p.key_algo_type = key.GetAlgo().Type();
  p.key_validity =
      KeyGenValidityFor(key.IsNonExpired(), key.GetExpireTime(), now);
  if (has_sub(key.SubAlgo())) {
    p.key_sub_algo = key.SubAlgo().Id();
    p.key_sub_algo_type = key.SubAlgo().Type();
  }

  p.has_s_key = subkey != nullptr;
  if (p.has_s_key) {
    p.s_key_algo = subkey->GetAlgo().Id();
    p.s_key_algo_type = subkey->GetAlgo().Type();
    p.s_key_validity =
        KeyGenValidityFor(subkey->IsNonExpired(), subkey->GetExpireTime(), now);
    if (has_sub(subkey->SubAlgo())) {
      p.s_key_sub_algo = subkey->SubAlgo().Id();
      p.s_key_sub_algo_type = subkey->SubAlgo().Type();
    }
  } else {
    p.s_key_validity = p.key_validity;
  }
  return p;
}

auto RequiredKeyVersion(const KeyGenerateInfo& key,
                        const QSharedPointer<KeyGenerateInfo>& subkey) -> int {
  bool pqc = key.GetAlgo().IsPostQuantum() || key.SubAlgo().IsPostQuantum();
  if (subkey != nullptr) {
    pqc = pqc || subkey->GetAlgo().IsPostQuantum() ||
          subkey->SubAlgo().IsPostQuantum();
  }
  return pqc ? 6 : 0;
}

auto GetAlgoByIdType(const QString& id, const QString& type,
                     const QList<KeyAlgo>& algos) -> std::tuple<bool, KeyAlgo> {
  QList<KeyAlgo> same_id;
  for (const auto& algo : algos) {
    if (algo.Id() != id) continue;
    if (!type.isEmpty() && algo.Type() != type) {
      same_id.append(algo);
      continue;
    }
    return {true, algo};
  }

  if (same_id.size() == 1) return {true, same_id.front()};
  return {};
}

auto OfferedKeyGenProfiles(int channel, const QList<KeyGenProfile>& profiles)
    -> QList<ResolvedKeyGenProfile> {
  return ResolveKeyGenProfiles(
      profiles, KeyGenerateInfo::GetSupportedKeyAlgo(channel),
      KeyGenerateInfo::GetSupportedSubkeyAlgo(channel),
      [channel](const KeyAlgo& algo) { return algo.SubAlgos(channel); });
}

auto CheckKeyGenIdentity(const QString& name, const QString& email,
                         const QString& comment) -> KeyGenIdentityCheck {
  const auto delimiters =
      QObject::tr("Must not contain '(', ')', '<', '>' or control characters.");

  KeyGenIdentityCheck check;
  const auto n = name.trimmed();
  if (n.isEmpty()) {
    check.name = QObject::tr("A name is required.");
  } else if (!IsValidUserIdComponent(n)) {
    check.name = delimiters;
  }

  const auto e = email.trimmed();
  if (!e.isEmpty() && !IsEmailAddress(e)) {
    check.email = QObject::tr("This is not a valid email address.");
  }

  if (!IsValidUserIdComponent(comment.trimmed())) check.comment = delimiters;
  return check;
}

namespace {

/// The old cache's profiles, read only while the settings have none.
auto LegacyKeyGenProfiles(const QJsonObject& stored) -> QJsonArray {
  if (stored.contains("profiles")) return {};
  const CacheObject cache(QString::fromLatin1(kLegacyKeyGenProfileCache));
  return cache.isArray() ? cache.array() : QJsonArray{};
}

}  // namespace

auto LoadStoredKeyGenProfiles() -> StoredKeyGenProfiles {
  const SettingsObject so(QString::fromLatin1(kKeyGenSettingsObject));
  const auto settings = KeyGenSettingsFromStored(so, LegacyKeyGenProfiles(so));
  return {settings.profiles, settings.default_profile};
}

auto NormalizeKeyAlgoId(const QString& engine_name) -> QString {
  auto id = engine_name.toLower();
  id.remove(u' ').remove(u'-').remove(u'_');

  // A hybrid is named after both halves; the generation id is the first.
  for (const auto& hybrid : {u"mldsa65"_s, u"mldsa87"_s}) {
    if (id.startsWith(hybrid)) return hybrid;
  }
  // GnuPG says "brainpoolp256r1", rPGP "Brainpool P-256".
  if (id.startsWith(u"brainpoolp"_s) && !id.endsWith(u"r1"_s)) id += u"r1"_s;
  return id;
}

auto SuggestSubkeyAlgo(SubkeyPurpose purpose, const QString& primary,
                       const QList<KeyAlgo>& supported) -> KeyAlgo {
  const auto can = [purpose](const KeyAlgo& a) {
    switch (purpose) {
      case SubkeyPurpose::kEncrypt:
        return a.CanEncrypt();
      case SubkeyPurpose::kSign:
        return a.CanSign();
      case SubkeyPurpose::kAuthenticate:
        return a.CanAuth();
    }
    return false;
  };

  // The primary key's own family, in order of preference. A curve keeps its
  // id across ECDSA and ECDH, so the capability decides which of the two.
  QStringList family;
  const auto encrypt = purpose == SubkeyPurpose::kEncrypt;
  if (primary == u"ed25519"_s) {
    family << (encrypt ? u"cv25519"_s : u"ed25519"_s);
  } else if (primary == u"ed448"_s) {
    family << (encrypt ? u"x448"_s : u"ed448"_s);
  } else if (primary.startsWith(u"dsa"_s)) {
    family << (encrypt ? u"elg"_s + primary.mid(3) : primary);
  } else if (primary == u"mldsa65"_s ||
             primary.startsWith(u"slhdsashake128"_s)) {
    family << (encrypt ? u"ky768"_s : primary) << u"ed25519"_s << u"cv25519"_s;
  } else if (primary == u"mldsa87"_s ||
             primary.startsWith(u"slhdsashake256"_s)) {
    family << (encrypt ? u"kyber1024"_s : primary) << u"ed448"_s << u"x448"_s;
  } else {
    // RSA and the named curves: the same id does both jobs.
    family << primary;
  }

  for (const auto& id : family) {
    for (const auto& algo : supported) {
      if (algo.Id() == id && can(algo)) return algo;
    }
  }

  for (const auto& algo : supported) {
    if (algo != KeyGenerateInfo::kNoneAlgo && can(algo)) return algo;
  }
  return KeyGenerateInfo::kNoneAlgo;
}

auto ParseSubkeyPurpose(const QString& name) -> std::optional<SubkeyPurpose> {
  if (name == u"encrypt"_s) return SubkeyPurpose::kEncrypt;
  if (name == u"sign"_s) return SubkeyPurpose::kSign;
  if (name == u"authenticate"_s) return SubkeyPurpose::kAuthenticate;
  return std::nullopt;
}

void ApplySubkeyPurpose(KeyGenerateInfo& info, SubkeyPurpose purpose,
                        const KeyAlgo& algo) {
  info.SetAlgo(algo);
  info.SetSubAlgo(KeyGenerateInfo::kNoneAlgo);
  info.SetAllowEncr(purpose == SubkeyPurpose::kEncrypt);
  info.SetAllowSign(purpose == SubkeyPurpose::kSign);
  info.SetAllowAuth(purpose == SubkeyPurpose::kAuthenticate);
}

void ApplyDefaultKeyVersion(KeyGenerateInfo& key,
                            const QSharedPointer<KeyGenerateInfo>& subkey) {
  const auto version = RequiredKeyVersion(key, subkey);
  key.SetKeyVersion(version != 0 ? version : 4);
}

auto DescribeKeyAlgo(const KeyAlgo& algo, const KeyAlgo& sub) -> QString {
  const auto none = [](const KeyAlgo& a) {
    return a == KeyGenerateInfo::kNoneAlgo || a.Id().isEmpty() ||
           a.Id() == u"none"_s;
  };
  if (none(algo)) return QObject::tr("None");

  // The names the algorithms go by, rather than the engine's ids and family
  // labels ("CV25519 (ECDH)"), which say the same thing twice.
  const auto one = [](const KeyAlgo& a) -> QString {
    const auto id = a.Id();
    static const QHash<QString, QString> kNamed{
        {u"ed25519"_s, u"Ed25519"_s},     {u"ed448"_s, u"Ed448"_s},
        {u"cv25519"_s, u"X25519"_s},      {u"x448"_s, u"X448"_s},
        {u"ky768"_s, u"ML-KEM-768"_s},    {u"kyber1024"_s, u"ML-KEM-1024"_s},
        {u"mldsa65"_s, u"ML-DSA-65"_s},   {u"mldsa87"_s, u"ML-DSA-87"_s},
        {u"secp256k1"_s, u"secp256k1"_s},
    };
    if (const auto it = kNamed.constFind(id); it != kNamed.cend()) return *it;

    const auto bits = [&id](const QString& prefix) {
      return id.mid(prefix.size()).remove(u"r1"_s);
    };
    if (id.startsWith(u"rsa"_s)) return u"RSA-"_s + bits(u"rsa"_s);
    if (id.startsWith(u"dsa"_s)) return u"DSA-"_s + bits(u"dsa"_s);
    if (id.startsWith(u"elg"_s)) return u"ElGamal-"_s + bits(u"elg"_s);
    if (id.startsWith(u"nistp"_s)) return u"NIST P-"_s + bits(u"nistp"_s);
    if (id.startsWith(u"brainpoolp"_s)) {
      return u"Brainpool P-"_s + bits(u"brainpoolp"_s);
    }
    return a.Name();
  };

  if (none(sub)) return one(algo);
  return u"%1 + %2"_s.arg(one(algo), one(sub));
}

auto DescribeKeyGenProfileAlgos(const ResolvedKeyGenProfile& r) -> QString {
  auto algos = DescribeKeyAlgo(r.primary, r.primary_sub);
  if (r.profile.has_s_key) {
    algos += u" \u00b7 "_s + DescribeKeyAlgo(r.subkey, r.subkey_sub);
  }
  return algos;
}

// ---- User id patterns (moved from ui/function/UidPattern.cpp, which the
// phone does not build). The strings keep their QObject context.

namespace {

constexpr int kTokenDefaultLen = 8;
constexpr int kTokenMinLen = 4;
constexpr int kTokenMaxLen = 32;
constexpr int kDomainLen = 8;

/// RFC 2606 / RFC 6761 reserved: a label under these is nobody's domain.
const std::array<const char*, 3> kReservedTlds = {"test", "example", "invalid"};

/// The values one expansion shares, drawn once.
struct Draw {
  QString token;  ///< kTokenMaxLen long; shorter tokens are its prefix
  QString domain;
  QString tld;
  QDateTime now;  ///< local time, for {date}, {time} and {timestamp}
};

auto Base36(const UidPatternRandom& random, int len) -> QString {
  static const QString kDigits = u"0123456789abcdefghijklmnopqrstuvwxyz"_s;
  QString out;
  out.reserve(len);
  for (int i = 0; i < len; ++i) {
    out.append(kDigits.at(static_cast<qsizetype>(random() % 36)));
  }
  return out;
}

/// The value of one placeholder, or nothing when it is not one.
auto Placeholder(const QString& name, const Draw& draw)
    -> std::optional<QString> {
  if (name == u"token"_s) return draw.token.left(kTokenDefaultLen);
  if (name.startsWith(u"token:"_s)) {
    bool ok = false;
    const auto len = name.mid(6).toInt(&ok);
    if (!ok || len < kTokenMinLen || len > kTokenMaxLen) return std::nullopt;
    return draw.token.left(len);
  }
  if (name == u"domain"_s) return draw.domain;
  if (name == u"tld"_s) return draw.tld;
  if (name == u"date"_s) return draw.now.toString(u"yyyyMMdd"_s);
  if (name.startsWith(u"date:"_s)) {
    // Any Qt date and time format, in the C locale so a name of a month or a
    // day reads the same whatever the language; nothing to format is no
    // placeholder.
    const auto format = name.mid(5);
    if (format.trimmed().isEmpty()) return std::nullopt;
    return QLocale::c().toString(draw.now, format);
  }
  if (name == u"time"_s) return draw.now.toString(u"HHmmss"_s);
  if (name == u"timestamp"_s) {
    return QString::number(draw.now.toSecsSinceEpoch());
  }
  if (name == u"timestamp:ms"_s) {
    return QString::number(draw.now.toMSecsSinceEpoch());
  }
  return std::nullopt;
}

/**
 * @brief Fill one part in.
 *
 * @param problem set to what is wrong with its syntax, if it is not null
 */
auto Expand(const QString& text, const Draw& draw,
            UidPatternProblem* problem = nullptr) -> QString {
  QString out;
  for (qsizetype i = 0; i < text.size(); ++i) {
    const auto c = text.at(i);
    const auto next = i + 1 < text.size() ? text.at(i + 1) : QChar();

    if ((c == u'{' && next == u'{') || (c == u'}' && next == u'}')) {
      out.append(c);
      ++i;
      continue;
    }

    if (c == u'}') {
      if (problem != nullptr) *problem = UidPatternProblem::kUnbalancedBrace;
      out.append(c);
      continue;
    }

    if (c != u'{') {
      out.append(c);
      continue;
    }

    const auto close = text.indexOf(u'}', i + 1);
    if (close < 0) {
      if (problem != nullptr) *problem = UidPatternProblem::kUnbalancedBrace;
      out.append(text.mid(i));
      break;
    }

    const auto name = text.mid(i + 1, close - i - 1).trimmed();
    if (auto value = Placeholder(name, draw)) {
      out.append(*value);
    } else {
      if (problem != nullptr && *problem == UidPatternProblem::kNone) {
        *problem = UidPatternProblem::kUnknownPlaceholder;
      }
      out.append(text.mid(i, close - i + 1));
    }
    i = close;
  }
  return out;
}

auto MakeDraw(const UidPatternRandom& random, const QDateTime& now) -> Draw {
  Draw draw;
  draw.token = Base36(random, kTokenMaxLen);
  draw.domain = Base36(random, kDomainLen);
  draw.tld = QString::fromLatin1(kReservedTlds.at(
      static_cast<std::size_t>(random() % kReservedTlds.size())));
  draw.now = now.toLocalTime();
  return draw;
}

}  // namespace

auto SystemUidPatternRandom() -> UidPatternRandom {
  return []() { return QRandomGenerator::global()->generate64(); };
}

auto ExpandUidPattern(const UidPattern& pattern, const UidPatternRandom& random,
                      const QDateTime& now) -> ExpandedUid {
  const auto draw = MakeDraw(random, now);
  return {Expand(pattern.name, draw).trimmed(),
          Expand(pattern.email, draw).trimmed(),
          Expand(pattern.comment, draw).trimmed()};
}

auto CheckUidPattern(const UidPattern& pattern) -> UidPatternCheck {
  // Any draw shows a syntax problem; the result is judged on a fixed one, and
  // the placeholders only ever give letters, digits and reserved names.
  quint64 counter = 0;
  const auto draw =
      MakeDraw([&counter]() { return ++counter; },
               QDateTime(QDate(2000, 1, 1), QTime(12, 0), QTimeZone::UTC));

  UidPatternCheck check;

  const auto name = Expand(pattern.name, draw, &check.name).trimmed();
  if (check.name == UidPatternProblem::kNone) {
    if (name.isEmpty()) {
      check.name = UidPatternProblem::kNameEmpty;
    } else if (!IsValidUserIdComponent(name)) {
      check.name = UidPatternProblem::kInvalidText;
    }
  }

  const auto email = Expand(pattern.email, draw, &check.email).trimmed();
  if (check.email == UidPatternProblem::kNone && !email.isEmpty() &&
      !IsEmailAddress(email)) {
    check.email = UidPatternProblem::kInvalidEmail;
  }

  const auto comment = Expand(pattern.comment, draw, &check.comment).trimmed();
  if (check.comment == UidPatternProblem::kNone &&
      !IsValidUserIdComponent(comment)) {
    check.comment = UidPatternProblem::kInvalidText;
  }

  return check;
}

auto DescribeUidPatternProblem(UidPatternProblem problem) -> QString {
  switch (problem) {
    case UidPatternProblem::kNone:
      return {};
    case UidPatternProblem::kUnknownPlaceholder:
      return QObject::tr(
          "Unknown placeholder. Use {token}, {token:N}, {domain}, {tld}, "
          "{date}, {date:FORMAT}, {time}, {timestamp} or {timestamp:ms}.");
    case UidPatternProblem::kUnbalancedBrace:
      return QObject::tr(
          "A brace is not closed. Write {{ or }} for a literal brace.");
    case UidPatternProblem::kNameEmpty:
      return QObject::tr("The name must not be empty.");
    case UidPatternProblem::kInvalidText:
      return QObject::tr(
          "Must not contain '(', ')', '<', '>' or control characters.");
    case UidPatternProblem::kInvalidEmail:
      return QObject::tr("This does not make a valid email address.");
  }
  return {};
}

auto UidPatternFromJson(const QJsonObject& obj) -> UidPattern {
  UidPattern pattern;
  if (const auto v = obj["name"]; v.isString()) pattern.name = v.toString();
  if (const auto v = obj["email"]; v.isString()) pattern.email = v.toString();
  if (const auto v = obj["comment"]; v.isString()) {
    pattern.comment = v.toString();
  }
  return pattern;
}

auto UidPatternToJson(const UidPattern& pattern) -> QJsonObject {
  return {
      {"name", pattern.name},
      {"email", pattern.email},
      {"comment", pattern.comment},
  };
}

auto KeyGenSettingsFromStored(const QJsonObject& stored,
                              const QJsonArray& legacy) -> KeyGenSO {
  KeyGenSO settings(stored);
  if (settings.profiles_stored) return settings;

  settings.profiles = KeyGenProfilesFromJson(legacy);
  if (settings.profiles.isEmpty()) {
    settings.profiles = MakeDefaultKeyGenProfiles();
  }
  return settings;
}

auto LoadKeyGenSettings() -> KeyGenSO {
  SettingsObject so(QString::fromLatin1(kKeyGenSettingsObject));

  auto settings = KeyGenSettingsFromStored(so, LegacyKeyGenProfiles(so));
  if (!settings.profiles_stored && !so.LoadFailed()) {
    // Written once, so the old cache is read only the first time. The cache is
    // left alone: a downgraded build still finds its profiles there.
    settings.profiles_stored = true;
    so.Store(settings.ToJson());
  }
  return settings;
}

void StoreKeyGenSettings(const KeyGenSO& settings) {
  SettingsObject so(QString::fromLatin1(kKeyGenSettingsObject));
  auto json = settings.ToJson();

  // Keys a newer build added are kept rather than dropped by an older one.
  for (auto it = so.constBegin(); it != so.constEnd(); ++it) {
    if (!json.contains(it.key())) json[it.key()] = it.value();
  }
  so.Store(json);
}

void StoreKeyGenExpertMode(bool on) {
  auto settings = LoadKeyGenSettings();
  if (settings.expert_mode == on) return;
  settings.expert_mode = on;
  StoreKeyGenSettings(settings);
}

}  // namespace GpgFrontend
