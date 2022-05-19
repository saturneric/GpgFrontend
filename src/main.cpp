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

#include "core/GpgCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "ui/GpgFrontendUIInit.h"

/**
 * \brief initialize the easylogging++ library.
 */
INITIALIZE_EASYLOGGINGPP

/**
 * \brief Store the jump buff and make it possible to recover from a crash.
 */
jmp_buf recover_env;

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
 * @brief init a new instance of QApplication.
 *
 * @param argc
 * @param argv
 */
extern QApplication* init_qapplication(int argc, char* argv[]);

/**
 * @brief destroy the instance of QApplication.
 *
 * @param app
 */
extern void destory_qapplication(QApplication* app);

/**
 * @brief initialize the logging system.
 *
 */
extern void init_logging_system();

/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[]) {
  // re
  signal(SIGSEGV, handle_signal);
  signal(SIGFPE, handle_signal);
  signal(SIGILL, handle_signal);

  // clean something before exit
  atexit(before_exit);

  // initialize qt resources
  Q_INIT_RESOURCE(gpgfrontend);

  // create qt application
  auto* app = init_qapplication(argc, argv);

  // init the logging system
  init_logging_system();

  // init the logging system for core
  GpgFrontend::InitLoggingSystem();

  /**
   * internationalisation. loop to restart main window
   * with changed translation when settings change.
   */
  int return_from_event_loop_code;

  do {
#ifndef WINDOWS
    int r = sigsetjmp(recover_env, 1);
#else
    int r = setjmp(recover_env);
#endif
    if (!r) {
#ifdef RELEASE
      try {
#endif
        // renew application
        if (app == nullptr) app = init_qapplication(argc, argv);

        // init ui library
        GpgFrontend::UI::InitGpgFrontendUI(app);

        // create main window
        return_from_event_loop_code = GpgFrontend::UI::RunGpgFrontendUI(app);
#ifdef RELEASE
      } catch (...) {
        // catch all unhandled exceptions and notify the user
        QMessageBox::information(
            nullptr, _("Unhandled Exception Thrown"),
            _("Oops, an unhandled exception was thrown "
              "during the running of the "
              "program, and now it needs to be restarted. This is not a "
              "serious problem, it may be the negligence of the programmer, "
              "please report this problem if you can."));
        return_from_event_loop_code = RESTART_CODE;
      }
#endif
    } else {
      // when signal is caught, restart the main window
      QMessageBox::information(
          nullptr, _("A serious error has occurred"),
          _("Oh no! GpgFrontend caught a serious error in the software, so it "
            "needs to be restarted. If the problem recurs, please manually "
            "terminate the program and report the problem to the developer."));
      return_from_event_loop_code = RESTART_CODE;
      LOG(INFO) << "return_from_event_loop_code" << return_from_event_loop_code;
    }

    // destory the application
    if (app) {
      destory_qapplication(app);
      app = nullptr;
    }

    LOG(INFO) << "loop refresh";
  } while (return_from_event_loop_code == RESTART_CODE);

  // exit the program
  return return_from_event_loop_code;
}
