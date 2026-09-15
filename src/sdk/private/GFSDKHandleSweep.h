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

#include <QList>
#include <QString>

/**
 * @file GFSDKHandleSweep.h
 * @brief The per-type halves of the unload sweep, and who to attribute to.
 *
 * Each handle type owns its own registry and its own destruction, so the
 * sweep cannot be written once: what it means to destroy a buffer, a result
 * and a list are three different things. This is the seam between them and
 * the single entry point teardown calls.
 */
namespace gf_sdk_internal {

/**
 * @brief The module this thread is currently working for.
 *
 * Empty when the SDK was not entered through the host -- a module's own
 * worker thread, or the application itself.
 *
 * @return the module identifier, or an empty string
 */
auto CurrentModuleId() -> QString;

/**
 * @brief Reclaim every outstanding buffer handle belonging to @p module_id.
 *
 * @param module_id the module being torn down
 * @return the entry point that issued each reclaimed handle
 */
auto SweepBufferHandles(const QString& module_id) -> QList<const char*>;

/// As SweepBufferHandles(), for gpg result handles.
auto SweepResultHandles(const QString& module_id) -> QList<const char*>;

/// As SweepBufferHandles(), for the three list handle types.
auto SweepListHandles(const QString& module_id) -> QList<const char*>;

}  // namespace gf_sdk_internal
