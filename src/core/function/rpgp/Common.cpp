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

#include "core/GFCoreRust.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/RustUtils.h"

namespace GpgFrontend {

auto GetEngineVersionRpgpImpl(OpenPGPContext& ctx) -> QString {
  return RustEngineVersion();
}

auto BuildOpenPGPContextRpgpImpl(int channel,
                                 const OpenPGPContextInitArgs& args) -> bool {
  auto& ctx = GpgFrontend::OpenPGPContext::CreateInstance(
      channel, [=]() -> ChannelObjectPtr {
        return BuildContext<OpenPGPContext>(channel, args);
      });

  if (!ctx.Good()) {
    LOG_E() << "Failed to create OpenPGPContext for GnuPG engine, channel:"
            << channel;
  }

  // Cached passphrases belong to the database, not to the channel it happened
  // to be opened on: another database given this channel later must not be
  // served them, and this one reopened elsewhere should still find them.
  const auto resource = ResolveKeyDatabaseResourceId("rpgp", args.db_path);
  const auto scope =
      ctx.Good() && !resource.IsEmpty()
          ? QString("%1:%2").arg(resource.backend, resource.location).toUtf8()
          : QByteArray{};
  Rust::gfr_set_channel_cache_scope(
      channel, scope.isEmpty() ? nullptr : scope.constData());

  return ctx.Good();
}
}  // namespace GpgFrontend
