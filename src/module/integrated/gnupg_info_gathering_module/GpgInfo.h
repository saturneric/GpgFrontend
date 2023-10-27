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

#include <nlohmann/json.hpp>

namespace GpgFrontend::Module::Integrated::GnuPGInfoGatheringModule {
/**
 * @brief  Use to record some info about gnupg
 *
 */
class GpgInfo {
 public:
  std::string GnuPGHomePath;  ///< value of ---homedir

  std::map<std::string, std::vector<std::string>> ComponentsInfo;        ///<
  std::map<std::string, std::vector<std::string>> ConfigurationsInfo;    ///<
  std::map<std::string, std::vector<std::string>> OptionsInfo;           ///<
  std::map<std::string, std::vector<std::string>> AvailableOptionsInfo;  ///<
};

/**
 * @brief  Use to record some info about gnupg components
 *
 */
struct GpgComponentInfo {
  std::string name;
  std::string desc;
  std::string version;
  std::string path;
  std::string binary_checksum;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GpgComponentInfo, name, desc, version, path,
                                   binary_checksum);

}  // namespace GpgFrontend::Module::Integrated::GnuPGInfoGatheringModule
