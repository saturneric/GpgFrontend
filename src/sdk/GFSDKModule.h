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

#include "GFSDKVisibility.h"

#include "GFSDKModuleModel.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Registers the module to receive dispatched events matching @p
 * event_id.
 *
 * The module ID is compared case-insensitively (stored lower-case) and the
 * event ID is stored upper-case.
 *
 * @param module_id Unique identifier of the calling module.
 * @param event_id  Event identifier to subscribe to.
 */
GF_SDK_EXPORT void GFModuleListenEvent(const char *module_id,
                                       const char *event_id);

/**
 * @brief Retrieves a runtime string value, returning a default if not set.
 *
 * @param namespace_    Runtime value namespace (lower-case).
 * @param key           Key within the namespace (lower-case).
 * @param default_value Value to return when the key is absent.
 * @return Caller-owned string; free with GFFreeMemory.
 */
GF_SDK_EXPORT const char *GFModuleRetrieveRTValueOrDefault(
    const char *namespace_, const char *key, const char *default_value);

/**
 * @brief Retrieves a runtime boolean value, returning a default if not set.
 *
 * @param namespace_    Runtime value namespace (lower-case).
 * @param key           Key within the namespace (lower-case).
 * @param default_value Default to return when the key is absent (0 = false).
 * @return 1 for true, 0 for false.
 */
GF_SDK_EXPORT int GFModuleRetrieveRTValueOrDefaultBool(const char *namespace_,
                                                       const char *key,
                                                       int default_value);

/**
 * @brief Inserts or updates a runtime string value.
 *
 * Both @p namespace_ and @p key are stored lower-case.
 *
 * @param namespace_ Runtime value namespace.
 * @param key        Key within the namespace.
 * @param value      String value to store.
 */
GF_SDK_EXPORT void GFModuleUpsertRTValue(const char *namespace_,
                                         const char *key, const char *value);

/**
 * @brief Inserts or updates a runtime boolean value.
 *
 * Both @p namespace_ and @p key are stored lower-case.
 *
 * @param namespace_ Runtime value namespace.
 * @param key        Key within the namespace.
 * @param value      Non-zero for true, 0 for false.
 */
GF_SDK_EXPORT void GFModuleUpsertRTValueBool(const char *namespace_,
                                             const char *key, int value);

/**
 * @brief Lists all direct child keys under a namespace/key prefix.
 *
 * @param namespace_          Runtime value namespace (lower-case).
 * @param key                 Parent key prefix (lower-case).
 * @param[out] child_keys     Set to a GFAllocateMemory-allocated array of
 *                            GFModuleStrDup strings on success; each element
 *                            and the array itself must be freed with
 *                            GFFreeMemory. Set to nullptr when the count is 0.
 * @return Number of child keys found, or 0 if none.
 */
GF_SDK_EXPORT int32_t GFModuleListRTChildKeys(const char *namespace_,
                                              const char *key,
                                              char ***child_keys);

/**
 * @brief Invokes the event callback chain for a module event.
 *
 * Looks up the original event identified by @p event->trigger_id and calls its
 * registered callback with the parameters supplied in @p argv.
 *
 * @param event     The module event whose trigger_id identifies the callback.
 * @param module_id Identifier of the module invoking the callback.
 * @param argv      Linked list of additional parameters; may be nullptr.
 */
GF_SDK_EXPORT void GFModuleTriggerModuleEventCallback(GFModuleEvent *event,
                                                      const char *module_id,
                                                      GFModuleEventParam *argv);
#ifdef __cplusplus
}
#endif
