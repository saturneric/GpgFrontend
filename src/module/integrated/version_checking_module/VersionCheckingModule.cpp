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

#include <GFSDKBasic.h>
#include <GFSDKBuildInfo.h>
#include <GFSDKExtra.h>
#include <GFSDKLog.h>
#include <spdlog/spdlog.h>

#include <QMetaType>
#include <QtNetwork>

#include "SoftwareVersion.h"
#include "VersionCheckTask.h"

extern void VersionCheckDone(const SoftwareVersion& version);

auto GFGetModuleGFSDKVersion() -> const char* {
  return GFModuleStrDup(GF_SDK_VERSION_STR);
}

auto GFGetModuleQtEnvVersion() -> const char* {
  return GFModuleStrDup(QT_VERSION_STR);
}

auto GFGetModuleID() -> const char* {
  return GFModuleStrDup("com.bktus.gpgfrontend.module.version_checking");
}

auto GFGetModuleVersion() -> const char* { return GFModuleStrDup("1.0.0"); }

auto GFGetModuleMetaData() -> GFModuleMetaData* {
  auto* p_meta = static_cast<GFModuleMetaData*>(
      GFAllocateMemory(sizeof(GFModuleMetaData)));
  auto* h_meta = p_meta;

  p_meta->key = "Name";
  p_meta->value = "VersionChecking";
  p_meta->next = static_cast<GFModuleMetaData*>(
      GFAllocateMemory(sizeof(GFModuleMetaData)));
  p_meta = p_meta->next;

  p_meta->key = "Description";
  p_meta->value = "Try checking gpgfrontend version";
  p_meta->next = static_cast<GFModuleMetaData*>(
      GFAllocateMemory(sizeof(GFModuleMetaData)));
  p_meta = p_meta->next;

  p_meta->key = "Author";
  p_meta->value = "Saturneric";
  p_meta->next = nullptr;

  return h_meta;
}

auto GFRegisterModule() -> int {
  GFModuleLogInfo("version checking module registering");
  return 0;
}

auto GFActiveModule() -> int {
  GFModuleLogInfo("version checking module activating");

  GFModuleListenEvent(GFGetModuleID(), GFModuleStrDup("APPLICATION_LOADED"));
  GFModuleListenEvent(GFGetModuleID(),
                      GFModuleStrDup("CHECK_APPLICATION_VERSION"));
  return 0;
}

auto GFExecuteModule(GFModuleEvent* event) -> int {
  GFModuleLogInfo(
      fmt::format("version checking module executing, event id: {}", event->id)
          .c_str());

  auto* task = new VersionCheckTask();
  QObject::connect(
      task, &VersionCheckTask::SignalUpgradeVersion, QThread::currentThread(),
      [event](const SoftwareVersion& version) {
        VersionCheckDone(version);

        char** event_argv =
            static_cast<char**>(GFAllocateMemory(sizeof(char**) * 1));
        event_argv[0] = GFModuleStrDup("0");

        GFModuleTriggerModuleEventCallback(event, GFGetModuleID(), 1,
                                           event_argv);
      });
  QObject::connect(task, &VersionCheckTask::SignalUpgradeVersion, task,
                   &QObject::deleteLater);
  task->Run();

  return 0;
}

auto GFDeactiveModule() -> int { return 0; }

auto GFUnregisterModule() -> int { return 0; }

void VersionCheckDone(const SoftwareVersion& version) {
  GFModuleLogDebug("filling software information info in rt...");

  GFModuleUpsertRTValue(GFGetModuleID(),
                        GFModuleStrDup("version.current_version"),
                        GFModuleStrDup(version.current_version.toUtf8()));
  GFModuleUpsertRTValue(GFGetModuleID(),
                        GFModuleStrDup("version.latest_version"),
                        GFModuleStrDup(version.latest_version.toUtf8()));
  GFModuleUpsertRTValueBool(
      GFGetModuleID(), GFModuleStrDup("version.current_version_is_drafted"),
      version.current_version_is_drafted ? 1 : 0);
  GFModuleUpsertRTValueBool(
      GFGetModuleID(),
      GFModuleStrDup("version.current_version_is_a_prerelease"),
      version.current_version_is_a_prerelease ? 1 : 0);
  GFModuleUpsertRTValueBool(
      GFGetModuleID(),
      GFModuleStrDup("version.current_version_publish_in_remote"),
      version.current_version_publish_in_remote ? 1 : 0);
  GFModuleUpsertRTValueBool(
      GFGetModuleID(),
      GFModuleStrDup("version.latest_prerelease_version_from_remote"),
      version.latest_prerelease_version_from_remote ? 1 : 0);
  GFModuleUpsertRTValueBool(GFGetModuleID(),
                            GFModuleStrDup("version.need_upgrade"),
                            version.NeedUpgrade() ? 1 : 0);
  GFModuleUpsertRTValueBool(GFGetModuleID(),
                            GFModuleStrDup("version.current_version_released"),
                            version.CurrentVersionReleased() ? 1 : 0);
  GFModuleUpsertRTValueBool(
      GFGetModuleID(), GFModuleStrDup("version.current_a_withdrawn_version"),
      version.VersionWithdrawn() ? 1 : 0);
  GFModuleUpsertRTValueBool(GFGetModuleID(),
                            GFModuleStrDup("version.loading_done"),
                            version.loading_done ? 1 : 0);

  GFModuleLogDebug("software information filled in rt");
}
