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

#include "GpgFrontend.h"
#include "GpgFrontendBuildInfo.h"
#include "core/function/GlobalSettingStation.h"

QApplication* init_qapplication(int argc, char* argv[]) {
  auto* app = new QApplication(argc, argv);
#ifndef MACOS
  app->setWindowIcon(QIcon(":gpgfrontend.png"));
#endif

#ifdef MACOS
  // support retina screen
  app->setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  // set the extra information of the build
  app->setApplicationVersion(BUILD_VERSION);
  app->setApplicationName(PROJECT_NAME);
  app->setQuitOnLastWindowClosed(true);

  // don't show icons in menus
  app->setAttribute(Qt::AA_DontShowIconsInMenus);

  // unicode in source
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));
  return app;
}

void destory_qapplication(QApplication* app) {
  app->quit();
  delete app;
}

void init_logging_system() {
  el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);
  el::Configurations defaultConf;
  defaultConf.setToDefault();

  // apply settings
  defaultConf.setGlobally(el::ConfigurationType::Format,
                          "%datetime %level [main] %func %msg");
  // apply settings no written to file
  defaultConf.setGlobally(el::ConfigurationType::ToFile, "false");

  // set the logger
  el::Loggers::reconfigureLogger("default", defaultConf);
}
