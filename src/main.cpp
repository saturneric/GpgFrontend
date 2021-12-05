/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "GpgFrontendBuildInfo.h"
#include "ui/MainWindow.h"
#include "ui/settings/GlobalSettingStation.h"

// Easy Logging Cpp
INITIALIZE_EASYLOGGINGPP

void init_logging();
void init_locale();

int main(int argc, char* argv[]) {
  // Qt
  Q_INIT_RESOURCE(gpgfrontend);

  // Qt App
  QApplication app(argc, argv);

  // logging system
  init_logging();

  // i18n
  init_locale();

  // App config
  QApplication::setApplicationVersion(BUILD_VERSION);
  QApplication::setApplicationName(PROJECT_NAME);

  // don't show icons in menus
  QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);

  // unicode in source
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));

#ifdef WINDOWS
  // css
  QFile file(RESOURCE_DIR(qApp->applicationDirPath()) + "/css/default.qss");
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());
  qApp->setStyleSheet(styleSheet);
  file.close();
#endif

  /**
   * internationalisation. loop to restart mainwindow
   * with changed translation when settings change.
   */
  int return_from_event_loop_code;

  do {
    QApplication::setQuitOnLastWindowClosed(true);

    /**
     * The function `gpgme_check_version' must be called before any other
     *  function in the library, because it initializes the thread support
     *  subsystem in GPGME. (from the info page) */
    gpgme_check_version(nullptr);

    /** set locale, because tests do also */
    gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
#ifndef _WIN32
    gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif

    auto main_window = std::make_unique<GpgFrontend::UI::MainWindow>();
    main_window->init();
    main_window->show();
    return_from_event_loop_code = QApplication::exec();

  } while (return_from_event_loop_code == RESTART_CODE);

  return return_from_event_loop_code;
}

void init_logging() {
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  ptime now = second_clock::local_time();

  el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);
  el::Configurations defaultConf;
  defaultConf.setToDefault();
  el::Loggers::reconfigureLogger("default", defaultConf);

  defaultConf.setGlobally(el::ConfigurationType::Format,
                          "%datetime %level %func %msg");

  auto logfile_path =
      (GpgFrontend::UI::GlobalSettingStation::GetInstance().GetLogDir() /
       to_iso_string(now));
  logfile_path.replace_extension(".log");
  defaultConf.setGlobally(el::ConfigurationType::Filename,
                          logfile_path.string());

  el::Loggers::reconfigureLogger("default", defaultConf);

  LOG(INFO) << _("Logfile Path") << logfile_path;
}

void init_locale() {
  auto& settings =
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("general") ||
      settings.lookup("general").getType() != libconfig::Setting::TypeGroup)
    settings.add("general", libconfig::Setting::TypeGroup);

  // set system default at first
  auto& general = settings["general"];
  if (!general.exists("lang"))
    general.add("lang", libconfig::Setting::TypeString) = "";

  GpgFrontend::UI::GlobalSettingStation::GetInstance().Sync();

  auto* locale_name = setlocale(LC_ALL, nullptr);
  LOG(INFO) << "current system locale" << locale_name;

  // read from settings file
  std::string lang;
  if (!general.lookupValue("lang", lang)) {
    LOG(ERROR) << _("Could not read properly from configure file");
  };

  LOG(INFO) << "lang" << lang;
  LOG(INFO) << "PROJECT_NAME" << PROJECT_NAME;
  LOG(INFO) << "locales path"
            << GpgFrontend::UI::GlobalSettingStation::GetInstance()
                   .GetLocaleDir()
                   .c_str();

  if (!lang.empty()) lang += ".UTF8";
  // GNU gettext settings
  locale_name = setlocale(LC_ALL, lang.c_str());
  if (locale_name == nullptr) {
    LOG(WARNING) << "set locale name failed";
  } else {
    LOG(INFO) << "locale name now" << locale_name;
  }

  bindtextdomain(PROJECT_NAME,
                 GpgFrontend::UI::GlobalSettingStation::GetInstance()
                     .GetLocaleDir()
                     .string()
                     .c_str());
  textdomain(PROJECT_NAME);
}
