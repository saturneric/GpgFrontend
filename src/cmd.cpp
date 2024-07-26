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

#include <qglobal.h>

#include "core/utils/BuildInfoUtils.h"

// GpgFrontend
#include "GpgFrontendContext.h"
#include "test/GpgFrontendTest.h"

namespace GpgFrontend {

auto PrintVersion() -> int {
  QTextStream stream(stdout);
  stream << PROJECT_NAME << " " << GetProjectVersion() << '\n';
  stream << "Copyright (©) 2021 Saturneric <eric@bktus.com>" << '\n'
         << QCoreApplication::tr(
                "This is free software; see the source for copying conditions.")
         << '\n'
         << '\n';

  stream << QCoreApplication::tr("Build DateTime: ")
         << QLocale().toString(GetProjectBuildTimestamp()) << '\n'
         << QCoreApplication::tr("Build Version: ") << GetProjectBuildVersion()
         << '\n'
         << QCoreApplication::tr("Source Code Version: ")
         << GetProjectBuildGitVersion() << '\n';

  stream << Qt::endl;
  return 0;
}

auto ParseLogLevel(const QString& log_level) -> int {
  if (log_level == "debug") {
    QLoggingCategory::setFilterRules(
        "core.debug=true\nui.debug=true\ntest.debug=true\nmodule.debug=true");
  } else if (log_level == "info") {
    QLoggingCategory::setFilterRules(
        "*.debug=false\ncore.info=true\nui.info=true\ntest.info="
        "true\nmodule.info=true");
  } else if (log_level == "warning") {
    QLoggingCategory::setFilterRules("*.debug=false\n*.info=false\n");
  } else if (log_level == "critical") {
    QLoggingCategory::setFilterRules(
        "*.debug=false\n*.info=false\n*.warning=false\n");
  } else {
    qWarning() << "unknown log level: " << log_level;
  }
  return 0;
}

auto RunTest(const GFCxtWPtr& p_ctx) -> int {
  GpgFrontend::GFCxtSPtr const ctx = p_ctx.lock();
  if (ctx == nullptr) {
    qWarning("cannot get gpgfrontend context for test running");
    return -1;
  }

  GpgFrontend::Test::GpgFrontendContext test_init_args;
  test_init_args.argc = ctx->argc;
  test_init_args.argv = ctx->argv;

  return GpgFrontend::Test::ExecuteAllTestCase(test_init_args);
}

}  // namespace GpgFrontend