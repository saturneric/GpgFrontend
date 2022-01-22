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

#ifndef _GPGFRONTENDTEST_H
#define _GPGFRONTENDTEST_H

#include <easyloggingpp/easylogging++.h>
#include <gpg-error.h>
#include <gtest/gtest.h>

#include <boost/date_time.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <libconfig.h++>
#include <memory>
#include <string>
#include <vector>

#include "gpg/GpgConstants.h"
#include "gpg/function/GpgKeyImportExporter.h"
#include "gpg/function/GpgKeyOpera.h"

class GpgCoreTest : public ::testing::Test {
 protected:
  // Secret Keys Imported
  std::vector<GpgFrontend::StdBypeArrayPtr> secret_keys_;

  // Program Location
  boost::filesystem::path parent_path =
      boost::dll::program_location().parent_path();

  // Configure File Location
  boost::filesystem::path config_path = parent_path / "conf" / "core.cfg";

  // Data File Directory Location
  boost::filesystem::path data_path;

  const int default_channel = 0;

  const int gpg_alone_channel = 512;

  GpgCoreTest() = default;

  ~GpgCoreTest() override = default;

  void SetUp() override {
    el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    el::Loggers::reconfigureLogger("default", defaultConf);

    defaultConf.setGlobally(el::ConfigurationType::Format,
                            "%datetime %level %func %msg");
    el::Loggers::reconfigureLogger("default", defaultConf);

    using namespace libconfig;
    Config cfg;
    ASSERT_NO_THROW(cfg.readFile(config_path.c_str()));
    Setting& root = cfg.getRoot();

    if (root.exists("data_path")) {
      std::string relative_data_path;
      root.lookupValue("data_path", relative_data_path);
      data_path = parent_path / relative_data_path;
    };

    configure_independent_database(root);

    configure_alone_gpg(root);

    dealing_private_keys(root);
    import_data();
    import_data_alone();
  }

  void TearDown() override {
    auto key_ids = std::make_unique<GpgFrontend::KeyIdArgsList>();
    key_ids->push_back("81704859182661FB");
    key_ids->push_back("06F1C7E7240C94E8");
    key_ids->push_back("8465C55B25C9B7D1");
    key_ids->push_back("021D89771B680FFB");
    GpgFrontend::GpgKeyOpera::GetInstance(default_channel)
        .DeleteKeys(std::move(key_ids));
  }

 private:
  void import_data() {
    for (const auto& secret_key : secret_keys_) {
      auto secret_key_copy =
          std::make_unique<GpgFrontend::ByteArray>(*secret_key);
      GpgFrontend::GpgKeyImportExporter::GetInstance(default_channel)
          .ImportKey(std::move(secret_key_copy));
    }
  }

  void import_data_alone() {
    for (auto& secret_key : secret_keys_) {
      auto secret_key_copy =
          std::make_unique<GpgFrontend::ByteArray>(*secret_key);
      GpgFrontend::GpgKeyImportExporter::GetInstance(gpg_alone_channel)
          .ImportKey(std::move(secret_key_copy));
    }
  }

  void dealing_private_keys(const libconfig::Setting& root) {
    if (root.exists("load_keys.private_keys")) {
      auto& private_keys = root.lookup("load_keys.private_keys");
      for (auto it = private_keys.begin(); it != private_keys.end(); it++) {
        if (it->exists("filename")) {
          std::string filename;
          it->lookupValue("filename", filename);
          auto data_file_path = data_path / filename;
          std::string data =
              GpgFrontend::read_all_data_in_file(data_file_path.string());
          secret_keys_.push_back(std::make_unique<std::string>(data));
        }
      }
    }
  }

  void configure_alone_gpg(const libconfig::Setting& root) {
    bool alone_gpg = false;
    if (root.exists("alone_gpg")) {
      root.lookupValue("alone_gpg", alone_gpg);
      if (alone_gpg && root.exists("alone_gpg")) {
        std::string alone_gpg_path;
        root.lookupValue("alone_gpg_path", alone_gpg_path);
        auto gpg_path = parent_path / alone_gpg_path;

        std::string relative_db_path;
        root.lookupValue("alone_gpg_db_path", relative_db_path);
        auto db_path = parent_path / relative_db_path;
        if (!boost::filesystem::exists(db_path)) {
          boost::filesystem::create_directory(db_path);
        } else {
          boost::filesystem::remove_all(db_path);
          boost::filesystem::create_directory(db_path);
        }
        GpgFrontend::GpgContext::CreateInstance(
            gpg_alone_channel,
            [&]() -> std::unique_ptr<GpgFrontend::GpgContext> {
              GpgFrontend::GpgContextInitArgs args;
              args.gpg_alone = true;
              args.independent_database = true;
              args.db_path = db_path.string();
              args.gpg_path = gpg_path.string();
              args.test_mode = true;
              return std::make_unique<GpgFrontend::GpgContext>(args);
            });
      }
    }
  }

  void configure_independent_database(const libconfig::Setting& root) {
    bool independent_database = false;
    if (root.exists("independent_database")) {
      root.lookupValue("independent_database", independent_database);
      if (independent_database && root.exists("independent_db_path")) {
        std::string relative_db_path;
        root.lookupValue("independent_db_path", relative_db_path);
        auto db_path = parent_path / relative_db_path;
        if (!boost::filesystem::exists(db_path)) {
          boost::filesystem::create_directory(db_path);
        } else {
          boost::filesystem::remove_all(db_path);
          boost::filesystem::create_directory(db_path);
        }

        GpgFrontend::GpgContext::CreateInstance(
            default_channel, [&]() -> std::unique_ptr<GpgFrontend::GpgContext> {
              GpgFrontend::GpgContextInitArgs args;
              args.test_mode = true;
              return std::make_unique<GpgFrontend::GpgContext>(args);
            });
      }
    }
  }
};

#endif  // _GPGFRONTENDTEST_H
