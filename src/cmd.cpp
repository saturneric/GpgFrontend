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

#include "cmd.h"

// std
#include <iostream>

// boost
#include <boost/program_options.hpp>

// GpgFrontend
#include "GpgFrontendBuildInfo.h"

namespace po = boost::program_options;

auto PrintVersion() -> int {
  std::cout << PROJECT_NAME << " "
            << "v" << VERSION_MAJOR << "." << VERSION_MINOR << "."
            << VERSION_PATCH << std::endl;
  std::cout
      << "Copyright (C) 2021 Saturneric <eric@bktus.com>" << std::endl
      << _("This is free software; see the source for copying conditions.")
      << std::endl
      << std::endl;

  std::cout << _("Build Timestamp: ") << BUILD_TIMESTAMP << std::endl
            << _("Build Version: ") << BUILD_VERSION << std::endl
            << _("Source Code Version: ") << GIT_VERSION << std::endl;

  return 0;
}

auto ParseLogLevel(const po::variables_map& vm) -> spdlog::level::level_enum {
  std::string log_level = vm["log-level"].as<std::string>();

  if (log_level == "trace") {
    return spdlog::level::trace;
  } else if (log_level == "debug") {
    return spdlog::level::debug;
  } else if (log_level == "info") {
    return spdlog::level::info;
  } else if (log_level == "warn") {
    return spdlog::level::warn;
  } else if (log_level == "error") {
    return spdlog::level::err;
  }
}