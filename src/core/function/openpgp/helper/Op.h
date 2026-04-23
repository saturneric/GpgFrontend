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

template <typename ImplFn>
using EngineOpImplTable = QContainer<EngineOpImpl<ImplFn>>;

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

struct EmptyOpTag {};
using EmptyOpTraits = OpTraits<EmptyOpTag>;
using EmptyOpTraitsBase = OpTraitsBase<EmptyOpTraits>;

template <typename Table, typename... Args>
auto DispatchByEngine(OpenPGPContext& ctx, const Table& table, Args&&... args)
    -> decltype(std::declval<typename Table::value_type::second_type>()(
        ctx, std::forward<Args>(args)...)) {
  const auto engine = ctx.Engine();
  for (const auto& [supported_engine, fn] : table) {
    if (supported_engine == engine) {
      return fn(ctx, std::forward<Args>(args)...);
    }
  }

  LOG_E() << "no implementation found for engine: " << static_cast<int>(engine);
  assert(false && "no implementation found for the current engine");
  return {};
}

template <typename Table, typename... Args>
auto DispatchByEngine(OpenPGPEngine engine, const Table& table, Args&&... args)
    -> decltype(std::declval<typename Table::value_type::second_type>()(
        std::forward<Args>(args)...)) {
  for (const auto& [supported_engine, fn] : table) {
    if (supported_engine == engine) {
      return fn(std::forward<Args>(args)...);
    }
  }

  LOG_E() << "no implementation found for engine: " << static_cast<int>(engine);
  assert(false && "no implementation found for the current engine");
  return {};
}

template <typename Derived, typename Fn>
struct DispatchOpTraitsBase;

template <typename Derived, typename Fn>
struct DispatchOpTraitsRaw;

template <typename Derived, typename R, typename... Args>
struct DispatchOpTraitsBase<Derived, R (*)(OpenPGPContext&, Args...)>
    : OpTraitsBase<Derived> {
  static auto Call(OpenPGPContext& ctx, Args... args) -> R {
    return DispatchByEngine(ctx, Derived::ImplTable(), args...);
  }
};

template <typename Derived, typename R, typename... Args>
struct DispatchOpTraitsRaw<Derived, R (*)(Args...)> : OpTraitsBase<Derived> {
  static auto Call(OpenPGPEngine engine, Args... args) -> R {
    return DispatchByEngine(engine, Derived::ImplTable(), args...);
  }
};

#define GF_DEF_OP_TRAITS(TAG, OPNAME, FN_REF, ...)                          \
  struct TAG {};                                                            \
  template <>                                                               \
  struct OpTraits<TAG>                                                      \
      : DispatchOpTraitsBase<OpTraits<TAG>, decltype(FN_REF)> {             \
    static constexpr const char* kOpName = OPNAME;                          \
    using ImplFn = decltype(FN_REF);                                        \
    static auto ImplTable() -> const auto& {                                \
      static const QContainer<EngineOpImpl<ImplFn>> kTable = {__VA_ARGS__}; \
      return kTable;                                                        \
    }                                                                       \
  };

#define GF_DEF_OP_TRAITS_PLUS(TAG, OPNAME, FN_REF, VERS_FN, ...)            \
  struct TAG {};                                                            \
  template <>                                                               \
  struct OpTraits<TAG>                                                      \
      : DispatchOpTraitsBase<OpTraits<TAG>, decltype(FN_REF)> {             \
    static constexpr const char* kOpName = OPNAME;                          \
    using ImplFn = decltype(FN_REF);                                        \
    static auto ImplTable() -> const auto& {                                \
      static const QContainer<EngineOpImpl<ImplFn>> kTable = {__VA_ARGS__}; \
      return kTable;                                                        \
    }                                                                       \
    static auto Versions() -> const auto& { return VERS_FN(); }             \
  };

#define GF_DEF_OP_TRAITS_RAW(TAG, OPNAME, FN_REF, ...)                      \
  struct TAG {};                                                            \
  template <>                                                               \
  struct OpTraits<TAG>                                                      \
      : DispatchOpTraitsRaw<OpTraits<TAG>, decltype(FN_REF)> {              \
    static constexpr const char* kOpName = OPNAME;                          \
    using ImplFn = decltype(FN_REF);                                        \
    static auto ImplTable() -> const auto& {                                \
      static const QContainer<EngineOpImpl<ImplFn>> kTable = {__VA_ARGS__}; \
      return kTable;                                                        \
    }                                                                       \
  };
}  // namespace GpgFrontend