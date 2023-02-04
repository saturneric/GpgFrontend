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

#include "CacheManager.h"

#include <algorithm>

#include "function/DataObjectOperator.h"
#include "nlohmann/json_fwd.hpp"
#include "spdlog/spdlog.h"

void GpgFrontend::CacheManager::SaveCache(std::string key,
                                          const nlohmann::json &value) {
  auto stored_data =
      GpgFrontend::DataObjectOperator::GetInstance().GetDataObject(
          "__cache_data_list");

  // get cache data list from file system
  nlohmann::json cache_data_list;
  if (stored_data.has_value()) {
    cache_data_list = std::move(stored_data.value());
  }

  if (!cache_data_list.is_array()) {
    cache_data_list.clear();
  }

  if (GpgFrontend::DataObjectOperator::GetInstance()
          .SaveDataObj(key, value)
          .empty()) {
    return;
  }

  if (std::find(cache_data_list.begin(), cache_data_list.end(), key) ==
      cache_data_list.end()) {
    cache_data_list.push_back(key);
  }

  GpgFrontend::DataObjectOperator::GetInstance().SaveDataObj(
      "__cache_data_list", cache_data_list);
}
