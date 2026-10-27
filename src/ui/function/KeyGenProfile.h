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

#include <optional>

#include "core/model/GpgKeyGenerateInfo.h"

namespace GpgFrontend::UI {

/**
 * @brief A named recipe for a new key: the algorithms and how long they live.
 *
 * Kept by name and algorithm id rather than as a KeyAlgo, so a profile saved
 * under one engine survives being read under another that lacks some of its
 * algorithms: it is only hidden there, never rewritten.
 */
struct GF_UI_EXPORT KeyGenProfile {
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
auto GF_UI_EXPORT KeyGenProfileDisplayName(const KeyGenProfile& profile)
    -> QString;

/**
 * @brief A profile's description as shown; see KeyGenProfileDisplayName().
 */
auto GF_UI_EXPORT KeyGenProfileDisplayDescription(const KeyGenProfile& profile)
    -> QString;

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
auto GF_UI_EXPORT KeyGenProfileFromJson(const QJsonObject& obj)
    -> std::optional<KeyGenProfile>;

/**
 * @brief The stored form of a profile.
 */
auto GF_UI_EXPORT KeyGenProfileToJson(const KeyGenProfile& profile)
    -> QJsonObject;

/**
 * @brief Read a stored list, skipping entries that are not profiles.
 */
auto GF_UI_EXPORT KeyGenProfilesFromJson(const QJsonArray& array)
    -> QList<KeyGenProfile>;

/**
 * @brief The stored form of a list of profiles.
 */
auto GF_UI_EXPORT KeyGenProfilesToJson(const QList<KeyGenProfile>& profiles)
    -> QJsonArray;

/**
 * @brief The profiles a new installation starts with.
 *
 * The first is the recommended modern default (Ed25519 / X25519).
 */
auto GF_UI_EXPORT MakeDefaultKeyGenProfiles() -> QList<KeyGenProfile>;

/**
 * @brief What a profile makes, as one string: equal for two profiles exactly
 * when they generate the same keys.
 *
 * Taken from the algorithms (id, type and hybrid pairing, ignoring case), the
 * subkey and the validities; never from the name, the description or whether
 * it is hidden, so a renamed or reworded copy of a profile is still that
 * profile.
 */
auto GF_UI_EXPORT KeyGenProfileFingerprint(const KeyGenProfile& profile)
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
auto GF_UI_EXPORT RestoreDefaultKeyGenProfiles(
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
auto GF_UI_EXPORT CheckKeyGenProfileName(const QString& name,
                                         const QList<KeyGenProfile>& profiles,
                                         int ignore_index = -1)
    -> KeyGenProfileNameProblem;

/**
 * @brief The translated sentence for a name problem; empty for kNone.
 */
auto GF_UI_EXPORT
DescribeKeyGenProfileNameProblem(KeyGenProfileNameProblem problem) -> QString;

/// The validity choices offered, in order. "custom" is not among them.
auto GF_UI_EXPORT KeyGenValidityPresets() -> QStringList;

/**
 * @brief Whether a validity string means the key never expires.
 */
auto GF_UI_EXPORT IsNeverExpiringValidity(const QString& validity) -> bool;

/**
 * @brief When a key given @p validity from @p now expires.
 *
 * Understands the presets and the "<N>d" (days) and "<N>t" (seconds) forms.
 * Anything else means two years. A never-expiring validity gives a date a
 * century away, which is only a placeholder: the caller marks the key as not
 * expiring.
 */
auto GF_UI_EXPORT ParseKeyGenValidity(const QString& validity,
                                      const QDateTime& now) -> QDateTime;

/**
 * @brief The validity string that describes an expiry.
 *
 * A preset when @p expire is that preset from @p now give or take a day, so
 * saving a profile right after choosing "1 year" keeps saying "1 year".
 * Otherwise the whole days to it, as "<N>d".
 */
auto GF_UI_EXPORT KeyGenValidityFor(bool non_expired, const QDateTime& expire,
                                    const QDateTime& now) -> QString;

/**
 * @brief How a validity string is shown: "2 years", "Never", "90 days".
 */
auto GF_UI_EXPORT KeyGenValidityDisplayName(const QString& validity) -> QString;

/**
 * @brief A profile matched against what an engine can generate.
 */
struct GF_UI_EXPORT ResolvedKeyGenProfile {
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
auto GF_UI_EXPORT ResolveKeyGenProfile(const KeyGenProfile& profile,
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
auto GF_UI_EXPORT ResolveKeyGenProfiles(const QList<KeyGenProfile>& profiles,
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
auto GF_UI_EXPORT PickDefaultKeyGenProfile(
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
void GF_UI_EXPORT ApplyKeyGenProfile(const ResolvedKeyGenProfile& resolved,
                                     const QDateTime& now, KeyGenerateInfo& key,
                                     QSharedPointer<KeyGenerateInfo>& subkey);

/**
 * @brief The profile that describes keys set up like these.
 *
 * Used by "Save as profile": the algorithms and validity are taken as they
 * stand, and the name is left for the caller.
 */
auto GF_UI_EXPORT KeyGenProfileFromInfo(
    const KeyGenerateInfo& key, const QSharedPointer<KeyGenerateInfo>& subkey,
    const QDateTime& now) -> KeyGenProfile;

/**
 * @brief The key format a key must have to hold these algorithms.
 *
 * Post-quantum algorithms exist only in the v6 format.
 *
 * @return 6 when any of them is post-quantum, else 0 (no requirement)
 */
auto GF_UI_EXPORT RequiredKeyVersion(
    const KeyGenerateInfo& key, const QSharedPointer<KeyGenerateInfo>& subkey)
    -> int;

}  // namespace GpgFrontend::UI
