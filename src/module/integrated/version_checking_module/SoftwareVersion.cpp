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

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace GpgFrontend::Module::Integrated::VersionCheckingModule {

int VersionCheckingModule::SoftwareVersion::version_compare(
    const std::string& a, const std::string& b) {
  auto remove_prefix = [](const std::string& version) {
    return version.front() == 'v' ? version.substr(1) : version;
  };

  std::string real_version_a = remove_prefix(a);
  std::string real_version_b = remove_prefix(b);

  SPDLOG_DEBUG("real version a: {}", real_version_a);
  SPDLOG_DEBUG("real version b: {}", real_version_b);

  std::vector<std::string> split_a, split_b;
  boost::split(split_a, real_version_a, boost::is_any_of("."));
  boost::split(split_b, real_version_b, boost::is_any_of("."));

  const int min_depth = std::min(split_a.size(), split_b.size());

  for (int i = 0; i < min_depth; ++i) {
    int num_a = 0, num_b = 0;

    try {
      num_a = boost::lexical_cast<int>(split_a[i]);
      num_b = boost::lexical_cast<int>(split_b[i]);
    } catch (boost::bad_lexical_cast&) {
      // Handle exception if needed
      return 0;
    }

    if (num_a != num_b) {
      return (num_a > num_b) ? 1 : -1;
    }
  }

  if (split_a.size() != split_b.size()) {
    return (split_a.size() > split_b.size()) ? 1 : -1;
  }

  return 0;
}

bool VersionCheckingModule::SoftwareVersion::NeedUpgrade() const {
  SPDLOG_DEBUG("compair version current {} latest {}, result {}",
               current_version, latest_version,
               version_compare(current_version, latest_version));

  SPDLOG_DEBUG("load done: {}, pre-release: {}, draft: {}", load_info_done,
               latest_prerelease, latest_draft);
  return load_info_done && !latest_prerelease && !latest_draft &&
         version_compare(current_version, latest_version) < 0;
}

bool VersionCheckingModule::SoftwareVersion::VersionWithDrawn() const {
  return load_info_done && !current_version_found && current_prerelease &&
         !current_draft;
}

bool VersionCheckingModule::SoftwareVersion::CurrentVersionReleased() const {
  return load_info_done && current_version_found;
}
}  // namespace GpgFrontend::Module::Integrated::VersionCheckingModule