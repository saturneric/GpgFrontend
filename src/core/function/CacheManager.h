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

#include "core/function/basic/GpgFunctionObject.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT CacheManager
    : public QObject,
      public SingletonFunctionObject<CacheManager> {
  Q_OBJECT
 public:
  explicit CacheManager(
      int channel = SingletonFunctionObject::GetDefaultChannel());

  ~CacheManager() override;

  void SaveCache(std::string key, const nlohmann::json& value,
                 bool flush = false);

  auto LoadCache(std::string key) -> nlohmann::json;

  auto LoadCache(std::string key, nlohmann::json default_value)
      -> nlohmann::json;

  auto ResetCache(std::string key) -> bool;

 private:
  class Impl;
  std::unique_ptr<Impl> p_;
};

}  // namespace GpgFrontend
