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

#pragma once

#include <functional>
#include <optional>

#include "GFShellExport.h"
#include "core/model/GpgKeyGenerateInfo.h"

namespace GpgFrontend {

/**
 * @brief A named recipe for a new key: the algorithms and how long they live.
 *
 * Kept by name and algorithm id rather than as a KeyAlgo, so a profile saved
 * under one engine survives being read under another that lacks some of its
 * algorithms: it is only hidden there, never rewritten.
 */
struct GF_SHELL_EXPORT KeyGenProfile {
  QString name;
  QString description;  ///< what the profile is for, in a sentence or two
  QString key_algo;
  QString key_algo_type;
  QString key_validity = u"2y"_s;
  QString key_sub_algo;  ///< the second half of a hybrid primary, if any
  QString key_sub_algo_type;
  bool has_s_key = false;
  QString s_key_algo;
  QString s_key_algo_type;
  QString s_key_validity = u"2y"_s;
  QString s_key_sub_algo;  ///< the second half of a hybrid subkey, if any
  QString s_key_sub_algo_type;
  bool hidden = false;  ///< kept, but never offered

  auto operator==(const KeyGenProfile& o) const -> bool;

  auto operator!=(const KeyGenProfile& o) const -> bool {
    return !(*this == o);
  }
};

/// The translation context of the built-in profiles' names and descriptions.
inline constexpr const char* kKeyGenProfileTrContext =
    "GpgFrontend::KeyGenProfile";

/**
 * @brief A profile's name as shown.
 *
 * The built-in profiles are stored in English, like any other, so a name
 * survives a change of language; they are translated only when shown. A name
 * the user gave has no translation and is shown as it is.
 */
auto GF_SHELL_EXPORT KeyGenProfileDisplayName(const KeyGenProfile& profile)
    -> QString;

/**
 * @brief A profile's description as shown; see KeyGenProfileDisplayName().
 */
auto GF_SHELL_EXPORT
KeyGenProfileDisplayDescription(const KeyGenProfile& profile) -> QString;

/// The longest description a profile may have.
constexpr int kKeyGenProfileDescriptionMaxLen = 200;

/// The longest name a profile may have. Room for a name that says what the
/// profile is made of, as the built-in ones do ("ML-DSA-65 + Ed25519 /
/// ML-KEM-768 + X25519 (Full PQC Hybrid)"), with space to spare for a
/// translation or a copy's " (2)"; a list cell elides what it cannot show.
constexpr int kKeyGenProfileNameMaxLen = 96;

/**
 * @brief Read one profile from its stored form.
 *
 * The form is the one the old dialog cached, so profiles saved by an earlier
 * release read unchanged.
 *
 * @param obj the stored object
 * @return the profile, or nothing when the object is not one
 */
auto GF_SHELL_EXPORT KeyGenProfileFromJson(const QJsonObject& obj)
    -> std::optional<KeyGenProfile>;

/**
 * @brief The stored form of a profile.
 */
auto GF_SHELL_EXPORT KeyGenProfileToJson(const KeyGenProfile& profile)
    -> QJsonObject;

/**
 * @brief Read a stored list, skipping entries that are not profiles.
 */
auto GF_SHELL_EXPORT KeyGenProfilesFromJson(const QJsonArray& array)
    -> QList<KeyGenProfile>;

/**
 * @brief The stored form of a list of profiles.
 */
auto GF_SHELL_EXPORT KeyGenProfilesToJson(const QList<KeyGenProfile>& profiles)
    -> QJsonArray;

/**
 * @brief The profiles a new installation starts with.
 *
 * The first is the recommended modern default (Ed25519 / X25519).
 */
auto GF_SHELL_EXPORT MakeDefaultKeyGenProfiles() -> QList<KeyGenProfile>;

/**
 * @brief What a profile makes, as one string: equal for two profiles exactly
 * when they generate the same keys.
 *
 * Taken from the algorithms (id, type and hybrid pairing, ignoring case), the
 * subkey and the validities; never from the name, the description or whether
 * it is hidden, so a renamed or reworded copy of a profile is still that
 * profile.
 */
auto GF_SHELL_EXPORT KeyGenProfileFingerprint(const KeyGenProfile& profile)
    -> QString;

/**
 * @brief The profiles with the built-in ones that are missing added back.
 *
 * Nothing is removed or changed: the user's own profiles stay, and a built-in
 * profile that is still there, renamed, edited in its wording or hidden, is
 * not added a second time. A built-in whose name another profile has taken
 * meanwhile comes back as "Name (2)".
 *
 * @param current the profiles as they are
 * @return them, followed by the missing built-ins in their usual order
 */
auto GF_SHELL_EXPORT RestoreDefaultKeyGenProfiles(
    const QList<KeyGenProfile>& current) -> QList<KeyGenProfile>;

/**
 * @brief Why a name cannot be given to a profile.
 */
enum class KeyGenProfileNameProblem {
  kNone,
  kEmpty,
  kTooLong,
  kReserved,  ///< "Custom", which names the unsaved choice
  kTaken,
};

/**
 * @brief Whether a name may be given to a profile among @p profiles.
 *
 * @param name what was typed
 * @param profiles the profiles it would join
 * @param ignore_index a profile not to compare against: the one being renamed,
 * or -1
 * @return the reason it may not, or kNone
 */
auto GF_SHELL_EXPORT CheckKeyGenProfileName(
    const QString& name, const QList<KeyGenProfile>& profiles,
    int ignore_index = -1) -> KeyGenProfileNameProblem;

/**
 * @brief The translated sentence for a name problem; empty for kNone.
 */
auto GF_SHELL_EXPORT
DescribeKeyGenProfileNameProblem(KeyGenProfileNameProblem problem) -> QString;

/// The validity choices offered, in order. "custom" is not among them.
auto GF_SHELL_EXPORT KeyGenValidityPresets() -> QStringList;

/**
 * @brief Whether a validity string means the key never expires.
 */
auto GF_SHELL_EXPORT IsNeverExpiringValidity(const QString& validity) -> bool;

/**
 * @brief When a key given @p validity from @p now expires.
 *
 * Understands the presets and the "<N>d" (days) and "<N>t" (seconds) forms.
 * Anything else means two years. A never-expiring validity gives a date a
 * century away, which is only a placeholder: the caller marks the key as not
 * expiring.
 */
auto GF_SHELL_EXPORT ParseKeyGenValidity(const QString& validity,
                                         const QDateTime& now) -> QDateTime;

/**
 * @brief The validity string that describes an expiry.
 *
 * A preset when @p expire is that preset from @p now give or take a day, so
 * saving a profile right after choosing "1 year" keeps saying "1 year".
 * Otherwise the whole days to it, as "<N>d".
 */
auto GF_SHELL_EXPORT KeyGenValidityFor(bool non_expired,
                                       const QDateTime& expire,
                                       const QDateTime& now) -> QString;

/**
 * @brief How a validity string is shown: "2 years", "Never", "90 days".
 */
auto GF_SHELL_EXPORT KeyGenValidityDisplayName(const QString& validity)
    -> QString;

/**
 * @brief A profile matched against what an engine can generate.
 */
struct GF_SHELL_EXPORT ResolvedKeyGenProfile {
  KeyGenProfile profile;
  KeyAlgo primary;
  KeyAlgo primary_sub;  ///< kNoneAlgo unless the primary is a hybrid
  KeyAlgo subkey;       ///< kNoneAlgo when there is no subkey
  KeyAlgo subkey_sub;   ///< kNoneAlgo unless the subkey is a hybrid
  bool post_quantum = false;
  bool ecc = false;
};

/// The second algorithms a hybrid algorithm may be paired with here.
using SubAlgosOf = std::function<QList<KeyAlgo>(const KeyAlgo&)>;

/**
 * @brief Match a profile against the algorithms an engine offers.
 *
 * @param profile the profile
 * @param primary_algos what the engine can generate as a primary key
 * @param subkey_algos what it can generate as a subkey
 * @param sub_algos_of the pairings a hybrid algorithm allows
 * @return the match, or nothing when any part of the profile is unavailable
 */
auto GF_SHELL_EXPORT ResolveKeyGenProfile(const KeyGenProfile& profile,
                                          const QList<KeyAlgo>& primary_algos,
                                          const QList<KeyAlgo>& subkey_algos,
                                          const SubAlgosOf& sub_algos_of)
    -> std::optional<ResolvedKeyGenProfile>;

/**
 * @brief The profiles that can be offered: visible and fully available.
 *
 * In stored order. Hidden profiles and ones the engine cannot generate are
 * left out.
 */
auto GF_SHELL_EXPORT ResolveKeyGenProfiles(const QList<KeyGenProfile>& profiles,
                                           const QList<KeyAlgo>& primary_algos,
                                           const QList<KeyAlgo>& subkey_algos,
                                           const SubAlgosOf& sub_algos_of)
    -> QList<ResolvedKeyGenProfile>;

/**
 * @brief Which offered profile is chosen when nothing else is said.
 *
 * The one named @p default_name (ignoring case), else the first Ed25519 one,
 * else the first.
 *
 * @return its index in @p offered, or -1 when there are none
 */
auto GF_SHELL_EXPORT PickDefaultKeyGenProfile(
    const QList<ResolvedKeyGenProfile>& offered, const QString& default_name)
    -> int;

/**
 * @brief Set up a key, and its subkey, the way a profile says.
 *
 * @param resolved the profile, resolved for the engine at hand
 * @param now when validity is counted from
 * @param key the primary key's parameters, updated
 * @param subkey replaced by a new subkey when the profile has one, else null.
 * A new subkey shares the primary key's "no passphrase" choice.
 */
void GF_SHELL_EXPORT ApplyKeyGenProfile(
    const ResolvedKeyGenProfile& resolved, const QDateTime& now,
    KeyGenerateInfo& key, QSharedPointer<KeyGenerateInfo>& subkey);

/**
 * @brief The profile that describes keys set up like these.
 *
 * Used by "Save as profile": the algorithms and validity are taken as they
 * stand, and the name is left for the caller.
 */
auto GF_SHELL_EXPORT KeyGenProfileFromInfo(
    const KeyGenerateInfo& key, const QSharedPointer<KeyGenerateInfo>& subkey,
    const QDateTime& now) -> KeyGenProfile;

/**
 * @brief The key format a key must have to hold these algorithms.
 *
 * Post-quantum algorithms exist only in the v6 format.
 *
 * @return 6 when any of them is post-quantum, else 0 (no requirement)
 */
auto GF_SHELL_EXPORT RequiredKeyVersion(
    const KeyGenerateInfo& key, const QSharedPointer<KeyGenerateInfo>& subkey)
    -> int;

/**
 * @brief The algorithm with this id, and this type when one is given.
 *
 * An algorithm whose id is unique is found whatever the type says, so a
 * profile stored before types were recorded still resolves.
 */
auto GF_SHELL_EXPORT GetAlgoByIdType(const QString& id, const QString& type,
                                     const QList<KeyAlgo>& algos)
    -> std::tuple<bool, KeyAlgo>;

/**
 * @brief The profiles a channel's engine can generate, in stored order.
 */
auto GF_SHELL_EXPORT OfferedKeyGenProfiles(int channel,
                                           const QList<KeyGenProfile>& profiles)
    -> QList<ResolvedKeyGenProfile>;

/**
 * @brief What is wrong with each part of a user id, as sentences to show.
 *
 * Empty strings where nothing is.
 */
struct GF_SHELL_EXPORT KeyGenIdentityCheck {
  QString name;
  QString email;
  QString comment;

