/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "Plugin.h"

#include <core/plugin/PluginManager.h>

namespace GpgFrontend::Plugin::SDK {

SPlugin::SPlugin(SPluginIdentifier id, SPluginVersion version,
                 SPluginMetaData meta_data)
    : Plugin::Plugin(id, version, meta_data) {}

bool SPlugin::Register() { return true; }

bool SPlugin::Active() { return true; }

int SPlugin::Exec(SEventRefrernce) { return 0; }

bool SPlugin::Deactive() { return true; }

SPluginIdentifier SPlugin::GetSPluginIdentifier() const {
  return GetPluginIdentifier();
}

Thread::TaskRunner* GetPluginTaskRunner(SPlugin* plugin) {
  if (plugin == nullptr) return nullptr;

  auto opt = GpgFrontend::Plugin::PluginManager::GetInstance()->GetTaskRunner(
      plugin->GetSPluginIdentifier());
  if (!opt.has_value()) {
    return nullptr;
  }

  return opt.value().get();
}
}  // namespace GpgFrontend::Plugin::SDK
