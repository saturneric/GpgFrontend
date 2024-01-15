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

#include "ui/GpgFrontendApplication.h"

#include <QTextCodec>

#include "GpgFrontendBuildInfo.h"

namespace GpgFrontend::UI {

GpgFrontendApplication::GpgFrontendApplication(int &argc, char *argv[])
    : QApplication(argc, argv) {
#ifndef MACOS
  this->setWindowIcon(QIcon(":gpgfrontend.png"));
#endif

  // set the extra information of the build
  GpgFrontendApplication::setApplicationVersion(BUILD_VERSION);
  GpgFrontendApplication::setApplicationName(PROJECT_NAME);
  GpgFrontendApplication::setApplicationDisplayName(PROJECT_NAME);
  GpgFrontendApplication::setOrganizationName(PROJECT_NAME);
  GpgFrontendApplication::setQuitOnLastWindowClosed(true);

  // don't show icons in menus
  GpgFrontendApplication::setAttribute(Qt::AA_DontShowIconsInMenus);

  // unicode in source
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));
}

bool GpgFrontendApplication::notify(QObject *receiver, QEvent *event) {
#ifdef RELEASE
  try {
    return QApplication::notify(receiver, event);
  } catch (const std::exception &ex) {
    GF_UI_LOG_ERROR("exception was caught in notify: {}", ex.what());
    QMessageBox::information(nullptr, _("Standard Exception Thrown"),
                             _("Oops, an standard exception was thrown "
                               "during the running of the "
                               "program. This is not a serious problem, it may "
                               "be the negligence of the programmer, "
                               "please report this problem if you can."));
  } catch (...) {
    GF_UI_LOG_ERROR("unknown exception was caught in notify");
    QMessageBox::information(
        nullptr, _("Unhandled Exception Thrown"),
        _("Oops, an unhandled exception was thrown "
          "during the running of the program. This is not a "
          "serious problem, it may be the negligence of the programmer, "
          "please report this problem if you can."));
  }
  return -1;
#else
  return QApplication::notify(receiver, event);
#endif
}

}  // namespace GpgFrontend::UI
