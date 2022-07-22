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

#include "GpgFrontendUIInit.h"

#include "core/function/GlobalSettingStation.h"
#include "core/thread/CtxCheckTask.h"
#include "core/thread/TaskRunnerGetter.h"
#include "ui/SignalStation.h"
#include "ui/UserInterfaceUtils.h"
#include "ui/main_window/MainWindow.h"

#if !defined(RELEASE) && defined(WINDOWS)
#include "core/function/GlobalSettingStation.h"
#endif

// init easyloggingpp library
INITIALIZE_EASYLOGGINGPP

namespace GpgFrontend::UI {

extern void init_logging_system();
extern void init_locale();

void InitGpgFrontendUI(QApplication* app) {
  // init logging system
  init_logging_system();

  // init locale
  init_locale();

#if !defined(RELEASE) && defined(WINDOWS)
  // css
  std::filesystem::path css_path =
      GpgFrontend::GlobalSettingStation::GetInstance().GetResourceDir() /
      "css" / "default.qss";
  QFile file(css_path.u8string().c_str());
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());
  qApp->setStyleSheet(styleSheet);
  file.close();
#endif

  // init signal station
  SignalStation::GetInstance();

  // init common utils
  CommonUtils::GetInstance();

  // create the thread to load the gpg context
  auto* init_ctx_task = new Thread::CtxCheckTask();

  // create and show loading window before starting the main window
  auto* waiting_dialog = new QProgressDialog();
  waiting_dialog->setMaximum(0);
  waiting_dialog->setMinimum(0);
  auto waiting_dialog_label =
      new QLabel(QString(_("Loading Gnupg Info...")) + "<br /><br />" +
                 _("If this process is too slow, please set the key "
                   "server address appropriately in the gnupg configuration "
                   "file (depending "
                   "on the network situation in your country or region)."));
  waiting_dialog_label->setWordWrap(true);
  waiting_dialog->setLabel(waiting_dialog_label);
  waiting_dialog->resize(420, 120);
  app->connect(init_ctx_task, &Thread::CtxCheckTask::SignalTaskFinished,
               waiting_dialog, [=]() {
                 LOG(INFO) << "Gpg context loaded";
                 waiting_dialog->finished(0);
                 waiting_dialog->deleteLater();
               });

  app->connect(waiting_dialog, &QProgressDialog::canceled, [=]() {
    LOG(INFO) << "cancel clicked";
    app->quit();
    exit(0);
  });

  // show the loading window
  waiting_dialog->setModal(true);
  waiting_dialog->setFocus();
  waiting_dialog->show();

  // new local event looper
  QEventLoop looper;
  app->connect(init_ctx_task, &Thread::CtxCheckTask::SignalTaskFinished,
               &looper, &QEventLoop::quit);

  // start the thread to load the gpg context
  Thread::TaskRunnerGetter::GetInstance().GetTaskRunner()->PostTask(
      init_ctx_task);

  // block the main thread until the gpg context is loaded
  looper.exec();
}

int RunGpgFrontendUI(QApplication* app) {
  // create main window and show it
  auto main_window = std::make_unique<GpgFrontend::UI::MainWindow>();
  main_window->Init();
  LOG(INFO) << "Main window inited";
  main_window->show();
  // start the main event loop
  return app->exec();
}

void init_logging_system() {
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);
  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
  el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);

  el::Configurations defaultConf;
  defaultConf.setToDefault();

  // apply settings
  defaultConf.setGlobally(el::ConfigurationType::Format,
                          "%datetime %level [ui] {%func} -> %msg");

  // apply settings no written to file
  defaultConf.setGlobally(el::ConfigurationType::ToFile, "false");

  // apply settings
  el::Loggers::reconfigureLogger("default", defaultConf)->reconfigure();

  // get the log directory
  auto logfile_path = (GlobalSettingStation::GetInstance().GetLogDir() /
                       to_iso_string(second_clock::local_time()));
  logfile_path.replace_extension(".log");
  defaultConf.setGlobally(el::ConfigurationType::Filename,
                          logfile_path.u8string());

  // apply settings written to file
  defaultConf.setGlobally(el::ConfigurationType::ToFile, "false");

  el::Loggers::reconfigureLogger("default", defaultConf);

  LOG(INFO) << _("log file path") << logfile_path;
}

/**
 * @brief setup the locale and load the translations
 *
 */
void init_locale() {
  // get the instance of the GlobalSettingStation
  auto& settings =
      GpgFrontend::GlobalSettingStation::GetInstance().GetUISettings();

  // create general settings if not exist
  if (!settings.exists("general") ||
      settings.lookup("general").getType() != libconfig::Setting::TypeGroup)
    settings.add("general", libconfig::Setting::TypeGroup);

  // set system default at first
  auto& general = settings["general"];
  if (!general.exists("lang"))
    general.add("lang", libconfig::Setting::TypeString) = "";

  // sync the settings to the file
  GpgFrontend::GlobalSettingStation::GetInstance().SyncSettings();

  LOG(INFO) << "current system locale" << setlocale(LC_ALL, nullptr);

  // read from settings file
  std::string lang;
  if (!general.lookupValue("lang", lang)) {
    LOG(ERROR) << _("could not read properly from configure file");
  };

  LOG(INFO) << "lang from settings" << lang;
  LOG(INFO) << "project name" << PROJECT_NAME;
  LOG(INFO) << "locales path"
            << GpgFrontend::GlobalSettingStation::GetInstance()
                   .GetLocaleDir()
                   .c_str();

#ifndef WINDOWS
  if (!lang.empty()) {
    std::string lc = lang.empty() ? "" : lang + ".UTF-8";

    // set LC_ALL
    auto* locale_name = setlocale(LC_ALL, lc.c_str());
    if (locale_name == nullptr) LOG(WARNING) << "set LC_ALL failed" << lc;
    auto language = getenv("LANGUAGE");
    // set LANGUAGE
    std::string language_env = language == nullptr ? "en" : language;
    language_env.insert(0, lang + ":");
    LOG(INFO) << "language env" << language_env;
    if (setenv("LANGUAGE", language_env.c_str(), 1)) {
      LOG(WARNING) << "set LANGUAGE failed" << language_env;
    };
  }
#else
  if (!lang.empty()) {
    std::string lc = lang.empty() ? "" : lang;

    // set LC_ALL
    auto* locale_name = setlocale(LC_ALL, lc.c_str());
    if (locale_name == nullptr) LOG(WARNING) << "set LC_ALL failed" << lc;

    auto language = getenv("LANGUAGE");
    // set LANGUAGE
    std::string language_env = language == nullptr ? "en" : language;
    language_env.insert(0, lang + ":");
    language_env.insert(0, "LANGUAGE=");
    LOG(INFO) << "language env" << language_env;
    if (putenv(language_env.c_str())) {
      LOG(WARNING) << "set LANGUAGE failed" << language_env;
    };
  }
#endif

  bindtextdomain(PROJECT_NAME, GpgFrontend::GlobalSettingStation::GetInstance()
                                   .GetLocaleDir()
                                   .u8string()
                                   .c_str());
  bind_textdomain_codeset(PROJECT_NAME, "utf-8");
  textdomain(PROJECT_NAME);
}

}  // namespace GpgFrontend::UI