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

#pragma once

#include "GpgFrontendModuleSDKExport.h"

extern "C" {

struct GPGFRONTEND_MODULE_SDK_EXPORT ModuleMetaData {
  const char *key;
  const char *value;
  ModuleMetaData *next;
};

struct GPGFRONTEND_MODULE_SDK_EXPORT ModuleEventParam {
  const char *name;
  const char *value;
  ModuleEventParam *next;
};

struct GPGFRONTEND_MODULE_SDK_EXPORT ModuleEvent {
  const char *id;
  const char *triggger_id;
  ModuleEventParam *params;
};

using ModuleAPIGetModuleID = auto (*)() -> const char *;

using ModuleAPIGetModuleVersion = auto (*)() -> const char *;

using ModuleAPIGetModuleMetaData = auto (*)() -> ModuleMetaData *;

using ModuleAPIRegisterModule = auto (*)() -> int;

using ModuleAPIActivateModule = auto (*)() -> int;

using ModuleAPIExecuteModule = auto (*)(ModuleEvent *) -> int;

using ModuleAPIDeactivateModule = auto (*)() -> int;

using ModuleAPIUnregisterModule = auto (*)() -> int;

void GPGFRONTEND_MODULE_SDK_EXPORT ListenEvent(const char *module_id,
                                               const char *event_id);

auto GPGFRONTEND_MODULE_SDK_EXPORT RetrieveRTValueOrDefault(
    const char *namespace_, const char *key, const char *default_value) -> const
    char *;

void GPGFRONTEND_MODULE_SDK_EXPORT UpsertRTValue(const char *namespace_,
                                                 const char *key,
                                                 const char *vaule);

auto GPGFRONTEND_MODULE_SDK_EXPORT ListRTChildKeys(const char *namespace_,
                                                   const char *key,
                                                   char ***child_keys)
    -> int32_t;

void GPGFRONTEND_MODULE_SDK_EXPORT TriggerModuleEventCallback(
    ModuleEvent *event, const char *module_id, int argc, char **argv);
};