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
#include <type_traits>

#include "core/function/openpgp/OpenPGPContext.h"
#include "core/function/openpgp/helper/OpSupport.h"

namespace GpgFrontend {

template <typename OpTag>
struct OpTraits;

template <typename ImplFn>
using EngineOpImpl = QPair<OpenPGPEngine, ImplFn>;

template <typename ImplFn>
using EngineOpImplTable = QContainer<EngineOpImpl<ImplFn>>;

template <typename Derived, typename OpTag>
struct OpTraitsBase : OpSupportTraits<OpTag> {
  static auto HasImpl(OpenPGPEngine engine) -> bool {
    for (const auto& [supported_engine, fn] : Derived::ImplTable()) {
      if (supported_engine == engine) return true;
    }
    return false;
  }

  // just check if there's an implementation for the engine, without checking
  // version support
  static auto IsRoutable(const OpenPGPContext& ctx) -> bool {
    return HasImpl(ctx.Engine());
  }

  static auto IsRoutable(OpenPGPEngine engine) -> bool {
    return HasImpl(engine);
  }

  // check if the operation can be called for the current engine, which means
  // both implementation and support requirements are satisfied
  static auto IsSupported(const OpenPGPContext& ctx) -> bool {
    return HasImpl(ctx.Engine()) && OpSupportTraits<OpTag>::IsSupported(ctx);
  }
};

template <typename Table, typename... Args>
auto DispatchByEngine(OpenPGPContext& ctx, const Table& table, Args&&... args)
    -> decltype(std::declval<typename Table::value_type::second_type>()(
        ctx, std::forward<Args>(args)...)) {
  using Ret = decltype(std::declval<typename Table::value_type::second_type>()(
      ctx, std::forward<Args>(args)...));

  const auto engine = ctx.Engine();

  for (const auto& [supported_engine, fn] : table) {
    if (supported_engine == engine) {
      return fn(ctx, std::forward<Args>(args)...);
    }
  }

  LOG_E() << "no implementation found for engine: " << static_cast<int>(engine);
  assert(false && "no implementation found for the current engine");

  if constexpr (std::is_void_v<Ret>) {
    return;
  } else {
    return Ret{};
  }
}

template <typename Table, typename... Args>
auto DispatchByEngine(OpenPGPEngine engine, const Table& table, Args&&... args)
    -> decltype(std::declval<typename Table::value_type::second_type>()(
        std::forward<Args>(args)...)) {
  using Ret = decltype(std::declval<typename Table::value_type::second_type>()(
      std::forward<Args>(args)...));

  for (const auto& [supported_engine, fn] : table) {
    if (supported_engine == engine) {
      return fn(std::forward<Args>(args)...);
    }
  }

  LOG_E() << "no implementation found for engine: " << static_cast<int>(engine);
  assert(false && "no implementation found for the current engine");

  if constexpr (std::is_void_v<Ret>) {
    return;
  } else {
    return Ret{};
  }
}

template <typename Derived, typename OpTag, typename Fn>
struct DispatchOpTraitsBase;

template <typename Derived, typename OpTag, typename Fn>
struct DispatchOpTraitsRaw;

template <typename Derived, typename OpTag, typename R, typename... Args>
struct DispatchOpTraitsBase<Derived, OpTag, R (*)(OpenPGPContext&, Args...)>
    : OpTraitsBase<Derived, OpTag> {
  static auto Call(OpenPGPContext& ctx, Args... args) -> R {
    assert((OpTraitsBase<Derived, OpTag>::IsSupported(ctx)) &&
           "operation implementation not found for the current engine");

    return DispatchByEngine(ctx, Derived::ImplTable(), args...);
  }

  static auto RoutableCall(OpenPGPContext& ctx, Args... args) -> R {
    assert((OpTraitsBase<Derived, OpTag>::IsRoutable(ctx)) &&
           "operation implementation not found for the current engine");

    return DispatchByEngine(ctx, Derived::ImplTable(), args...);
  }
};

template <typename Derived, typename OpTag, typename R, typename... Args>
struct DispatchOpTraitsRaw<Derived, OpTag, R (*)(Args...)>
    : OpTraitsBase<Derived, OpTag> {
  static auto Call(OpenPGPEngine engine, Args... args) -> R {
    assert((OpTraitsBase<Derived, OpTag>::IsRoutable(engine)) &&
           "operation implementation not found for the current engine");

    return DispatchByEngine(engine, Derived::ImplTable(), args...);
  }
};

#define GF_DEF_OP_IMPL_TRAITS(TAG, FN_REF, ...)                             \
  template <>                                                               \
  struct OpTraits<TAG>                                                      \
      : DispatchOpTraitsBase<OpTraits<TAG>, TAG, decltype(FN_REF)> {        \
    using ImplFn = decltype(FN_REF);                                        \
    static auto ImplTable() -> const auto& {                                \
      static const QContainer<EngineOpImpl<ImplFn>> kTable = {__VA_ARGS__}; \
      return kTable;                                                        \
    }                                                                       \
  };

#define GF_DEF_OP_IMPL_TRAITS_RAW(TAG, FN_REF, ...)                         \
  template <>                                                               \
  struct OpTraits<TAG>                                                      \
      : DispatchOpTraitsRaw<OpTraits<TAG>, TAG, decltype(FN_REF)> {         \
    using ImplFn = decltype(FN_REF);                                        \
    static auto ImplTable() -> const auto& {                                \
      static const QContainer<EngineOpImpl<ImplFn>> kTable = {__VA_ARGS__}; \
      return kTable;                                                        \
    }                                                                       \
  };

}  // namespace GpgFrontend