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
 * @file main.cpp
 * @brief gf_mob: GpgFrontend for phones.
 *
 * The window comes first and is never blocked: the profile is opened on a
 * worker (StartupController), its questions answered through the
 * InteractionBridge, and the core started once it is open.
 */

#include <QtQml/qqmlextensionplugin.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include "AppServices.h"
#include "GpgFrontendResource.h"
#include "boot/BootContext.h"
#include "boot/BootLog.h"
#include "boot/Initialize.h"
#include "core/GFCoreLog.h"
#include "core/utils/BuildInfoUtils.h"

Q_IMPORT_QML_PLUGIN(GpgFrontend_MobilePlugin)

auto main(int argc, char* argv[]) -> int {
  using namespace GpgFrontend;

  QGuiApplication app(argc, argv);
  // The desktop's names: the same profile layout, the same GF_PROFILE.
  SetApplicationIdentity();
  InitResources();

  auto ctx = QSharedPointer<BootContext>::create(argc, argv);
  ctx->gather_external_gnupg_info = false;

  // What `--log-level` is on the desktop: the top layer of the log level. An
  // application started by the system has no command line, so it is read from
  // the environment (adb shell setprop / a debugger's launch settings).
  // The desktop's names (trace, debug, info, warn, error); a number, the
  // level's value, as before.
  if (const auto level = qEnvironmentVariable("GF_LOG_LEVEL");
      !level.isEmpty()) {
    bool numeric = false;
    const auto value = level.toInt(&numeric);
    const auto parsed =
        numeric ? std::optional<int>(value) : ParseLogLevelName(level);
    if (parsed) {
      ctx->cli_log_level = *parsed;
      ApplyLogLevel(*parsed);
    } else {
      qWarning() << "ignoring unrecognized GF_LOG_LEVEL:" << level;
    }
  }

  // A fatal startup error (the context's fatal_error) is shown by the
  // startup controller; nothing to relaunch: a phone application is started
  // by the system.
  Mobile::AppServices services(ctx);

  QQmlApplicationEngine engine;
  engine.loadFromModule("GpgFrontend.Mobile", "Shell");
  if (engine.rootObjects().isEmpty()) return 1;

  services.startup.Start(
      qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst()));

  const auto rtn = QGuiApplication::exec();

  if (!services.Shutdown()) {
    AbandonProcess(Mobile::StartupController::kAbandonedExitCode);
  }
  return rtn;
}
