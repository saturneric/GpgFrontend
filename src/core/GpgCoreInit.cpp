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

#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/ChannelObject.h"
#include "core/function/basic/SingletonStorage.h"
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/model/SettingsObject.h"
#include "core/module/ModuleManager.h"
#include "core/struct/settings_object/KeyDatabaseListSO.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

void DestroyGpgFrontendCore() {
  // kill all daemon if necessary
  auto settings = GlobalSettingStation::GetInstance().GetSettings();
  auto kill_all_gnupg_daemon_at_close =
      settings.value("gnupg/kill_all_gnupg_daemon_at_close", false).toBool();
  if (kill_all_gnupg_daemon_at_close) {
    GpgAdvancedOperator::KillAllGpgComponents();
  }

  SingletonStorageCollection::Destroy();
}

auto VerifyGpgconfPath(const QFileInfo& gnupg_install_fs_path) -> bool {
  return gnupg_install_fs_path.isAbsolute() && gnupg_install_fs_path.exists() &&
         gnupg_install_fs_path.isFile();
}

auto VerifyKeyDatabasePath(const QFileInfo& key_database_fs_path) -> bool {
  return key_database_fs_path.isAbsolute() && key_database_fs_path.exists() &&
         key_database_fs_path.isDir();
}

auto SearchGpgconfPath(const QList<QString>& candidate_paths) -> QString {
  for (const auto& path : candidate_paths) {
    if (VerifyGpgconfPath(QFileInfo(path))) {
      // return a unify path
      return QFileInfo(path).absoluteFilePath();
    }
  }
  return {};
}

auto SearchKeyDatabasePath(const QList<QString>& candidate_paths) -> QString {
  for (const auto& path : candidate_paths) {
    if (VerifyKeyDatabasePath(QFileInfo(path))) {
      // return a unify path
      return QFileInfo(path).absoluteFilePath();
    }
  }
  return {};
}

auto InitGpgME(const QString& gpgconf_path, const QString& gnupg_path) -> bool {
  // init gpgme subsystem and get gpgme library version
  Module::UpsertRTValue("core", "gpgme.version",
                        QString(gpgme_check_version(nullptr)));

  gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
#ifdef LC_MESSAGES
  gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif

  if (!gpgconf_path.isEmpty()) {
    CheckGpgError(gpgme_set_engine_info(GPGME_PROTOCOL_GPGCONF,
                                        gpgconf_path.toUtf8(), nullptr));
  }

  if (!gnupg_path.isEmpty()) {
    CheckGpgError(gpgme_set_engine_info(GPGME_PROTOCOL_OpenPGP,
                                        gnupg_path.toUtf8(), nullptr));
  }

  gpgme_ctx_t p_ctx;
  CheckGpgError(gpgme_new(&p_ctx));

  // get engine info
  auto* engine_info = gpgme_ctx_get_engine_info(p_ctx);
  // Check ENV before running
  bool find_openpgp = false;
  bool find_gpgconf = false;
  bool find_cms = false;

  while (engine_info != nullptr) {
    if (strcmp(engine_info->version, "1.0.0") == 0) {
      engine_info = engine_info->next;
      continue;
    }

    switch (engine_info->protocol) {
      case GPGME_PROTOCOL_OpenPGP:
        find_openpgp = true;

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
        find_cms = true;
        Module::UpsertRTValue("core", "gpgme.engine.cms", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.cms_path",
                              QString(engine_info->file_name));

        break;
      case GPGME_PROTOCOL_GPGCONF:
        find_gpgconf = true;

        Module::UpsertRTValue("core", "gpgme.engine.gpgconf", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.gpgconf_path",
                              QString(engine_info->file_name));
        break;
      case GPGME_PROTOCOL_ASSUAN:

        Module::UpsertRTValue("core", "gpgme.engine.assuan", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.assuan_path",
                              QString(engine_info->file_name));
        break;
      case GPGME_PROTOCOL_G13:
        break;
      case GPGME_PROTOCOL_UISERVER:
        break;
      case GPGME_PROTOCOL_SPAWN:
        break;
      case GPGME_PROTOCOL_DEFAULT:
        break;
      case GPGME_PROTOCOL_UNKNOWN:
        break;
    }
    engine_info = engine_info->next;
  }

  // release gpgme context
  gpgme_release(p_ctx);

  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{"0.0.0"});

  // conditional check: only support gpg 2.1.x now
  if (!(GFCompareSoftwareVersion(gnupg_version, "2.1.0") >= 0 && find_gpgconf &&
        find_openpgp && find_cms)) {
    FLOG_W("gpgme env check failed, abort");
    return false;
  }

  Module::UpsertRTValue("core", "env.state.gpgme", 1);
  return true;
}

