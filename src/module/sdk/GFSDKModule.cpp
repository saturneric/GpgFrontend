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

#include "GFSDKModule.h"

#include <core/module/ModuleManager.h>
#include <core/utils/CommonUtils.h>

#include "GFSDKBasic.h"

void GFModuleListenEvent(const char *module_id, const char *event_id) {
  return GpgFrontend::Module::ModuleManager::GetInstance().ListenEvent(
      GpgFrontend::GFUnStrDup(module_id).toLower(),
      GpgFrontend::GFUnStrDup(event_id).toUpper());
}

auto GFModuleRetrieveRTValueOrDefault(const char *namespace_, const char *key,
                                      const char *default_value) -> const
    char * {
  return GpgFrontend::GFStrDup(
      GpgFrontend::Module::RetrieveRTValueTypedOrDefault(
          GpgFrontend::GFUnStrDup(namespace_), GpgFrontend::GFUnStrDup(key),
          GpgFrontend::GFUnStrDup(default_value)));
}

void GFModuleUpsertRTValue(const char *namespace_, const char *key,
                           const char *vaule) {
  GpgFrontend::Module::UpsertRTValue(
      GpgFrontend::GFUnStrDup(namespace_).toLower(),
      GpgFrontend::GFUnStrDup(key).toLower(), GpgFrontend::GFUnStrDup(vaule));
}

void GFModuleUpsertRTValueBool(const char *namespace_, const char *key,
                               int value) {
  GpgFrontend::Module::UpsertRTValue(
      GpgFrontend::GFUnStrDup(namespace_).toLower(),
      GpgFrontend::GFUnStrDup(key).toLower(), value != 0);
}

auto GFModuleListRTChildKeys(const char *namespace_, const char *key,
                             char ***child_keys) -> int32_t {
  *child_keys = nullptr;
  auto keys = GpgFrontend::Module::ListRTChildKeys(
      GpgFrontend::GFUnStrDup(namespace_).toLower(),
      GpgFrontend::GFUnStrDup(key).toLower());

  if (keys.empty()) return 0;

  *child_keys =
      static_cast<char **>(GFAllocateMemory(sizeof(char **) * keys.size()));

  for (int i = 0; i < keys.size(); i++) {
    (*child_keys)[i] = GpgFrontend::GFStrDup(keys[i]);
  }

  return static_cast<int32_t>(keys.size());
}

void GFModuleTriggerModuleEventCallback(GFModuleEvent *module_event,
                                        const char *module_id, int argc,
                                        char **argv) {
  auto data_object = GpgFrontend::TransferParams();
  for (int i = 0; i < argc; i++) {
    data_object->AppendObject(GpgFrontend::GFUnStrDup(argv[i]));
  }

  auto event = GpgFrontend::Module::ModuleManager::GetInstance().SearchEvent(
      GpgFrontend::GFUnStrDup(module_event->trigger_id).toLower());
  if (!event) return;

  event.value()->ExecuteCallback(GpgFrontend::GFUnStrDup(module_id),
                                 data_object);
}