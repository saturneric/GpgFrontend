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

#include "core/function/gpg/GpgAssuanHelper.h"
#include "core/function/gpg/GpgContext.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT GpgComponentManager
    : public SingletonFunctionObject<GpgComponentManager> {
 public:
  /**
   * @brief Construct a new Gpg Assuan Helper object
   *
   * @param channel
   */
  explicit GpgComponentManager(int channel);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto GetGpgAgentVersion() -> QString;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto GetScdaemonVersion() -> QString;

  /**
   * @brief
   *
   */
  auto ReloadGpgAgent() -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  auto GpgKillAgent() -> bool;

  /**
   * @brief
   *
   * @return auto
   */
  void Reset();

 private:
  GpgContext& ctx_ =
      GpgContext::GetInstance(SingletonFunctionObject::GetChannel());
  GpgAssuanHelper& assuan_ =
      GpgAssuanHelper::GetInstance(SingletonFunctionObject::GetChannel());

  QString gpg_agent_version_;
  QString scdaemon_version_;
};

};  // namespace GpgFrontend