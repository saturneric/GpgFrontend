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

#include "Common.h"

#include "core/function/gpg/GpgComponentManager.h"
#include "core/function/gpg/GpgContext.h"
#include "core/module/ModuleManager.h"

namespace GpgFrontend {

auto GetEngineVersionGnuPGImpl(OpenPGPContext& ctx) -> QString {
  auto ver =
      GpgComponentManager::GetInstance(ctx.GetChannel()).GetGpgAgentVersion();

  if (!ver.isEmpty()) {
    return ver;
  }

  auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{});

  if (!gnupg_version.isEmpty()) {
    return gnupg_version;
  }

  return "0.0.0";
}

auto BuildOpenPGPContextGnuPGImpl(int channel,
                                  const OpenPGPContextInitArgs& args) -> bool {
  auto& ctx = GpgFrontend::OpenPGPContext::CreateInstance(
      channel, [=]() -> ChannelObjectPtr {
        return BuildContext<GpgContext>(channel, args);
      });

  if (!ctx.Good()) {
    LOG_E() << "Failed to create OpenPGPContext for GnuPG engine, channel:"
            << channel;
  }

  return ctx.Good();
}

}  // namespace GpgFrontend