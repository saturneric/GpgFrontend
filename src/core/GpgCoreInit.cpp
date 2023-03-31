/**
 * Copyright (C) 2021 Saturneric
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
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "GpgCoreInit.h"

#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "GpgFunctionObject.h"
#include "core/GpgContext.h"
#include "core/function/GlobalSettingStation.h"
#include "function/gpg/GpgAdvancedOperator.h"
#include "spdlog/spdlog.h"
#include "thread/Task.h"
#include "thread/TaskRunner.h"
#include "thread/TaskRunnerGetter.h"

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
      "[%H:%M:%S.%e] [T:%t] [%=4n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

#ifdef DEBUG
  core_logger->set_level(spdlog::level::trace);
#else
  core_logger->set_level(spdlog::level::info);
#endif

  // flush policy
  core_logger->flush_on(spdlog::level::err);
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

void init_gpgfrontend_core() {
  /* Initialize the locale environment. */
  SPDLOG_DEBUG("locale: {}", setlocale(LC_CTYPE, nullptr));
  // init gpgme subsystem
  gpgme_check_version(nullptr);
  gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
#ifdef LC_MESSAGES
  gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif

  // get settings
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  // read settings
  bool forbid_all_gnupg_connection = false;
  try {
    forbid_all_gnupg_connection =
        settings.lookup("network.forbid_all_gnupg_connection");
  } catch (...) {
    SPDLOG_ERROR("setting operation error: forbid_all_gnupg_connection");
  }

  bool auto_import_missing_key = false;
  try {
    auto_import_missing_key =
        settings.lookup("network.auto_import_missing_key");
  } catch (...) {
    SPDLOG_ERROR("setting operation error: auto_import_missing_key");
  }

  // read from settings file
  bool use_custom_key_database_path = false;
  try {
    use_custom_key_database_path =
        settings.lookup("general.use_custom_key_database_path");
  } catch (...) {
    SPDLOG_ERROR("setting operation error: use_custom_key_database_path");
  }

  SPDLOG_DEBUG("core loaded if use custom key databse path: {}",
               use_custom_key_database_path);

  std::string custom_key_database_path;
  try {
    custom_key_database_path = static_cast<std::string>(
        settings.lookup("general.custom_key_database_path"));

  } catch (...) {
    SPDLOG_ERROR("setting operation error: custom_key_database_path");
  }

  SPDLOG_DEBUG("core loaded custom key databse path: {}",
               custom_key_database_path);

  // init default channel
  auto& default_ctx = GpgFrontend::GpgContext::CreateInstance(
      GPGFRONTEND_DEFAULT_CHANNEL, [=]() -> std::unique_ptr<ChannelObject> {
        GpgFrontend::GpgContextInitArgs args;

        // set key database path
        if (use_custom_key_database_path && !custom_key_database_path.empty()) {
          args.db_path = custom_key_database_path;
        }

        args.offline_mode = forbid_all_gnupg_connection;
        args.auto_import_missing_key = auto_import_missing_key;

        return std::unique_ptr<ChannelObject>(new GpgContext(args));
      });

  // exit if failed
  if (!default_ctx.good()) {
    SPDLOG_ERROR("default gpgme context init error, exit.");
    QCoreApplication::exit();
  };

  // async init no-ascii channel
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_GPG)
      ->PostTask(
          new Thread::Task([=](Thread::Task::DataObjectPtr data_obj) -> int {
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

                  args.offline_mode = forbid_all_gnupg_connection;
                  args.auto_import_missing_key = auto_import_missing_key;

                  return std::unique_ptr<ChannelObject>(new GpgContext(args));
                });
            if (!ctx.good()) SPDLOG_ERROR("no-ascii channel init error");

            return ctx.good() ? 0 : -1;
          }));

  // try to restart all components
  GpgFrontend::GpgAdvancedOperator::GetInstance().RestartGpgComponents();
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