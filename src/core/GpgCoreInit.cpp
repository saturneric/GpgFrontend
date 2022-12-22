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

#include "GpgFunctionObject.h"
#include "core/GpgContext.h"
#include "core/function/GlobalSettingStation.h"

// init easyloggingpp library
INITIALIZE_EASYLOGGINGPP

namespace GpgFrontend {

/**
 * @brief setup logging system and do proper initialization
 *
 */
void InitLoggingSystem() {
  using namespace boost::posix_time;
  using namespace boost::gregorian;

  el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);
  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
  el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);
  el::Configurations defaultConf;
  defaultConf.setToDefault();

  // apply settings
  defaultConf.setGlobally(el::ConfigurationType::Format,
                          "%datetime %level [core] {%func} -> %msg");

  // apply settings no written to file
  defaultConf.setGlobally(el::ConfigurationType::ToFile, "false");

  // set the logger
  el::Loggers::reconfigureLogger("default", defaultConf);

  // get the log directory
  auto logfile_path = (GlobalSettingStation::GetInstance().GetLogDir() /
                       to_iso_string(second_clock::local_time()));
  logfile_path.replace_extension(".log");
  defaultConf.setGlobally(el::ConfigurationType::Filename,
                          logfile_path.u8string());

  // apply settings written to file
  defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");

  // set the logger
  el::Loggers::reconfigureLogger("default", defaultConf);

  LOG(INFO) << _("log file path") << logfile_path;
}

void ResetGpgFrontendCore() { reset_gpgfrontend_core(); }

void init_gpgfrontend_core() {
  // read from settings file

  bool use_custom_key_database_path = false;
  try {
    auto& settings =
        GpgFrontend::GlobalSettingStation::GetInstance().GetUISettings();
    use_custom_key_database_path =
        settings.lookup("general.use_custom_key_database_path");
  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error")
               << _("use_custom_key_database_path");
  }

  LOG(INFO) << "core loaded if use custom key databse path: "
            << use_custom_key_database_path;

  std::string custom_key_database_path;
  try {
    auto& settings =
        GpgFrontend::GlobalSettingStation::GetInstance().GetUISettings();
    custom_key_database_path = static_cast<std::string>(
        settings.lookup("general.custom_key_database_path"));

  } catch (...) {
    LOG(ERROR) << _("Setting Operation Error") << _("custom_key_database_path");
  }

  LOG(INFO) << "core loaded custom key databse path: "
            << custom_key_database_path;

  // init default channel
  GpgFrontend::GpgContext::CreateInstance(
      GPGFRONTEND_DEFAULT_CHANNEL, [=]() -> std::unique_ptr<ChannelObject> {
        GpgFrontend::GpgContextInitArgs args;

        // set key database path
        if (use_custom_key_database_path && !custom_key_database_path.empty()) {
          args.db_path = custom_key_database_path;
        }

        return std::unique_ptr<ChannelObject>(new GpgContext(args));
      });

  // init non-ascii channel
  GpgFrontend::GpgContext::CreateInstance(
      GPGFRONTEND_NON_ASCII_CHANNEL, [=]() -> std::unique_ptr<ChannelObject> {
        GpgFrontend::GpgContextInitArgs args;
        args.ascii = false;

        // set key database path
        if (use_custom_key_database_path && !custom_key_database_path.empty()) {
          args.db_path = custom_key_database_path;
        }

        return std::unique_ptr<ChannelObject>(new GpgContext(args));
      });
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