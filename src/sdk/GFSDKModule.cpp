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

#include "GFSDKModule.h"

#include <core/module/ModuleManager.h>

#include "GFSDKBasic.h"
#include "private/GFSDKPrivat.h"

void GFModuleListenEvent(const char *module_id, const char *event_id) {
  return GpgFrontend::Module::ModuleManager::GetInstance().ListenEvent(
      GFUnStrDup(module_id).toLower(), GFUnStrDup(event_id).toUpper());
}

auto GFModuleRetrieveRTValueOrDefault(const char *namespace_, const char *key,
                                      const char *default_value) -> const
    char * {
  return GFStrDup(GpgFrontend::Module::RetrieveRTValueTypedOrDefault(
      GFUnStrDup(namespace_), GFUnStrDup(key), GFUnStrDup(default_value)));
}

void GFModuleUpsertRTValue(const char *namespace_, const char *key,
                           const char *vaule) {
  GpgFrontend::Module::UpsertRTValue(GFUnStrDup(namespace_).toLower(),
                                     GFUnStrDup(key).toLower(),
                                     GFUnStrDup(vaule));
}

void GFModuleUpsertRTValueBool(const char *namespace_, const char *key,
                               int value) {
  GpgFrontend::Module::UpsertRTValue(GFUnStrDup(namespace_).toLower(),
                                     GFUnStrDup(key).toLower(), value != 0);
}

auto GFModuleListRTChildKeys(const char *namespace_, const char *key,
                             char ***child_keys) -> int32_t {
  *child_keys = nullptr;
  auto keys = GpgFrontend::Module::ListRTChildKeys(
      GFUnStrDup(namespace_).toLower(), GFUnStrDup(key).toLower());

  if (keys.empty()) return 0;

  *child_keys =
      static_cast<char **>(GFAllocateMemory(sizeof(char **) * keys.size()));

  for (decltype(keys.size()) i = 0; i < keys.size(); i++) {
    (*child_keys)[i] = GFStrDup(keys[i]);
  }

  return static_cast<int32_t>(keys.size());
}

void GFModuleTriggerModuleEventCallback(GFModuleEvent *module_event,
                                        const char *module_id,
                                        GFModuleEventParam *p_argv) {
  auto argv = ConvertEventParamsToMap(p_argv);
  auto event = GpgFrontend::Module::ModuleManager::GetInstance().SearchEvent(
      GFUnStrDup(module_event->trigger_id).toLower());
  if (!event) return;

  event.value()->ExecuteCallback(GFUnStrDup(module_id), argv);
}

auto GFModuleRetrieveRTValueOrDefaultBool(const char *namespace_,
                                          const char *key, int default_value)
    -> int {
  return static_cast<const int>(
      GpgFrontend::Module::RetrieveRTValueTypedOrDefault(
          GFUnStrDup(namespace_), GFUnStrDup(key),
          static_cast<bool>(default_value)));
}
