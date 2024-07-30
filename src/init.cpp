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

#include "init.h"

#include "core/GpgCoreInit.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/gpg/GpgAdvancedOperator.h"
#include "core/module/ModuleInit.h"
#include "core/thread/TaskRunnerGetter.h"
#include "ui/GpgFrontendUIInit.h"

// main
#include "GpgFrontendContext.h"

namespace GpgFrontend {

#if defined(_WIN32) || defined(WIN32)
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

void InitGlobalPathEnv() {
  // read settings
  bool use_custom_gnupg_install_path =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/use_custom_gnupg_install_path", false)
          .toBool();

  QString custom_gnupg_install_path =
      GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/custom_gnupg_install_path")
          .toString();

  // add custom gnupg install path into env $PATH
  if (use_custom_gnupg_install_path && !custom_gnupg_install_path.isEmpty()) {
    QString path_value = getenv("PATH");

    setenv("PATH",
           (QDir(custom_gnupg_install_path).absolutePath() + ":" + path_value)
               .toUtf8(),
           1);
    QString modified_path_value = getenv("PATH");
  }

  if (GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("gnupg/enable_gpgme_debug_log", false)
          .toBool()) {
    qputenv("GPGME_DEBUG",
            QString("9:%1").arg(QDir::currentPath() + "/gpgme.log").toUtf8());
  }
}

void InitGlobalBasicEnv(const GFCxtWPtr &p_ctx, bool gui_mode) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) {
    return;
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
  core_init_args.load_default_gpg_context = ctx->load_default_gpg_context;

  // then load core
  InitGpgFrontendCore(core_init_args);
}

/**
 * @brief setup the locale and load the translations
 *
 */
void InitLocale() {
  // get the instance of the GlobalSettingStation
  auto settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetSettings();

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

void ShutdownGlobalBasicEnv(const GFCxtWPtr &p_ctx) {
  GFCxtSPtr ctx = p_ctx.lock();
  if (ctx == nullptr) {
    return;
  }

  // clear password cache
  if (GlobalSettingStation::GetInstance()
          .GetSettings()
          .value("basic/clear_gpg_password_cache", false)
          .toBool()) {
    GpgAdvancedOperator::ClearGpgPasswordCache([](int, DataObjectPtr) {});
  }

  Thread::TaskRunnerGetter::GetInstance().StopAllTeakRunner();

  DestroyGpgFrontendCore();
}

}  // namespace GpgFrontend
