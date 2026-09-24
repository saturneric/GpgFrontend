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

namespace GpgFrontend::UI {

/**
 * @brief Withdraw everything the Host holds on behalf of @p module.
 *
 * THE withdrawal: run when a module is deactivated, when its activation
 * fails, and at shutdown -- always after its entry gate has closed and
 * before its own deactivate hook, so the hook never races a Host call into
 * the state it is tearing down. Idempotent, and callable from any thread.
 *
 * At once, on the calling thread: its commands are withdrawn and it becomes
 * a closed caller (CommandRegistry::RemoveAllFor), its translations go, and
 * its native widget registrations. Queued to the GUI thread, which owns them:
 * its UI script, and every live instance of its widgets, whose containers
 * drop them.
 */
void GF_UI_EXPORT ModuleUiTeardown(const QString& module);

/// The module is being activated again: undo what ModuleUiTeardown closed.
void GF_UI_EXPORT ModuleUiReopen(const QString& module);

}  // namespace GpgFrontend::UI
