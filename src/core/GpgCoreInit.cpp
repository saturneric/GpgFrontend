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

#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <boost/date_time.hpp>

#include "GpgFunctionObject.h"
#include "core/GpgContext.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/module/ModuleManager.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunner.h"
#include "core/thread/TaskRunnerGetter.h"

namespace GpgFrontend {

/**
 * @brief setup logging system and do proper initialization
 *
 */
void InitCoreLoggingSystem() {
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  // get the log directory
  auto logfile_path =
      (GlobalSettingStation::GetInstance().GetLogDir() / "core");
  logfile_path.replace_extension(".log");

  // sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
  sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      logfile_path.u8string(), 1048576 * 32, 8));

  // thread pool
  spdlog::init_thread_pool(1024, 2);

  // logger
  auto core_logger = std::make_shared<spdlog::async_logger>(
      "core", begin(sinks), end(sinks), spdlog::thread_pool());
  core_logger->set_pattern(
      "[%H:%M:%S.%e] [T:%t] [%=6n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

#ifdef DEBUG
  core_logger->set_level(spdlog::level::trace);
#else
  core_logger->set_level(spdlog::level::info);
#endif

  // flush policy
#ifdef DEBUG
  core_logger->flush_on(spdlog::level::debug);
#else
  core_logger->flush_on(spdlog::level::err);
#endif
  spdlog::flush_every(std::chrono::seconds(5));

  // register it as default logger
  spdlog::set_default_logger(core_logger);
}

void ShutdownCoreLoggingSystem() {
#ifdef WINDOWS
  // Under VisualStudio, this must be called before main finishes to workaround
  // a known VS issue
  spdlog::drop_all();
  spdlog::shutdown();
#endif
}

void ResetGpgFrontendCore() { reset_gpgfrontend_core(); }

