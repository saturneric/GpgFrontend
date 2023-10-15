/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/**
 * \mainpage GpgFrontend Develop Document Main Page
 */

#include <csetjmp>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <string>

#include "core/GpgConstants.h"
#include "core/GpgCoreInit.h"
#include "core/function/GlobalSettingStation.h"
// #include "plugin/GpgFrontendPluginInit.h"
#include "spdlog/spdlog.h"
#include "ui/GpgFrontendApplication.h"
#include "ui/GpgFrontendUIInit.h"

/**
 * \brief Store the jump buff and make it possible to recover from a crash.
 */
#ifdef FREEBSD
sigjmp_buf recover_env;
#else
jmp_buf recover_env;
#endif

constexpr int CRASH_CODE = ~0;  ///<

/**
 * @brief handle the signal SIGSEGV
 *
 * @param sig
 */
extern void handle_signal(int sig);

/**
 * @brief processes before exit the program.
 *
 */
extern void before_exit();

/**
 * @brief initialize the logging system.
 *
 */
extern void init_logging_system();

/**
 * @brief initialize the logging system.
 *
 */
extern void shutdown_logging_system();

/**
 * @brief init global PATH env
 *
 */
extern void init_global_path_env();

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[]) {
#ifdef RELEASE
  // re
  signal(SIGSEGV, handle_signal);
  signal(SIGFPE, handle_signal);
  signal(SIGILL, handle_signal);
#endif

  // clean something before exit
  atexit(before_exit);

  // initialize qt resources
  Q_INIT_RESOURCE(gpgfrontend);

  // create qt application
  auto* app =
      GpgFrontend::UI::GpgFrontendApplication::GetInstance(argc, argv, true);

  // init the logging system for main
  init_logging_system();

  // init the logging system for core
  GpgFrontend::InitCoreLoggingSystem();

  // init the logging system for ui
  GpgFrontend::UI::InitUILoggingSystem();

  // init the logging system for ui
  // GpgFrontend::Plugin::InitPluginLoggingSystem();

  // change path to search for related
  init_global_path_env();

  /**
   * internationalisation. loop to restart main window
   * with changed translation when settings change.
   */
  int return_from_event_loop_code;
  int restart_count = 0;

  do {
#ifndef WINDOWS
    int r = sigsetjmp(recover_env, 1);
#else
    int r = setjmp(recover_env);
#endif
    if (!r) {
      // init ui library
      GpgFrontend::UI::InitGpgFrontendUI(app);

      // create main window
      return_from_event_loop_code = GpgFrontend::UI::RunGpgFrontendUI(app);
    } else {
      SPDLOG_ERROR("recover from a crash");
      // when signal is caught, restart the main window
      auto* message_box = new QMessageBox(
          QMessageBox::Critical, _("A serious error has occurred"),
          _("Oh no! GpgFrontend caught a serious error in the software, so "
            "it needs to be restarted. If the problem recurs, please "
            "manually terminate the program and report the problem to the "
            "developer."),
          QMessageBox::Ok, nullptr);
      message_box->exec();
      return_from_event_loop_code = CRASH_CODE;
    }

    restart_count++;

    SPDLOG_DEBUG("restart loop refresh, event loop code: {}, restart count: {}",
                 return_from_event_loop_code, restart_count);
  } while (return_from_event_loop_code == RESTART_CODE && restart_count < 3);

  // shutdown the logging system for core
  // GpgFrontend::Plugin::ShutdownPluginLoggingSystem();

  // shutdown the logging system for ui
  GpgFrontend::UI::ShutdownUILoggingSystem();

  // shutdown the logging system for core
  GpgFrontend::ShutdownCoreLoggingSystem();

  // log for debug
  SPDLOG_INFO("GpgFrontend about to exit.");

  // deep restart mode
  if (return_from_event_loop_code == DEEP_RESTART_CODE ||
      return_from_event_loop_code == CRASH_CODE) {
    // log for debug
    SPDLOG_DEBUG(
        "deep restart or cash loop status caught, restart a new application");
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
  };

  // exit the program
  return return_from_event_loop_code;
}
