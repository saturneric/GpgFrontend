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

#include "init.h"

#include <qcoreapplication.h>
#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <boost/date_time.hpp>
#include <filesystem>
#include <string>

#include "core/GpgCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/MemoryUtils.h"
#include "module/GpgFrontendModuleInit.h"
#include "module/sdk/Log.h"
#include "test/GpgFrontendTest.h"
#include "ui/GpgFrontendUIInit.h"

// main
#include "GpgFrontendContext.h"

namespace GpgFrontend {

#ifdef WINDOWS
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

void InitMainLoggingSystem(spdlog::level::level_enum level) {
  // sinks
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(
      SecureCreateSharedObject<spdlog::sinks::stderr_color_sink_mt>());

  // logger
  auto main_logger = SecureCreateSharedObject<spdlog::logger>(
      "main", begin(sinks), end(sinks));
  main_logger->set_pattern(
      "[%H:%M:%S.%e] [T:%t] [%=6n] %^[%=8l]%$ [%s:%#] [%!] -> %v (+%ius)");

  // set the level of logger
  main_logger->set_level(level);

#ifdef DEBUG
  // flush policy
  main_logger->flush_on(spdlog::level::trace);
#else
  // flush policy
  main_logger->flush_on(spdlog::level::err);
#endif

  spdlog::flush_every(std::chrono::seconds(3));

  // register it as default logger
  spdlog::set_default_logger(main_logger);
}

void InitLoggingSystem(const GFCxtSPtr &ctx) {
  // init the logging system for main
  InitMainLoggingSystem(ctx->log_level);

  // init the logging system for core
  InitCoreLoggingSystem(ctx->log_level);

  // shutdown the logging system for modules
  Module::LoadGpgFrontendModulesLoggingSystem(ctx->log_level);

  // init the logging system for test
  Test::InitTestLoggingSystem(ctx->log_level);

  if (ctx->load_ui_env) {
    // init the logging system for ui
    UI::InitUILoggingSystem(ctx->log_level);
  }
}

void ShutdownLoggingSystem(const GFCxtSPtr &ctx) {
  if (ctx->load_ui_env) {
    // shutdown the logging system for ui
    UI::ShutdownUILoggingSystem();
  }

  // shutdown the logging system for test
  Test::ShutdownTestLoggingSystem();

  // shutdown the logging system for modules
  Module::ShutdownGpgFrontendModulesLoggingSystem();

  // shutdown the logging system for core
  ShutdownCoreLoggingSystem();

#ifdef WINDOWS
  // Under VisualStudio, this must be called before main finishes to workaround
  // a known VS issue
  spdlog::drop_all();
  spdlog::shutdown();
#endif
}

void InitGlobalPathEnv() {
  // read settings
  bool use_custom_gnupg_install_path =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.use_custom_gnupg_install_path", false);

  std::string custom_gnupg_install_path =
      GlobalSettingStation::GetInstance().LookupSettings(
          "general.custom_gnupg_install_path", std::string{});

  // add custom gnupg install path into env $PATH
  if (use_custom_gnupg_install_path && !custom_gnupg_install_path.empty()) {
    std::string path_value = getenv("PATH");
    SPDLOG_DEBUG("Current System PATH: {}", path_value);
    setenv("PATH",
           ((std::filesystem::path{custom_gnupg_install_path}).u8string() +
            ":" + path_value)
               .c_str(),
           1);
    std::string modified_path_value = getenv("PATH");
    SPDLOG_DEBUG("Modified System PATH: {}", modified_path_value);
  }
}

void InitGlobalBasicalEnv(const GFCxtWPtr &p_ctx, bool gui_mode) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) {
    return;
  }

  // initialize logging system
  InitLoggingSystem(ctx);

  // change path to search for related
  InitGlobalPathEnv();

  ctx->InitApplication(gui_mode);

  // should load module system first
  Module::ModuleInitArgs module_init_args;
  module_init_args.log_level = ctx->log_level;
  Module::LoadGpgFrontendModules(module_init_args);

  if (ctx->load_ui_env) {
    // then preload ui
    UI::PreInitGpgFrontendUI();
  }

  CoreInitArgs core_init_args;
  core_init_args.gather_external_gnupg_info = ctx->gather_external_gnupg_info;
  core_init_args.load_default_gpg_context = ctx->load_default_gpg_context;

  // then load core
  InitGpgFrontendCore(core_init_args);
}

void ShutdownGlobalBasicalEnv(const GFCxtWPtr &p_ctx) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) {
    return;
  }

  Thread::TaskRunnerGetter::GetInstance().StopAllTeakRunner();

  ShutdownLoggingSystem(ctx);
}

}  // namespace GpgFrontend
