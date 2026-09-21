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

#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

/// The data object the user's trusted module build keys are stored under.
inline constexpr auto kModuleTrustedBuildKeysObject = "module_trusted_keys";

/// The data object per-module authorizations are stored under.
inline constexpr auto kModuleAuthorizationsObject = "module_authorizations";

/**
 * @brief A module build key the user has decided to trust.
 *
 * ## One source of truth: the key
 *
 * The public key is stored and the fingerprint is DERIVED for display. Two
 * stored spellings of one fact can disagree, and the one consulted in a
 * comparison would not necessarily be the one shown to the person -- which is
 * the only way a trust decision can be wrong without anything looking wrong.
 *
 * ## It is a BUILD key, not a publisher identity
 *
 * Module signing keys are ephemeral and per build tree, exactly as this
 * Host's own is. This is not a durable identity for whoever wrote the module,
 * and trusting one says nothing about the next build from the same hands: a
 * module signed under a different build key matches no record here and
 * returns to Pending, needing a fresh decision. That is deliberate, not a
 * rough edge -- there is no publisher PKI, no rotation and no succession.
 */
struct ModuleTrustedBuildKeySO {
  /// The 32 raw Ed25519 bytes, as 64 lower-case hex characters. Canonical.
  QString build_key;

  /// Whatever the user called it. Never used for matching.
  QString label;

  /// ISO-8601 UTC, for the Controller to show. Never a policy input.
  QString first_trusted;

  ModuleTrustedBuildKeySO() = default;

  explicit ModuleTrustedBuildKeySO(const QJsonObject& j) {
    if (const auto v = j["build_key"]; v.isString()) build_key = v.toString();
    if (const auto v = j["label"]; v.isString()) label = v.toString();
    if (const auto v = j["first_trusted"]; v.isString()) {
      first_trusted = v.toString();
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    j["build_key"] = build_key;
    j["label"] = label;
    j["first_trusted"] = first_trusted;
    return j;
  }
};

/**
 * @brief The user's decision to enable one external module.
 *
 * Separate from trusting a key, and neither implies the other. Trusting a
 * build key does not enable every module signed by it; enabling a module does
 * not bypass key trust. Both are required, and both are revocable.
 *
 * The key this was granted under is recorded, so an approval cannot be
 * inherited by a module re-signed under a different one.
 */
struct ModuleAuthorizationSO {
  QString module_id;

  /// The build key this authorization was granted under, same canonical
  /// spelling as ModuleTrustedBuildKeySO::build_key.
  QString build_key;

  bool enabled = false;

  QString approved_at;

  ModuleAuthorizationSO() = default;

  explicit ModuleAuthorizationSO(const QJsonObject& j) {
    if (const auto v = j["module_id"]; v.isString()) module_id = v.toString();
    if (const auto v = j["build_key"]; v.isString()) build_key = v.toString();
    if (const auto v = j["enabled"]; v.isBool()) enabled = v.toBool();
    if (const auto v = j["approved_at"]; v.isString()) {
      approved_at = v.toString();
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    j["module_id"] = module_id;
    j["build_key"] = build_key;
    j["enabled"] = enabled;
    j["approved_at"] = approved_at;
    return j;
  }
};

/// A list object rather than one object per module.
///
/// `module.<id>.so` cannot be enumerated without already knowing every id,
/// which a trust store has to be able to do: listing what the user has
/// decided, and revoking it, are the whole point.
struct ModuleTrustedBuildKeyListSO {
  QContainer<ModuleTrustedBuildKeySO> keys;

  ModuleTrustedBuildKeyListSO() = default;

  explicit ModuleTrustedBuildKeyListSO(const QJsonObject& j) {
    if (const auto v = j["keys"]; v.isArray()) {
      for (const auto& e : v.toArray()) {
        if (e.isObject()) keys.append(ModuleTrustedBuildKeySO(e.toObject()));
      }
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    QJsonArray a;
    for (const auto& k : keys) a.push_back(k.ToJson());
    j["keys"] = a;
    return j;
  }
};

struct ModuleAuthorizationListSO {
  QContainer<ModuleAuthorizationSO> authorizations;

  ModuleAuthorizationListSO() = default;

  explicit ModuleAuthorizationListSO(const QJsonObject& j) {
    if (const auto v = j["authorizations"]; v.isArray()) {
      for (const auto& e : v.toArray()) {
        if (e.isObject()) {
          authorizations.append(ModuleAuthorizationSO(e.toObject()));
        }
      }
    }
  }

  [[nodiscard]] auto ToJson() const -> QJsonObject {
    QJsonObject j;
    QJsonArray a;
    for (const auto& x : authorizations) a.push_back(x.ToJson());
    j["authorizations"] = a;
    return j;
  }
};

}  // namespace GpgFrontend
