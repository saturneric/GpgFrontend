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

EngineSupportIf::EngineSupportIf(OpenPGPEngine engine, QString version)
    : version_req_(std::move(version)), engine_req_(engine) {}

auto EngineSupportIf::IsSupport(const OpenPGPContext& ctx) const -> bool {
  // Check the engine first and bail out early on a mismatch. EngineVersion()
  // dispatches to the active engine's implementation, which for GnuPG performs a
  // live gpg-agent query; querying it for a context whose engine cannot satisfy
  // the requirement anyway is both wasteful and, for placeholder contexts,
  // dangerous (it can reach code that assumes a GpgContext).
  if (ctx.Engine() != engine_req_) return false;
  if (GFCompareSoftwareVersion(version_req_, ctx.EngineVersion()) > 0) {
    return false;
  }
  return true;
}

auto GpgContextSupportIf(int channel,
                         const QContainer<EngineSupportIf>& if_cond) -> bool {
  return std::any_of(
      if_cond.begin(), if_cond.end(), [=](const EngineSupportIf& cond) -> bool {
        return cond.IsSupport(OpenPGPContext::GetInstance(channel));
      });
}

auto GpgContextSupportIfWithLog(int channel, const QString& log,
                                const QContainer<EngineSupportIf>& if_cond)
    -> bool {
  auto supported = GpgContextSupportIf(channel, if_cond);
  if (!supported) {
    auto& ctx = OpenPGPContext::GetInstance(channel);
    LOG_W() << "engine check failed, channel: " << channel
            << ", engine: " << ConvertOpenPGPEngine2String(ctx.Engine())
            << ", version: " << ctx.EngineVersion() << ", message: " << log;
  }
  return supported;
}
}  // namespace GpgFrontend