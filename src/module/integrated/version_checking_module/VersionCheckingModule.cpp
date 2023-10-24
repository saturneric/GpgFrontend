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

#include "VersionCheckingModule.h"

#include <qobject.h>

#include "Log.h"
#include "SoftwareVersion.h"
#include "VersionCheckTask.h"
#include "core/module/Module.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend::Module::Integrated::VersionCheckingModule {

VersionCheckingModule::VersionCheckingModule()
    : Module("com.bktus.gpgfrontend.module.integrated.versionchecking", "1.0.0",
             ModuleMetaData{{"description", "try to check gpgfrontend version"},
                            {"author", "saturneric"}}) {
  connect(this, &VersionCheckingModule::SignalVersionCheckDone, this,
          &VersionCheckingModule::SlotVersionCheckDone);
}

VersionCheckingModule::~VersionCheckingModule() = default;

bool VersionCheckingModule::Register() {
  MODULE_LOG_INFO("version checking module registering");
  listenEvent("APPLICATION_LOADED");
  listenEvent("CHECK_APPLICATION_VERSION");
  return true;
}

bool VersionCheckingModule::Active() {
  MODULE_LOG_INFO("version checking module activating");
  return true;
}

int VersionCheckingModule::Exec(EventRefrernce event) {
  MODULE_LOG_INFO("version checking module executing, event id: {}",
                  event->GetIdentifier());

  auto* task = new VersionCheckTask();
  connect(task, &VersionCheckTask::SignalUpgradeVersion, this,
          &VersionCheckingModule::SignalVersionCheckDone);
  getTaskRunner()->PostTask(task);
  return 0;
}

bool VersionCheckingModule::Deactive() { return true; }

void VersionCheckingModule::SlotVersionCheckDone(SoftwareVersion version) {
  MODULE_LOG_DEBUG("registering software information info to rt");
  UpsertRTValueTyped(GetModuleIdentifier(), "version.current_version",
                     version.current_version);
  UpsertRTValueTyped(GetModuleIdentifier(), "version.loading_done",
                     version.loading_done);
  UpsertRTValueTyped(GetModuleIdentifier(), "version.latest_version",
                     version.latest_version);
  UpsertRTValueTyped(GetModuleIdentifier(),
                     "version.current_version_is_drafted",
                     version.current_version_is_drafted);
  UpsertRTValueTyped(GetModuleIdentifier(),
                     "version.current_version_is_a_prerelease",
                     version.current_version_is_a_prerelease);
  UpsertRTValueTyped(GetModuleIdentifier(),
                     "version.current_version_publish_in_remote",
                     version.current_version_publish_in_remote);
  UpsertRTValueTyped(GetModuleIdentifier(),
                     "version.latest_prerelease_version_from_remote",
                     version.latest_prerelease_version_from_remote);
  UpsertRTValueTyped(GetModuleIdentifier(), "version.need_upgrade",
                     version.NeedUpgrade());
  UpsertRTValueTyped(GetModuleIdentifier(), "version.current_version_released",
                     version.CurrentVersionReleased());
  UpsertRTValueTyped(GetModuleIdentifier(),
                     "version.current_a_withdrawn_version",
                     version.VersionWithdrawn());
  MODULE_LOG_DEBUG("register software information to rt done");
}
}  // namespace GpgFrontend::Module::Integrated::VersionCheckingModule
