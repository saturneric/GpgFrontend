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
#include "ui/function/KeyGenProfile.h"
#include "ui/widgets/MetaListPanel.h"

namespace GpgFrontend::UI {

/**
 * @brief Where the key generation wizard is.
 */
enum class KeyGenerateStep {
  kIdentity,    ///< who the key belongs to
  kKeyType,     ///< a profile
  kProtection,  ///< how long it lives and whether a passphrase guards it
  kReview,      ///< everything, once more, before it is made
  kKeyDetails,  ///< Expert Mode's second step: every choice by hand
};

/// How many steps the wizard has, for "Step 2 of 4". Key Type and Key Details
/// are one step: Expert Mode decides which of the two is shown.
constexpr int kKeyGenerateStepCount = 4;

/// How many pages: the steps, and Key Details beside Key Type.
constexpr int kKeyGeneratePageCount = 5;

/**
 * @brief Which step a page is, from 0, for "Step N of M".
 */
auto GF_UI_EXPORT KeyGenerateStepNumber(KeyGenerateStep step) -> int;

/**
 * @brief The page after @p step, or nothing after the last.
 *
 * @param step the page being left
 * @param expert whether Expert Mode is on, which picks the second step
 */
auto GF_UI_EXPORT NextKeyGenerateStep(KeyGenerateStep step, bool expert)
    -> std::optional<KeyGenerateStep>;

/**
 * @brief What a step is called in the header above it.
 */
auto GF_UI_EXPORT KeyGenerateStepTitle(KeyGenerateStep step) -> QString;

/**
 * @brief What is wrong with each part of a user id, as sentences to show.
 *
 * Empty strings where nothing is.
 */
struct GF_UI_EXPORT KeyGenIdentityCheck {
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
auto GF_UI_EXPORT CheckKeyGenIdentity(const QString& name, const QString& email,
                                      const QString& comment)
    -> KeyGenIdentityCheck;

/**
 * @brief What the wizard knows that decides whether a step may be left.
 */
struct GF_UI_EXPORT KeyGenerateState {
  QString name;
  QString email;
  QString comment;
  bool algorithms_complete = false;  ///< every algorithm chosen in full
};

/**
 * @brief Whether the primary button may leave a step forwards.
 *
 * Every step after the identity asks again for what came before, so the final
 * button can never make a key the earlier steps would have refused.
 */
auto GF_UI_EXPORT CanAdvanceKeyGenerate(KeyGenerateStep step,
                                        const KeyGenerateState& state) -> bool;

/**
 * @brief What a tile on the Key Type step stands for.
 */
enum class KeyGenTileRole {
  kModern,            ///< Ed25519 / X25519: the usual choice
  kCompatibility,     ///< RSA: for software that accepts nothing newer
  kHybridEncryption,  ///< classical signing, post-quantum encryption
};

/**
 * @brief One tile: its role and the offered profile it chooses.
 */
struct GF_UI_EXPORT FeaturedKeyGenProfile {
  KeyGenTileRole role;
  int index;  ///< into the offered profiles
};

/**
 * @brief Which offered profiles get a tile, and as what.
 *
 * At most one per role, in role order, and only roles the engine can make:
 * - Modern: the first Ed25519 profile that is not post-quantum.
 * - Compatibility: the classical (RSA) profile with the longest key.
 * - Hybrid encryption: the first profile whose primary key is classical and
 *   whose subkey is post-quantum. A profile that signs post-quantum too is no
 *   "hybrid encryption"; it stays among the other profiles.
 *
 * The default profile from Settings decides what is selected, not which tiles
 * there are, so a tile always means what its title says.
 *
 * @param offered the profiles the engine can generate
 */
auto GF_UI_EXPORT
FeaturedKeyGenProfiles(const QList<ResolvedKeyGenProfile>& offered)
    -> QList<FeaturedKeyGenProfile>;

/**
 * @brief A tile's title: "Modern", "Compatibility", "Hybrid encryption".
 */
auto GF_UI_EXPORT KeyGenTileTitle(KeyGenTileRole role) -> QString;

/**
 * @brief What a tile's keys are for and what to expect of them.
 */
auto GF_UI_EXPORT KeyGenTileDescription(KeyGenTileRole role) -> QString;

/**
 * @brief How an algorithm is called: "Ed25519", "RSA-4096", or both halves of
 * a hybrid, "ML-KEM-768 + X25519".
 */
auto GF_UI_EXPORT DescribeKeyAlgo(const KeyAlgo& algo, const KeyAlgo& sub)
    -> QString;

/**
 * @brief A profile's algorithms in one line: "Ed25519 · X25519".
 */
auto GF_UI_EXPORT DescribeKeyGenProfileAlgos(const ResolvedKeyGenProfile& r)
    -> QString;

/**
 * @brief What a profile makes, said for a reader, and what of it the engine
 * at hand cannot make.
 */
struct GF_UI_EXPORT KeyGenProfileFacts {
  QString signing;      ///< the primary key: "Ed25519", "ML-DSA-65 + Ed25519"
  QString encryption;   ///< the subkey, the primary key if it encrypts, "None"
  QString validity;     ///< "2 years", or both lifetimes when they differ
  QString algorithms;   ///< one line for a list: "Ed25519 · X25519"
  QStringList missing;  ///< algorithms the engine cannot generate, by name
};

/**
 * @brief Describe a profile, whether or not the engine can make it.
 *
 * Names come from every algorithm the application knows, so a profile from
 * another engine still says what it is; `missing` says which of them this one
 * lacks.
 *
 * @param profile the profile
 * @param primary_algos what the engine can generate as a primary key
 * @param subkey_algos what it can generate as a subkey
 * @param sub_algos_of the pairings a hybrid algorithm allows on it
 */
auto GF_UI_EXPORT DescribeKeyGenProfileFacts(
    const KeyGenProfile& profile, const QList<KeyAlgo>& primary_algos,
    const QList<KeyAlgo>& subkey_algos, const SubAlgosOf& sub_algos_of)
    -> KeyGenProfileFacts;

/**
 * @brief What a key may be used for: "Certify, Sign".
 */
auto GF_UI_EXPORT DescribeKeyUsage(const KeyGenerateInfo& info) -> QString;

/**
 * @brief When a key expires, as a date, or "Never".
 */
auto GF_UI_EXPORT DescribeKeyExpiry(const KeyGenerateInfo& info,
                                    bool expiry_supported) -> QString;

/**
 * @brief Everything the review step shows.
 *
 * @param key the primary key, with the user id set
 * @param subkey the subkey, or null
 * @param profile_name the profile chosen, or empty for one set by hand
 * @param key_version_shown whether the engine lets the format be chosen
 * @param expiry_supported whether the engine sets expiry dates
 */
auto GF_UI_EXPORT KeyGenerateReviewRows(
    const KeyGenerateInfo& key, const QSharedPointer<KeyGenerateInfo>& subkey,
    const QString& profile_name, bool key_version_shown, bool expiry_supported)
    -> QList<MetaListRow>;

}  // namespace GpgFrontend::UI
