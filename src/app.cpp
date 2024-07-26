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

#include "GpgFrontendContext.h"
#include "core/GpgConstants.h"
#include "core/GpgCoreInit.h"
#include "core/module/ModuleInit.h"
#include "ui/GpgFrontendUIInit.h"

// main
#include "init.h"

namespace GpgFrontend {

constexpr int kCrashCode = ~0;  ///<

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
auto StartApplication(const GFCxtWPtr& p_ctx) -> int {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) {
    qWarning("cannot get gpgfrontend context.");
    return -1;
  }

  auto* app = ctx->GetApp();
  if (app == nullptr) {
    qWarning("cannot get QApplication from gpgfrontend context.");
    return -1;
  }

  /**
   * internationalization. loop to restart main window
   * with changed translation when settings change.
   */
  int return_from_event_loop_code;
  int restart_count = 0;

  do {
    // refresh locale settings
    if (restart_count > 0) InitLocale();

    // after that load ui totally
    GpgFrontend::UI::InitGpgFrontendUI(app);

    // finally create main window
    return_from_event_loop_code = GpgFrontend::UI::RunGpgFrontendUI(app);

    restart_count++;

  } while (return_from_event_loop_code == GpgFrontend::kRestartCode &&
           restart_count < 99);

  // first should shutdown the module system
  GpgFrontend::Module::ShutdownGpgFrontendModules();

  // then shutdown the core
  GpgFrontend::DestroyGpgFrontendCore();

  // deep restart mode
  if (return_from_event_loop_code == GpgFrontend::kDeepRestartCode ||
      return_from_event_loop_code == kCrashCode) {
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
  };

  // exit the program
  return return_from_event_loop_code;
}

}  // namespace GpgFrontend
