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

#include "GFShellExport.h"
#include "core/profile/ProfileLoader.h"

namespace GpgFrontend {

/// What to tell someone about a profile that would not open.
struct GF_SHELL_EXPORT ProfileLoadErrorText {
  QString title;
  QString text;
};

/**
 * @brief The words for @p error, the same in every interface.
 *
 * @return nothing for a failure that needs none: the person backed out, or
 * declined a reset that was already explained
 */
auto GF_SHELL_EXPORT DescribeProfileLoadError(const ProfileLoadError &error)
    -> std::optional<ProfileLoadErrorText>;

}  // namespace GpgFrontend