auto GetGnuPGPathByGpgConf(const QString& gnupg_install_fs_path) -> QString {
  auto* process = new QProcess();
  process->setProgram(gnupg_install_fs_path);
  process->start();
  process->waitForFinished(1000);
  auto output_buffer = process->readAllStandardOutput();
  process->deleteLater();

  if (output_buffer.isEmpty()) return {};

  auto line_split_list = QString(output_buffer).split("\n");
  for (const auto& line : line_split_list) {
    auto info_split_list = line.split(":");

    if (info_split_list.size() != 3) continue;

    auto component_name = info_split_list[0].trimmed();
    auto component_desc = info_split_list[1].trimmed();
    auto component_path = info_split_list[2].trimmed();

    if (component_name.toLower() == "gpg") {
#if defined(_WIN32) || defined(WIN32)
      // replace some special substrings on windows platform
      component_path.replace("%3a", ":");
#endif
      QFileInfo file_info(component_path);
      if (file_info.exists() && file_info.isFile()) {
        return file_info.absoluteFilePath();
      }
      return {};
    }
  }
  return "";
}
auto DetectGpgConfPath() -> QString {
  auto settings = GlobalSettingStation::GetInstance().GetSettings();
  auto use_custom_gnupg_install_path =
      settings.value("gnupg/use_custom_gnupg_install_path", false).toBool();
  auto custom_gnupg_install_path =
      settings.value("gnupg/custom_gnupg_install_path", QString{}).toString();

  QString gnupg_install_fs_path;
  // user defined
  if (use_custom_gnupg_install_path && !custom_gnupg_install_path.isEmpty()) {
    // check gpgconf path
    gnupg_install_fs_path = custom_gnupg_install_path;
#if defined(_WIN32) || defined(WIN32)
    gnupg_install_fs_path += "/gpgconf.exe";
#else
    gnupg_install_fs_path += "/gpgconf";
#endif

    if (!VerifyGpgconfPath(QFileInfo(gnupg_install_fs_path))) {
      LOG_W() << "core loaded custom gpgconf path is illegal: "
              << gnupg_install_fs_path;
      gnupg_install_fs_path = "";
    }
  }

  // custom not found or not defined then fallback to default candidate path
  if (gnupg_install_fs_path.isEmpty()) {
    // platform detection
#if defined(__APPLE__) && defined(__MACH__)
    gnupg_install_fs_path = SearchGpgconfPath(
        {"/usr/local/bin/gpgconf", "/opt/homebrew/bin/gpgconf"});
#elif defined(_WIN32) || defined(WIN32)
    gnupg_install_fs_path =
        SearchGpgconfPath({"C:/Program Files (x86)/gnupg/bin/gpgconf.exe"});
#else
    // unix or linux or another platforms
    gnupg_install_fs_path =
        SearchGpgconfPath({"/usr/local/bin/gpgconf", "/usr/bin/gpgconf"});
#endif
  }

  if (!gnupg_install_fs_path.isEmpty()) {
    return QFileInfo(gnupg_install_fs_path).absoluteFilePath();
  }
  return "";
}

auto DetectGnuPGPath(QString gpgconf_path) -> QString {
  return GetGnuPGPathByGpgConf(gpgconf_path);
}

