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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "ui/GpgFrontendApplication.h"

#include "GpgFrontendBuildInfo.h"

namespace GpgFrontend::UI {

GpgFrontendApplication::GpgFrontendApplication(int &argc, char *argv[])
    : QApplication(argc, argv) {
#ifndef MACOS
  this->setWindowIcon(QIcon(":gpgfrontend.png"));
#endif

#ifdef MACOS
  // support retina screen
  this->setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  // set the extra information of the build
  this->setApplicationVersion(BUILD_VERSION);
  this->setApplicationName(PROJECT_NAME);
  this->setQuitOnLastWindowClosed(true);

  // don't show icons in menus
  this->setAttribute(Qt::AA_DontShowIconsInMenus);

  // unicode in source
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));
}

GpgFrontendApplication *GpgFrontendApplication::GetInstance(int argc,
                                                            char *argv[],
                                                            bool new_instance) {
  static GpgFrontendApplication *instance = nullptr;
  static int static_argc = argc;
  static char **static_argv = argv;

  if (new_instance || !instance) {
    if (instance != nullptr) {
      instance->quit();
      delete instance;
    }
    instance = new GpgFrontendApplication(static_argc, static_argv);
  }
  return instance;
}

bool GpgFrontendApplication::notify(QObject *receiver, QEvent *event) {
  bool app_done = true;
  try {
    app_done = QApplication::notify(receiver, event);
  } catch (const std::exception &ex) {
    LOG(INFO) << "Exception caught in notify: " << ex.what();
    QMessageBox::information(nullptr, _("Standard Exception Thrown"),
                             _("Oops, an standard exception was thrown "
                               "during the running of the "
                               "program. This is not a serious problem, it may "
                               "be the negligence of the programmer, "
                               "please report this problem if you can."));
  } catch (...) {
    LOG(INFO) << "Unknown exception caught in notify";
    QMessageBox::information(
        nullptr, _("Unhandled Exception Thrown"),
        _("Oops, an unhandled exception was thrown "
          "during the running of the program. This is not a "
          "serious problem, it may be the negligence of the programmer, "
          "please report this problem if you can."));
  }
  return app_done;
}

}  // namespace GpgFrontend::UI
