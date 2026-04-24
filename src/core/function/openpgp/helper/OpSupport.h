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

#include <cassert>

#include "core/model/GFEngineSupportIf.h"

namespace GpgFrontend {

template <typename OpTag>
struct OpSupportTraits;

template <typename Derived>
struct OpSupportTraitsBase {
  /**
   * @brief Return the operation support requirements.
   *
   * Each EngineSupportIf contains engine requirement and optional version
   * requirement.
   */
  static auto SupportRequirements() -> const auto& {
    return Derived::Versions();
  }

  /**
   * @brief Check whether this operation is supported by current context.
   */
  static auto IsSupported(const OpenPGPContext& ctx) -> bool {
    auto versions = Derived::Versions();
    return std::any_of(versions.begin(), versions.end(),
                       [&](const EngineSupportIf& requirement) -> bool {
                         return requirement.IsSupport(ctx);
                       });
  }
};

template <typename OpTag>
auto IsOpSupported(const OpenPGPContext& ctx) -> bool {
  return OpSupportTraits<OpTag>::IsSupported(ctx);
}

template <typename OpTag>
auto IsOpSupported(int channel) -> bool {
  auto& ctx = OpenPGPContext::GetInstance(channel);
  return OpSupportTraits<OpTag>::IsSupported(ctx);
}

template <typename OpTag>
auto GetOpSupportRequirements() -> const auto& {
  return OpSupportTraits<OpTag>::SupportRequirements();
}

#define GF_DEF_OP_SUPPORT_TRAITS(TAG, OPNAME, ...)                          \
  struct TAG {};                                                            \
  template <>                                                               \
  struct OpSupportTraits<TAG> : OpSupportTraitsBase<OpSupportTraits<TAG>> { \
    static constexpr const char* kOpName = OPNAME;                          \
    static auto Versions() -> const auto& {                                 \
      static const QContainer<EngineSupportIf> kVersions = {__VA_ARGS__};   \
      return kVersions;                                                     \
    }                                                                       \
  };

}  // namespace GpgFrontend