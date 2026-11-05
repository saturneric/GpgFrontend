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

#pragma once

#include <QtQml/qqmlregistration.h>

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <memory>

#include "boot/BootContext.h"

class QQuickWindow;
class QQmlEngine;
class QJSEngine;
class QThread;

namespace GpgFrontend::Mobile {

class InteractionBridge;
struct StartupJob;

/**
 * @brief Takes the application from its first frame to a ready core.
 *
 * | State          | Thread     | What happens                                |
 * |----------------|------------|---------------------------------------------|
 * | kSHELL         | interface  | the window is up, nothing opened yet        |
 * | kOPENING_PROFILE | gf-startup | select, mount and open the profile; its   |
 * |                |            | questions go through the InteractionBridge  |
 * | kINIT_CORE     | interface  | properties copied to qApp, core started     |
 * | kREADY         | interface  | the core is fully loaded                    |
 * | kFAILED        | interface  | startup cannot go on; message says why      |
 * | kSHUTTING_DOWN | interface  | Shutdown() is running                       |
 *
 * Objects reached from the startup worker, and why that is safe:
 * - CoreSignalStation, a lazily created QObject: created here, on the
 *   interface thread, before the worker starts.
 * - The startup properties: written to AppPropertyStore from the worker,
 *   copied to qApp here when the worker has finished (MirrorToApp()).
 * - GlobalSettingStation, ProfileLoader, ProfileSession, ProfileLock and the
 *   secret store are not QObjects; the worker is the only thread using them
 *   until it has finished.
 * - PreInit() installs the Qt message handler and initialises libsodium,
 *   neither of which has thread affinity.
 * The worker's job is shared with the worker itself, so nothing it uses is
 * destroyed while it runs, however Shutdown() ends.
 */
class StartupController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(GpgFrontend::Mobile::StartupController::State state READ GetState
                 NOTIFY SignalStateChanged)
  Q_PROPERTY(int progress READ Progress NOTIFY SignalProgressChanged)
  Q_PROPERTY(QString message READ Message NOTIFY SignalStateChanged)

 public:
  enum class State {
    kSHELL,
    kOPENING_PROFILE,
    kINIT_CORE,
    kREADY,
    kFAILED,
    kSHUTTING_DOWN,
  };
  Q_ENUM(State)

  StartupController(QSharedPointer<BootContext> ctx,
                    InteractionBridge* bridge, QObject* parent = nullptr);
  ~StartupController() override;

  static void SetInstance(StartupController* instance);
  static auto create(QQmlEngine*, QJSEngine*) -> StartupController*;

  /**
   * @brief Open the profile once @p window has submitted its first frame.
   *
   * A frame submitted is not one on screen, and a window that is never
   * exposed submits none, so a fallback timer starts the work regardless.
   */
  void Start(QQuickWindow* window);

  /// Open the profile now, on the worker.
  void BeginOpening();

  /**
   * @brief Stop: cancel every question, wait for the worker, shut the core.
   *
   * @param timeout_ms how long the worker may take to finish
   * @return false when the worker did not finish in time: nothing was torn
   * down, and the caller must end the process with AbandonProcess()
   */
  auto Shutdown(int timeout_ms = 5000) -> bool;

  /// Startup cannot go on; shows @p message. Any thread.
  void ReportFatal(const QString& message);

  [[nodiscard]] auto GetState() const -> State;
  [[nodiscard]] auto Progress() const -> int;
  [[nodiscard]] auto Message() const -> QString;

  /// Exit code for a process ended by AbandonProcess() after Shutdown().
  static constexpr int kAbandonedExitCode = 70;

 signals:
  void SignalStateChanged();
  void SignalProgressChanged();
  void SignalReady();

 private:
  QSharedPointer<BootContext> ctx_;
  InteractionBridge* bridge_;
  std::shared_ptr<StartupJob> job_;
  QThread* worker_ = nullptr;
  /// Since the controller was made: what the startup log measures by.
  QElapsedTimer clock_;
  State state_ = State::kSHELL;
  int progress_ = 0;
  QString message_;
  bool begun_ = false;
  bool core_started_ = false;

  void set_state(State state);
  void on_profile_opened();
  void start_core();
  void fail(const QString& message);
};

}  // namespace GpgFrontend::Mobile
