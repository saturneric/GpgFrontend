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

#include "VersionCheckingPlugin.h"

#include "Plugin.h"
#include "Task.h"
#include "VersionCheckTask.h"

namespace GpgFrontend::Plugin::IntegradedPlugin::VersionCheckingPlugin {

VersionCheckingPlugin::VersionCheckingPlugin()
    : SPlugin("com.bktus.gpgfrontend.plugin.integrated.versionchecking",
              "1.0.0",
              SDK::SPluginMetaData{
                  {"description", "try to check gpgfrontend version"},
                  {"author", "saturneric"}}) {}

bool VersionCheckingPlugin::Register() {
  SPDLOG_INFO("version checking plugin registering");
  return true;
}

bool VersionCheckingPlugin::Active() {
  SPDLOG_INFO("version checking plugin activating");

  listenEvent("APPLICATION_STARTED");
  return true;
}

int VersionCheckingPlugin::Exec(SDK::SEventRefrernce event) {
  SPDLOG_INFO("version checking plugin ececuting");
  SDK::PostTask(SDK::GetPluginTaskRunner(this), new VersionCheckTask());
  return 0;
}

bool VersionCheckingPlugin::Deactive() { return true; }
}  // namespace GpgFrontend::Plugin::IntegradedPlugin::VersionCheckingPlugin
