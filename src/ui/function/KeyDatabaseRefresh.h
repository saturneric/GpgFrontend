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

#include "core/function/openpgp/KeyDatabaseRuntime.h"

namespace GpgFrontend::UI {

/**
 * @brief Subscribe the key-cache flush that backs
 * UISignalStation::SignalKeyDatabaseRefresh.
 *
 * The flush runs on the gpg task runner rather than inline, because it walks
 * every open channel; SignalKeyDatabaseRefreshDone follows when it finishes,
 * and that is what deferred UI (the import detail dialog, for one) waits on.
 * Idempotent.
 */
void GF_UI_EXPORT InstallKeyDatabaseRefreshHandler();

/**
 * @brief Keep @p owner on its key database when a new Runtime replaces the
 * one its channel belongs to.
 *
 * On every SignalKeyDatabaseRuntimeChanged the database behind the channel is
 * found again by its id: @p rebound is told its new channel, or @p lost is
 * called when the database is gone. Until then a lease keeps the old channel's
 * objects alive, so nothing @p owner still holds from it is reclaimed under
 * it. Lives as long as @p owner.
 *
 * @param owner what holds the channel
 * @param current reads the channel it holds now
 * @param rebound told the new channel
 * @param lost called once the database is gone; MarkKeyDatabaseLost() when
 * empty
 * @param suspended called when the channel is retired ahead of a new Runtime
 * (a folder about to move), before the lease is let go: drop anything still
 * read from the old channel's objects, such as a model built from them
 */
void GF_UI_EXPORT KeepBoundToKeyDatabase(QWidget* owner,
                                         std::function<int()> current,
                                         std::function<void(int)> rebound,
                                         std::function<void()> lost = {},
                                         std::function<void()> suspended = {});

/**
 * @brief Show that the key database @p widget works on is gone.
 *
 * Disabled rather than closed, so whatever it shows can still be read; a
 * window says so in its title.
 */
void GF_UI_EXPORT MarkKeyDatabaseLost(QWidget* widget);

/**
 * @brief Undo MarkKeyDatabaseLost(), for a database found again.
 *
 * Does nothing to a widget that was not marked.
 *
 * @param widget the widget marked lost
 */
void GF_UI_EXPORT MarkKeyDatabaseFound(QWidget* widget);

/**
 * @brief Bring the saved key database configuration into use, without a
 * restart.
 *
 * Runs ApplyKeyDatabaseRuntime() off the GUI thread and says how it went in
 * the status bar, and in a message when something could not be done. A
 * request made while one is running is folded into a single follow-up.
 *
 * Shows a modal waiting dialog for as long as it runs, which the user cannot
 * close, so nothing is started on a database while the databases are being
 * replaced.
 *
 * @param moves managed database folders to move first
 * @param parent the window to show the waiting dialog over; the last one
 * given, or the active window, when null
 */
void GF_UI_EXPORT
ApplyKeyDatabaseChanges(const QContainer<KeyDatabaseMove>& moves = {},
                        QWidget* parent = nullptr);

/**
 * @brief Whether an ApplyKeyDatabaseChanges() is still running.
 *
 * When it stops, UISignalStation::SignalKeyDatabaseChangesApplied is emitted.
 */
auto GF_UI_EXPORT IsApplyingKeyDatabaseChanges() -> bool;

}  // namespace GpgFrontend::UI
