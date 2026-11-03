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

#include <functional>

#include "boot/BootContext.h"

namespace GpgFrontend {

class ProfileLoaderDelegate;

/// Starts a new instance of the program, for a deep restart.
using RelaunchFunction =
    std::function<bool(const QString &program, const QStringList &arguments)>;

/**
 * @brief Name the application as every GpgFrontend program does: the profile
 * layout, the settings and the log locations all follow from these.
 */
void SetApplicationIdentity();

/**
 * @brief Leave at once when teardown cannot finish: the profile is given back,
 * a pending deep restart is still honoured, and no destructor runs.
 *
 * @param exit_code the process's exit code
 */
[[noreturn]] void AbandonProcess(int exit_code);

/**
 * @brief The logging and libsodium a profile needs before it can be opened.
 *
 * @param p_ctx
 * @return false when the process cannot go on; the context's fatal_error says
 * why
 */
auto PreInit(const BootCxtWPtr &p_ctx) -> bool;

/**
 * @brief Mount and open @p profile, the way every program does it.
 *
 * Mounts it, reads the properties it resolves, runs PreInit() and installs the
 * platform's secret store, then opens it. Everything that needs a human goes
 * through @p delegate.
 *
 * @param p_ctx
 * @param profile the profile that was selected
 * @param delegate answers the loader's questions
 * @return whether the profile is open; when not and the context's fatal_error
 * is set, the process cannot go on at all
 */
auto OpenProfile(const BootCxtWPtr &p_ctx,
                 const QSharedPointer<Profile> &profile,
                 ProfileLoaderDelegate &delegate) -> bool;

/**
 * @brief init global PATH env
 *
 */
void InitGlobalPathEnv();

/**
 * @brief
 *
 */
void InitLocale();

/**
 * @brief
 *
 * @param p_ctx
 * @param before_core run after the modules load and before the core starts
 */
void InitGlobalBasicEnv(const BootCxtWPtr &p_ctx,
                        const std::function<void()> &before_core = {});

/**
 * @brief
 *
 * @param p_ctx
 * @param before_core run after the modules load and before the core starts
 */
void InitGlobalBasicEnvSync(const BootCxtWPtr &p_ctx,
                            const std::function<void()> &before_core = {});

/**
 * @brief
 *
 * @param p_ctx
 * @param relaunch starts the next instance when a deep restart is pending;
 * without it there is none
 */
void ShutdownGlobalBasicEnv(const BootCxtWPtr &p_ctx,
                            const RelaunchFunction &relaunch = {});
}  // namespace GpgFrontend