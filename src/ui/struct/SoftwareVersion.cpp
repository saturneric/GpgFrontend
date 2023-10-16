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

int GpgFrontend::UI::SoftwareVersion::version_compare(const std::string& a,
                                                      const std::string& b) {
  auto temp_a = a, temp_b = b;

  if (!temp_a.empty() && temp_a.front() == 'v') {
    temp_a = temp_a.erase(0, 1);
    SPDLOG_DEBUG("real version a: {}", temp_a);
  }

  if (!temp_b.empty() && temp_b.front() == 'v') {
    temp_b.erase(0, 1);
    SPDLOG_DEBUG("real version b: {}", temp_b);
  }

  // First, split the string.
  std::vector<std::string> va, vb;
  boost::split(va, temp_a, boost::is_any_of("."));
  boost::split(vb, temp_b, boost::is_any_of("."));

  // Compare the numbers step by step, but only as deep as the version
  // with the least elements allows.
  const int depth =
      std::min(static_cast<int>(va.size()), static_cast<int>(vb.size()));
  int ia = 0, ib = 0;
  for (int i = 0; i < depth; ++i) {
    try {
      ia = boost::lexical_cast<int>(va[i]);
      ib = boost::lexical_cast<int>(vb[i]);
    } catch (boost::bad_lexical_cast& ignored) {
      break;
    }
    if (ia != ib) break;
  }

  // Return the required number.
  if (ia > ib)
    return 1;
  else if (ia < ib)
    return -1;
  else {
    // In case of equal versions, assumes that the version
    // with the most elements is the highest version.
    if (va.size() > vb.size())
      return 1;
    else if (va.size() < vb.size())
      return -1;
  }

  // Everything is equal, return 0.
  return 0;
}

bool GpgFrontend::UI::SoftwareVersion::NeedUpgrade() const {
  SPDLOG_DEBUG("compair version current {} latest {}, result {}",
               current_version, latest_version,
               version_compare(current_version, latest_version));

  SPDLOG_DEBUG("load done: {}, pre-release: {}, draft: {}", load_info_done,
               latest_prerelease, latest_draft);
  return load_info_done && !latest_prerelease && !latest_draft &&
         version_compare(current_version, latest_version) < 0;
}

bool GpgFrontend::UI::SoftwareVersion::VersionWithDrawn() const {
  return load_info_done && !current_version_found && current_prerelease &&
         !current_draft;
}

bool GpgFrontend::UI::SoftwareVersion::CurrentVersionReleased() const {
  return load_info_done && current_version_found;
}