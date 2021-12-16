/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include <csetjmp>
#include <csignal>
#include <cstdlib>

#include "GpgFrontendBuildInfo.h"
#include "gpg/GpgContext.h"
#include "gpg/function/GpgKeyGetter.h"
#include "ui/MainWindow.h"
#include "ui/settings/GlobalSettingStation.h"

// Easy Logging Cpp
INITIALIZE_EASYLOGGINGPP

// Recover buff
jmp_buf recover_env;

extern void init_logging();
extern void init_locale();
extern void handle_signal(int sig);

int main(int argc, char* argv[]) {
  // Register Signals
  signal(SIGSEGV, handle_signal);
  signal(SIGFPE, handle_signal);
  signal(SIGILL, handle_signal);

  // Qt
  Q_INIT_RESOURCE(gpgfrontend);

  // Qt App
  QApplication app(argc, argv);

#ifndef MACOS
  QApplication::setWindowIcon(QIcon(":gpgfrontend.png"));
#endif

#ifdef MACOS
  // support retina screen
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  // logging system
  init_logging();

  // App config
  QApplication::setApplicationVersion(BUILD_VERSION);
  QApplication::setApplicationName(PROJECT_NAME);

  // don't show icons in menus
  QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);

  // unicode in source
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

#if defined(WINDOWS)
  // css
  boost::filesystem::path css_path = GpgFrontend::UI::GlobalSettingStation::GetInstance().GetResourceDir() / "css" / "default.qss";
  QFile file(css_path.string().c_str());
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());
  qApp->setStyleSheet(styleSheet);
  file.close();
#endif

  auto* init_ctx_thread = QThread::create([]() {
    // Create & Check Gnupg Context Status
    if (!GpgFrontend::GpgContext::GetInstance().good()) {
      QMessageBox::critical(
          nullptr, _("ENV Loading Failed"),
          _("Gnupg(gpg) is not installed correctly, please follow the "
            "ReadME "
            "instructions in Github to install Gnupg and then open "
            "GpgFrontend."));
      QCoreApplication::quit();
      exit(0);
    }
    // Try fetching key
    GpgFrontend::GpgKeyGetter::GetInstance().FetchKey();
  });

  QApplication::connect(init_ctx_thread, &QThread::finished, init_ctx_thread,
                        &QThread::deleteLater);

  // Waiting Dialog
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
  QApplication::connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    LOG(INFO) << "cancel clicked";
    init_ctx_thread->terminate();
    QCoreApplication::quit();
    exit(0);
  });

  // Show Waiting Dialog
  waiting_dialog->show();
  waiting_dialog->setFocus();

  init_ctx_thread->start();
  while (init_ctx_thread->isRunning()) {
    QApplication::processEvents();
  }
  waiting_dialog->finished(0);
  waiting_dialog->deleteLater();

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
        // i18n
        init_locale();

        QApplication::setQuitOnLastWindowClosed(true);

        auto main_window = std::make_unique<GpgFrontend::UI::MainWindow>();
        main_window->init();
        main_window->show();
        return_from_event_loop_code = QApplication::exec();

      } catch (...) {
        QMessageBox::information(
            nullptr, _("Unhandled Exception Thrown"),
            _("Oops, an unhandled exception was thrown "
              "during the running of the "
              "program, and now it needs to be restarted. This is not a "
              "serious problem, it may be the negligence of the programmer, "
              "please report this problem if you can."));
        continue;
      }

    } else {
      QMessageBox::information(
          nullptr, _("A serious error has occurred"),
          _("Oh no! GpgFrontend caught a serious error in the software, so it "
            "needs to be restarted. If the problem recurs, please manually "
            "terminate the program and report the problem to the developer."));
      return_from_event_loop_code = RESTART_CODE;
      LOG(INFO) << "return_from_event_loop_code" << return_from_event_loop_code;
      continue;
    }
    LOG(INFO) << "loop refresh";
  } while (return_from_event_loop_code == RESTART_CODE);

  return return_from_event_loop_code;
}
