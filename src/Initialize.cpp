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

#include "Initialize.h"

#include <sodium.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "core/GFCoreInit.h"
#include "core/GFCoreLog.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/module/ModuleInit.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "ui/GpgFrontendUIInit.h"

// main
#include "Application.h"
#include "GpgFrontendContext.h"

namespace GpgFrontend {

#ifdef Q_OS_WINDOWS
int setenv(const char *name, const char *value, int overwrite) {
  if (!overwrite) {
    int errcode = 0;
    size_t envsize = 0;
    errcode = getenv_s(&envsize, NULL, 0, name);
    if (errcode || envsize) return errcode;
  }
  return _putenv_s(name, value);
}
#endif

namespace {}  // namespace

void PreInit(const GFCxtWPtr &p_ctx) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) return;

  auto *app = ctx->GetApp();

  const int ring_capacity =
      app->property("GFLogRingBufferCapacity").toInt() > 0
          ? app->property("GFLogRingBufferCapacity").toInt()
          : 1024;

  GFLogManager::Instance().InitRingBuffer(ring_capacity);
  qInstallMessageHandler(GFMessageHandler);

  if (sodium_init() < 0) {
    qCritical() << "Failed to initialize libsodium.";

    QMessageBox::critical(
        nullptr, "Initialization Failed",
        QString("Failed to initialize libsodium.\n"
                "Secure random generation and local encryption are not "
                "available."));

    std::_Exit(1);
  }

  const int log_level = app->property("GFLogLevel").toInt();
  QLoggingCategory::setFilterRules(BuildQtLoggingFilterRules(log_level));
  SetGFLogLevel(log_level);

#ifdef RELEASE
  qSetMessagePattern(
      "[%{time yyyyMMdd h:mm:ss.zzz}] [%{category}] "
      "[%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-"
      "critical}C%{endif}%{if-fatal}F%{endif}] [%{threadid}] - "
      "%{message}");
#else
  qSetMessagePattern(
      "[%{time yyyyMMdd h:mm:ss.zzz}] [%{category}] "
      "[%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-"
      "critical}C%{endif}%{if-fatal}F%{endif}] [%{threadid}] %{file}:%{line} - "
      "%{message}");
#endif
}

void InitGlobalPathEnv() {
  // read settings
  bool use_custom_gnupg_install_path =
      GetSettings()
          .value("gnupg/use_custom_gnupg_install_path", false)
          .toBool();

  auto custom_gnupg_install_path =
      GetSettings().value("gnupg/custom_gnupg_install_path").toString();

  // add custom gnupg install path into env $PATH
  if (use_custom_gnupg_install_path && !custom_gnupg_install_path.isEmpty()) {
    QString path_value = getenv("PATH");

    setenv("PATH",
           (QDir(custom_gnupg_install_path).absolutePath() + ":" + path_value)
               .toUtf8(),
           1);
    QString modified_path_value = getenv("PATH");
  }

  if (GetSettings().value("gnupg/enable_gpgme_debug_log", false).toBool()) {
    qputenv("GPGME_DEBUG",
            QString("9:%1").arg(QDir::currentPath() + "/gpgme.log").toUtf8());
  }
}

void InitGpgFrontendCoreAsync(CoreInitArgs core_init_args) {
  // initialize global register table of init state
  Module::UpsertRTValue("core", "env.state.gpgme", 0);
  Module::UpsertRTValue("core", "env.state.ctx", 0);
  Module::UpsertRTValue("core", "env.state.basic", 0);
  Module::UpsertRTValue("core", "env.state.all", 0);

  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(new Thread::Task(
          [core_init_args](DataObjectPtr) -> int {
            // then load core
            if (InitGpgFrontendCore(core_init_args) != 0) {
              Module::UpsertRTValue("core", "env.state.basic", -1);
              QTextStream(stdout)
                  << "Fatal: InitGpgFrontendCore() Failed" << Qt::endl;
            };
            return 0;
          },
          "core_init_task"));
}