void InitGpgFrontendCore(CoreInitArgs args) {
  // initialize global register table
  Module::UpsertRTValue("core", "env.state.gpgme", 0);
  Module::UpsertRTValue("core", "env.state.ctx", 0);
  Module::UpsertRTValue("core", "env.state.basic", 0);
  Module::UpsertRTValue("core", "env.state.all", 0);

  auto gpgconf_install_fs_path = DetectGpgConfPath();
  auto gnupg_install_fs_path = DetectGnuPGPath(gpgconf_install_fs_path);
  LOG_I() << "detected gpgconf path: " << gpgconf_install_fs_path;
  LOG_I() << "detected gnupg path: " << gnupg_install_fs_path;

  // initialize library gpgme
  if (!InitGpgME(gpgconf_install_fs_path, gnupg_install_fs_path)) {
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        QCoreApplication::tr("GpgME initiation failed"));
    return;
  }

  auto settings = GlobalSettingStation::GetInstance().GetSettings();

  // read settings from config file
  auto forbid_all_gnupg_connection =
      settings.value("network/forbid_all_gnupg_connection", false).toBool();

  auto auto_import_missing_key =
      settings.value("network/auto_import_missing_key", false).toBool();

  auto use_pinentry_as_password_input_dialog =
      settings
          .value("gnupg/use_pinentry_as_password_input_dialog",
                 QString::fromLocal8Bit(qgetenv("container")) != "flatpak")
          .toBool();

  // try to restart all components
  auto restart_all_gnupg_components_on_start =
      settings.value("gnupg/restart_gpg_agent_on_start", false).toBool();

  auto key_database_list =
      KeyDatabaseListSO(SettingsObject("key_database_list"));
  const auto key_databases = key_database_list.key_databases;

  auto* task = new Thread::Task(
      [=](const DataObjectPtr&) -> int {
        // key database path
        QList<KeyDatabaseItemSO> buffered_key_dbs;

        // try to use user defined key database
        if (!key_databases.empty()) {
          for (const auto& key_database : key_databases) {
            if (VerifyKeyDatabasePath(QFileInfo(key_database.path))) {
              auto key_database_fs_path =
                  QFileInfo(key_database.path).absoluteFilePath();
              LOG_D() << "load gpg key database: " << key_database.path;
              buffered_key_dbs.append(key_database);
            } else {
              LOG_W() << "gpg key database path is not suitable: "
                      << key_database.path;
            }
          }
        } else {
          QString key_database_fs_path;

#if defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
          // use user's home path by default
          key_database_fs_path =
              SearchKeyDatabasePath({QDir::home().path() + "/.gnupg"});
#endif
          if (key_database_fs_path.isEmpty()) {
            key_database_fs_path = Module::RetrieveRTValueTypedOrDefault<>(
                "core", "gpgme.ctx.default_database_path", QString{});
          }

          // add the default key database path
          if (!key_database_fs_path.isEmpty()) {
            auto so = SettingsObject("key_database_list");
            auto key_database_list = KeyDatabaseListSO(so);

            auto key_database = KeyDatabaseItemSO();
            key_database.name = "Default";
            key_database.path = key_database_fs_path;
            key_database_list.key_databases.append(key_database);
            so.Store(key_database_list.ToJson());
            buffered_key_dbs.append(key_database);
          }
        }

        if (args.load_default_gpg_context) {
          int channel_index = kGpgFrontendDefaultChannel;
          for (const auto& key_db : buffered_key_dbs) {
            // init ctx, also checking the basic env
            auto& ctx = GpgFrontend::GpgContext::CreateInstance(
                channel_index, [=]() -> ChannelObjectPtr {
                  GpgFrontend::GpgContextInitArgs args;

                  // set key database path
                  if (!key_db.path.isEmpty()) {
                    args.db_name = key_db.name;
                    args.db_path = key_db.path;
                  }

                  // set custom gnupg path
                  if (!gnupg_install_fs_path.isEmpty()) {
                    args.gpgconf_path = gnupg_install_fs_path;
                  }

                  args.offline_mode = forbid_all_gnupg_connection;
                  args.auto_import_missing_key = auto_import_missing_key;
                  args.use_pinentry = use_pinentry_as_password_input_dialog;

                  LOG_D() << "new gpgme context, channel" << channel_index
                          << ", key db name" << args.db_name << "key db path"
                          << args.db_path << "";

                  return ConvertToChannelObjectPtr<>(
                      SecureCreateUniqueObject<GpgContext>(args,
                                                           channel_index));
                });

            // exit if failed
            if (channel_index == kGpgFrontendDefaultChannel && !ctx.Good()) {
              FLOG_W() << "gnupg default context init error, key database: "
                       << key_db.name << "key database path: " << key_db.path;
              CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
                  QCoreApplication::tr("GpgME Context initiation failed"));
              return -1;
            }

            FLOG_D() << "gnupg context init success, index" << channel_index
                     << " key database: " << key_db.name
                     << "key database path: " << key_db.path;

            channel_index++;
          }
          Module::UpsertRTValue("core", "env.state.ctx", 1);
        }

        if (args.load_default_gpg_context) {
          // load keys from all key dbs
          for (int channel : GpgContext::GetAllChannelId()) {
            if (!GpgKeyGetter::GetInstance(channel).FlushKeyCache()) {
              CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
                  QCoreApplication::tr("Gpg Key Database initiation failed"));
            };
          }
        }

        FLOG_D(
            "basic env checking finished, including gpgme, ctx, and key infos");

        Module::UpsertRTValue("core", "env.state.basic", 1);
        CoreSignalStation::GetInstance()->SignalGoodGnupgEnv();

        if (restart_all_gnupg_components_on_start) {
          GpgAdvancedOperator::RestartGpgComponents();
        }

        return 0;
      },
      "core_init_task");

  QObject::connect(task, &Thread::Task::SignalTaskEnd, []() {

  });

  // start the thread to check ctx and gnupg state
  // it may take a few seconds or minutes
  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(task);
}

void StartMonitorCoreInitializationStatus() {
  auto* task = new Thread::Task(
      [=](const DataObjectPtr&) -> int {
        for (;;) {
          if (Module::RetrieveRTValueTypedOrDefault<>("core", "env.state.basic",
                                                      0)) {
            break;
          }

          LOG_D() << "monitor: core env is still initializing, waiting...";
          QThread::msleep(15);
        }

        // waiting for module first
        for (;;) {
          if (Module::ModuleManager::GetInstance().IsAllModulesRegistered())
            break;

          LOG_D() << "monitor: some modules are still going to be registered, "
                     "waiting...";
          QThread::msleep(15);
        }
        LOG_D() << "monitor: good, all module are registered.";

        LOG_D()
            << "monitor: core is fully initialized, sending signal to ui...";
        Module::UpsertRTValue("core", "env.state.all", 1);
        CoreSignalStation::GetInstance()->SignalCoreFullyLoaded();
        return 0;
      },
      "waiting_core_init_task");

  QObject::connect(task, &Thread::Task::SignalTaskEnd, [=]() {
    LOG_D() << "monitor task ended, call back to main thead.";
  });

  // start the thread to check ctx and gnupg state
  // it may take a few seconds or minutes
  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_External_Process)
      ->PostTask(task);
}

}  // namespace GpgFrontend