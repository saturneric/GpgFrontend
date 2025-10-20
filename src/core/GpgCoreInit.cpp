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
#include "GpgCoreInit.h"

#include <gpgme.h>

#include "core/function/CacheManager.h"
#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/ChannelObject.h"
#include "core/function/basic/SingletonStorage.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

void DestroyGpgFrontendCore() {
  // stop all task runner
  Thread::TaskRunnerGetter::GetInstance().StopAllTeakRunner();

  CacheManager::GetInstance().FlushCacheStorage();

  // destroy all singleton objects
  SingletonStorageCollection::Destroy();
}

auto VerifyGpgconfPath(const QFileInfo& gnupg_install_fs_path) -> bool {
  return gnupg_install_fs_path.isAbsolute() && gnupg_install_fs_path.exists() &&
         gnupg_install_fs_path.isFile();
}

auto SearchGpgconfPath(const QStringList& candidate_paths) -> QString {
  for (const auto& path : candidate_paths) {
    if (VerifyGpgconfPath(QFileInfo(path))) {
      // return a unify path
      return QFileInfo(path).absoluteFilePath();
    }
  }
  return {};
}

auto GetDefaultKeyDatabasePath(const QString& gpgconf_path) -> QString {
  QString default_db_path;

  // portable mode
  if (GlobalSettingStation::GetInstance().IsProtableMode()) {
    default_db_path =
        GlobalSettingStation::GetInstance().GetAppDataPath() + "/db";
    LOG_D() << "default key db in protable mode:" << default_db_path;
  } else {
    if (gpgconf_path.isEmpty()) return {};

    QFileInfo info(gpgconf_path);
    if (!info.exists() || !info.isFile()) return {};

    auto* p = new QProcess();
    p->setProgram(info.absoluteFilePath());
    p->setArguments({"--list-dirs", "homedir"});
    p->start();

    p->waitForFinished();
    default_db_path = p->readAll().trimmed();
    p->deleteLater();
  }

  QFileInfo info(default_db_path);
  default_db_path = info.absoluteFilePath();

  // update GRT
  Module::UpsertRTValue("core", "gpgme.ctx.default_database_path",
                        default_db_path);

  return default_db_path;
}

auto InitGpgME() -> bool {
  const auto* ver = gpgme_check_version(nullptr);
  if (ver == nullptr) {
    LOG_E() << "gpgme_check_version() failed, abort...";
    return false;
    ;
  }
  Module::UpsertRTValue("core", "gpgme.version", QString(ver));

  // require gnupg version > 2.1.0
  if (gpgme_set_global_flag("require-gnupg", "2.1.0") != 0) {
    LOG_E() << "gpgme_set_global_flag() with argument 'require-gnupg' failed, "
               "abort...";
    return false;
  }

#ifdef Q_OS_WINDOWS
  auto w32spawn_dir =
      GlobalSettingStation::GetInstance().GetAppDir() + "/../gnupg/bin";
  if (gpgme_set_global_flag("w32-inst-dir",
                            w32spawn_dir.toUtf8().constData()) != 0) {
    LOG_E() << "gpgme_set_global_flag() with argument 'w32spawn_dir' failed, "
               "abort...";
    return false;
  }
#endif

  if (CheckGpgError(
          gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr))) !=
      GPG_ERR_NO_ERROR) {
    LOG_E() << "gpgme_set_locale() with argument LC_CTYPE failed, abort...";
    return false;
  }

#ifdef LC_MESSAGES
  if (CheckGpgError(gpgme_set_locale(nullptr, LC_MESSAGES,
                                     setlocale(LC_MESSAGES, nullptr))) !=
      GPG_ERR_NO_ERROR) {
    LOG_E() << "gpgme_set_locale() with argument LC_MESSAGES failed, abort...";
    return false;
  }
