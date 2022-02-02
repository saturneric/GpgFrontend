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
#include "gpg/GpgFunctionObject.h"
#include "ui/main_window/MainWindow.h"
#include "ui/thread/CtxCheckThread.h"

#if !defined(RELEASE) && defined(WINDOWS)
#include "ui/settings/GlobalSettingStation.h"
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
 */
extern void init_logging();

/**
 * @brief
 *
 */
extern void init_certs();

/**
 * @brief
 *
 */
extern void init_locale();

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

  // initialize logging system
  init_logging();

  // init certs for tls connection
  init_certs();

  // set the extra information of the build
  QApplication::setApplicationVersion(BUILD_VERSION);
  QApplication::setApplicationName(PROJECT_NAME);

  // don't show icons in menus
  QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);

  // unicode in source
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

#if !defined(RELEASE) && defined(WINDOWS)
  // css
  boost::filesystem::path css_path =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetResourceDir() /
      "css" / "default.qss";
  QFile file(css_path.string().c_str());
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());
  qApp->setStyleSheet(styleSheet);
  file.close();
#endif

#ifdef GPG_STANDALONE_MODE
  LOG(INFO) << "GPG_STANDALONE_MODE Enabled";
  auto gpg_path = GpgFrontend::UI::GlobalSettingStation::GetInstance()
                      .GetStandaloneGpgBinDir();
  auto db_path = GpgFrontend::UI::GlobalSettingStation::GetInstance()
                     .GetStandaloneDatabaseDir();
  GpgFrontend::GpgContext::CreateInstance(
      GpgFrontend::SingletonFunctionObject<
          GpgFrontend::GpgContext>::GetDefaultChannel(),
      std::make_unique<GpgFrontend::GpgContext>(true, db_path.string(), true,
                                                gpg_path.string()));
#endif

  // create the thread to load the gpg context
  auto* init_ctx_thread = new GpgFrontend::UI::CtxCheckThread();
  QApplication::connect(init_ctx_thread, &QThread::finished, init_ctx_thread,
                        &QThread::deleteLater);

  // create and show loading window before starting the main window
  auto* waiting_dialog = new QProgressDialog();
  waiting_dialog->setMaximum(0);
  waiting_dialog->setMinimum(0);
  auto waiting_dialog_label =
      new QLabel(QString(_("Loading Gnupg Info...")) + "<br /><br />" +
                 _("If this process is too slow, please set the key "
                   "server address appropriately in the gnupg configuration "
                   "file (depending "
                   "on the network situation in your country or region)."));
  waiting_dialog_label->setWordWrap(true);
  waiting_dialog->setLabel(waiting_dialog_label);
  waiting_dialog->resize(420, 120);
  QApplication::connect(init_ctx_thread, &QThread::finished, [=]() {
    waiting_dialog->finished(0);
    waiting_dialog->deleteLater();
  });
  QApplication::connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    LOG(INFO) << "cancel clicked";
    if (init_ctx_thread->isRunning()) init_ctx_thread->terminate();
    QCoreApplication::quit();
    exit(0);
  });

  // show the loading window
  waiting_dialog->show();
  waiting_dialog->setFocus();

  // start the thread to load the gpg context
  init_ctx_thread->start();
  QEventLoop loop;
  QApplication::connect(init_ctx_thread, &QThread::finished, &loop,
                        &QEventLoop::quit);
  loop.exec();

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
      try {
        // init the i18n support
        init_locale();

        QApplication::setQuitOnLastWindowClosed(true);

        // create main window and show it
        auto main_window = std::make_unique<GpgFrontend::UI::MainWindow>();
        main_window->init();
        main_window->show();
        return_from_event_loop_code = QApplication::exec();

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
