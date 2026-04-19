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

#include "GFEngineSupportIf.h"

#include "core/utils/CommonUtils.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {
EngineSupportIf::EngineSupportIf()
    : version_req_("0.0.0"), engine_req_(OpenPGPEngine::kGNUPG) {}
EngineSupportIf::EngineSupportIf(OpenPGPEngine engine,
                                 QString version_requirement)
    : version_req_(std::move(version_requirement)), engine_req_(engine) {}

auto EngineSupportIf::IsSupport(const GpgContext& ctx) const -> bool {
  auto version = ctx.EngineVersion();
  auto engine = ctx.Engine();

  if (engine != engine_req_) {
    LOG_D() << "Engine type not match. Required: "
            << ConvertOpenPGPEngine2String(engine_req_)
            << ", actual: " << ConvertOpenPGPEngine2String(engine);
    return false;
  }

  if (GFCompareSoftwareVersion(version_req_, version) > 0) {
    LOG_D() << "Engine version not match. Required: " << version_req_
            << ", actual: " << version;
    return false;
  }

  return true;
}

auto GpgContextSupportIf(int channel,
                         const QContainer<EngineSupportIf>& if_cond) -> bool {
  return std::any_of(if_cond.begin(), if_cond.end(),
                     [=](const EngineSupportIf& cond) -> bool {
                       return cond.IsSupport(GpgContext::GetInstance(channel));
                     });
}
}  // namespace GpgFrontend