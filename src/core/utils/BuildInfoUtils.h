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

/**
 * @brief Return the project name (e.g. "GpgFrontend").
 * @return project name string
 */
auto GF_CORE_EXPORT GetProjectName() -> QString;

/**
 * @brief Return the project organisation name.
 * @return organisation name string
 */
auto GF_CORE_EXPORT GetProjectOrganization() -> QString;

/**
 * @brief Return the release version string (e.g. "2.1.3").
 * @return version string
 */
auto GF_CORE_EXPORT GetProjectVersion() -> QString;

/**
 * @brief Return the build-specific version string (may include commit info).
 * @return build version string
 */
auto GF_CORE_EXPORT GetProjectBuildVersion() -> QString;

/**
 * @brief Return the UTC timestamp at which the binary was built.
 * @return build timestamp as QDateTime
 */
auto GF_CORE_EXPORT GetProjectBuildTimestamp() -> QDateTime;

/**
 * @brief Return the Git branch name recorded at build time.
 * @return git branch name string
 */
auto GF_CORE_EXPORT GetProjectBuildGitBranchName() -> QString;

/**
 * @brief Return the Git commit hash recorded at build time.
 * @return git commit hash string
 */
auto GF_CORE_EXPORT GetProjectBuildGitCommitHash() -> QString;

/**
 * @brief Return the git-describe version string recorded at build time.
 * @return git-describe version string
 */
auto GF_CORE_EXPORT GetProjectBuildGitVersion() -> QString;

/**
 * @brief Return the Qt version the application was built against.
 * @return Qt version string
 */
auto GF_CORE_EXPORT GetProjectQtVersion() -> QString;

/**
 * @brief Return the OpenSSL version linked at build time.
 * @return OpenSSL version string
 */
auto GF_CORE_EXPORT GetProjectOpenSSLVersion() -> QString;

/**
 * @brief Return the libarchive version linked at build time.
 * @return libarchive version string
 */
auto GF_CORE_EXPORT GetProjectLibarchiveVersion() -> QString;

/**
 * @brief Return the GpgME version linked at build time.
 * @return GpgME version string
 */
auto GF_CORE_EXPORT GetProjectGpgMEVersion() -> QString;

/**
 * @brief Return the Assuan library version linked at build time.
 * @return Assuan version string
 */
auto GF_CORE_EXPORT GetProjectAssuanVersion() -> QString;

/**
 * @brief Return the gpg-error library version linked at build time.
 * @return gpg-error version string
 */
auto GF_CORE_EXPORT GetProjectGpgErrorVersion() -> QString;

/**
 * @brief Return the HTTP User-Agent string used by the application.
 * @return User-Agent header value string
 */
auto GF_CORE_EXPORT GetHttpRequestUserAgent() -> QString;

/**
 * @brief Return whether release commit hash verification is enabled.
 * @return true if commit hash checking is active
 */
auto GF_CORE_EXPORT IsCheckReleaseCommitHash() -> bool;

/**
 * @brief Return whether the binary was compiled with Rust support.
 * @return true if Rust support is available
 */
auto GF_CORE_EXPORT HasRustSupport() -> bool;

/**
 * @brief Return the localised display name of the application.
 * @return application display name string
 */
auto GF_CORE_EXPORT GetAppDisplayName() -> QString;

/**
 * @brief Return the libsodium version linked at build time.
 * @return libsodium version string
 */
auto GF_CORE_EXPORT GetSodiumVersion() -> QString;

}  // namespace GpgFrontend
