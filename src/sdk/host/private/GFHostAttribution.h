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

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file GFHostAttribution.h
 * @brief Which module the host is currently working on behalf of.
 *
 * HOST-INTERNAL, and no longer part of the public SDK: a module never asks
 * this and never sets it. It survives for two jobs that are genuinely the
 * host's -- recording which module a handle was issued to, so teardown can
 * reclaim what was leaked, and naming a module in a log line.
 *
 * It is NOT an authorization input. Authorization is the context a call
 * carries, because a module's own worker threads never pass through here at
 * all -- which was the whole reason the context exists.
 *
 * Every handle the SDK issues is recorded against a module so that teardown
 * can say who leaked what, and reclaim it. The SDK has no other way to know:
 * the host API table is one static table shared by every module, so a call
 * arriving through it carries no identity of its own.
 *
 * The answer is therefore kept per thread, set by the host at each point
 * where it hands control to module code, and cleared when control comes back.
 *
 * ## What this attributes, and what it does not
 *
 * A handle created while a module's callback is on the stack is attributed to
 * that module. A handle created on a thread the module started for itself is
 * not -- nothing on that thread ever passed through the host, so there is
 * nothing to have set the current module. Those handles are recorded with no
 * owner and are swept only at process exit rather than at unload.
 *
 * That is a real limit rather than a bug to fix here: attributing them would
 * mean the module telling the SDK who it is, which is a claim rather than an
 * observation, and a module that gets it wrong would have another module's
 * handles freed underneath it.
 */

/**
 * @brief Begin attributing handles on this thread to @p module_id.
 *
 * Nestable: the previous value is returned and must be handed back to
 * GFSdkLeaveModule(). Prefer the RAII form below in C++.
 *
 * @param module_id borrowed, and copied; may be NULL to attribute nothing
 * @return the previously current module, to restore later
 */
const char* GFSdkEnterModule(const char* module_id);

/**
 * @brief Stop attributing handles on this thread, restoring @p previous.
 *
 * @param previous exactly what GFSdkEnterModule() returned
 */
void GFSdkLeaveModule(const char* previous);

/**
 * @brief The module this thread is currently working for, or NULL.
 *
 * @return borrowed, valid until the next Enter/Leave on this thread
 */
const char* GFSdkCurrentModule(void);

/**
 * @brief Reclaim every handle still outstanding for @p module_id.
 *
 * ONLY safe once no module code can run. Teardown establishes that in its
 * quiesce step, which is why this is called from step 5 of that sequence and
 * from nowhere else: a handle the module is still using would be wiped and
 * freed underneath it.
 *
 * Each reclaimed handle is logged with the entry point that issued it, so a
 * leak becomes a named diagnostic rather than a silent loss. For a secret it
 * also means the bytes are wiped rather than left in the heap for the life of
 * the process.
 *
 * @param module_id the module being torn down; NULL sweeps nothing
 * @return how many handles were reclaimed
 */
size_t GFSdkSweepModuleHandles(const char* module_id);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/**
 * @brief RAII form of GFSdkEnterModule()/GFSdkLeaveModule().
 *
 * Header-only and calling only the exported C entry points, so gf_core can
 * use it even though it cannot link the sdk: the symbols resolve when the
 * application links both, exactly as GFGetHostApi() already does.
 */
class GFSdkModuleAttributionScope {
 public:
  explicit GFSdkModuleAttributionScope(const char* module_id)
      : previous_(GFSdkEnterModule(module_id)) {}

  ~GFSdkModuleAttributionScope() { GFSdkLeaveModule(previous_); }

  GFSdkModuleAttributionScope(const GFSdkModuleAttributionScope&) = delete;
  auto operator=(const GFSdkModuleAttributionScope&)
      -> GFSdkModuleAttributionScope& = delete;

 private:
  const char* previous_;
};
#endif
