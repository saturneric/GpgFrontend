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
 * @brief gf_mob_tests: the mobile application's tests.
 *
 * Starts the way gf_mob starts -- the same StartupController, bridge and
 * lifecycle, on a QGuiApplication -- waits for a ready core, then runs every
 * test on a worker while this thread keeps the event loop going. The exit
 * code is GoogleTest's.
 *
 * With --scenario=NAME it instead acts out one startup and exits with a code
 * a test checks (StartupTest.cpp): what happens there ends a process, which a
 * test cannot do to its own.
 */

#include <QtQml/qqmlextensionplugin.h>
#include <gtest/gtest.h>

#include <QDir>
#include <QGuiApplication>
#include <QLockFile>
#include <QStandardPaths>
#include <QTimer>
#include <thread>

#include "AppServices.h"
#include "GpgFrontendResource.h"
#include "boot/BootContext.h"
#include "boot/Initialize.h"
#include "core/utils/BuildInfoUtils.h"
#include "crypto/TextCryptoController.h"

Q_IMPORT_QML_PLUGIN(GpgFrontend_MobilePlugin)

namespace {

using GpgFrontend::Mobile::AppServices;
using GpgFrontend::Mobile::InteractionBridge;
using GpgFrontend::Mobile::PassphraseBroker;
using GpgFrontend::Mobile::StartupController;
using GpgFrontend::Mobile::TextCryptoController;

auto KindName(InteractionBridge::Kind kind) -> QString {
  switch (kind) {
    case InteractionBridge::Kind::kAPP_KEY_PIN:
      return "app_key_pin";
    case InteractionBridge::Kind::kCONFIRM_KEY_RESET:
      return "confirm_key_reset";
    case InteractionBridge::Kind::kCONFIRM_FORCE_UNLOCK:
      return "confirm_force_unlock";
  }
  return {};
}

auto StateName(StartupController::State state) -> QString {
  return QString::fromLatin1(
      QMetaEnum::fromType<StartupController::State>().valueToKey(
          static_cast<int>(state)));
}

void Say(const QString& line) {
  std::fputs((line + "\n").toUtf8().constData(), stdout);
  std::fflush(stdout);
}

/// Take the profile's lock the way another process would hold it.
auto HoldProfileLock() -> std::unique_ptr<QLockFile> {
  const auto root =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir().mkpath(root);
  auto lock = std::make_unique<QLockFile>(root + "/profile.lock");
  if (!lock->tryLock(0)) Say("could not take the profile lock");
  return lock;
}

auto RunScenario(const QString& name, AppServices& services) -> int {
  auto& startup = services.startup;
  auto& bridge = services.bridge;
  QObject::connect(&startup, &StartupController::SignalStateChanged,
                   [&]() { Say("state: " + StateName(startup.GetState())); });

  if (name == "stuck-worker") {
    startup.BeginOpening();
    QTimer::singleShot(200, [&startup]() {
      if (!startup.Shutdown(500)) {
        GpgFrontend::AbandonProcess(StartupController::kAbandonedExitCode);
      }
      QCoreApplication::exit(1);  // the worker was meant to still be busy
    });
    return QCoreApplication::exec();
  }

  if (name == "passphrase-shutdown") {
    // Quit while an operation waits at a passphrase prompt: the prompt is
    // refused, the operation ends, and the core shuts down.
    TextCryptoController crypto;
    QObject::connect(&startup, &StartupController::SignalReady,
                     [&crypto]() { crypto.encrypt("message", {}); });
    QObject::connect(
        &services.passphrases, &PassphraseBroker::SignalCurrentChanged, [&]() {
          if (services.passphrases.Current().isEmpty()) return;
          Say("asked: passphrase");
          const bool finished = services.Shutdown(5000);
          Say(finished ? "shutdown: finished" : "shutdown: abandoned");
          QCoreApplication::exit(finished ? 0 : 1);
        });
    startup.BeginOpening();
    QTimer::singleShot(45000, []() {
      Say("scenario timed out");
      QCoreApplication::exit(3);
    });
    return QCoreApplication::exec();
  }

  const auto lock = HoldProfileLock();
  bool asked = false;

  if (name == "held-lock") {
    QObject::connect(&bridge, &InteractionBridge::SignalPending,
                     [&](int id, InteractionBridge::Kind kind) {
                       asked = true;
                       Say("asked: " + KindName(kind));
                       bridge.decline(id);
                     });
    QObject::connect(&startup, &StartupController::SignalStateChanged, [&]() {
      const auto state = startup.GetState();
      if (state == StartupController::State::kFAILED ||
          state == StartupController::State::kREADY) {
        QCoreApplication::exit(
            asked && state == StartupController::State::kFAILED ? 0 : 1);
      }
    });
  } else if (name == "pending-shutdown") {
    QObject::connect(
        &bridge, &InteractionBridge::SignalPending,
        [&](int, InteractionBridge::Kind kind) {
          Say("asked: " + KindName(kind));
          // Quit with the question still on screen.
          const bool finished = startup.Shutdown(5000);
          Say(finished ? "shutdown: finished" : "shutdown: abandoned");
          QCoreApplication::exit(finished ? 0 : 1);
        });
  } else {
    Say("unknown scenario " + name);
    return 2;
  }

  startup.BeginOpening();
  QTimer::singleShot(45000, []() {
    Say("scenario timed out");
    QCoreApplication::exit(3);
  });
  return QCoreApplication::exec();
}

}  // namespace

auto main(int argc, char* argv[]) -> int {
  using namespace GpgFrontend;

  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
  }

  QGuiApplication app(argc, argv);
  SetApplicationIdentity();
  InitResources();

  // Started exactly as the application is -- not in the core's unit-test
  // mode, which leaves the default key database to the core's own fixtures.
  auto ctx = QSharedPointer<BootContext>::create(argc, argv);

  Mobile::AppServices services(ctx);
  auto& startup = services.startup;

  for (const auto& arg : QCoreApplication::arguments()) {
    if (arg.startsWith("--scenario=")) {
      return RunScenario(arg.mid(11), services);
    }
  }

  // Up to a ready core first, as the application does.
  QEventLoop ready;
  QObject::connect(&startup, &Mobile::StartupController::SignalStateChanged,
                   &ready, [&]() {
                     const auto state = startup.GetState();
                     if (state == Mobile::StartupController::State::kREADY ||
                         state == Mobile::StartupController::State::kFAILED) {
                       ready.quit();
                     }
                   });
  QTimer::singleShot(60000, &ready, &QEventLoop::quit);
  startup.BeginOpening();
  ready.exec();
  if (startup.GetState() != Mobile::StartupController::State::kREADY) {
    qCritical().noquote() << "the startup did not finish:" << startup.Message();
    services.Shutdown();
    return 1;
  }

  int result = 1;
  QEventLoop done;
  std::thread tests([&]() {
    testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();
    QMetaObject::invokeMethod(&done, &QEventLoop::quit, Qt::QueuedConnection);
  });
  done.exec();
  tests.join();

  ctx->watchdog_exit_code = result;
  services.Shutdown();
  return result;
}
