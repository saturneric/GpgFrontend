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

//
// Created by eric on 07.01.2023.
//

#include "GpgAdvancedOperator.h"

#include "core/function/gpg/GpgCommandExecutor.h"
namespace GpgFrontend {

auto GpgAdvancedOperator::ClearGpgPasswordCache() -> bool {
  auto [ret, out] = exec_.GpgConfExecuteSync({{"--reload", "gpg-agent"}});
  return ret == 0;
}

auto GpgAdvancedOperator::ReloadAllGpgComponents() -> bool {
  auto [ret, out] = exec_.GpgConfExecuteSync({{"--reload", "all"}});
  return ret == 0;
}

auto GpgAdvancedOperator::KillAllGpgComponents() -> bool {
  auto [ret, out] = exec_.GpgConfExecuteSync({{"--kill", "all"}});
  return ret == 0;
}

auto GpgAdvancedOperator::ResetConfigures() -> bool {
  auto [ret, out] = exec_.GpgConfExecuteSync({{"--apply-defaults"}});
  return ret == 0;
}

auto GpgAdvancedOperator::LaunchAllGpgComponents() -> bool {
  auto [ret, out] = exec_.GpgConfExecuteSync({{"--launch", "all"}});
  return ret == 0;
}

auto GpgAdvancedOperator::RestartGpgComponents() -> bool {
  if (!KillAllGpgComponents()) return false;
  return LaunchAllGpgComponents();
}

GpgAdvancedOperator::GpgAdvancedOperator(int channel)
    : SingletonFunctionObject<GpgAdvancedOperator>(channel) {}
}  // namespace GpgFrontend