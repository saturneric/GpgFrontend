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

/**
 * \mainpage GpgFrontend Develop Document Main Page
 */

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>

#include "app.h"
#include "cmd.h"
#include "type.h"

namespace po = boost::program_options;

/**
 *
 * @param argc
 * @param argv
 * @return
 */
auto main(int argc, char* argv[]) -> int {
  po::options_description desc("Allowed options");

  desc.add_options()("help,h", "produce help message")(
      "version,v", "show version information")(
      "log-level,l", po::value<std::string>()->default_value("info"),
      "set log level (trace, debug, info, warn, error)");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  InitArgs args;
  args.argc = argc;
  args.argv = argv;
  args.log_level = spdlog::level::info;

  if (vm.count("help") != 0U) {
    std::cout << desc << "\n";
    return 0;
  }

  if (vm.count("version") != 0U) {
    return PrintVersion();
  }

  if (vm.count("log-level") != 0U) {
    args.log_level = ParseLogLevel(vm);
  }

  return StartApplication(args);
}
