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

#include "GFSDKExport.h"

extern "C" {

#include <stdint.h>

struct GFModuleMetaData {
  const char *key;
  const char *value;
  GFModuleMetaData *next;
};

struct GFModuleEventParam {
  const char *name;
  const char *value;
  GFModuleEventParam *next;
};

struct GFModuleEvent {
  const char *id;
  const char *trigger_id;
  GFModuleEventParam *params;
};

using GFModuleAPIGetModuleGFSDKVersion = auto (*)() -> const char *;

using GFModuleAPIGetModuleQtEnvVersion = auto (*)() -> const char *;

using GFModuleAPIGetModuleID = auto (*)() -> const char *;

using GFModuleAPIGetModuleVersion = auto (*)() -> const char *;

using GFModuleAPIGetModuleMetaData = auto (*)() -> GFModuleMetaData *;

using GFModuleAPIRegisterModule = auto (*)() -> int;

using GFModuleAPIActivateModule = auto (*)() -> int;

using GFModuleAPIExecuteModule = auto (*)(GFModuleEvent *) -> int;

using GFModuleAPIDeactivateModule = auto (*)() -> int;

using GFModuleAPIUnregisterModule = auto (*)() -> int;

void GPGFRONTEND_MODULE_SDK_EXPORT GFModuleListenEvent(const char *module_id,
                                                       const char *event_id);

auto GPGFRONTEND_MODULE_SDK_EXPORT GFModuleRetrieveRTValueOrDefault(
    const char *namespace_, const char *key, const char *default_value) -> const
    char *;

auto GPGFRONTEND_MODULE_SDK_EXPORT GFModuleRetrieveRTValueOrDefaultBool(
    const char *namespace_, const char *key, int default_value) -> const int;

void GPGFRONTEND_MODULE_SDK_EXPORT GFModuleUpsertRTValue(const char *namespace_,
                                                         const char *key,
                                                         const char *vaule);

void GPGFRONTEND_MODULE_SDK_EXPORT
GFModuleUpsertRTValueBool(const char *namespace_, const char *key, int value);

auto GPGFRONTEND_MODULE_SDK_EXPORT GFModuleListRTChildKeys(
    const char *namespace_, const char *key, char ***child_keys) -> int32_t;

void GPGFRONTEND_MODULE_SDK_EXPORT GFModuleTriggerModuleEventCallback(
    GFModuleEvent *event, const char *module_id, GFModuleEventParam *argv);
};