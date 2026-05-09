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

#include "core/GFLog.h"
#include "ui/GpgFrontendApplication.h"

namespace {
auto BoolText(bool value) -> QString {
  return value ? QStringLiteral("true") : QStringLiteral("false");
}

auto DisplayPath(const QString& path) -> QString {
  return path.isEmpty() ? QStringLiteral("<empty>") : path;
}
}  // namespace

namespace GpgFrontend {

void GpgFrontendContext::load_env_conf_set_properties() {
  auto env_config = QDir::currentPath() + "/ENV.ini";
  if (!QFileInfo(env_config).exists()) {
    qInfo() << "No ENV.ini found, skipping loading environment config.";
    return;
  }

  QSettings s(env_config, QSettings::IniFormat);

  property("GFSelfCheck", s.value("SelfCheck", false).toBool());
  property("GFSecureLevel", s.value("SecureLevel", 0).toInt());
  property(
      "GFLogLevel",
      s.value("LogLevel", static_cast<int>(GFLogLevel::kCRITICAL)).toInt());
  property("GFPortableMode", s.value("PortableMode", false).toBool());
  property("GFGnuPGOfflineMode", s.value("GnuPGOfflineMode", false).toBool());
  property("GFPinentryProgramPath",
           s.value("PinentryProgramPath", "").toString());
  property("GFLogRingBufferCapacity",
           s.value("LogRingBufferCapacity", 1024).toInt());

  const auto self_check = property("GFSelfCheck").toInt();
  const auto secure_level = property("GFSecureLevel").toInt();
  const auto log_level = property("GFLogLevel").toInt();
  const auto portable_mode = property("GFPortableMode").toBool();
  const auto gpg_offline_mode = property("GFGnuPGOfflineMode").toBool();
  const auto pinentry_program_path =
      property("GFPinentryProgramPath").toString();
  const auto ring_buffer_capacity = property("GFLogRingBufferCapacity").toInt();

  qInfo().noquote().nospace()
      << "\n"
      << "================ GpgFrontend Startup Environment ================\n"
      << "Self Check              : " << self_check << "\n"
      << "Secure Level            : " << secure_level << "\n"
      << "Log Level               : " << log_level << "\n"
      << "Portable Mode           : " << BoolText(portable_mode) << "\n"
      << "GnuPG Offline Mode      : " << BoolText(gpg_offline_mode) << "\n"
      << "Pinentry Program Path   : " << DisplayPath(pinentry_program_path)
      << "\n"
      << "Log Ring Buffer Capacity: " << ring_buffer_capacity << "\n"
      << "==================================================================";
}

void GpgFrontendContext::InitApplication() {
  app_ = new UI::GpgFrontendApplication(argc, argv);

  load_env_conf_set_properties();
}

auto GpgFrontendContext::GetApp() -> QApplication* { return app_; }

GpgFrontendContext::GpgFrontendContext(int argc, char** argv)
    : argc(argc), argv(argv) {}

GpgFrontendContext::~GpgFrontendContext() { delete app_; }

auto GpgFrontendContext::property(const char* name) -> QVariant {
  if (app_ != nullptr) return app_->property(name);
  return {};
}

auto GpgFrontendContext::property(const char* name, const QVariant& value)
    -> bool {
  if (app_ != nullptr) return app_->setProperty(name, value);
  return false;
}
}  // namespace GpgFrontend