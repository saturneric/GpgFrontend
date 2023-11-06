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

#include "CommonUtils.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace GpgFrontend {

auto BeautifyFingerprint(BypeArrayConstRef fingerprint) -> std::string {
  auto len = fingerprint.size();
  std::stringstream out;
  decltype(len) count = 0;
  while (count < len) {
    if ((count != 0U) && !(count % 5)) out << " ";
    out << fingerprint[count];
    count++;
  }
  return out.str();
}

auto CompareSoftwareVersion(const std::string& a, const std::string& b) -> int {
  auto remove_prefix = [](const std::string& version) {
    return version.front() == 'v' ? version.substr(1) : version;
  };

  std::string real_version_a = remove_prefix(a);
  std::string real_version_b = remove_prefix(b);

  std::vector<std::string> split_a;
  std::vector<std::string> split_b;
  boost::split(split_a, real_version_a, boost::is_any_of("."));
  boost::split(split_b, real_version_b, boost::is_any_of("."));

  const auto min_depth = std::min(split_a.size(), split_b.size());

  for (auto i = 0U; i < min_depth; ++i) {
    int num_a = 0;
    int num_b = 0;

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
}  // namespace GpgFrontend