  [[nodiscard]] auto Ok() const -> bool {
    return name.isEmpty() && email.isEmpty() && comment.isEmpty();
  }
};

/**
 * @brief Whether a user id can be given to a new key.
 *
 * A name is required; an email is optional but must be an address when
 * given. The name and comment become part of a mail address, so its delimiters
 * are not allowed in them.
 */
auto GF_SHELL_EXPORT CheckKeyGenIdentity(const QString& name,
                                         const QString& email,
                                         const QString& comment)
    -> KeyGenIdentityCheck;

/// The settings object the key generation settings are stored under.
inline constexpr const char* kKeyGenSettingsObject = "key_gen_settings";

/// The profiles a profile stores, and the one Quick Generate uses.
struct StoredKeyGenProfiles {
  QList<KeyGenProfile> profiles;
  QString default_profile;  ///< a profile's stored name; empty: the built-in
};

/**
 * @brief The key generation profiles stored in kKeyGenSettingsObject, or the
 * defaults when it holds none.
 *
 * Read only, profiles never written coming from the old cache as
 * LoadKeyGenSettings() finds them: an interface that just generates keys uses
 * what is there, and leaves the migration to the settings that write back.
 */
auto GF_SHELL_EXPORT LoadStoredKeyGenProfiles() -> StoredKeyGenProfiles;

/**
 * @brief What a new subkey is for.
 */
enum class SubkeyPurpose {
  kEncrypt,
  kSign,
  kAuthenticate,
};

/**
 * @brief An existing key's algorithm, as a generation algorithm id.
 *
 * The engines name a key's algorithm in their own ways ("RSA3072" and
 * "RSA 3072", "ML-DSA-65_ED25519"); this is the id the generation tables use
 * for it ("rsa3072", "mldsa65").
 */
auto GF_SHELL_EXPORT NormalizeKeyAlgoId(const QString& engine_name) -> QString;

/**
 * @brief The algorithm a subkey for @p purpose should use under @p primary.
 *
 * The primary key's own family when it has one for the purpose: an Ed25519
 * key gets an X25519 encryption subkey and an Ed25519 signing one, an RSA key
 * an RSA subkey of the same length, a post-quantum key a post-quantum one.
 * Otherwise the first algorithm on offer that can do the job.
 *
 * @param purpose what the subkey is for
 * @param primary the primary key's algorithm id ("ed25519", "rsa3072")
 * @param supported what may be generated under this key
 * @return the algorithm, or kNoneAlgo when nothing on offer can do the job
 */
auto GF_SHELL_EXPORT SuggestSubkeyAlgo(SubkeyPurpose purpose,
                                       const QString& primary,
                                       const QList<KeyAlgo>& supported)
    -> KeyAlgo;

/**
 * @brief A purpose by its stored or scripted name: "encrypt", "sign" or
 * "authenticate"; nullopt for anything else.
 */
auto GF_SHELL_EXPORT ParseSubkeyPurpose(const QString& name)
    -> std::optional<SubkeyPurpose>;

/**
 * @brief Make @p info a subkey of @p algo for one job only: an algorithm that
 * could also sign is not made to.
 */
void GF_SHELL_EXPORT ApplySubkeyPurpose(KeyGenerateInfo& info,
                                        SubkeyPurpose purpose,
                                        const KeyAlgo& algo);

/**
 * @brief Give @p key the format its algorithms need: v6 for a post-quantum
 * one (RequiredKeyVersion()), else the interoperable v4.
 */
void GF_SHELL_EXPORT ApplyDefaultKeyVersion(
    KeyGenerateInfo& key, const QSharedPointer<KeyGenerateInfo>& subkey);

/**
 * @brief How an algorithm is called: "Ed25519", "RSA-4096", or both halves of
 * a hybrid, "ML-KEM-768 + X25519".
 */
auto GF_SHELL_EXPORT DescribeKeyAlgo(const KeyAlgo& algo, const KeyAlgo& sub)
    -> QString;

/**
 * @brief A profile's algorithms in one line: "Ed25519 · X25519".
 */
auto GF_SHELL_EXPORT DescribeKeyGenProfileAlgos(const ResolvedKeyGenProfile& r)
    -> QString;

// ---- The made-up user ids of an anonymous key (the desktop's "Fill In
// Anonymously" and Quick Generate; the phone's as well), from a pattern kept
// with the key generation settings.

/**
 * @brief How a quick user id is made up: one template per part.
 *
 * Each part is literal text with placeholders in braces:
 *
 * - `{token}`: 8 random letters and digits; `{token:N}` for N of them (4-32).
 *   Every `{token}` in one user id is the same, so the name and the email can
 *   share it; a shorter one is the start of a longer one.
 * - `{domain}`: a random 8-character domain label.
 * - `{tld}`: one of the reserved top-level domains (test, example, invalid),
 *   so a made-up address can never reach anybody.
 * - `{date}`: today, as yyyyMMdd; `{date:FORMAT}` in any Qt date and time
 *   format, such as `{date:yyyy-MM-dd}` or `{date:yyMMdd-HHmm}`.
 * - `{time}`: the time of day, as HHmmss.
 * - `{timestamp}`: seconds since 1970-01-01 UTC; `{timestamp:ms}` in
 *   milliseconds.
 * - `{{` and `}}`: a literal brace.
 *
 * Deliberately nothing that names this computer or its user: a public key is
 * published, and a user id made from the login name would say who made it.
 */
struct GF_SHELL_EXPORT UidPattern {
  QString name = u"anon-{token}"_s;
  QString email = u"anon-{token}@{domain}.{tld}"_s;
  QString comment;

