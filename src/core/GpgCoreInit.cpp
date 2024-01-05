/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include <boost/date_time.hpp>

#include "core/function/CoreSignalStation.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/basic/ChannelObject.h"
#include "core/function/basic/SingletonStorage.h"
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/function/gpg/GpgContext.h"
#include "core/function/gpg/GpgKeyGetter.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunner.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/MemoryUtils.h"

namespace GpgFrontend {

void DestroyGpgFrontendCore() { SingletonStorageCollection::Destroy(); }

auto VerifyGpgconfPath(const std::filesystem::path& gnupg_install_fs_path)
    -> bool {
  return gnupg_install_fs_path.is_absolute() &&
         std::filesystem::exists(gnupg_install_fs_path) &&
         std::filesystem::is_regular_file(gnupg_install_fs_path);
}

auto VerifyKeyDatabasePath(const std::filesystem::path& key_database_fs_path)
    -> bool {
  return key_database_fs_path.is_absolute() &&
         std::filesystem::exists(key_database_fs_path) &&
         std::filesystem::is_directory(key_database_fs_path);
}

auto SearchGpgconfPath(const std::vector<std::string>& candidate_paths)
    -> std::filesystem::path {
  for (const auto& path : candidate_paths) {
    if (VerifyGpgconfPath(std::filesystem::path{path})) {
      return std::filesystem::path{path};
    }
  }
  return {};
}

auto SearchKeyDatabasePath(const std::vector<std::string>& candidate_paths)
    -> std::filesystem::path {
  for (const auto& path : candidate_paths) {
    GF_CORE_LOG_DEBUG("searh for candidate key database path: {}", path);
    if (VerifyKeyDatabasePath(std::filesystem::path{path})) {
      return std::filesystem::path{path};
    }
  }
  return {};
}

auto InitGpgME() -> bool {
  // init gpgme subsystem and get gpgme library version
  Module::UpsertRTValue("core", "gpgme.version",
                        std::string(gpgme_check_version(nullptr)));

  gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
#ifdef LC_MESSAGES
  gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif

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

    GF_CORE_LOG_DEBUG(
        "gpg context engine info: {} {} {} {}",
        gpgme_get_protocol_name(engine_info->protocol),
        std::string(engine_info->file_name == nullptr ? "null"
                                                      : engine_info->file_name),
        std::string(engine_info->home_dir == nullptr ? "null"
                                                     : engine_info->home_dir),
        std::string(engine_info->version ? "null" : engine_info->version));

    switch (engine_info->protocol) {
      case GPGME_PROTOCOL_OpenPGP:
        find_openpgp = true;

        Module::UpsertRTValue("core", "gpgme.engine.openpgp", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.app_path",
                              std::string(engine_info->file_name));
        Module::UpsertRTValue("core", "gpgme.ctx.gnupg_version",
                              std::string(engine_info->version));
        Module::UpsertRTValue("core", "gpgme.ctx.database_path",
                              std::string(engine_info->home_dir == nullptr
                                              ? "default"
                                              : engine_info->home_dir));
        break;
      case GPGME_PROTOCOL_CMS:
        find_cms = true;
        Module::UpsertRTValue("core", "gpgme.engine.cms", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.cms_path",
                              std::string(engine_info->file_name));

        break;
      case GPGME_PROTOCOL_GPGCONF:
        find_gpgconf = true;

        Module::UpsertRTValue("core", "gpgme.engine.gpgconf", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.gpgconf_path",
                              std::string(engine_info->file_name));
        break;
      case GPGME_PROTOCOL_ASSUAN:

        Module::UpsertRTValue("core", "gpgme.engine.assuan", 1);
        Module::UpsertRTValue("core", "gpgme.ctx.assuan_path",
                              std::string(engine_info->file_name));
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
      "core", "gpgme.ctx.gnupg_version", std::string{"0.0.0"});
  GF_CORE_LOG_DEBUG("got gnupg version from rt: {}", gnupg_version);

  // conditional check: only support gpg 2.1.x now
  if (!(CompareSoftwareVersion(gnupg_version, "2.1.0") >= 0 && find_gpgconf &&
        find_openpgp && find_cms)) {
    GF_CORE_LOG_ERROR("gpgme env check failed, abort");
    return false;
  }

  Module::UpsertRTValue("core", "env.state.gpgme", 1);
  return true;
}

void InitGpgFrontendCore(CoreInitArgs args) {
  // initialize global register table
  Module::UpsertRTValue("core", "env.state.gpgme", 0);
  Module::UpsertRTValue("core", "env.state.ctx", 0);
  Module::UpsertRTValue("core", "env.state.gnupg", 0);
  Module::UpsertRTValue("core", "env.state.basic", 0);
  Module::UpsertRTValue("core", "env.state.all", 0);

  // initialize locale environment
  GF_CORE_LOG_DEBUG("locale: {}", setlocale(LC_CTYPE, nullptr));

  // initialize library gpgme
  if (!InitGpgME()) {
    CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
        _("GpgME inilization failed"));
    return;
  }

