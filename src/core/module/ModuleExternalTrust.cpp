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

#include "core/module/ModuleExternalTrust.h"

#include <QDateTime>

#include "core/model/SettingsObject.h"
#include "core/struct/settings_object/ModuleTrustSO.h"
#include "core/utils/CommonUtils.h"

namespace GpgFrontend::Module {

namespace {

/// The canonical spelling a decision is stored and matched under.
auto KeyText(const QByteArray& build_key) -> QString {
  return QString::fromLatin1(build_key.toHex());
}

/// Load a list object, refusing to proceed if it exists and could not be read.
///
/// The distinction matters more here than anywhere else this pattern is used:
/// an unreadable trust store read as "empty" is indistinguishable from a user
/// who has trusted nothing, and the repair -- writing a fresh empty object --
/// would destroy every decision they had made. So a failed load is reported
/// and nothing is written.
template <typename T>
auto LoadList(const char* name, T& out) -> bool {
  SettingsObject so(name);
  if (so.LoadFailed()) {
    LOG_W() << "module trust store could not be read:" << name
            << "-- refusing to treat it as empty";
    return false;
  }
  out = T(so);
  return true;
}

template <typename T>
auto StoreList(const char* name, const T& value) -> bool {
  SettingsObject so(name);
  if (so.LoadFailed()) {
    LOG_W() << "module trust store could not be read:" << name
            << "-- refusing to overwrite it";
    return false;
  }
  return so.Store(value.ToJson());
}

auto NowUtc() -> QString {
  return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

}  // namespace

auto ModuleBuildKeyFingerprint(const QByteArray& build_key) -> QString {
  if (build_key.isEmpty()) return {};
  return BeautifyFingerprint(QString::fromLatin1(build_key.toHex().toUpper()));
}

auto ListTrustedModuleBuildKeys() -> QList<ModuleTrustedBuildKey> {
  ModuleTrustedBuildKeyListSO stored;
  if (!LoadList(kModuleTrustedBuildKeysObject, stored)) return {};

  QList<ModuleTrustedBuildKey> keys;
  keys.reserve(stored.keys.size());
  for (const auto& k : stored.keys) {
    keys.append({QByteArray::fromHex(k.build_key.toLatin1()), k.label,
                 k.first_trusted});
  }
  return keys;
}

auto IsModuleBuildKeyTrusted(const QByteArray& build_key) -> bool {
  // An empty key is never trusted, and never "matches the empty record".
  // Without this an unreadable or absent key would compare equal to a stored
  // blank and admit whatever carried it.
  if (build_key.size() != 32) return false;

  ModuleTrustedBuildKeyListSO stored;
  if (!LoadList(kModuleTrustedBuildKeysObject, stored)) return false;

  const auto text = KeyText(build_key);
  for (const auto& k : stored.keys) {
    if (!k.build_key.isEmpty() && k.build_key == text) return true;
  }
  return false;
}

auto TrustModuleBuildKey(const QByteArray& build_key, const QString& label)
    -> bool {
  if (build_key.size() != 32) return false;

  ModuleTrustedBuildKeyListSO stored;
  if (!LoadList(kModuleTrustedBuildKeysObject, stored)) return false;

  const auto text = KeyText(build_key);
  for (auto& k : stored.keys) {
    if (k.build_key == text) {
      // Already trusted. The label may be updated; first_trusted may not,
      // because it records when the decision was made and re-confirming is
      // not making it again.
      k.label = label;
      return StoreList(kModuleTrustedBuildKeysObject, stored);
    }
  }

  ModuleTrustedBuildKeySO fresh;
  fresh.build_key = text;
  fresh.label = label;
  fresh.first_trusted = NowUtc();
  stored.keys.append(fresh);
  return StoreList(kModuleTrustedBuildKeysObject, stored);
}

auto RevokeModuleBuildKey(const QByteArray& build_key) -> bool {
  if (build_key.size() != 32) return false;

  ModuleTrustedBuildKeyListSO stored;
  if (!LoadList(kModuleTrustedBuildKeysObject, stored)) return false;

  const auto text = KeyText(build_key);
  QContainer<ModuleTrustedBuildKeySO> kept;
  for (const auto& k : stored.keys) {
    if (k.build_key != text) kept.append(k);
  }
  stored.keys = kept;

  // The authorizations granted under it are deliberately NOT deleted. They
  // are gated on the key, so they are already inert; deleting them would mean
  // re-trusting the key later silently re-enabled a set of modules the user
  // would not be shown again.
  return StoreList(kModuleTrustedBuildKeysObject, stored);
}

auto IsExternalModuleEnabled(const QString& module_id,
                             const QByteArray& build_key) -> bool {
  if (module_id.isEmpty() || build_key.size() != 32) return false;

  ModuleAuthorizationListSO stored;
  if (!LoadList(kModuleAuthorizationsObject, stored)) return false;

  const auto text = KeyText(build_key);
  for (const auto& a : stored.authorizations) {
    // Both, and the key half is what stops an approval being inherited by a
    // module re-signed under a different build key.
    if (a.module_id == module_id && a.build_key == text) return a.enabled;
  }
  return false;
}

auto SetExternalModuleEnabled(const QString& module_id,
                              const QByteArray& build_key, bool enabled)
    -> bool {
  if (module_id.isEmpty() || build_key.size() != 32) return false;

  ModuleAuthorizationListSO stored;
  if (!LoadList(kModuleAuthorizationsObject, stored)) return false;

  const auto text = KeyText(build_key);
  for (auto& a : stored.authorizations) {
    if (a.module_id == module_id && a.build_key == text) {
      a.enabled = enabled;
      a.approved_at = enabled ? NowUtc() : QString();
      return StoreList(kModuleAuthorizationsObject, stored);
    }
  }

  ModuleAuthorizationSO fresh;
  fresh.module_id = module_id;
  fresh.build_key = text;
  fresh.enabled = enabled;
  fresh.approved_at = enabled ? NowUtc() : QString();
  stored.authorizations.append(fresh);
  return StoreList(kModuleAuthorizationsObject, stored);
}

auto ExternalModuleAuthorization(const QString& module_id,
                                 const QByteArray& build_key)
    -> ModuleAuthorizationState {
  if (!IsModuleBuildKeyTrusted(build_key)) {
    return ModuleAuthorizationState::kKEY_UNTRUSTED;
  }
  if (!IsExternalModuleEnabled(module_id, build_key)) {
    return ModuleAuthorizationState::kNOT_ENABLED;
  }
  return ModuleAuthorizationState::kTRUSTED_AND_ENABLED;
}

auto ModuleAuthorizationStateToString(ModuleAuthorizationState state)
    -> const char* {
  switch (state) {
    case ModuleAuthorizationState::kTRUSTED_AND_ENABLED:
      return "trusted and enabled";
    case ModuleAuthorizationState::kKEY_UNTRUSTED:
      return "its build key has not been trusted";
    case ModuleAuthorizationState::kNOT_ENABLED:
      return "it has not been enabled";
  }
  return "unknown";
}

}  // namespace GpgFrontend::Module
