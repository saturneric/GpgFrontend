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

#include "GpgCoreTest.h"

#include "core/function/gpg/GpgKeyImportExporter.h"
#include "core/utils/IOUtils.h"
#include "core/utils/MemoryUtils.h"
#include "spdlog/spdlog.h"

namespace GpgFrontend::Test {

void GpgCoreTest::import_private_keys(const libconfig::Setting& root) {
  if (root.exists("load_keys.private_keys")) {
    auto& private_keys = root.lookup("load_keys.private_keys");
    for (auto& private_key : private_keys) {
      if (private_key.exists("filename")) {
        std::string filename;
        private_key.lookupValue("filename", filename);
        auto data_file_path = data_path_ / filename;
        std::string data = ReadAllDataInFile(data_file_path.string());
        auto secret_key_copy = SecureCreateSharedObject<std::string>(data);
        GpgKeyImportExporter::GetInstance(kGpgFrontendDefaultChannel)
            .ImportKey(secret_key_copy);
      }
    }
  }
}

void GpgCoreTest::TearDown() {}

void GpgCoreTest::SetUp() {
  libconfig::Config cfg;
  SPDLOG_INFO("test case config file path: {}", config_path_.string());
  ASSERT_NO_THROW(cfg.readFile(config_path_.string()));
  auto& root = cfg.getRoot();
  import_private_keys(root);
}
}  // namespace GpgFrontend::Test
