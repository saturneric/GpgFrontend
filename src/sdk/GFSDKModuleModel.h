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

#pragma once

extern "C" {

/**
 * @brief A singly-linked list node for arbitrary key-value module metadata.
 *
 * Returned by GFModuleAPIGetModuleMetaData to describe the module.
 */
struct GFModuleMetaData {
  const char *key;          ///< Metadata key string.
  const char *value;        ///< Metadata value string.
  GFModuleMetaData *next;   ///< Next node, or nullptr at end of list.
};

/**
 * @brief A singly-linked list node carrying one named parameter of an event.
 */
struct GFModuleEventParam {
  const char *name;           ///< Parameter name.
  const char *value;          ///< Parameter value.
  GFModuleEventParam *next;   ///< Next parameter, or nullptr at end of list.
};

/**
 * @brief Describes a module event dispatched by the event system.
 */
struct GFModuleEvent {
  const char *id;             ///< Unique event identifier (upper-case).
  const char *trigger_id;     ///< ID of the event that triggered this one.
  GFModuleEventParam *params; ///< Linked list of event parameters; may be nullptr.
};

/// Returns the GpgFrontend SDK version the module was compiled against.
using GFModuleAPIGetModuleGFSDKVersion = auto (*)() -> const char *;

/// Returns the Qt version the module was compiled against.
using GFModuleAPIGetModuleQtEnvVersion = auto (*)() -> const char *;

/// Returns the module's unique identifier string (lower-case, dot-separated).
using GFModuleAPIGetModuleID = auto (*)() -> const char *;

/// Returns the module's version string.
using GFModuleAPIGetModuleVersion = auto (*)() -> const char *;

/// Returns a linked list of key-value metadata describing the module.
using GFModuleAPIGetModuleMetaData = auto (*)() -> GFModuleMetaData *;

/// Called once to register the module with the module manager. Return 0 on success.
using GFModuleAPIRegisterModule = auto (*)() -> int;

/// Called to activate the module; subscribe to events here. Return 0 on success.
using GFModuleAPIActivateModule = auto (*)() -> int;

/// Called when the module receives an event to handle. Return 0 on success.
using GFModuleAPIExecuteModule = auto (*)(GFModuleEvent *) -> int;

/// Called to deactivate the module; unsubscribe from events here. Return 0 on success.
using GFModuleAPIDeactivateModule = auto (*)() -> int;

/// Called once to unregister and clean up the module. Return 0 on success.
using GFModuleAPIUnregisterModule = auto (*)() -> int;
};