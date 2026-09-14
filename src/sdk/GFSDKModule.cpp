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
      GFStrView(module_id).toLower(), GFStrView(event_id).toUpper());
}

auto GFModuleRetrieveRTValueOrDefault(const char *namespace_, const char *key,
                                      const char *default_value) -> const
    char * {
  return GFStrDup(GpgFrontend::Module::RetrieveRTValueTypedOrDefault(
      GFStrView(namespace_), GFStrView(key), GFStrView(default_value)));
}

void GFModuleUpsertRTValue(const char *namespace_, const char *key,
                           const char *vaule) {
  GpgFrontend::Module::UpsertRTValue(GFStrView(namespace_).toLower(),
                                     GFStrView(key).toLower(),
                                     GFStrView(vaule));
}

void GFModuleUpsertRTValueBool(const char *namespace_, const char *key,
                               int value) {
  GpgFrontend::Module::UpsertRTValue(GFStrView(namespace_).toLower(),
                                     GFStrView(key).toLower(), value != 0);
}

auto GFModuleListRTChildKeys(const char *namespace_, const char *key,
                             char ***child_keys) -> int32_t {
  *child_keys = nullptr;
  auto keys = GpgFrontend::Module::ListRTChildKeys(
      GFStrView(namespace_).toLower(), GFStrView(key).toLower());

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
  // Every field of the event, and the event node itself, is reclaimed on
  // BOTH paths. This used to consume only trigger_id, leaking module_event
  // and module_event->id on every single module callback, plus module_id as
  // well whenever the event was not found.
  auto argv = ConvertEventParamsToMap(p_argv);

  QString trigger_id;
  if (module_event != nullptr) {
    trigger_id = GFUnStrDup(module_event->trigger_id);
    GFUnStrDup(module_event->id);
    GpgFrontend::SMAFree(static_cast<void *>(module_event));
  }

  auto caller_id = GFStrView(module_id);

  auto event = GpgFrontend::Module::ModuleManager::GetInstance().SearchEvent(
      trigger_id.toLower());
  if (!event) return;

  event.value()->ExecuteCallback(caller_id, argv);
}

auto GFModuleRetrieveRTValueOrDefaultBool(const char *namespace_,
                                          const char *key, int default_value)
    -> int {
  return static_cast<const int>(
      GpgFrontend::Module::RetrieveRTValueTypedOrDefault(
          GFStrView(namespace_), GFStrView(key),
          static_cast<bool>(default_value)));
}
