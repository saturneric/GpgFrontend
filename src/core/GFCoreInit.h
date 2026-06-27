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

#include "core/function/openpgp/OpenPGPContext.h"

namespace GpgFrontend {

/**
 * @brief Core initialization arguments.
 */
struct CoreInitArgs {
  bool gather_external_gnupg_info;
  bool unit_test_mode;
};

/**
 * @brief Destroy and clean up the GpgFrontend core, stopping all task
 * runners, flushing cache storage, and destroying singleton objects.
 */
void GF_CORE_EXPORT DestroyGpgFrontendCore();

/**
 * @brief Whether the core has begun shutting down.
 *
 * Set at the very start of DestroyGpgFrontendCore(). Background subsystems
 * (notably the DataObjectOperator GC task) check this before posting new
 * long-running work: spawning a task during teardown can race the singleton
 * storage being destroyed and dereference it after it is gone.
 *
 * @return true once DestroyGpgFrontendCore() has started.
 */
auto GF_CORE_EXPORT IsCoreShuttingDown() -> bool;

/**
 * @brief Initialize the GpgFrontend core subsystem, including GnuPG
 * and rPGP engine detection, basic path resolution, and default OpenPGP
 * context construction.
 *
 * @param args Core initialization parameters.
 * @return int 0 on success, -1 on failure.
 */
auto GF_CORE_EXPORT InitGpgFrontendCore(CoreInitArgs args) -> int;

/**
 * @brief Build an OpenPGP context on the given channel with the specified
 * initialization arguments (engine type, database name/path, offline mode,
 * auto-import settings).
 *
 * @param channel The channel index for the new context.
 * @param args Context initialization arguments.
 * @return true if the context was created successfully.
 * @return false otherwise.
 */
auto GF_CORE_EXPORT BuildOpenPGPContext(int channel,
                                        OpenPGPContextInitArgs args) -> bool;

/**
 * @brief Start a background task that monitors the core initialization
 * status and signals when the core (including modules and key databases)
 * is fully loaded.
 */
void GF_CORE_EXPORT StartMonitorCoreInitializationStatus();

/**
 * @brief Initialize the GpgME (GPG Made Easy) library, verify backend
 * engines (OpenPGP, GPGCONF, CMS), and enforce minimum GnuPG version
 * requirements.
 *
 * @return true if GpgME initialized successfully.
 * @return false otherwise.
 */
auto GF_CORE_EXPORT InitGpgME() -> bool;

/**
 * @brief Resolve and initialize the basic filesystem paths used by the
 * core, including the gpgconf executable path, GnuPG binary path, and
 * the default key database (home) directory.
 *
 * @return true if basic path initialization succeeded.
 * @return false otherwise.
 */
auto GF_CORE_EXPORT InitBasicPath() -> bool;

}  // namespace GpgFrontend
