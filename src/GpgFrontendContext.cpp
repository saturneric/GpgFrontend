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

#include "GpgFrontendContext.h"

#include <qapplication.h>
#include <qcoreapplication.h>
#include <qobject.h>
#include <qthread.h>

#include "ui/GpgFrontendApplication.h"

namespace GpgFrontend {

void GpgFrontendContext::load_env_conf_set_properties() {
  auto env_config = QDir::currentPath() + "/ENV.ini";
  if (!QFileInfo(env_config).exists()) return;

  QSettings s(env_config, QSettings::IniFormat);

  property("GFSelfCheck", s.value("SelfCheck", false).toBool());
  property("GFSecureLevel", s.value("SecureLevel", 0).toInt());
  property("GFPortableMode", s.value("PortableMode", false).toBool());

  property("GFShowConsoleOnWindows",
           s.value("ShowConsoleOnWindows", false).toBool());

  qInfo() << "ENV" << "GFSelfCheck" << property("GFSelfCheck").toInt();
  qInfo() << "ENV" << "GFSecureLevel" << property("GFSecureLevel").toInt();
  qInfo() << "ENV" << "GFPortableMode" << property("GFPortableMode").toBool();

  qInfo() << "ENV" << "GFShowConsoleOnWindows"
          << property("GFShowConsoleOnWindows").toBool();
}

void GpgFrontendContext::InitApplication() {
  app_ = new UI::GpgFrontendApplication(argc, argv);

  load_env_conf_set_properties();
}

auto GpgFrontendContext::GetApp() -> QApplication* { return app_; }

GpgFrontendContext::GpgFrontendContext(int argc, char** argv)
    : argc(argc), argv(argv) {}

GpgFrontendContext::~GpgFrontendContext() {
  if (app_ != nullptr) {
    free(app_);
  }
}

auto GpgFrontendContext::property(const char* name) -> QVariant {
  if (app_ != nullptr) return app_->property(name);
  return {};
}

auto GpgFrontendContext::property(const char* name,
                                  const QVariant& value) -> bool {
  if (app_ != nullptr) return app_->setProperty(name, value);
  return false;
}
}  // namespace GpgFrontend