/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include "core/function/gpg/GpgContext.h"
namespace GpgFrontend {

/**
 * @brief Interface for checking if the current engine support the requirement
 * for a specific function or feature.
 *
 */
class GF_CORE_EXPORT EngineSupportIf {
 public:
  /**
   * @brief Construct a new Engine Support If object
   *
   */
  EngineSupportIf();

  /**
   * @brief Construct a new Engine Support If object
   *
   * @param engine
   * @param version_requirement
   */
  EngineSupportIf(OpenPGPEngine engine, QString version);

  /**
   * @brief Check if the engine support the requirement
   *
   * @param ctx
   * @return auto
   */
  [[nodiscard]] auto IsSupport(const GpgContext& ctx) const -> bool;

 private:
  QString version_req_;
  OpenPGPEngine engine_req_;
};

/**
 * @brief Check if the current engine support any of the requirements in the
 * list
 *
 * @param if_cond
 * @return true
 * @return false
 */
auto GF_CORE_EXPORT GpgContextSupportIf(
    int channel, const QContainer<EngineSupportIf>& if_cond) -> bool;
}  // namespace GpgFrontend
