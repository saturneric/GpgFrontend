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

/**
 * \mainpage GpgFrontend Develop Document Main Page
 */

#include <qcommandlineparser.h>
#include <qloggingcategory.h>

//
#include "GpgFrontendContext.h"
#include "core/utils/MemoryUtils.h"

//
#include "app.h"
#include "cmd.h"
#include "init.h"

/**
 *
 * @param argc
 * @param argv
 * @return
 */
auto main(int argc, char* argv[]) -> int {
  // initialize qt resources
  Q_INIT_RESOURCE(gpgfrontend);

  GpgFrontend::GFCxtSPtr const ctx =
      GpgFrontend::SecureCreateSharedObject<GpgFrontend::GpgFrontendContext>(
          argc, argv);
  ctx->InitApplication();

#ifdef RELEASE
  QLoggingCategory::setFilterRules("*.debug=false\n*.info=false\n");
  qSetMessagePattern(
      "[%{time yyyyMMdd h:mm:ss.zzz}] [%{category}] "
      "[%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-"
      "critical}C%{endif}%{if-fatal}F%{endif}] [%{threadid}] - "
      "%{message}");
#else
  QLoggingCategory::setFilterRules("*.debug=false");
  qSetMessagePattern(
      "[%{time yyyyMMdd h:mm:ss.zzz}] [%{category}] "
      "[%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-"
      "critical}C%{endif}%{if-fatal}F%{endif}] [%{threadid}] %{file}:%{line} - "
      "%{message}");
#endif

  auto rtn = 0;

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOptions({
      {{"v", "version"}, "show version information"},
      {{"t", "test"}, "run all unit test cases"},
      {{"e", "environment"}, "show environment information"},
      {{"l", "log-level"}, "set log level (debug, info, warn, error)", "none"},
  });

  parser.process(*ctx->GetApp());

  if (parser.isSet("v")) {
    return GpgFrontend::PrintVersion();
  }

  if (parser.isSet("l")) {
    GpgFrontend::ParseLogLevel(parser.value("l"));
  }

  if (parser.isSet("e")) {
    return GpgFrontend::PrintEnvInfo();
  }

  if (parser.isSet("t")) {
    ctx->gather_external_gnupg_info = false;
    ctx->unit_test_mode = true;

    InitGlobalBasicEnvSync(ctx);
    rtn = RunTest(ctx);
    ShutdownGlobalBasicEnv(ctx);
    return rtn;
  }

  ctx->gather_external_gnupg_info = true;
  ctx->unit_test_mode = false;

  InitGlobalBasicEnv(ctx, true);

  rtn = StartApplication(ctx);
  ShutdownGlobalBasicEnv(ctx);
  return rtn;
}
