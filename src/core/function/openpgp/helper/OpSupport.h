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

// Primary template — specialised by GF_DEF_OP_SUPPORT_TRAITS for each op tag.
template <typename OpTag>
struct OpSupportTraits;

/**
 * @brief CRTP base providing engine/version support checking for an operation.
 *
 * Derived specialisations supply a static Versions() method returning a list
 * of EngineSupportIf requirements. IsSupported() returns true when the current
 * context satisfies at least one requirement.
 *
 * @tparam Derived the concrete OpSupportTraits specialisation
 */
template <typename Derived>
struct OpSupportTraitsBase {
  /**
   * @brief Return the engine/version requirements for this operation.
   *
   * @return const reference to the list of EngineSupportIf requirements
   */
  static auto SupportRequirements() -> const auto& {
    return Derived::Versions();
  }

  /**
   * @brief Return whether @p ctx satisfies at least one engine/version
   * requirement.
   *
   * @param ctx OpenPGP context to test
   * @return true if the context's engine and version match any requirement
   */
  static auto IsSupported(const OpenPGPContext& ctx) -> bool {
    auto versions = Derived::Versions();
    return std::any_of(versions.begin(), versions.end(),
                       [&](const EngineSupportIf& requirement) -> bool {
                         return requirement.IsSupport(ctx);
                       });
  }
};

/**
 * @brief Return whether the operation identified by @p OpTag is supported by @p
 * ctx.
 *
 * @tparam OpTag operation tag type defined by GF_DEF_OP_SUPPORT_TRAITS
 * @param ctx OpenPGP context to test
 * @return true if the operation is supported
 */
template <typename OpTag>
auto IsOpSupported(const OpenPGPContext& ctx) -> bool {
  return OpSupportTraits<OpTag>::IsSupported(ctx);
}

/**
 * @brief Return whether the operation identified by @p OpTag is supported on @p
 * channel.
 *
 * @tparam OpTag operation tag type defined by GF_DEF_OP_SUPPORT_TRAITS
 * @param channel OpenPGP context channel to test
 * @return true if the operation is supported on that channel
 */
template <typename OpTag>
auto IsOpSupported(int channel) -> bool {
  auto& ctx = OpenPGPContext::GetInstance(channel);
  return OpSupportTraits<OpTag>::IsSupported(ctx);
}

/**
 * @brief Return the engine/version requirements for the operation identified by
 * @p OpTag.
 *
 * @tparam OpTag operation tag type defined by GF_DEF_OP_SUPPORT_TRAITS
 * @return const reference to the list of EngineSupportIf requirements
 */
template <typename OpTag>
auto GetOpSupportRequirements() -> const auto& {
  return OpSupportTraits<OpTag>::SupportRequirements();
}

/**
 * @brief Define an operation tag and its engine/version support requirements.
 *
 * Creates an empty struct named @p TAG and a specialisation of OpSupportTraits
 * for it. The variadic arguments are EngineSupportIf initialisers specifying
 * which engines and minimum versions support this operation.
 *
 * Usage:
 * @code
 * GF_DEF_OP_SUPPORT_TRAITS(MyOpTag, "op_my_op",
 *     {OpenPGPEngine::kGNUPG, "2.2.0"},
 *     {OpenPGPEngine::kRPGP,  "0.1.0"});
 * @endcode
 */
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
