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

#include "ModuleSettingsPolicy.h"

#include <QMap>
#include <QRegularExpression>

namespace GpgFrontend::Module {

namespace {

/// The groups two modules already had their users' data in.
auto LegacyGroups() -> const QMap<QString, QString>& {
  static const QMap<QString, QString> kGroups = {
      {"com.bktus.gpgfrontend.module.email", "email"},
      {"com.bktus.gpgfrontend.module.key_server_sync", "key_server_sync"},
  };
  return kGroups;
}

/// Host settings a module may share. Each is here for a stated reason.
auto HostAllowlist() -> const QMap<QString, HostSettingAccess>& {
  static const QMap<QString, HostSettingAccess> kAllowlist = {
      // Whether the user wants update checks at all. Asked by the setup
      // wizard and honoured by the update-check module, which also offers it
      // in its own settings page -- one switch, not two that drift apart.
      {"network/prohibit_update_check", HostSettingAccess::kREAD_WRITE},
  };
  return kAllowlist;
}

}  // namespace

auto ModuleSettingsGroup(const QString& module_id) -> QString {
  const auto legacy = LegacyGroups().value(module_id);
  if (!legacy.isEmpty()) return legacy;
  return QStringLiteral("modules/") + module_id;
}

auto HostSettingAccessFor(const QString& key) -> HostSettingAccess {
  return HostAllowlist().value(key, HostSettingAccess::kNONE);
}

auto IsValidModuleSettingKey(const QString& key) -> bool {
  static const QRegularExpression kSegment(
      QStringLiteral("^[A-Za-z0-9_.\\-]+$"));
  if (key.isEmpty() || key.size() > 256) return false;
  for (const auto& segment : key.split('/')) {
    if (segment.isEmpty() || segment == "." || segment == "..") return false;
    if (!kSegment.match(segment).hasMatch()) return false;
  }
  return true;
}

auto ResolveModuleSettingKey(const QString& module_id, ModuleSettingScope scope,
                             const QString& key, bool write) -> QString {
  if (module_id.isEmpty() || !IsValidModuleSettingKey(key)) return {};
  switch (scope) {
    case ModuleSettingScope::kMODULE:
      return ModuleSettingsGroup(module_id) + "/" + key;
    case ModuleSettingScope::kHOST: {
      const auto access = HostSettingAccessFor(key);
      if (access == HostSettingAccess::kNONE) return {};
      if (write && access != HostSettingAccess::kREAD_WRITE) return {};
      return key;
    }
  }
  return {};
}

}  // namespace GpgFrontend::Module