#endif

  gpgme_ctx_t p_ctx;
  if (CheckGpgError(gpgme_new(&p_ctx)) != GPG_ERR_NO_ERROR) {
    LOG_E() << "gpgme_new() failed, abort...";
    return false;
  }

  // get engine info
  auto* engine_info = gpgme_ctx_get_engine_info(p_ctx);
  if (engine_info == nullptr) {
    LOG_E() << "gpgme_ctx_get_engine_info() failed, abort...";
    return false;
  }

  // Check ENV before running
  bool has_gpgconf = false;
  bool has_openpgp = false;

  while (engine_info != nullptr) {
    if (strcmp(engine_info->version, "1.0.0") == 0) {
      engine_info = engine_info->next;
      continue;
    }

    switch (engine_info->protocol) {
      case GPGME_PROTOCOL_OpenPGP:
        has_openpgp = true;

        Module::UpsertRTValue("core", "gpgme.engine.openpgp", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.app_path",
                              QString(engine_info->file_name));
        Module::UpsertRTValue("core", "gpgme.ctx.gnupg_version",
                              QString(engine_info->version));
        Module::UpsertRTValue(
            "core", "gpgme.ctx.default_database_path",
            QString(engine_info->home_dir == nullptr ? ""
                                                     : engine_info->home_dir));
        break;
      case GPGME_PROTOCOL_CMS:
        Module::UpsertRTValue("core", "gpgme.engine.cms", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.cms_path",
                              QString(engine_info->file_name));

        break;
      case GPGME_PROTOCOL_GPGCONF:
        has_gpgconf = true;

        Module::UpsertRTValue("core", "gpgme.engine.gpgconf", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.gpgconf_path",
                              QString(engine_info->file_name));
        break;
      case GPGME_PROTOCOL_ASSUAN:
      case GPGME_PROTOCOL_G13:
      case GPGME_PROTOCOL_UISERVER:
      case GPGME_PROTOCOL_SPAWN:
      case GPGME_PROTOCOL_DEFAULT:
      case GPGME_PROTOCOL_UNKNOWN:
        break;
    }
    engine_info = engine_info->next;
  }

  // release gpgme context
  gpgme_release(p_ctx);

  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{"0.0.0"});
  const auto gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{});

  if (!has_gpgconf) {
    LOG_E() << "cannot get gpgconf backend engine, abort...";
    return false;
  }

  if (!has_openpgp) {
    LOG_E() << "cannot get openpgp backend engine, abort...";
    return false;
  }

  // ensure, and check twice: only support gpg > 2.1.0
  if (!(GFCompareSoftwareVersion(gnupg_version, "2.1.0") >= 0)) {
    FLOG_F("gpgme env check failed, abort");
    return false;
  }

  return true;
}

auto RefreshGpgMEBackendEngine(const QString& gpgconf_path,
                               const QString& gnupg_path,
                               const QString& home_path) -> bool {
  if (!gpgconf_path.isEmpty()) {
    auto err = CheckGpgError(gpgme_set_engine_info(
        GPGME_PROTOCOL_GPGCONF, gpgconf_path.toUtf8(), nullptr));
    if (err != GPG_ERR_NO_ERROR) {
      LOG_W() << "cannot set gpgconf path of gpgme, fallback using default "
                 "gpgconf path, target gpgconf path:"
              << gpgconf_path;
    }
  }

  if (!gnupg_path.isEmpty()) {
    auto err = CheckGpgError(gpgme_set_engine_info(
        GPGME_PROTOCOL_OpenPGP, gnupg_path.toUtf8(), home_path.toUtf8()));
    if (err != GPG_ERR_NO_ERROR) {
      LOG_W() << "cannot set gnupg path and home path of gpgme, fallback using "
                 "default gpgconf path, target gnupg path:"
              << gnupg_path << "target home path: " << home_path;
    }
  }

  return true;
}

auto GetComponentPathsByGpgConf(const QString& gpgconf_install_fs_path)
    -> bool {
  auto* process = new QProcess();
  process->setProgram(gpgconf_install_fs_path);
  process->setArguments({"--check-programs"});
  process->start();
  process->waitForFinished(30000);

  auto output_buffer = process->readAllStandardOutput();
  process->deleteLater();

  if (output_buffer.isEmpty()) return false;

  auto line_split_list = QString(output_buffer).split("\n");
  for (const auto& line : line_split_list) {
    auto info_split_list = line.split(":");

    if (info_split_list.size() != 6) continue;

    auto component_name = info_split_list[0].trimmed().toLower();
    auto component_desc = info_split_list[1].trimmed();
    auto component_path = info_split_list[2].trimmed();
    auto exists = info_split_list[3].trimmed();
    auto runnable = info_split_list[4].trimmed();

#ifdef Q_OS_WINDOWS
    // replace some special substrings on windows platform
    component_path.replace("%3a", ":");
#endif

    if (exists != "1" || runnable != "1") continue;

    QFileInfo file_info(component_path);
    if (!file_info.exists() || !file_info.isFile()) continue;

    Module::UpsertRTValue(
        "core", QString("gnupg.components.%1.checked").arg(component_name), 1);
    Module::UpsertRTValue(
        "core", QString("gnupg.components.%1.path").arg(component_name),
        file_info.absoluteFilePath());

    LOG_D() << "gpg components checked: " << component_name
            << "path: " << file_info.absoluteFilePath();
  }

  return true;
}

