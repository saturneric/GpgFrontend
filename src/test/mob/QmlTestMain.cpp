/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

/**
 * @file QmlTestMain.cpp
 * @brief gf_mob_qml_tests: the application's pages, tested in QML.
 *
 * The same plugin and singletons as the application, but no startup: the
 * pages are tested on their own, with an empty key list.
 */

#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

#include "AppServices.h"
#include "boot/BootContext.h"

Q_IMPORT_QML_PLUGIN(GpgFrontend_MobilePlugin)

class Setup : public QObject {
  Q_OBJECT

 public slots:
  void applicationAvailable() {
    services_ = std::make_unique<GpgFrontend::Mobile::AppServices>(
        QSharedPointer<GpgFrontend::BootContext>::create(0, nullptr));
  }

 private:
  std::unique_ptr<GpgFrontend::Mobile::AppServices> services_;
};

QUICK_TEST_MAIN_WITH_SETUP(gf_mob_qml_tests, Setup)

#include "QmlTestMain.moc"
