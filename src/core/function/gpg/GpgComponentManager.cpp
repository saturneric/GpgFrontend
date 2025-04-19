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

#include "GpgComponentManager.h"

namespace GpgFrontend {

GpgComponentManager::GpgComponentManager(int channel)
    : GpgFrontend::SingletonFunctionObject<GpgComponentManager>(channel) {}

auto GpgComponentManager::GetGpgAgentVersion() -> QString {
  if (!gpg_agent_version_.isEmpty()) return gpg_agent_version_;

  auto [r, s] =
      assuan_.SendDataCommand(GpgComponentType::kGPG_AGENT, "GETINFO version");
  if (s.isEmpty()) {
    LOG_D() << "invalid response of GETINFO version: " << s;
    return {};
  }

  gpg_agent_version_ = s.front();
  return gpg_agent_version_;
}

auto GpgComponentManager::GetScdaemonVersion() -> QString {
  if (!scdaemon_version_.isEmpty()) return scdaemon_version_;

  auto [r, s] = assuan_.SendDataCommand(GpgComponentType::kGPG_AGENT,
                                        "SCD GETINFO version");
  if (s.isEmpty()) {
    LOG_D() << "invalid response of SCD GETINFO version: " << s;
    return {};
  }

  scdaemon_version_ = s.front();
  return scdaemon_version_;
}

auto GpgComponentManager::ReloadGpgAgent() -> bool {
  auto [r, s] =
      assuan_.SendStatusCommand(GpgComponentType::kGPG_AGENT, "RELOADAGENT");
  if (r != GPG_ERR_NO_ERROR) {
    LOG_D() << "invalid response of RELOADAGENT: " << s;
    return false;
  }

  return true;
}

auto GpgComponentManager::GpgKillAgent() -> bool {
  auto [r, s] =
      assuan_.SendStatusCommand(GpgComponentType::kGPG_AGENT, "KILLAGENT");
  if (r != GPG_ERR_NO_ERROR) {
    LOG_D() << "invalid response of KILLAGENT: " << s;
    return false;
  }

  return true;
}

}  // namespace GpgFrontend