  auto operator==(const UidPattern& o) const -> bool {
    return name == o.name && email == o.email && comment == o.comment;
  }
};

/**
 * @brief A user id made from a pattern.
 */
struct GF_SHELL_EXPORT ExpandedUid {
  QString name;
  QString email;
  QString comment;
};

/// Where the randomness comes from; any 64 bits will do.
using UidPatternRandom = std::function<quint64()>;

/**
 * @brief The system's random source, for real use.
 */
auto GF_SHELL_EXPORT SystemUidPatternRandom() -> UidPatternRandom;

/**
 * @brief Fill a pattern in.
 *
 * An unknown placeholder is left as it is written; CheckUidPattern() is what
 * says it is wrong.
 *
 * @param pattern the pattern
 * @param random the random source
 * @param now what {date}, {time} and {timestamp} are; read in local time
 */
auto GF_SHELL_EXPORT ExpandUidPattern(const UidPattern& pattern,
                                      const UidPatternRandom& random,
                                      const QDateTime& now) -> ExpandedUid;

/**
 * @brief What is wrong with one part of a pattern.
 */
enum class UidPatternProblem {
  kNone,
  kUnknownPlaceholder,
  kUnbalancedBrace,
  kNameEmpty,     ///< the name part gives an empty name
  kInvalidText,   ///< the result holds '(', ')', '<', '>' or control chars
  kInvalidEmail,  ///< the result is not an email address
};

/**
 * @brief What is wrong with each part of a pattern.
 */
struct GF_SHELL_EXPORT UidPatternCheck {
  UidPatternProblem name = UidPatternProblem::kNone;
  UidPatternProblem email = UidPatternProblem::kNone;
  UidPatternProblem comment = UidPatternProblem::kNone;

