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

#include "BuildInfoUtils.h"

#include <archive.h>
#include <assuan.h>
#include <gpg-error.h>
#include <gpgme.h>
#include <openssl/opensslv.h>

#include "GpgFrontendBuildInfo.h"

namespace GpgFrontend {

auto GetProjectName() -> QString { return {PROJECT_NAME}; }

auto GetProjectVersion() -> QString {
  return (QStringList{} << "v" << VERSION_MAJOR << "." << VERSION_MINOR << "."
                        << VERSION_PATCH)
      .join("");
}

auto GetProjectBuildVersion() -> QString { return BUILD_VERSION; }

auto GetProjectBuildTimestamp() -> QDateTime {
  return QDateTime::fromString(BUILD_TIMESTAMP, Qt::ISODate);
}

auto GetProjectBuildGitBranchName() -> QString { return GIT_BRANCH_NAME; }

auto GetProjectBuildGitCommitHash() -> QString { return GIT_COMMIT_HASH; }

auto GetProjectBuildGitVersion() -> QString { return GIT_VERSION; }

auto GetProjectQtVersion() -> QString { return {qVersion()}; }

auto GetProjectOpenSSLVersion() -> QString {
  return {OPENSSL_FULL_VERSION_STR};
}

auto GetProjectGpgMEVersion() -> QString { return {GPGME_VERSION}; }

auto GetProjectAssuanVersion() -> QString { return {ASSUAN_VERSION}; }

auto GetProjectGpgErrorVersion() -> QString { return {GPG_ERROR_VERSION}; }

auto GetProjectLibarchiveVersion() -> QString {
  return {ARCHIVE_VERSION_ONLY_STRING};
}

auto GetHttpRequestUserAgent() -> QString { return HTTP_REQUEST_USER_AGENT; }

};  // namespace GpgFrontend