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

#include "core/GpgCoreInit.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/PassphraseGenerator.h"
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
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);
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
    CRYPTO_secure_malloc_init(static_cast<size_t>(32 * 1024 * 1024), 32);
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

void NewAppSecureKey(const GFBuffer &pin) {
  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  auto &gss = GlobalSettingStation::GetInstance();

  auto key = PassphraseGenerator::GenerateBytesByOpenSSL(256);
  if (!key) {
    qCritical()
        << "generate app secure key failed, using qt random generator...";
    if (secure_level > 2) {
      QMessageBox::warning(
          nullptr, QObject::tr("Secure Key Generation Failed"),
          QObject::tr(
              "Failed to generate a secure application key using OpenSSL. "
              "A less secure fallback key will be used. Please check your "
              "system's cryptography support."),
          QMessageBox::Ok);
    }
    key = GFBuffer(QRandomGenerator64::securelySeeded().generate());
  }

  // set app secure key
  gss.SetAppSecureKey(*key);

  if (secure_level > 2 && !pin.Empty()) {
    auto e_key = GFBufferFactory::Encrypt(pin, *key);
    if (!e_key) {
      qCritical() << "encrypt app secure key failed! Won't write it to disk.";
      QMessageBox::critical(
          nullptr, QObject::tr("Encrypt Key Failed"),
          QObject::tr("Failed to encrypt the secure key with your PIN. The key "
                      "will not be saved to disk."),
          QMessageBox::Ok);
      return;
    }
    key = e_key;
  }

  auto path = gss.GetAppSecureKeyPath();

  if (!GFBufferFactory::ToFile(path, *key)) {
    qCritical() << "write app secure key failed: " << path;
    if (secure_level > 2) {
      QMessageBox::critical(
          nullptr, QObject::tr("Save Key Failed"),
          QObject::tr(
              "Failed to save the secure key to disk at: %1\n"
              "Please check your storage or try running as administrator.")
              .arg(path),
          QMessageBox::Ok);
      abort();
    }
  }
}

auto InitAppSecureKey(const GFBuffer &pin) -> bool {
  const auto secure_level = qApp->property("GFSecureLevel").toInt();
  auto &gss = GlobalSettingStation::GetInstance();

  auto path = gss.GetAppSecureKeyPath();
  if (!QFileInfo(path).exists()) {
    NewAppSecureKey(pin);
    return true;
  }

  auto key = GFBufferFactory::FromFile(path);
  if (!key) {
    qCritical() << "read app secure key failed: " << path;
    if (secure_level > 2) {
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr(
              "Failed to read the application secure key from disk at: %1\n"
              "Please ensure the key file exists and is accessible, or try "
              "re-initializing the secure key.")
              .arg(path),
          QMessageBox::Ok);
    }
    return false;
  }

  // we have to decrypt the app secure key
  if (secure_level > 2 && !pin.Empty()) {
    auto r_key = GFBufferFactory::Decrypt(pin, *key);

    if (!r_key) {
      qCritical() << "decrypt app secure key failed: " << path;
      QMessageBox::critical(
          nullptr, QObject::tr("App Secure Key Error"),
          QObject::tr("Failed to decrypt the application secure key. Your PIN "
                      "may be incorrect, or the key file may be "
                      "corrupted.Please clear the secure key and try again."),
          QMessageBox::Ok);
      return false;
    }

    key = r_key;
  }

  gss.SetAppSecureKey(*key);
  return true;
}

}  // namespace GpgFrontend
