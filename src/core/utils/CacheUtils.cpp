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

#include "CacheUtils.h"

#include "core/function/CacheManager.h"

namespace GpgFrontend {

void SetTempCacheValue(const std::string& key, const std::string& value) {
  nlohmann::json cache_json = value;
  CacheManager::GetInstance().SaveCache(key, cache_json);
}

auto GetTempCacheValue(const std::string& key) -> std::string {
  nlohmann::json cache_json = "";
  cache_json = CacheManager::GetInstance().LoadCache(key, cache_json);
  return cache_json.template get<std::string>();
}

void ResetTempCacheValue(const std::string& key) {
  CacheManager::GetInstance().ResetCache(key);
}

}  // namespace GpgFrontend