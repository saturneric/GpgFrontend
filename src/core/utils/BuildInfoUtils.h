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
 * @brief Return the on-disk profile identity of this build.
 *
 * This is what QCoreApplication::applicationName() is set to, and therefore
 * what decides the QSettings scope and the AppLocalData directory. Non-stable
 * builds get their own, so a nightly can never read or rewrite the profile of
 * an installed release.
 *
 * @return profile name, without spaces
 */
auto GF_CORE_EXPORT GetAppProfileName() -> QString;

/**
 * @brief Return whether this is an official stable release build.
 * @return true when built with -DGPGFRONTEND_BUILD_STABLE=ON
 */
auto GF_CORE_EXPORT IsStableBuild() -> bool;

/**
 * @brief Return whether this is a portable build.
 *
 * A portable build keeps its data in the directory above the application rather
 * than in the OS user-data location. This is decided when the artifact is built
 * and never at run time: it chooses which directory holds the user's keys, so
 * it must not be switchable by dropping a file beside the binary.
 *
 * @return true when built with -DGPGFRONTEND_BUILD_PORTABLE=ON
 */
auto GF_CORE_EXPORT IsPortableBuild() -> bool;

/**
 * @brief Return whether this build can verify its own libraries and binaries.
 *
 * The signatures the check compares against are only made when an official
 * stable release is built, so on a nightly or a local build there is nothing to
 * verify and the check could only ever fail. Callers use this to force the
 * self-check off and to leave its control out of the interface entirely, rather
 * than offering a switch that must stay off.
 *
 * @return true when the self-check is meaningful for this build
 */
auto GF_CORE_EXPORT IsSelfCheckAvailable() -> bool;

/**
 * @brief Return the profile layout version this build understands.
 * @return schema version number
 */
auto GF_CORE_EXPORT GetAppProfileSchemaVersion() -> int;

/**
 * @brief Return the oldest layout version that can still read what this build
 * writes.
 *
 * Stamped into a profile as its @c min_reader_version, and into a package as
 * its @c min_reader, where it is the only thing a reader is allowed to refuse
 * on. Deliberately not the schema version: a layout that gained a field an
 * older build simply ignores is still readable by that build, and saying
 * otherwise would strand a user who downgrades.
 *
 * @return the oldest schema version allowed to open this build's output
 */
auto GF_CORE_EXPORT GetAppProfileMinReaderSchema() -> int;

/**
 * @brief Return the libsodium version linked at build time.
 * @return libsodium version string
 */
auto GF_CORE_EXPORT GetSodiumVersion() -> QString;

}  // namespace GpgFrontend
