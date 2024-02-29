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

#include "Module.h"

#include <core/module/ModuleManager.h>
#include <core/utils/CommonUtils.h>

#include "Basic.h"

void ListenEvent(const char *module_id, const char *event_id) {
  return GpgFrontend::Module::ModuleManager::GetInstance().ListenEvent(
      module_id, event_id);
}

auto RetrieveRTValueOrDefault(const char *namespace_, const char *key,
                              const char *default_value) -> const char * {
  return GpgFrontend::GFStrDup(
      GpgFrontend::Module::RetrieveRTValueTypedOrDefault(
          QString::fromUtf8(namespace_), QString::fromUtf8(key),
          QString::fromUtf8(default_value)));
}

void UpsertRTValue(const char *namespace_, const char *key, const char *vaule) {
  GpgFrontend::Module::UpsertRTValue(QString::fromUtf8(namespace_),
                                     QString::fromUtf8(key),
                                     QString::fromUtf8(vaule));
}

auto ListRTChildKeys(const char *namespace_, const char *key,
                     char ***child_keys) -> int32_t {
  *child_keys = nullptr;
  auto keys = GpgFrontend::Module::ListRTChildKeys(namespace_, key);

  if (keys.empty()) return 0;

  *child_keys =
      static_cast<char **>(AllocateMemory(sizeof(char **) * keys.size()));

  for (int i = 0; i < keys.size(); i++) {
    (*child_keys)[i] = GpgFrontend::GFStrDup(keys[i]);
  }

  return static_cast<int32_t>(keys.size());
}

void TriggerModuleEventCallback(ModuleEvent *module_event,
                                const char *module_id, int argc, char **argv) {
  auto data_object = GpgFrontend::TransferParams();
  for (int i = 0; i < argc; i++) {
    data_object->AppendObject(QString::fromUtf8(argv[i]));
  }

  auto event = GpgFrontend::Module::ModuleManager::GetInstance().SearchEvent(
      module_event->triggger_id);
  if (!event) return;

  event.value()->ExecuteCallback(QString::fromUtf8(module_id), data_object);
}