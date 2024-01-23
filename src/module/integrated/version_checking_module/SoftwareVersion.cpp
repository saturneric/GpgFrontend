/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "SoftwareVersion.h"

#include "core/utils/CommonUtils.h"
#include "module/sdk/Log.h"

namespace GpgFrontend::Module::Integrated::VersionCheckingModule {

auto VersionCheckingModule::SoftwareVersion::NeedUpgrade() const -> bool {
  MODULE_LOG_DEBUG("compair version current {} latest {}, result {}",
                   current_version, latest_version,
                   CompareSoftwareVersion(current_version, latest_version));

  MODULE_LOG_DEBUG("load done: {}, pre-release: {}, draft: {}", loading_done,
                   latest_prerelease_version_from_remote,
                   latest_draft_from_remote);
  return loading_done && !latest_prerelease_version_from_remote &&
         !latest_draft_from_remote &&
         CompareSoftwareVersion(current_version, latest_version) < 0;
}

auto VersionCheckingModule::SoftwareVersion::VersionWithdrawn() const -> bool {
  return loading_done && !current_version_publish_in_remote &&
         current_version_is_a_prerelease && !current_version_is_drafted;
}

auto VersionCheckingModule::SoftwareVersion::CurrentVersionReleased() const
    -> bool {
  return loading_done && current_version_publish_in_remote;
}
}  // namespace GpgFrontend::Module::Integrated::VersionCheckingModule