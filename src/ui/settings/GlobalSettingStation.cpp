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

#include "GlobalSettingStation.h"

std::unique_ptr<GlobalSettingStation> GlobalSettingStation::_instance = nullptr;

GlobalSettingStation& GlobalSettingStation::GetInstance() {
  if (_instance == nullptr) {
    _instance = std::make_unique<GlobalSettingStation>();
  }
  return *_instance;
}

void GlobalSettingStation::Sync() noexcept {
  using namespace libconfig;
  try {
    ui_cfg.writeFile(ui_config_path.c_str());
    LOG(INFO) << _("Updated ui configuration successfully written to")
              << ui_config_path;

  } catch (const FileIOException& fioex) {
    LOG(ERROR) << _("I/O error while writing ui configuration file")
               << ui_config_path;
  }
}

GlobalSettingStation::GlobalSettingStation() noexcept {
  using namespace boost::filesystem;
  using namespace libconfig;

  el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);

  LOG(INFO) << _("App Path") << app_path;
  LOG(INFO) << _("App Configure Path") << app_configure_path;
  LOG(INFO) << _("App Data Path") << app_data_path;
  LOG(INFO) << _("App Log Path") << app_log_path;
  LOG(INFO) << _("App Locale Path") << app_locale_path;

  if (!is_directory(app_configure_path)) create_directory(app_configure_path);

  if (!is_directory(app_data_path)) create_directory(app_data_path);

  if (!is_directory(app_log_path)) create_directory(app_log_path);

  if (!is_directory(ui_config_dir_path)) create_directory(ui_config_dir_path);

  if (!exists(ui_config_path)) {
    try {
      this->ui_cfg.writeFile(ui_config_path.c_str());
      LOG(INFO) << _("UserInterface configuration successfully written to")
                << ui_config_path;

    } catch (const FileIOException& fioex) {
      LOG(ERROR)
          << _("I/O error while writing UserInterface configuration file")
          << ui_config_path;
    }
  } else {
    try {
      this->ui_cfg.readFile(ui_config_path.c_str());
      LOG(INFO) << _("UserInterface configuration successfully read from")
                << ui_config_path;
    } catch (const FileIOException& fioex) {
      LOG(ERROR) << _("I/O error while reading UserInterface configure file");
    } catch (const ParseException& pex) {
      LOG(ERROR) << _("Parse error at ") << pex.getFile() << ":"
                 << pex.getLine() << " - " << pex.getError();
    }
  }
}