auto DecideGpgConfPath(const QString& default_gpgconf_path) -> QString {
  auto settings = GetSettings();
  auto use_custom_gnupg_install_path =
      settings.value("gnupg/use_custom_gnupg_install_path", false).toBool();
  auto custom_gnupg_install_path =
      settings.value("gnupg/custom_gnupg_install_path", QString{}).toString();

  QString gpgconf_install_fs_path;
  // user defined
  if (use_custom_gnupg_install_path && !custom_gnupg_install_path.isEmpty()) {
    // check gpgconf path
    gpgconf_install_fs_path = custom_gnupg_install_path;
#ifdef Q_OS_WINDOWS
    gpgconf_install_fs_path += "/gpgconf.exe";
#else
    gpgconf_install_fs_path += "/gpgconf";
#endif

    if (!VerifyGpgconfPath(QFileInfo(gpgconf_install_fs_path))) {
      LOG_W() << "the gpgconf path by settings is illegal, path: "
              << gpgconf_install_fs_path;
      gpgconf_install_fs_path = "";
    }
  }

  if (gpgconf_install_fs_path.isEmpty() &&
      !default_gpgconf_path.trimmed().isEmpty()) {
    LOG_I() << "using default gpgconf path found by gpgme: "
            << default_gpgconf_path;
    return QFileInfo(default_gpgconf_path).absoluteFilePath();
  }

  // custom not found or not defined then fallback to default candidate path
  if (gpgconf_install_fs_path.isEmpty()) {
    // platform detection
#ifdef Q_OS_MACOS
    gpgconf_install_fs_path = SearchGpgconfPath(
        {"/usr/local/bin/gpgconf", "/opt/homebrew/bin/gpgconf"});
#elif defined(Q_OS_WINDOWS)
    gpgconf_install_fs_path =
        SearchGpgconfPath({"C:/Program Files (x86)/gnupg/bin/gpgconf.exe"});
#else
    // unix or linux or another platforms
    gpgconf_install_fs_path =
        SearchGpgconfPath({"/usr/local/bin/gpgconf", "/usr/bin/gpgconf"});
#endif
  }

  if (!gpgconf_install_fs_path.isEmpty()) {
    return QFileInfo(gpgconf_install_fs_path).absoluteFilePath();
  }
  return "";
}

auto DecideGnuPGPath(const QString& default_gnupg_path) -> QString {
  QFileInfo info(default_gnupg_path);

  if (default_gnupg_path.isEmpty() || !info.exists() || !info.isFile()) {
    return Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gnupg.components.gpg.path", QString{});
  }

  return default_gnupg_path;
}

auto InitBasicPath() -> bool {
  auto default_gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});

  auto default_gnupg_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});

  LOG_I() << "default gpgconf path found by gpgme: " << default_gpgconf_path;
  LOG_I() << "default gnupg path found by gpgme: " << default_gnupg_path;

  auto target_gpgconf_path = DecideGpgConfPath(default_gpgconf_path);

  if (!GetComponentPathsByGpgConf(default_gpgconf_path)) {
    LOG_E() << "Cannot get components paths by gpgconf!"
            << "GpgFrontend cannot start under this situation!";
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        QCoreApplication::tr("Cannot get Infos from GpgConf"));
    return false;
  }

  auto target_gnupg_path = default_gnupg_path;

  if (target_gpgconf_path != default_gpgconf_path) {
    target_gnupg_path = DecideGnuPGPath(target_gnupg_path);
  }

  LOG_I() << "gpgconf path used: " << target_gpgconf_path;
  LOG_I() << "gnupg path used: " << target_gnupg_path;

  if (target_gpgconf_path.isEmpty()) {
    LOG_E() << "Cannot find gpgconf!"
            << "GpgFrontend cannot start under this situation!";
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        QCoreApplication::tr("Cannot Find GpgConf"));
    return false;
  }

  if (target_gnupg_path.isEmpty()) {
    LOG_E() << "Cannot find GnuPG by gpgconf!"
            << "GpgFrontend cannot start under this situation!";
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        QCoreApplication::tr("Cannot Find GnuPG"));
    return false;
  }

  auto default_home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.default_database_path", QString{});
  if (default_home_path.trimmed().isEmpty()) {
    default_home_path = GetDefaultKeyDatabasePath(target_gpgconf_path);
  }

  LOG_I() << "home path provided by gpgconf: " << default_home_path;
  if (default_home_path.isEmpty()) {
    LOG_E() << "Cannot find default home path by gpgconf!"
            << "GpgFrontend cannot start under this situation!";
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        QCoreApplication::tr("Cannot Find Default Home Path"));
    return false;
  }

  if (!QDir(default_home_path).exists()) QDir(default_home_path).mkpath(".");

  RefreshGpgMEBackendEngine(target_gpgconf_path, target_gnupg_path,
                            default_home_path);

  Module::UpsertRTValue("core", "gpgme.ctx.gpgconf_path",
                        QString(target_gpgconf_path));
  Module::UpsertRTValue("core", "gpgme.ctx.app_path",
                        QString(target_gnupg_path));
  Module::UpsertRTValue("core", "gpgme.ctx.default_database_path",
                        QString(default_home_path));

  return true;
}

