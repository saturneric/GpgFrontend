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

#include "ui/function/KeyGenProfile.h"

#include "ui/function/KeyGenerateHelper.h"

namespace GpgFrontend::UI {

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

}  // namespace GpgFrontend::UI
