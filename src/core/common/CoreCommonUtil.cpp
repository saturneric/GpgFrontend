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
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com><eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "CoreCommonUtil.h"

#include <string>

namespace GpgFrontend {

std::unique_ptr<CoreCommonUtil> CoreCommonUtil::instance_ = nullptr;  ///<

CoreCommonUtil *CoreCommonUtil::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = std::make_unique<CoreCommonUtil>();
  }
  return instance_.get();
}

void CoreCommonUtil::SetTempCacheValue(const std::string key,
                                       const std::string value) {
  temp_cache_[key] = value;
}

std::string CoreCommonUtil::GetTempCacheValue(const std::string key) {
  std::string temp_cache_value;
  std::swap(temp_cache_value, temp_cache_[key]);
  return temp_cache_value;
}

void CoreCommonUtil::ResetTempCacheValue(const std::string key) {
  std::string temp_cache_value;
  std::swap(temp_cache_value, temp_cache_[key]);
}

}  // namespace GpgFrontend