auto InitGpgFrontendCore(CoreInitArgs args) -> int {
  // initialize gpgme
  if (!InitGpgME()) {
    LOG_E() << "Oops, GpgME init failed!"
            << "GpgFrontend cannot start under this situation!";
    Module::UpsertRTValue("core", "env.state.gpgme", -1);
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        QCoreApplication::tr("GpgME Initiation Failed"));
    return -1;
  }

  Module::UpsertRTValue("core", "env.state.gpgme", 1);

  // decide gpgconf, gnupg and default home path
  if (!InitBasicPath()) {
    LOG_E() << "Oops, Basic Path init failed!"
            << "GpgFrontend cannot start under this situation!";
    return -1;
  }

  auto default_gpgconf_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gpgconf_path", QString{});
  auto default_gnupg_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.app_path", QString{});
  auto default_home_path = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.default_database_path", QString{});

  auto settings = GetSettings();

  // read settings from config file
  auto forbid_all_gnupg_connection =
      settings.value("network/forbid_all_gnupg_connection", false).toBool();

  forbid_all_gnupg_connection = forbid_all_gnupg_connection ||
                                qApp->property("GFGnuPGOfflineMode").toBool();

  LOG_D() << "forbid all gnupg network connection:"
          << forbid_all_gnupg_connection;

  auto auto_import_missing_key =
      settings.value("network/auto_import_missing_key", false).toBool();

  // force to use pinentry in flatpak container
  auto use_pinentry_as_password_input_dialog =
      IsFlatpakENV() ||
      settings.value("gnupg/use_pinentry_as_password_input_dialog", true)
          .toBool();

  // unit test mode
  if (args.unit_test_mode) {
    Module::UpsertRTValue("core", "env.state.basic", 1);
    Module::UpsertRTValue("core", "env.state.key_dbs", 1);
    CoreSignalStation::GetInstance()->SignalGoodGnupgEnv();
    LOG_I() << "Basic ENV Checking Finished";
    return 0;
  }

  auto key_dbs = GetKeyDatabaseInfoBySettings();
  assert(!key_dbs.isEmpty());

  if (key_dbs.isEmpty()) {
    LOG_E() << "Cannot find any valid key database!"
            << "GpgFrontend cannot start under this situation!";
    Module::UpsertRTValue("core", "env.state.ctx", -1);
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        QCoreApplication::tr("No valid Key Database"));
  }

  // load default context
  auto& default_ctx = GpgFrontend::GpgContext::CreateInstance(
      kGpgFrontendDefaultChannel, [=]() -> ChannelObjectPtr {
        GpgFrontend::GpgContextInitArgs args;

        if (!key_dbs.isEmpty()) {
          const auto& default_key_db_info = key_dbs.front();
          args.db_name = default_key_db_info.name;
          args.db_path = default_key_db_info.path;
        }

        args.offline_mode = forbid_all_gnupg_connection;
        args.auto_import_missing_key = auto_import_missing_key;
        args.use_pinentry = use_pinentry_as_password_input_dialog;

        LOG_D() << "gpgme default context at channel 0, key db name:"
                << args.db_name << "key db path:" << args.db_path;

        return ConvertToChannelObjectPtr<>(SecureCreateUniqueObject<GpgContext>(
            args, kGpgFrontendDefaultChannel));
      });

  if (!default_ctx.Good()) {
    LOG_E() << "Init GpgME Default Context failed!"
            << "GpgFrontend cannot start under this situation!";
    Module::UpsertRTValue("core", "env.state.ctx", -1);
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        QCoreApplication::tr("GpgME Default Context Initiation Failed"));
    return -1;
  }

  Module::UpsertRTValue("core", "env.state.ctx", 1);

  if (!GpgKeyGetter::GetInstance(kGpgFrontendDefaultChannel).FlushKeyCache()) {
    LOG_E() << "Init GpgME Default Key Database failed!"
            << "GpgFrontend cannot start under this situation!";
    Module::UpsertRTValue("core", "env.state.ctx", -1);
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        QCoreApplication::tr("Gpg Default Key Database Initiation Failed"));
    return -1;
  };

  Module::UpsertRTValue("core", "env.state.basic", 1);
  CoreSignalStation::GetInstance()->SignalGoodGnupgEnv();
  LOG_I() << "Basic ENV Checking Finished";

  auto* task = new Thread::Task(
      [=](const DataObjectPtr&) -> int {
        int channel_index = kGpgFrontendDefaultChannel + 1;
        for (int i = 1; i < key_dbs.size(); i++) {
          const auto& key_db = key_dbs[i];

          // init ctx, also checking the basic env
          auto& ctx = GpgFrontend::GpgContext::CreateInstance(
              channel_index, [=]() -> ChannelObjectPtr {
                GpgFrontend::GpgContextInitArgs args;

                // set key database path
                if (!key_db.path.isEmpty()) {
                  args.db_name = key_db.name;
                  args.db_path = key_db.path;
                }

                args.offline_mode = forbid_all_gnupg_connection;
                args.auto_import_missing_key = auto_import_missing_key;
                args.use_pinentry = use_pinentry_as_password_input_dialog;

                LOG_D() << "new gpgme context, channel" << channel_index
                        << ", key db name" << args.db_name << "key db path"
                        << args.db_path << "";

                return ConvertToChannelObjectPtr<>(
                    SecureCreateUniqueObject<GpgContext>(args, channel_index));
              });

          if (!ctx.Good()) {
            LOG_E() << "gpgme context init failed, index:" << channel_index;
            continue;
          }

          if (!GpgKeyGetter::GetInstance(ctx.GetChannel()).FlushKeyCache()) {
            LOG_E() << "gpgme context init key cache failed, index:"
                    << channel_index;
            continue;
          }

          channel_index++;
        }

        Module::UpsertRTValue("core", "env.state.key_dbs", 1);

        return 0;
      },
      "core_key_dbs_init_task");

  QObject::connect(task, &Thread::Task::SignalTaskEnd, []() {
    LOG_I() << "All Key Database(s) Initialize Finished";
  });

  // start the thread to check ctx and gnupg state
  // it may take a few seconds or minutes
  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(task);

  return 0;
}

