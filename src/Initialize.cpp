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

#include <openssl/crypto.h>
#include <openssl/err.h>

#include <cstddef>

#include "core/GpgCoreInit.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/module/ModuleInit.h"
#include "core/module/ModuleManager.h"
#include "core/thread/TaskRunnerGetter.h"
#include "ui/GpgFrontendUIInit.h"

// main
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

void PreInit(const GFCxtWPtr &p_ctx) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) return;

  auto *app = ctx->GetApp();

#ifdef Q_OS_WINDOWS
  const auto console = app->property("GFShowConsoleOnWindows").toBool();

  if (console && AllocConsole()) {
    Q_ASSERT(freopen("CONOUT$", "w", stdout) != nullptr);
    Q_ASSERT(freopen("CONOUT$", "w", stderr) != nullptr);
    Q_ASSERT(freopen("CONIN$", "r", stdin) != nullptr);
    setvbuf(stdout, NULL, _IONBF, 0);

    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context,
                              const QString &msg) {
      auto message = qFormatLogMessage(type, context, msg);
      fprintf(stdout, "%s\n", message.toLocal8Bit().constData());
      fflush(stdout);
    });

    std::ios::sync_with_stdio(true);
  }

#endif

  // High Secure Level
  const auto secure_level = app->property("GFSecureLevel").toInt();
  if (secure_level > 1) {
    // OpenSSL Alloc 32 MB Secure Memory
    auto ret =
        CRYPTO_secure_malloc_init(static_cast<size_t>(32 * 1024 * 1024), 32);
    if (ret == 0) {
      std::array<char, 256> err_buf;
      ERR_error_string_n(ERR_get_error(), err_buf.data(), sizeof(err_buf));

      qWarning() << "Failed to initialize OpenSSL secure memory:"
                 << QString::fromLatin1(err_buf.data(), err_buf.size());

      QMessageBox::warning(
          nullptr, "Initialization Failed",
          QString("Failed to initialize OpenSSL secure memory.\n"
                  "Some secure operations may not be available.\n"
                  "Reason: %1")
              .arg(QString::fromLatin1(err_buf.data(), err_buf.size())));
    }

    Q_ASSERT(CRYPTO_secure_malloc_initialized());
  }

#ifdef RELEASE
  QLoggingCategory::setFilterRules("*.debug=false\n*.info=false\n");
  qSetMessagePattern(
      "[%{time yyyyMMdd h:mm:ss.zzz}] [%{category}] "
      "[%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-"
      "critical}C%{endif}%{if-fatal}F%{endif}] [%{threadid}] - "
      "%{message}");
#else
  QLoggingCategory::setFilterRules("*.debug=false");
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

void ShutdownGlobalBasicEnv(const GFCxtWPtr &p_ctx) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) {
    return;
  }

  // On window platform, the gpg-agent is running as a subprocess. It will be
  // closed automatically when the application is closing.
#ifndef Q_OS_WINDOWS

  auto clear_gpg_password_cache =
      GetSettings().value("basic/clear_gpg_password_cache", false).toBool();

  auto kill_all_gnupg_daemon_at_close =
      GetSettings()
          .value("gnupg/kill_all_gnupg_daemon_at_close", true)
          .toBool();

  if (ctx->unit_test_mode || kill_all_gnupg_daemon_at_close) {
    for (const auto &channel : GpgContext::GetAllChannelId()) {
      assert(GpgAdvancedOperator::GetInstance(channel).KillAllGpgComponents());
    }
  } else if (clear_gpg_password_cache) {
    for (const auto &channel : GpgContext::GetAllChannelId()) {
      assert(GpgAdvancedOperator::GetInstance(channel).ClearGpgPasswordCache());
    }
  }

#endif

  // first should shutdown the module system
  GpgFrontend::Module::ShutdownGpgFrontendModules();

  // then shutdown the core
  GpgFrontend::DestroyGpgFrontendCore();

  // deep restart mode
  if (ctx->rtn == GpgFrontend::kDeepRestartCode ||
      ctx->rtn == GpgFrontend::kCrashCode) {
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
  };
}

}  // namespace GpgFrontend