  // start the thread to check ctx and gnupg state
  // it may take a few seconds or minutes
  GpgFrontend::Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Default)
      ->PostTask(new Thread::Task(
          [args](const DataObjectPtr&) -> int {
            // read settings from config file
            auto forbid_all_gnupg_connection =
                GlobalSettingStation::GetInstance().LookupSettings(
                    "network.forbid_all_gnupg_connection", false);

            auto auto_import_missing_key =
                GlobalSettingStation::GetInstance().LookupSettings(
                    "network.auto_import_missing_key", false);

            auto use_custom_key_database_path =
                GlobalSettingStation::GetInstance().LookupSettings(
                    "general.use_custom_key_database_path", false);

            auto custom_key_database_path =
                GlobalSettingStation::GetInstance().LookupSettings(
                    "general.custom_key_database_path", std::string{});

            auto use_custom_gnupg_install_path =
                GlobalSettingStation::GetInstance().LookupSettings(
                    "general.use_custom_gnupg_install_path", false);

            auto custom_gnupg_install_path =
                GlobalSettingStation::GetInstance().LookupSettings(
                    "general.custom_gnupg_install_path", std::string{});

            auto use_pinentry_as_password_input_dialog =
                GpgFrontend::GlobalSettingStation::GetInstance().LookupSettings(
                    "general.use_pinentry_as_password_input_dialog", false);

            GF_CORE_LOG_DEBUG("core loaded if use custom key databse path: {}",
                              use_custom_key_database_path);
            GF_CORE_LOG_DEBUG("core loaded custom key databse path: {}",
                              custom_key_database_path);

            std::filesystem::path gnupg_install_fs_path;
            // user defined
            if (!custom_gnupg_install_path.empty()) {
              // check gpgconf path
              gnupg_install_fs_path = custom_gnupg_install_path;
#ifdef WINDOWS
              custom_gnupg_install_fs_path /= "gpgconf.exe";
#else
              gnupg_install_fs_path /= "gpgconf";
#endif

              if (!VerifyGpgconfPath(gnupg_install_fs_path)) {
                use_custom_gnupg_install_path = false;
                GF_CORE_LOG_ERROR(
                    "core loaded custom gpgconf path is illegal: {}",
                    gnupg_install_fs_path.u8string());
              } else {
                GF_CORE_LOG_DEBUG("core loaded custom gpgconf path: {}",
                                  gnupg_install_fs_path.u8string());
              }
            } else {
#ifdef MACOS
              use_custom_gnupg_install_path = true;
              gnupg_install_fs_path = SearchGpgconfPath(
                  {"/usr/local/bin/gpgconf", "/opt/homebrew/bin/gpgconf"});
              GF_CORE_LOG_DEBUG("core loaded searched gpgconf path: {}",
                                gnupg_install_fs_path.u8string());
#endif
            }

            // check key database path
            std::filesystem::path key_database_fs_path;
            // user defined
            if (!custom_key_database_path.empty()) {
              key_database_fs_path = custom_key_database_path;
              if (VerifyKeyDatabasePath(key_database_fs_path)) {
                GF_CORE_LOG_ERROR(
                    "core loaded custom gpg key database is illegal: {}",
                    key_database_fs_path.u8string());
              } else {
                use_custom_key_database_path = true;
                GF_CORE_LOG_DEBUG(
                    "core loaded custom gpg key database path: {}",
                    key_database_fs_path.u8string());
              }
            } else {
#ifdef MACOS
              use_custom_key_database_path = true;
              key_database_fs_path = SearchKeyDatabasePath(
                  {QDir::home().filesystemPath() / ".gnupg"});
              GF_CORE_LOG_DEBUG("core loaded searched key database path: {}",
                                key_database_fs_path.u8string());
#endif
            }

            if (args.load_default_gpg_context) {
              // init ctx, also checking the basical env
              auto& ctx = GpgFrontend::GpgContext::CreateInstance(
                  kGpgFrontendDefaultChannel, [=]() -> ChannelObjectPtr {
                    GpgFrontend::GpgContextInitArgs args;

                    // set key database path
                    if (use_custom_key_database_path &&
                        !key_database_fs_path.empty()) {
                      args.db_path = key_database_fs_path.u8string();
                    }

                    // set custom gnupg path
                    if (use_custom_gnupg_install_path) {
                      args.custom_gpgconf = true;
                      args.custom_gpgconf_path =
                          gnupg_install_fs_path.u8string();
                    }

                    args.offline_mode = forbid_all_gnupg_connection;
                    args.auto_import_missing_key = auto_import_missing_key;
                    args.use_pinentry = use_pinentry_as_password_input_dialog;

                    return ConvertToChannelObjectPtr<>(
                        SecureCreateUniqueObject<GpgContext>(
                            args, kGpgFrontendDefaultChannel));
                  });

              // exit if failed
              if (!ctx.Good()) {
                GF_CORE_LOG_ERROR("default gnupg context init error, abort");
                CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
                    _("GpgME Context inilization failed"));
                return -1;
              }
              Module::UpsertRTValue("core", "env.state.ctx", 1);
            }

            // if gnupg-info-gathering module activated
            if (args.gather_external_gnupg_info &&
                Module::IsModuleAcivate("com.bktus.gpgfrontend.module."
                                        "integrated.gnupg-info-gathering")) {
              GF_CORE_LOG_DEBUG("gnupg-info-gathering is activated");

              // gather external gnupg info
              Module::TriggerEvent(
                  "GPGFRONTEND_CORE_INITLIZED",
                  [](const Module::EventIdentifier& /*e*/,
                     const Module::Event::ListenerIdentifier& l_id,
                     DataObjectPtr o) {
                    GF_CORE_LOG_DEBUG(
                        "received event GPGFRONTEND_CORE_INITLIZED callback "
                        "from module: {}",
                        l_id);

                    if (l_id ==
                        "com.bktus.gpgfrontend.module.integrated.gnupg-info-"
                        "gathering") {
                      GF_CORE_LOG_DEBUG(
                          "received callback from gnupg-info-gathering ");

                      // try to restart all components
                      GpgFrontend::GpgAdvancedOperator::RestartGpgComponents();
                      Module::UpsertRTValue("core", "env.state.gnupg", 1);

                      // announce that all checkings were finished
                      GF_CORE_LOG_INFO(
                          "all env checking finished, including gpgme, "
                          "ctx and gnupg");
                      Module::UpsertRTValue("core", "env.state.all", 1);
                    }
                  });
            } else {
              GF_CORE_LOG_DEBUG("gnupg-info-gathering is not activated");
              Module::UpsertRTValue("core", "env.state.all", 1);
            }

            if (args.load_default_gpg_context) {
              if (!GpgKeyGetter::GetInstance().FlushKeyCache()) {
                CoreSignalStation::GetInstance()->SignalBadGnupgEnv(
                    _("Gpg Key Detabase inilization failed"));
              };
            }
            GF_CORE_LOG_INFO(
                "basic env checking finished, including gpgme, ctx, and key "
                "infos");
            Module::UpsertRTValue("core", "env.state.basic", 1);
            CoreSignalStation::GetInstance()->SignalGoodGnupgEnv();

            return 0;
          },
          "core_init_task"));
}

}  // namespace GpgFrontend