void InitGpgFrontendCore() {
  /* Initialize the locale environment. */
  SPDLOG_DEBUG("locale: {}", setlocale(LC_CTYPE, nullptr));
  // init gpgme subsystem
  gpgme_check_version(nullptr);
  gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
#ifdef LC_MESSAGES
  gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif

  // read settings
  bool forbid_all_gnupg_connection =
      GlobalSettingStation::GetInstance().LookupSettings(
          "network.forbid_all_gnupg_connection", false);

  bool auto_import_missing_key =
      GlobalSettingStation::GetInstance().LookupSettings(
          "network.auto_import_missing_key", false);

  bool use_custom_key_database_path =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.use_custom_key_database_path", false);

  std::string custom_key_database_path =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.custom_key_database_path", std::string{});

  bool use_custom_gnupg_install_path =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.use_custom_gnupg_install_path", false);

  std::string custom_gnupg_install_path =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.custom_gnupg_install_path", std::string{});

  bool use_pinentry_as_password_input_dialog =
      GpgFrontend::GlobalSettingStation::GetInstance().LookupSettings(
          "general.use_pinentry_as_password_input_dialog", false);

  SPDLOG_DEBUG("core loaded if use custom key databse path: {}",
               use_custom_key_database_path);
  SPDLOG_DEBUG("core loaded custom key databse path: {}",
               custom_key_database_path);

  // check gpgconf path
  std::filesystem::path custom_gnupg_install_fs_path =
      custom_gnupg_install_path;
#ifdef WINDOWS
  custom_gnupg_install_fs_path /= "gpgconf.exe";
#else
  custom_gnupg_install_fs_path /= "gpgconf";
#endif

  if (!custom_gnupg_install_fs_path.is_absolute() ||
      !std::filesystem::exists(custom_gnupg_install_fs_path) ||
      !std::filesystem::is_regular_file(custom_gnupg_install_fs_path)) {
    use_custom_gnupg_install_path = false;
    SPDLOG_ERROR("core loaded custom gpgconf path is illegal: {}",
                 custom_gnupg_install_fs_path.u8string());
  } else {
    SPDLOG_DEBUG("core loaded custom gpgconf path: {}",
                 custom_gnupg_install_fs_path.u8string());
  }

  // check key database path
  std::filesystem::path custom_key_database_fs_path = custom_key_database_path;
  if (!custom_key_database_fs_path.is_absolute() ||
      !std::filesystem::exists(custom_key_database_fs_path) ||
      !std::filesystem::is_directory(custom_key_database_fs_path)) {
    use_custom_key_database_path = false;
    SPDLOG_ERROR("core loaded custom gpg key database is illegal: {}",
                 custom_key_database_fs_path.u8string());
  } else {
    SPDLOG_DEBUG("core loaded custom gpg key database path: {}",
                 custom_key_database_fs_path.u8string());
  }

  // init default channel
  auto& default_ctx = GpgFrontend::GpgContext::CreateInstance(
      GPGFRONTEND_DEFAULT_CHANNEL, [=]() -> std::unique_ptr<ChannelObject> {
        GpgFrontend::GpgContextInitArgs args;

        // set key database path
        if (use_custom_key_database_path && !custom_key_database_path.empty()) {
          args.db_path = custom_key_database_path;
        }

        if (use_custom_gnupg_install_path) {
          args.custom_gpgconf = true;
          args.custom_gpgconf_path = custom_gnupg_install_fs_path.u8string();
        }

        args.offline_mode = forbid_all_gnupg_connection;
        args.auto_import_missing_key = auto_import_missing_key;
        args.use_pinentry = use_pinentry_as_password_input_dialog;

        return std::unique_ptr<ChannelObject>(new GpgContext(args));
      });

  // exit if failed
  if (!default_ctx.good()) {
    SPDLOG_ERROR("default gnupg context init error");
  };

  // async init no-ascii channel
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(new Thread::Task(
          [=](Thread::DataObjectPtr data_obj) -> int {
            // init non-ascii channel
            auto& ctx = GpgFrontend::GpgContext::CreateInstance(
                GPGFRONTEND_NON_ASCII_CHANNEL,
                [=]() -> std::unique_ptr<ChannelObject> {
                  GpgFrontend::GpgContextInitArgs args;
                  args.ascii = false;

                  // set key database path
                  if (use_custom_key_database_path &&
                      !custom_key_database_path.empty()) {
                    args.db_path = custom_key_database_path;
                  }

                  if (use_custom_gnupg_install_path) {
                    args.custom_gpgconf = true;
                    args.custom_gpgconf_path =
                        custom_gnupg_install_fs_path.u8string();
                  }

                  args.offline_mode = forbid_all_gnupg_connection;
                  args.auto_import_missing_key = auto_import_missing_key;
                  args.use_pinentry = use_pinentry_as_password_input_dialog;

                  return std::unique_ptr<ChannelObject>(new GpgContext(args));
                });

            if (!ctx.good()) SPDLOG_ERROR("no-ascii channel init error");
            return ctx.good() ? 0 : -1;
          },
          "default_channel_ctx_init"));

  Module::ListenRTPublishEvent(
      &default_ctx,
      Module::GetRealModuleIdentifier(
          "com.bktus.gpgfrontend.module.integrated.gnupginfogathering"),
      "gnupg.gathering_done",
      [=](Module::Namespace, Module::Key, int, std::any) {
        SPDLOG_DEBUG(
            "gnupginfogathering gnupg.gathering_done changed, restarting gpg "
            "components");
        // try to restart all components
        GpgFrontend::GpgAdvancedOperator::GetInstance().RestartGpgComponents();
      });
  Module::TriggerEvent("GPGFRONTEND_CORE_INITLIZED");
}

void reset_gpgfrontend_core() { SingletonStorageCollection::GetInstance(true); }

void new_default_settings_channel(int channel) {
  GpgFrontend::GpgContext::CreateInstance(
      channel, [&]() -> std::unique_ptr<ChannelObject> {
        GpgFrontend::GpgContextInitArgs args;
        return std::unique_ptr<ChannelObject>(new GpgContext(args));
      });
}

}  // namespace GpgFrontend