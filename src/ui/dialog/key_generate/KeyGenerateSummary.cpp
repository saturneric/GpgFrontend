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

#include "ui/dialog/key_generate/KeyGenerateSummary.h"

#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "ui/function/KeyGenerateHelper.h"

namespace GpgFrontend::UI {

auto KeyGenerateStepTitle(KeyGenerateStep step) -> QString {
  switch (step) {
    case KeyGenerateStep::kIdentity:
      return QObject::tr("Identity");
    case KeyGenerateStep::kKeyType:
      return QObject::tr("Key Type");
    case KeyGenerateStep::kProtection:
      return QObject::tr("Validity & Protection");
    case KeyGenerateStep::kReview:
      return QObject::tr("Review");
    case KeyGenerateStep::kKeyDetails:
      return QObject::tr("Key Details");
  }
  return {};
}

auto KeyGenerateStepNumber(KeyGenerateStep step) -> int {
  return step == KeyGenerateStep::kKeyDetails
             ? KeyGenerateStepNumber(KeyGenerateStep::kKeyType)
             : static_cast<int>(step);
}

auto NextKeyGenerateStep(KeyGenerateStep step, bool expert)
    -> std::optional<KeyGenerateStep> {
  switch (step) {
    case KeyGenerateStep::kIdentity:
      return expert ? KeyGenerateStep::kKeyDetails : KeyGenerateStep::kKeyType;
    case KeyGenerateStep::kKeyType:
    case KeyGenerateStep::kKeyDetails:
      return KeyGenerateStep::kProtection;
    case KeyGenerateStep::kProtection:
      return KeyGenerateStep::kReview;
    case KeyGenerateStep::kReview:
      return std::nullopt;
  }
  return std::nullopt;
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

auto CanAdvanceKeyGenerate(KeyGenerateStep step, const KeyGenerateState& state)
    -> bool {
  const auto identity_ok =
      CheckKeyGenIdentity(state.name, state.email, state.comment).Ok();
  switch (step) {
    case KeyGenerateStep::kIdentity:
      return identity_ok;
    case KeyGenerateStep::kKeyType:
    case KeyGenerateStep::kKeyDetails:
    case KeyGenerateStep::kProtection:
    case KeyGenerateStep::kReview:
      return identity_ok && state.algorithms_complete;
  }
  return false;
}

auto FeaturedKeyGenProfiles(const QList<ResolvedKeyGenProfile>& offered)
    -> QList<FeaturedKeyGenProfile> {
  int modern = -1;
  int compatible = -1;
  int hybrid = -1;
  for (int i = 0; i < offered.size(); ++i) {
    const auto& r = offered.at(i);
    if (modern < 0 && !r.post_quantum && r.primary.Id() == u"ed25519"_s) {
      modern = i;
    }
    if (!r.post_quantum && !r.ecc &&
        (compatible < 0 ||
         r.primary.KeyLength() > offered.at(compatible).primary.KeyLength())) {
      compatible = i;
    }
    if (hybrid < 0 && r.profile.has_s_key && !r.primary.IsPostQuantum() &&
        r.subkey.IsPostQuantum()) {
      hybrid = i;
    }
  }

  QList<FeaturedKeyGenProfile> featured;
  if (modern >= 0) featured.append({KeyGenTileRole::kModern, modern});
  if (compatible >= 0) {
    featured.append({KeyGenTileRole::kCompatibility, compatible});
  }
  if (hybrid >= 0) {
    featured.append({KeyGenTileRole::kHybridEncryption, hybrid});
  }
  return featured;
}

auto KeyGenTileTitle(KeyGenTileRole role) -> QString {
  switch (role) {
    case KeyGenTileRole::kModern:
      return QObject::tr("Modern");
    case KeyGenTileRole::kCompatibility:
      return QObject::tr("Compatibility");
    case KeyGenTileRole::kHybridEncryption:
      return QObject::tr("Hybrid encryption");
  }
  return {};
}

auto KeyGenTileDescription(KeyGenTileRole role) -> QString {
  switch (role) {
    case KeyGenTileRole::kModern:
      return QObject::tr(
          "Small, fast keys for signing and encryption, understood by all "
          "current OpenPGP software.");
    case KeyGenTileRole::kCompatibility:
      return QObject::tr(
          "For older systems and software that accept only RSA. Larger and "
          "slower to use.");
    case KeyGenTileRole::kHybridEncryption:
      // Said plainly, because the name invites the opposite reading: only
      // what is encrypted to the key is post-quantum, and only software that
      // already speaks ML-KEM can do that encrypting.
      return QObject::tr(
          "Encryption to you is also protected against future quantum "
          "computers; signatures still use Ed25519. Only recent software "
          "with ML-KEM support can encrypt to this key.");
  }
  return {};
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

auto DescribeKeyGenProfileFacts(const KeyGenProfile& profile,
                                const QList<KeyAlgo>& primary_algos,
                                const QList<KeyAlgo>& subkey_algos,
                                const SubAlgosOf& sub_algos_of)
    -> KeyGenProfileFacts {
  // Every algorithm the application knows, to name what an engine lacks.
  const auto known_primary = KeyGenerateInfo::kPrimaryKeyAlgos +
                             KeyGenerateInfo::kHybridPrimaryKeyAlgo;
  const auto known_subkey =
      KeyGenerateInfo::kSubKeyAlgos + KeyGenerateInfo::kHybridSubKeyAlgos;
  const auto known_any = known_primary + known_subkey;

  const auto known = [](const QString& id, const QString& type,
                        const QList<KeyAlgo>& in) -> std::optional<KeyAlgo> {
    if (id.isEmpty()) return std::nullopt;
    auto [found, algo] = GetAlgoByIdType(id, type, in);
    return found ? std::optional<KeyAlgo>(algo) : std::nullopt;
  };
  const auto name = [&known, &known_any](const QString& id,
                                         const QString& type) -> QString {
    const auto algo = known(id, type, known_any);
    return algo ? DescribeKeyAlgo(*algo, KeyGenerateInfo::kNoneAlgo) : id;
  };
  const auto pair = [&name](const QString& id, const QString& type,
                            const QString& sub_id, const QString& sub_type) {
    return sub_id.isEmpty()
               ? name(id, type)
               : u"%1 + %2"_s.arg(name(id, type), name(sub_id, sub_type));
  };

  KeyGenProfileFacts facts;
  facts.signing = pair(profile.key_algo, profile.key_algo_type,
                       profile.key_sub_algo, profile.key_sub_algo_type);

  if (profile.has_s_key) {
    facts.encryption =
        pair(profile.s_key_algo, profile.s_key_algo_type,
             profile.s_key_sub_algo, profile.s_key_sub_algo_type);
  } else if (const auto primary =
                 known(profile.key_algo, profile.key_algo_type, known_primary);
             primary && primary->CanEncrypt()) {
    facts.encryption = QObject::tr("%1, the primary key").arg(facts.signing);
  } else {
    facts.encryption = QObject::tr("None");
  }

  facts.validity = KeyGenValidityDisplayName(profile.key_validity);
  if (profile.has_s_key && profile.s_key_validity != profile.key_validity) {
    facts.validity =
        QObject::tr("Primary key %1, subkey %2")
            .arg(facts.validity,
                 KeyGenValidityDisplayName(profile.s_key_validity));
  }

  facts.algorithms = facts.signing;
  if (profile.has_s_key) facts.algorithms += u" \u00b7 "_s + facts.encryption;

  // What this engine lacks, each named once, the hybrid halves included.
  const auto check = [&](const QString& id, const QString& type,
                         const QString& sub_id, const QString& sub_type,
                         const QList<KeyAlgo>& offered) {
    const auto algo = known(id, type, offered);
    if (!algo) {
      facts.missing << name(id, type);
      return;
    }
    if (!sub_id.isEmpty() && !known(sub_id, sub_type, sub_algos_of(*algo))) {
      facts.missing << name(sub_id, sub_type);
    }
  };
  check(profile.key_algo, profile.key_algo_type, profile.key_sub_algo,
        profile.key_sub_algo_type, primary_algos);
  if (profile.has_s_key) {
    check(profile.s_key_algo, profile.s_key_algo_type, profile.s_key_sub_algo,
          profile.s_key_sub_algo_type, subkey_algos);
  }
  facts.missing.removeDuplicates();
  return facts;
}

auto DescribeKeyUsage(const KeyGenerateInfo& info) -> QString {
  QStringList usage;
  if (info.IsAllowCert()) usage << QObject::tr("Certify");
  if (info.IsAllowSign()) usage << QObject::tr("Sign");
  if (info.IsAllowEncr()) usage << QObject::tr("Encrypt");
  if (info.IsAllowAuth()) usage << QObject::tr("Authenticate");
  return usage.isEmpty() ? QObject::tr("None") : usage.join(u", "_s);
}

auto DescribeKeyExpiry(const KeyGenerateInfo& info, bool expiry_supported)
    -> QString {
  if (!expiry_supported || info.IsNonExpired()) return QObject::tr("Never");
  return QLocale().toString(info.GetExpireTime().date(), QLocale::LongFormat);
}

auto KeyGenerateReviewRows(const KeyGenerateInfo& key,
                           const QSharedPointer<KeyGenerateInfo>& subkey,
                           const QString& profile_name, bool key_version_shown,
                           bool expiry_supported) -> QList<MetaListRow> {
  QList<MetaListRow> rows;
  const auto value = [&rows](const QString& caption, const QString& v,
                             bool emphasis = false) {
    MetaListRow row;
    row.caption = caption;
    row.value = v;
    row.emphasis = emphasis;
    rows.append(row);
    return &rows.last();
  };
  const auto section = [&rows](const QString& caption) {
    MetaListRow row;
    row.kind = MetaRowKind::kSection;
    row.caption = caption;
    rows.append(row);
  };

  section(QObject::tr("Identity"));
  value(QObject::tr("User ID"), key.GetUserid(), true);

  section(QObject::tr("Key"));
  value(QObject::tr("Profile"),
        profile_name.isEmpty() ? QObject::tr("Custom") : profile_name);
  value(QObject::tr("Primary key"),
        DescribeKeyAlgo(key.GetAlgo(), key.SubAlgo()));
  value(QObject::tr("Primary key usage"), DescribeKeyUsage(key));
  value(QObject::tr("Primary key expires"),
        DescribeKeyExpiry(key, expiry_supported));

  if (subkey != nullptr) {
    value(QObject::tr("Subkey"),
          DescribeKeyAlgo(subkey->GetAlgo(), subkey->SubAlgo()));
    value(QObject::tr("Subkey usage"), DescribeKeyUsage(*subkey));
    value(QObject::tr("Subkey expires"),
          DescribeKeyExpiry(*subkey, expiry_supported));
  } else {
    value(QObject::tr("Subkey"), QObject::tr("None"))->dimmed = true;
  }

  if (key_version_shown) {
    value(QObject::tr("Key format"), key.GetKeyVersion() == 6
                                         ? QObject::tr("v6 (Modern)")
                                         : QObject::tr("v4 (Compatible)"));
  }

  section(QObject::tr("Protection"));
  auto* passphrase =
      value(QObject::tr("Passphrase"),
            key.IsNoPassPhrase() ? QObject::tr("None")
                                 : QObject::tr("Asked for when generating"));
  if (key.IsNoPassPhrase()) {
    passphrase->danger = true;
    passphrase->detail = QObject::tr(
        "Anyone who obtains the secret key file can use it without a "
        "passphrase.");
  }

  return rows;
}

}  // namespace GpgFrontend::UI