void StartMonitorCoreInitializationStatus() {
  auto* task = new Thread::Task(
      [=](const DataObjectPtr&) -> int {
        int core_init_state = Module::RetrieveRTValueTypedOrDefault<>(
            "core", "env.state.basic", 0);
        for (;;) {
          if (core_init_state != 0) break;
          core_init_state = Module::RetrieveRTValueTypedOrDefault<>(
              "core", "env.state.basic", 0);

          LOG_D() << "monitor: core env is still initializing, waiting...";
          QThread::msleep(100);
        }

        if (core_init_state < 0) return -1;

        // waiting for module first
        bool module_init_done =
            Module::ModuleManager::GetInstance().IsAllModulesRegistered();
        for (;;) {
          if (module_init_done) break;
          module_init_done =
              Module::ModuleManager::GetInstance().IsAllModulesRegistered();

          LOG_D() << "monitor: some modules are still going to be registered, "
                     "waiting...";
          QThread::msleep(15);
        }
        LOG_D() << "monitor: good, all module are registered.";

        int key_db_init_state = Module::RetrieveRTValueTypedOrDefault<>(
            "core", "env.state.key_dbs", 0);
        for (;;) {
          if (key_db_init_state != 0) break;
          key_db_init_state = Module::RetrieveRTValueTypedOrDefault<>(
              "core", "env.state.key_dbs", 0);

          LOG_D() << "monitor: key dbs are still initializing, waiting...";
          QThread::msleep(15);
        }
        LOG_D() << "monitor: good, all key db are loaded.";

        LOG_D()
            << "monitor: core is fully initialized, sending signal to ui...";
        Module::UpsertRTValue("core", "env.state.all", 1);
        CoreSignalStation::GetInstance()->SignalCoreFullyLoaded();
        return 0;
      },
      "waiting_core_init_task");

  QObject::connect(task, &Thread::Task::SignalTaskEnd, [=]() {
    LOG_D() << "monitor: monitor task ended, call back to main thead.";
  });

  // start the thread to check ctx and gnupg state
  // it may take a few seconds or minutes
  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_External_Process)
      ->PostTask(task);
}

}  // namespace GpgFrontend