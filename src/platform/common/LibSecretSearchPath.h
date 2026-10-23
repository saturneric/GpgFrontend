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

namespace GpgFrontend {

/// SONAME reported to the user, and the last candidate tried.
constexpr auto kLibSecretSoname = "libsecret-1.so.0";

/// Environment variable that puts a library ahead of every other candidate.
constexpr auto kLibSecretPathEnv = "GPGFRONTEND_LIBSECRET_PATH";

/**
 * @brief Files to try, in order, when loading libsecret.
 *
 * An AppImage bundles its own glib and puts it ahead of the host's, so the
 * host's libsecret -- built against whatever glib that distribution ships --
 * cannot resolve its own symbols there. The bundled copy therefore has to be
 * named explicitly, and the host's kept as a fallback for the case where the
 * bundle is the broken one.
 *
 * Both inputs are parameters rather than environment reads so that the order,
 * which is the whole point of this function, can be tested without a test
 * having to mutate an environment it does not own.
 *
 * @param app_dir $APPDIR: an AppImage's mount point, empty otherwise. Ignored
 * unless absolute, since a relative one would name whatever directory the
 * process happens to sit in.
 * @param override_path $GPGFRONTEND_LIBSECRET_PATH, empty when unset
 * @return candidates, most specific first; never empty and free of duplicates
 */
auto LibSecretSearchPaths(const QString& app_dir, const QString& override_path)
    -> QStringList;

}  // namespace GpgFrontend