  [[nodiscard]] auto Ok() const -> bool {
    return name == UidPatternProblem::kNone &&
           email == UidPatternProblem::kNone &&
           comment == UidPatternProblem::kNone;
  }
};

/**
 * @brief Whether a pattern makes a usable user id, whatever the random draw.
 *
 * The email part may be empty: a user id needs no address. The comment is
 * usually empty.
 */
auto GF_SHELL_EXPORT CheckUidPattern(const UidPattern& pattern)
    -> UidPatternCheck;

/**
 * @brief The translated sentence for a problem; empty for kNone.
 */
auto GF_SHELL_EXPORT DescribeUidPatternProblem(UidPatternProblem problem)
    -> QString;

/**
 * @brief Read a stored pattern; missing parts take their defaults.
 */
auto GF_SHELL_EXPORT UidPatternFromJson(const QJsonObject& obj) -> UidPattern;

/**
 * @brief The stored form of a pattern.
 */
auto GF_SHELL_EXPORT UidPatternToJson(const UidPattern& pattern) -> QJsonObject;

/// The durable cache the profiles lived in before they moved to Settings.
inline constexpr const char* kLegacyKeyGenProfileCache =
    "key_gen_easy_mode_config";

/**
 * @brief The key generation settings (kKeyGenSettingsObject): the profiles,
 * which one Quick Generate uses, and how it makes up a user id.
 */
struct KeyGenSO {
  QList<KeyGenProfile> profiles;
  QString default_profile;  ///< a profile's stored name; empty for the built-in
  UidPattern uid_pattern;
  /// Whether the key and subkey dialogs open with every choice shown. One
  /// switch for all of them: whoever wants it in one wants it in the others.
  bool expert_mode = false;
  /// Whether `profiles` was ever written. An installation that has deleted
  /// every profile is not one that never had any.
  bool profiles_stored = false;

