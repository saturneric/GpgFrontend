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

#pragma once

#include <gtest/gtest.h>

#include <boost/date_time.hpp>
#include <boost/dll.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <filesystem>

#include "core/function/GlobalSettingStation.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend::Test {

class GpgCoreTest : public ::testing::Test {
 public:
  void SetUp() override;

  void TearDown() override;

 private:
  void import_private_keys(const libconfig::Setting& root);

  // Configure File Location
  std::filesystem::path config_path_ =
      GlobalSettingStation::GetInstance().GetAppDir() / "test" / "conf" /
      "core.cfg";

  // Data File Directory Location
  std::filesystem::path data_path_ =
      GlobalSettingStation::GetInstance().GetAppDir() / "test" / "data";
};

}  // namespace GpgFrontend::Test