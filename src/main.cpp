/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

int main(int argc, char* argv[]) {
  Q_INIT_RESOURCE(gpgfrontend);
  QApplication app(argc, argv);

  init_logging();

  // get application path
  auto app_path = GlobalSettingStation::GetInstance().GetAppDir();

  QApplication::setApplicationVersion(BUILD_VERSION);
  QApplication::setApplicationName(PROJECT_NAME);

  // dont show icons in menus
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
  auto& settings = GlobalSettingStation::GetInstance().GetUISettings();

  if (!settings.exists("general") ||
      settings.lookup("general").getType() != libconfig::Setting::TypeGroup)
    settings.add("general", libconfig::Setting::TypeGroup);

  auto& general = settings["general"];
  if (!general.exists("lang"))
    general.add("lang", libconfig::Setting::TypeString) =
        QLocale::system().name().toStdString();

  GlobalSettingStation::GetInstance().Sync();

  QTranslator translator, translator2;
  int return_from_event_loop_code;

  LOG(INFO) << "Resource Directory" << RESOURCE_DIR(app_path);

  do {
    QApplication::removeTranslator(&translator);
    QApplication::removeTranslator(&translator2);

    std::string lang;
    if (!general.lookupValue("lang", lang)) {
      LOG(ERROR) << "could not read properly from configure file";
    };

    translator.load(QString::fromStdString(RESOURCE_DIR(app_path).string() +
                                           "/ts/" + "gpgfrontend_" + lang));
    QApplication::installTranslator(&translator);

    // set qt translations
    translator2.load(QString::fromStdString(RESOURCE_DIR(app_path).string() +
                                            "/ts/qt_" + lang));
    QApplication::installTranslator(&translator2);

    QApplication::setQuitOnLastWindowClosed(true);

    /**
     * The function `gpgme_check_version' must be called before any other
     *  function in the library, because it initializes the thread support
     *  subsystem in GPGME. (from the info page) */
    gpgme_check_version(nullptr);

    // the locale set here is used for the other setlocale calls which have
    // nullptr
    // -> nullptr means use default, which is configured here
    setlocale(LC_ALL, lang.c_str());

    /** set locale, because tests do also */
    gpgme_set_locale(nullptr, LC_CTYPE, setlocale(LC_CTYPE, nullptr));
#ifndef _WIN32
    gpgme_set_locale(nullptr, LC_MESSAGES, setlocale(LC_MESSAGES, nullptr));
#endif

    GpgFrontend::UI::MainWindow window;
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
      (GlobalSettingStation::GetInstance().GetLogDir() / to_iso_string(now));
  logfile_path.replace_extension(".log");
  defaultConf.setGlobally(el::ConfigurationType::Filename,
                          logfile_path.string());

  el::Loggers::reconfigureLogger("default", defaultConf);

  LOG(INFO) << "Logfile Path" << logfile_path;
}
