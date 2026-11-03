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

namespace GpgFrontend {

/**
 * @brief Re-read every key database the application is using.
 *
 * Flushes each live database's key cache on the GPG task runner, each under an
 * engine lease so a reload cannot reclaim the channel mid-flush; a retired
 * Runtime's databases are skipped, nothing new is read from them any more.
 * When it is done, @p done runs on @p receiver's thread.
 *
 * What the desktop's key-database refresh does under its signal, and what any
 * other interface calls after it changed the keyring.
 *
 * @param receiver the object @p done is delivered to; nothing is delivered if
 * it is gone by then
 * @param done called once the caches are flushed
 */
void GF_CORE_EXPORT RefreshKeyDatabases(QObject* receiver,
                                        std::function<void()> done);

}  // namespace GpgFrontend
