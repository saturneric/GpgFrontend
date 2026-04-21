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

#include "core/model/GFEngineSupportIf.h"

namespace GpgFrontend {

template <typename OpTag>
struct OpTraits;

template <typename ImplFn>
using EngineOpImpl = QPair<OpenPGPEngine, ImplFn>;

template <typename Derived>
struct OpTraitsBase {
  static auto Versions() -> const auto& {
    static const QContainer<EngineSupportIf> kVersions = {
        {OpenPGPEngine::kGNUPG, "2.2.0"},
        {OpenPGPEngine::kRPGP, "0.1.0"},
    };
    return kVersions;
  }
};

template <typename Table, typename... Args>
auto DispatchByEngine(GpgContext& ctx, const Table& table, Args&&... args)
    -> GpgError {
  const auto engine = ctx.Engine();
  for (const auto& [supported_engine, fn] : table) {
    if (supported_engine == engine) {
      return fn(ctx, std::forward<Args>(args)...);
    }
  }
  return GPG_ERR_NOT_SUPPORTED;
}

}  // namespace GpgFrontend