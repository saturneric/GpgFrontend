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

#include "core/GpgCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/LogUtils.h"
#include "module/GpgFrontendModuleInit.h"
#include "ui/GpgFrontendUIInit.h"

// main
#include "GpgFrontendContext.h"
#include "main.h"

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

void InitLoggingSystem(const GFCxtSPtr &ctx) {
  RegisterSyncLogger("core", ctx->log_level);

  RegisterSyncLogger("main", ctx->log_level);

  RegisterSyncLogger("module", ctx->log_level);

  if (ctx->load_ui_env) {
    // init the logging system for ui
    RegisterSyncLogger("ui", ctx->log_level);
  } else {
    RegisterSyncLogger("test", ctx->log_level);
  }
}

void InitGlobalPathEnv() {
  // read settings
  bool use_custom_gnupg_install_path =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("basic/use_custom_gnupg_install_path", false)
          .toBool();

  QString custom_gnupg_install_path =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("basic/custom_gnupg_install_path")
          .toString();

  // add custom gnupg install path into env $PATH
  if (use_custom_gnupg_install_path && !custom_gnupg_install_path.isEmpty()) {
    QString path_value = getenv("PATH");
    GF_MAIN_LOG_DEBUG("Current System PATH: {}", path_value);
    setenv("PATH",
           (QDir(custom_gnupg_install_path).absolutePath() + ":" + path_value)
               .toUtf8(),
           1);
    QString modified_path_value = getenv("PATH");
    GF_MAIN_LOG_DEBUG("Modified System PATH: {}", modified_path_value);
  }
}

void InitGlobalBasicalEnv(const GFCxtWPtr &p_ctx, bool gui_mode) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) {
    return;
  }

  // initialize logging system
  SetDefaultLogLevel(ctx->log_level);
  InitLoggingSystem(ctx);

  // change path to search for related
  InitGlobalPathEnv();

  // init application
  ctx->InitApplication();

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

  DestroyGpgFrontendCore();
}

}  // namespace GpgFrontend
