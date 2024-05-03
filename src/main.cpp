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

#include "GpgFrontendContext.h"
#include "app.h"
#include "cmd.h"
#include "init.h"

//
#include "core/utils/MemoryUtils.h"

/**
 *
 * @param argc
 * @param argv
 * @return
 */
auto main(int argc, char* argv[]) -> int {
  GpgFrontend::GFCxtSPtr const ctx =
      GpgFrontend::SecureCreateSharedObject<GpgFrontend::GpgFrontendContext>(
          argc, argv);
  ctx->InitApplication();

  auto rtn = 0;

  // initialize qt resources
  Q_INIT_RESOURCE(gpgfrontend);

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOptions({
      {{"v", "version"}, "show version information"},
      {{"t", "test"}, "run all unit test cases"},
      {{"l", "log-level"},
       "set log level (trace, debug, info, warn, error)",
       "debug"},
  });

  parser.process(*ctx->GetApp());

  ctx->log_level = spdlog::level::info;

  if (parser.isSet("v")) {
    return GpgFrontend::PrintVersion();
  }

  if (parser.isSet("l")) {
    ctx->log_level = GpgFrontend::ParseLogLevel(parser.value("l"));
  }

  if (parser.isSet("t")) {
    ctx->gather_external_gnupg_info = false;
    ctx->load_default_gpg_context = false;

    InitGlobalBasicEnv(ctx, false);
    rtn = RunTest(ctx);
    ShutdownGlobalBasicEnv(ctx);
    return rtn;
  }

  ctx->gather_external_gnupg_info = true;
  ctx->load_default_gpg_context = true;
  InitGlobalBasicEnv(ctx, true);

  rtn = StartApplication(ctx);
  ShutdownGlobalBasicEnv(ctx);
  return rtn;
}
