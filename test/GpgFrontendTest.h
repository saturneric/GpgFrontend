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

#ifndef _GPGFRONTENDTEST_H
#define _GPGFRONTENDTEST_H

#include <easyloggingpp/easylogging++.h>
#include <gpg-error.h>
#include <gtest/gtest.h>

#include <boost/date_time/gregorian/parsers.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <memory>
#include <string>
#include <vector>

#include "gpg/GpgConstants.h"
#include "gpg/function/GpgKeyImportExportor.h"

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

  int default_channel = 0;

  GpgCoreTest() = default;

  virtual ~GpgCoreTest() = default;

  virtual void SetUp() {
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

    dealing_private_keys(root);
    import_data();
  }

  virtual void TearDown() {}

 private:
  void import_data() {
    GpgFrontend::GpgContext::GetInstance(default_channel)
        .SetPassphraseCb(GpgFrontend::GpgContext::test_passphrase_cb);
    for (auto& secret_key : secret_keys_) {
      GpgFrontend::GpgKeyImportExportor::GetInstance(default_channel)
          .ImportKey(std::move(secret_key));
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

  void configure_independent_database(const libconfig::Setting& root) {
    bool independent_database = false;
    if (root.exists("independent_database")) {
      root.lookupValue("independent_database", independent_database);
      if (independent_database && root.exists("independent_db_path")) {
        default_channel = 1;
        std::string relative_db_path;
        root.lookupValue("independent_db_path", relative_db_path);
        auto db_path = parent_path / relative_db_path;
        if (!boost::filesystem::exists(db_path)) {
          boost::filesystem::create_directory(db_path);
        }
        GpgFrontend::GpgContext::CreateInstance(
            1,
            std::make_unique<GpgFrontend::GpgContext>(true, db_path.c_str()));
      }
    }
  }
};

#endif  // _GPGFRONTENDTEST_H
