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

namespace GpgFrontend::Module {

// Reserved for future module initialisation arguments.
struct ModuleInitArgs {};

/**
 * @brief What the user asked the module loader to do.
 *
 * An enum rather than the bare strings this was compared against in eleven
 * places across three files. The value is PERSISTED, so an unrecognised one is
 * a real case rather than a hypothetical: before this, a typo in the settings
 * file was indistinguishable from having chosen "only_integrated".
 */
enum class ModuleLoadingPolicy {
  kDISABLE,          ///< load nothing at all
  kONLY_INTEGRATED,  ///< only the modules shipped with the application
  kALL,              ///< also external ones
};

/// The outcome of reading a persisted policy.
struct GF_CORE_EXPORT ModuleLoadingPolicyParse {
  ModuleLoadingPolicy policy = ModuleLoadingPolicy::kONLY_INTEGRATED;

  /// False when the stored value was not a known key. The caller reports it;
  /// silently repairing a value the user cannot see is how a setting comes to
  /// mean something other than what it says.
  bool recognised = true;
};

/**
 * @brief Read a persisted policy key, failing closed.
 *
 * Pure -- no settings store, no logging -- so every case including the
 * malformed ones is testable directly.
 *
 * An unrecognised value falls back to kONLY_INTEGRATED: the shipped default,
 * and the most restrictive policy that still loads the application's own
 * modules. kDISABLE would be safer still, but would turn one typo into a
 * silent feature outage, which is the worse failure for a value nobody can
 * see.
 *
 * @param key the stored string
 * @return the policy, and whether the key was actually understood
 */
auto GF_CORE_EXPORT ParseModuleLoadingPolicy(const QString& key)
    -> ModuleLoadingPolicyParse;

/**
 * @brief The key a policy is stored as. The inverse of the parse.
 *
 * @param policy the policy
 * @return the settings key for it
 */
auto GF_CORE_EXPORT ModuleLoadingPolicyKey(ModuleLoadingPolicy policy)
    -> QString;

/**
 * @brief Load, register, and activate all built-in and configured modules.
 *
 * Must be called once during application startup after the ModuleManager
 * singleton has been created.
 *
 * @param args reserved initialisation arguments (currently unused)
 */
void GF_CORE_EXPORT LoadGpgFrontendModules(ModuleInitArgs args);

/**
 * @brief Deactivate and unregister all modules, then shut down the module
 * system.
 *
 * Must be called once during application shutdown before the ModuleManager
 * singleton is destroyed.
 */
void GF_CORE_EXPORT ShutdownGpgFrontendModules();

};  // namespace GpgFrontend::Module
