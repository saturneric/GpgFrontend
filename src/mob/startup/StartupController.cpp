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

#include "startup/StartupController.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <thread>

#include "boot/Initialize.h"
#include "core/function/CoreSignalStation.h"
#include "core/profile/ProfileLoader.h"
#include "core/profile/ProfileSession.h"
#include "core/utils/BuildInfoUtils.h"
#include "core/utils/TranslationUtils.h"
#include "model/ProfilesController.h"
#include "plat/common/PlatformDisplay.h"
#include "startup/InteractionBridge.h"
#include "startup/MobileProfileLoaderDelegate.h"

namespace GpgFrontend::Mobile {

/**
 * @brief Everything the startup worker uses, owned jointly by the worker.
 *
 * Opening the profile is a sequence of blocking calls -- the lock, a PIN
 * typed by a person, Argon2 -- so it runs off the interface thread. The loader
 * stays alive as long as the job: the session it opened lives on in it.
 */
struct StartupJob {
  QSharedPointer<BootContext> ctx;
  MobileProfileLoaderDelegate delegate;

  bool ok = false;
  QString error;

  StartupJob(QSharedPointer<BootContext> context,
             InteractionBridge* bridge)
      : ctx(std::move(context)), delegate(bridge) {}

  void Run() {
    // Test seam: a job that is slow to finish, for the shutdown timeout.
    if (const auto delay =
            qEnvironmentVariableIntValue("GF_STARTUP_TEST_DELAY_MS");
        delay > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    // The profile opened last time another was chosen (ProfilesController).
    ProfilesController::ApplyChosenProfile(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    ctx->SelectProfile();
    if (!ctx->profile_error.isEmpty()) {
      error = ctx->profile_error;
      return;
    }

    // Mounted, its properties resolved, the early init done and the secret
    // store installed, then opened: what every program does.
    if (!OpenProfile(ctx, MakeProfile(ctx->profile_selection), delegate)) {
      error = ctx->fatal_error.isEmpty() ? describe_failure() : ctx->fatal_error;
      return;
    }
    ok = true;
  }

 private:
  // The desktop's words for each failure (DescribeProfileLoadError).
  [[nodiscard]] auto describe_failure() const -> QString {
    const auto failure = delegate.LastError();
    if (!failure) return QObject::tr("The profile could not be opened.");
    const auto text = DescribeProfileLoadError(*failure);
    if (!text) return QObject::tr("Opening the profile was cancelled.");
    return text->title + "\n\n" + text->text;
  }
};

namespace {

QPointer<StartupController> g_instance;

/// How long Start() waits for a first frame before opening anyway.
constexpr int kFirstFrameFallbackMs = 500;

}  // namespace

StartupController::StartupController(QSharedPointer<BootContext> ctx,
                                     InteractionBridge* bridge, QObject* parent)
    : QObject(parent), ctx_(std::move(ctx)), bridge_(bridge) {
  clock_.start();
  // Created here so it belongs to this thread; the core emits through it from
  // its own threads, and this class listens with queued connections.
  CoreSignalStation::GetInstance();
}

StartupController::~StartupController() = default;

void StartupController::SetInstance(StartupController* instance) {
  g_instance = instance;
}

auto StartupController::create(QQmlEngine*, QJSEngine*) -> StartupController* {
  QJSEngine::setObjectOwnership(g_instance, QJSEngine::CppOwnership);
  return g_instance;
}

void StartupController::Start(QQuickWindow* window) {
  qInfo() << "startup: interface loaded after" << clock_.elapsed() << "ms";
  if (window != nullptr) {
    connect(window, &QQuickWindow::frameSwapped, this,
            &StartupController::BeginOpening, Qt::SingleShotConnection);
    // The loading page is on screen: the launch screen over it can go.
    connect(window, &QQuickWindow::frameSwapped, this, &HideLaunchScreen,
            Qt::SingleShotConnection);
  } else {
    HideLaunchScreen();
  }
  QTimer::singleShot(kFirstFrameFallbackMs, this,
                     &StartupController::BeginOpening);
}

void StartupController::BeginOpening() {
  if (begun_) return;
  begun_ = true;
  set_state(State::kOPENING_PROFILE);

  job_ = std::make_shared<StartupJob>(ctx_, bridge_);
  worker_ = QThread::create([job = job_]() { job->Run(); });
  // Freed with this controller: by then Shutdown() has waited for it, or the
  // process is being abandoned and frees nothing.
  worker_->setParent(this);
  worker_->setObjectName(QStringLiteral("gf-startup"));
  connect(worker_, &QThread::finished, this,
          &StartupController::on_profile_opened);
  worker_->start();
}

void StartupController::on_profile_opened() {
  if (state_ == State::kSHUTTING_DOWN) return;

  if (!job_->ok) {
    fail(job_->error);
    return;
  }
  start_core();
}

void StartupController::start_core() {
  set_state(State::kINIT_CORE);

  auto* station = CoreSignalStation::GetInstance();
  connect(
      station, &CoreSignalStation::SignalCoreInitProgress, this,
      [this](int percent) {
        progress_ = percent;
        emit SignalProgressChanged();
      },
      Qt::QueuedConnection);
  connect(
      station, &CoreSignalStation::SignalCoreFullyLoaded, this,
      [this]() {
        if (state_ != State::kINIT_CORE) return;
        progress_ = 100;
        emit SignalProgressChanged();
        set_state(State::kREADY);
        emit SignalReady();
      },
      Qt::QueuedConnection);
  connect(
      station, &CoreSignalStation::SignalBadOpenPGPEnv, this,
      [this](BadOpenPGPEnvReason, const QString& detail) {
        fail(tr("The OpenPGP engine could not start: %1").arg(detail));
      },
      Qt::QueuedConnection);

  core_started_ = true;
  InitGlobalBasicEnv(ctx_);
  InitLocale();
  InstallTranslations(QLocale());
}

auto StartupController::Shutdown(int timeout_ms) -> bool {
  set_state(State::kSHUTTING_DOWN);
  bridge_->CancelAll();

  if (worker_ != nullptr && !worker_->wait(QDeadlineTimer(timeout_ms))) {
    // Still opening the profile -- deriving a key, say. Tearing the core down
    // under it would free what it is using; leave everything as it is.
    qWarning() << "startup worker still running after" << timeout_ms
               << "ms; abandoning shutdown";
    return false;
  }

  if (core_started_) {
    ctx_->rtn = kNonRestartCode;
    ShutdownGlobalBasicEnv(ctx_);
  } else if (ProfileSession::Loaded()) {
    // Mounted but the core never started: a PIN declined, an Open() that
    // failed, a quit before the ready signal arrived. Nothing else gives the
    // profile back, and a package's extracted tree holds its key.
    ProfileSession::Instance().Unload(ProfileUnmountMode::kNORMAL);
  }
  return true;
}

void StartupController::ReportFatal(const QString& message) {
  QMetaObject::invokeMethod(
      this, [this, message]() { fail(message); }, Qt::QueuedConnection);
}

auto StartupController::GetState() const -> State { return state_; }

auto StartupController::Progress() const -> int { return progress_; }

auto StartupController::Message() const -> QString { return message_; }

void StartupController::set_state(State state) {
  if (state_ == state) return;
  state_ = state;
  qInfo() << "startup:" << state << "after" << clock_.elapsed() << "ms";
  emit SignalStateChanged();
}

void StartupController::fail(const QString& message) {
  if (state_ == State::kSHUTTING_DOWN || state_ == State::kFAILED) return;
  message_ = message;
  qWarning().noquote() << "startup failed:" << message;
  set_state(State::kFAILED);
}

}  // namespace GpgFrontend::Mobile
