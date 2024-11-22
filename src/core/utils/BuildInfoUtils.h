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

#include "core/GpgFrontendCoreExport.h"

namespace GpgFrontend {

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectName() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectVersion() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectBuildVersion() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectBuildTimestamp() -> QDateTime;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectBuildGitBranchName() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectBuildGitCommitHash() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectBuildGitVersion() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectQtVersion() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectOpenSSLVersion() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetProjectLibarchiveVersion() -> QString;

/**
 * @brief
 *
 * @return QString
 */
auto GPGFRONTEND_CORE_EXPORT GetHttpRequestUserAgent() -> QString;

}  // namespace GpgFrontend