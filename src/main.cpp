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
#include <cstdlib>

#include "GpgFrontendBuildInfo.h"
#include "core/GpgFunctionObject.h"
#include "ui/GpgFrontendUIInit.h"
#include "ui/main_window/MainWindow.h"

#if !defined(RELEASE) && defined(WINDOWS)
#include "core/function/GlobalSettingStation.h"
#endif

/**
 * \brief initialize the easylogging++ library.
 */
INITIALIZE_EASYLOGGINGPP

/**
 * \brief Store the jump buff and make it possible to recover from a crash.
 */
jmp_buf recover_env;

/**
 * @brief
 *
 * @param sig
 */
extern void handle_signal(int sig);

/**
 * @brief
 *
 */
extern void before_exit();

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
  QApplication app(argc, argv);

#ifndef MACOS
  QApplication::setWindowIcon(QIcon(":gpgfrontend.png"));
#endif

#ifdef MACOS
  // support retina screen
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  // set the extra information of the build
  QApplication::setApplicationVersion(BUILD_VERSION);
  QApplication::setApplicationName(PROJECT_NAME);

  // don't show icons in menus
  QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);

  // unicode in source
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

#if !defined(RELEASE) && defined(WINDOWS)
  // css
  std::filesystem::path css_path =
      GpgFrontend::GlobalSettingStation::GetInstance().GetResourceDir() /
      "css" / "default.qss";
  QFile file(css_path.u8string().c_str());
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());
  qApp->setStyleSheet(styleSheet);
  file.close();
#endif

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
        QApplication::setQuitOnLastWindowClosed(true);

        // init ui library
        GpgFrontend::UI::InitGpgFrontendUI();

        // create main window
        return_from_event_loop_code = GpgFrontend::UI::RunGpgFrontendUI();
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
        continue;
      }
#endif

    } else {
      // when signal is caught, restart the main window
      QMessageBox::information(
          nullptr, _("A serious error has occurred"),
          _("Oh no! GpgFrontend caught a serious error in the software, so it "
            "needs to be restarted. If the problem recurs, please manually "
            "terminate the program and report the problem to the developer."));
      QCoreApplication::quit();
      return_from_event_loop_code = RESTART_CODE;
      LOG(INFO) << "return_from_event_loop_code" << return_from_event_loop_code;
      continue;
    }
    LOG(INFO) << "loop refresh";
  } while (return_from_event_loop_code == RESTART_CODE);

  // exit the program
  return return_from_event_loop_code;
}