  explicit KeyGenSO(const QJsonObject& j) {
    if (const auto v = j["profiles"]; v.isArray()) {
      profiles = KeyGenProfilesFromJson(v.toArray());
      profiles_stored = true;
    }
    if (const auto v = j["default_profile"]; v.isString()) {
      default_profile = v.toString();
    }
    if (const auto v = j["uid_pattern"]; v.isObject()) {
      uid_pattern = UidPatternFromJson(v.toObject());
    }
    if (const auto v = j["expert_mode"]; v.isBool()) {
      expert_mode = v.toBool();
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    j["profiles"] = KeyGenProfilesToJson(profiles);
    j["default_profile"] = default_profile;
    j["uid_pattern"] = UidPatternToJson(uid_pattern);
    j["expert_mode"] = expert_mode;
    return j;
  }
};

/**
 * @brief The settings as they should be, from what is stored.
 *
 * Profiles never written to the settings come from the cache the old dialog
 * kept them in, and failing that from the defaults.
 *
 * @param stored the settings object's content
 * @param legacy the old cache's profile array; empty when there is none
 * @return the settings, with `profiles_stored` saying whether they changed
 */
auto GF_SHELL_EXPORT KeyGenSettingsFromStored(const QJsonObject& stored,
                                              const QJsonArray& legacy)
    -> KeyGenSO;

/**
 * @brief The key generation settings, migrating them on first use.
 */
auto GF_SHELL_EXPORT LoadKeyGenSettings() -> KeyGenSO;

/**
 * @brief Store the key generation settings. Keys a newer build added are
 * kept.
 */
void GF_SHELL_EXPORT StoreKeyGenSettings(const KeyGenSO& settings);

/**
 * @brief Remember whether the key dialogs open in Expert Mode.
 *
 * Only that one value: a dialog holding an older copy of the settings must not
 * write the rest of it back.
 */
void GF_SHELL_EXPORT StoreKeyGenExpertMode(bool on);

}  // namespace GpgFrontend