void InitGlobalBasicEnv(const GFCxtWPtr &p_ctx, bool gui_mode) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) return;

  // Start rotating file logging as early as possible. The settings station
  // constructs on first access and creates the log directory, so from here on
  // the whole startup sequence -- including failures that leave the app stuck
  // on the loading screen -- is persisted to disk for later diagnosis (the
  // ring buffer's earlier entries are flushed in by InitFileLogger). Skipped
  // under the unit-test runner to avoid writing into the log directory.
  if (!ctx->unit_test_mode) {
    GFLogManager::Instance().InitFileLogger(
        GlobalSettingStation::GetInstance().GetAppLogPath());
  }

  // change path to search for related
  InitGlobalPathEnv();

  // should load module system first
  Module::ModuleInitArgs module_init_args;
  Module::LoadGpgFrontendModules(module_init_args);

  // then preload ui
  UI::PreInitGpgFrontendUI();

  CoreInitArgs core_init_args;
  core_init_args.gather_external_gnupg_info = ctx->gather_external_gnupg_info;
  core_init_args.unit_test_mode = ctx->unit_test_mode;

  InitGpgFrontendCoreAsync(core_init_args);

  // monitor
  StartMonitorCoreInitializationStatus();
}

/**
 * @brief setup the locale and load the translations
 *
 */
void InitLocale() {
  // get the instance of the GlobalSettingStation
  auto settings = GpgFrontend::GetSettings();

  // read from settings file
  auto lang = settings.value("basic/lang").toString();
  qInfo() << "current system default locale: " << QLocale().name();
  qInfo() << "locale settings from config: " << lang;

  auto target_locale =
      lang.trimmed().isEmpty() ? QLocale::system() : QLocale(lang);
  qInfo() << "application's target locale: " << target_locale.name();

  // initialize locale environment
  qDebug("locale info: %s",
         setlocale(LC_CTYPE, target_locale.amText().toUtf8()));
  QLocale::setDefault(target_locale);
}

void InitGlobalBasicEnvSync(const GFCxtWPtr &p_ctx) {
  QEventLoop loop;
  QCoreApplication::connect(CoreSignalStation::GetInstance(),
                            &CoreSignalStation::SignalGoodGnupgEnv, &loop,
                            &QEventLoop::quit);
  InitGlobalBasicEnv(p_ctx, false);

  auto env_state =
      Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.all", 0);
  if (env_state == 1) {
    qDebug() << "global basic env initialized before the event loop start";
    return;
  }

  loop.exec();
}

namespace {

/// Hard deadline for the whole shutdown sequence. On a healthy machine teardown
/// is sub-second; if we are still alive after this, something is wedged (most
/// commonly gpgme_release() blocked on a stuck gpg-agent on Windows).
constexpr int kShutdownWatchdogTimeoutMs = 10000;

/// Relaunch parameters captured up-front (on the main thread, while qApp is
/// still valid) so that a pending deep restart can be honoured even if teardown
/// wedges and the watchdog has to force-exit. Without this, a wedged shutdown
/// would std::_Exit() before the relaunch at the end of ShutdownGlobalBasicEnv,
/// so the app would simply vanish instead of restarting -- exactly the symptom
/// seen on Windows when several key databases each leave a slow-to-kill agent.
std::atomic<bool> g_relaunch_pending = false;
std::atomic<bool> g_relaunch_done = false;
QString g_relaunch_program;   // NOLINT(*-avoid-non-const-global-variables)
QStringList g_relaunch_args;  // NOLINT(*-avoid-non-const-global-variables)

/**
 * @brief Relaunch the application for a pending deep restart, exactly once.
 *
 * Called both from the normal end-of-teardown path and from the watchdog when
 * teardown wedges; a compare-exchange guard guarantees only the first caller
 * spawns a new instance. Safe to call from the detached watchdog thread:
 * QProcess::startDetached() does not require an event loop.
 */
void PerformDeepRestartRelaunch() {
  if (!g_relaunch_pending.load()) return;

  bool expected = false;
  if (!g_relaunch_done.compare_exchange_strong(expected, true)) return;

  if (g_relaunch_program.isEmpty()) return;

#ifdef Q_OS_MACOS
  GpgFrontend::RelaunchApplication(g_relaunch_args);
#else
  QProcess::startDetached(g_relaunch_program, g_relaunch_args);
#endif
}

/**
 * @brief Arm a detached watchdog that force-exits the process if shutdown
 * hangs.
 *
 * The GUI is already gone by the time we shut down the environment, so a wedged
 * gpg child must never be allowed to keep GpgFrontend alive as a headless
 * background process. If the deadline elapses we relaunch (when a deep restart
 * is pending) and then bypass the (possibly blocking) C++/Qt static destructors
 * and terminate immediately via std::_Exit().
 *
 * Uses no Qt/logging facilities from the timer thread since they may already be
 * torn down by the time it fires; plain stderr is safe.
 */
void ArmShutdownWatchdog(int timeout_ms) {
  std::thread([timeout_ms]() -> void {
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    std::fputs(
        "shutdown watchdog fired: teardown wedged, forcing process exit\n",
        stderr);
    std::fflush(stderr);
    // Honour a pending deep restart even though teardown wedged, otherwise the
    // process would just disappear instead of coming back.
    PerformDeepRestartRelaunch();
    std::_Exit(0);
  }).detach();
}

}  // namespace

