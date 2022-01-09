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

#include <boost/date_time.hpp>
#include <vmime/vmime.hpp>

#include "ui/settings/GlobalSettingStation.h"

std::vector<boost::filesystem::path> get_files_of_directory(
    const boost::filesystem::path& _path) {
  namespace fs = boost::filesystem;
  std::vector<fs::path> path_list;
  if (!_path.empty()) {
    fs::recursive_directory_iterator end;

    for (fs::recursive_directory_iterator i(_path); i != end; ++i) {
      const fs::path cp = (*i);
      path_list.push_back(cp);
    }
  }
  return path_list;
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

  LOG(INFO) << _("logfile Path") << logfile_path;
}

void init_certs() {
  std::vector<vmime::shared_ptr<vmime::security::cert::X509Certificate>>
      root_certs;
  auto cert_file_paths = get_files_of_directory(
      GpgFrontend::UI::GlobalSettingStation::GetInstance().GetCertsDir());

  auto& _instance = GpgFrontend::UI::GlobalSettingStation::GetInstance();
  for (const auto& cert_file_path : cert_file_paths) {
    _instance.AddRootCert(cert_file_path);
  }
  LOG(INFO) << _("root certs loaded") << _instance.GetRootCerts().size();
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

  LOG(INFO) << "current system locale" << setlocale(LC_ALL, nullptr);

  // read from settings file
  std::string lang;
  if (!general.lookupValue("lang", lang)) {
    LOG(ERROR) << _("could not read properly from configure file");
  };

  LOG(INFO) << "lang from settings" << lang;
  LOG(INFO) << "PROJECT_NAME" << PROJECT_NAME;
  LOG(INFO) << "locales path"
            << GpgFrontend::UI::GlobalSettingStation::GetInstance()
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

  bindtextdomain(PROJECT_NAME,
                 GpgFrontend::UI::GlobalSettingStation::GetInstance()
                     .GetLocaleDir()
                     .string()
                     .c_str());
  bind_textdomain_codeset(PROJECT_NAME, "utf-8");
  textdomain(PROJECT_NAME);
}