void ShutdownGlobalBasicEnv(const GFCxtWPtr &p_ctx) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) {
    return;
  }

  // Capture relaunch parameters now, on the main thread and while qApp is still
  // valid, so a pending deep restart can be honoured even if teardown wedges
  // and the watchdog has to force-exit (see PerformDeepRestartRelaunch).
  if (ctx->rtn == GpgFrontend::kDeepRestartCode ||
      ctx->rtn == GpgFrontend::kCrashCode) {
    QStringList args = qApp->arguments();
    if (!args.isEmpty()) {
      g_relaunch_program = args.takeFirst();
      g_relaunch_args = args;
      g_relaunch_pending.store(true);
    }
  }

  // Backstop: never linger headless if teardown wedges. Not armed under the
  // unit-test runner, which has its own timeout and must fail loudly instead of
  // being force-exited with a success code.
  if (!ctx->unit_test_mode) ArmShutdownWatchdog(kShutdownWatchdogTimeoutMs);

  // Kill the gnupg daemons *before* destroying the contexts. This applies on
  // Windows too: the gpg-agent there does not exit with its parent (--daemon is
  // effectively a no-op), so a live agent would otherwise keep an assuan
  // connection open and block gpgme_release() during DestroyGpgFrontendCore().
  auto clear_gpg_password_cache =
      GetSettings().value("basic/clear_gpg_password_cache", false).toBool();

  auto kill_all_gnupg_daemon_at_close =
      GetSettings()
          .value("gnupg/kill_all_gnupg_daemon_at_close", true)
          .toBool();

  if (ctx->unit_test_mode || kill_all_gnupg_daemon_at_close) {
    for (const auto &channel : OpenPGPContext::GetAllChannelId()) {
      auto &ctx = OpenPGPContext::GetInstance(channel);
      if (ctx.Engine() != OpenPGPEngine::kGNUPG) continue;

      GpgAdvancedOperator::GetInstance(channel).KillAllGpgComponents();
    }
  } else if (clear_gpg_password_cache) {
    for (const auto &channel : OpenPGPContext::GetAllChannelId()) {
      auto &ctx = OpenPGPContext::GetInstance(channel);
      if (ctx.Engine() != OpenPGPEngine::kGNUPG) continue;

      GpgAdvancedOperator::GetInstance(channel).ClearGpgPasswordCache();
    }
  }

  qDebug() << "GpgFrontend is shutting down...";

  // first should shutdown the module system
  GpgFrontend::Module::ShutdownGpgFrontendModules();

  qDebug() << "GpgFrontend modules shutdown completed.";

  // then shutdown the core
  GpgFrontend::DestroyGpgFrontendCore();

  qInfo() << "GpgFrontend exited normally.";

  // deep restart mode: relaunch via the same helper the watchdog uses, so a
  // normal teardown and a wedged one share one code path and never both spawn.
  if (g_relaunch_pending.load()) {
    qInfo() << "relaunching application with deep restart mode, code: "
            << ctx->rtn;
    PerformDeepRestartRelaunch();
  }
}

}  // namespace GpgFrontend
