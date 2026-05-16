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

// Primary template — specialised by GF_DEF_OP_IMPL_TRAITS for each op tag.
template <typename OpTag>
struct OpTraits;

// A (engine, implementation-function) pair used to build dispatch tables.
template <typename ImplFn>
using EngineOpImpl = QPair<OpenPGPEngine, ImplFn>;

// An ordered list of EngineOpImpl entries forming a dispatch table for one op.
template <typename ImplFn>
using EngineOpImplTable = QContainer<EngineOpImpl<ImplFn>>;

/**
 * @brief CRTP base adding engine-availability checks to OpSupportTraits.
 *
 * Provides HasImpl(), IsRoutable(), and IsSupported() on top of the support
 * requirements from OpSupportTraitsBase. Subclasses must supply ImplTable()
 * returning the list of (engine, function) pairs.
 *
 * @tparam Derived the concrete OpTraits specialisation
 * @tparam OpTag the operation tag type
 */
template <typename Derived, typename OpTag>
struct OpTraitsBase : OpSupportTraits<OpTag> {
  /**
   * @brief Return whether a concrete implementation exists for @p engine.
   *
   * @param engine engine to check
   * @return true if the impl table contains an entry for @p engine
   */
  static auto HasImpl(OpenPGPEngine engine) -> bool {
    for (const auto& [supported_engine, fn] : Derived::ImplTable()) {
      if (supported_engine == engine) return true;
    }
    return false;
  }

  /**
   * @brief Return whether any implementation exists for the engine of @p ctx.
   *
   * Does not check version requirements — use IsSupported() for that.
   *
   * @param ctx OpenPGP context whose engine is checked
   * @return true if an implementation is available
   */
  static auto IsRoutable(const OpenPGPContext& ctx) -> bool {
    return HasImpl(ctx.Engine());
  }

  /**
   * @brief Return whether any implementation exists for the given engine enum.
   *
   * @param engine engine to check
   * @return true if an implementation is available
   */
  static auto IsRoutable(OpenPGPEngine engine) -> bool {
    return HasImpl(engine);
  }

  /**
   * @brief Return whether the operation is both implemented and
   * version-supported on @p ctx.
   *
   * @param ctx OpenPGP context to test
   * @return true if an implementation exists and all version requirements are
   * met
   */
  static auto IsSupported(const OpenPGPContext& ctx) -> bool {
    return HasImpl(ctx.Engine()) && OpSupportTraits<OpTag>::IsSupported(ctx);
  }
};

/**
 * @brief Dispatch a call to the engine-specific implementation in @p table
 * (context overload).
 *
 * Iterates the impl table and invokes the first entry whose engine matches
 * @p ctx.Engine(). Asserts and returns a default if no match is found.
 *
 * @tparam Table impl table type (EngineOpImplTable)
 * @tparam Args  additional argument types
 * @param ctx   OpenPGP context supplying the active engine
 * @param table dispatch table of (engine, function) pairs
 * @param args  arguments forwarded to the selected implementation
 * @return return value of the selected implementation, or default-constructed
 * Ret on mismatch
 */
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

/**
 * @brief Dispatch a call to the engine-specific implementation in @p table (raw
 * engine overload).
 *
 * Like the context overload but selects the implementation by @p engine enum
 * and does not pass a context to the implementation function.
 *
 * @tparam Table impl table type (EngineOpImplTable)
 * @tparam Args  additional argument types
 * @param engine engine enum value used for dispatch
 * @param table  dispatch table of (engine, function) pairs
 * @param args   arguments forwarded to the selected implementation
 * @return return value of the selected implementation, or default-constructed
 * Ret on mismatch
 */
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

// Forward declarations for the partial specializations below.
template <typename Derived, typename OpTag, typename Fn>
struct DispatchOpTraitsBase;

template <typename Derived, typename OpTag, typename Fn>
struct DispatchOpTraitsRaw;

/**
 * @brief OpTraitsBase specialization for operations whose impl takes
 * OpenPGPContext&.
 *
 * Provides Call() (which asserts full IsSupported) and RoutableCall() (which
 * only asserts IsRoutable, bypassing version checks).
 *
 * @tparam Derived the concrete OpTraits specialization
 * @tparam OpTag   operation tag type
 * @tparam R       return type of the implementation function
 * @tparam Args    remaining argument types after OpenPGPContext&
 */
template <typename Derived, typename OpTag, typename R, typename... Args>
struct DispatchOpTraitsBase<Derived, OpTag, R (*)(OpenPGPContext&, Args...)>
    : OpTraitsBase<Derived, OpTag> {
  /**
   * @brief Invoke the implementation for @p ctx's engine (asserts full
   * support).
   */
  static auto Call(OpenPGPContext& ctx, Args... args) -> R {
    assert((OpTraitsBase<Derived, OpTag>::IsSupported(ctx)) &&
           "operation implementation not found for the current engine");

    return DispatchByEngine(ctx, Derived::ImplTable(), args...);
  }

  /**
   * @brief Invoke the implementation for @p ctx's engine (asserts impl exists,
   * ignores version).
   */
  static auto RoutableCall(OpenPGPContext& ctx, Args... args) -> R {
    assert((OpTraitsBase<Derived, OpTag>::IsRoutable(ctx)) &&
           "operation implementation not found for the current engine");

    return DispatchByEngine(ctx, Derived::ImplTable(), args...);
  }
};

/**
 * @brief OpTraitsBase specialisation for raw operations whose impl takes only
 * args (no context).
 *
 * Provides Call() dispatching by OpenPGPEngine enum.
 *
 * @tparam Derived the concrete OpTraits specialisation
 * @tparam OpTag   operation tag type
 * @tparam R       return type of the implementation function
 * @tparam Args    argument types of the implementation function
 */
template <typename Derived, typename OpTag, typename R, typename... Args>
struct DispatchOpTraitsRaw<Derived, OpTag, R (*)(Args...)>
    : OpTraitsBase<Derived, OpTag> {
  /**
   * @brief Invoke the implementation for the given @p engine (asserts impl
   * exists).
   */
  static auto Call(OpenPGPEngine engine, Args... args) -> R {
    assert((OpTraitsBase<Derived, OpTag>::IsRoutable(engine)) &&
           "operation implementation not found for the current engine");

    return DispatchByEngine(engine, Derived::ImplTable(), args...);
  }
};

/**
 * @brief Specialise OpTraits<TAG> for a context-taking operation with an engine
 * dispatch table.
 *
 * @p FN_REF is the reference implementation used to deduce the function
 * signature. The variadic arguments are {engine, &impl_fn} pairs forming the
 * dispatch table.
 *
 * Usage:
 * @code
 * GF_DEF_OP_IMPL_TRAITS(MyOpTag, &MyOpGnuPGImpl,
 *     {OpenPGPEngine::kGNUPG, &MyOpGnuPGImpl},
 *     {OpenPGPEngine::kRPGP,  &MyOpRpgpImpl});
 * @endcode
 */
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

/**
 * @brief Specialise OpTraits<TAG> for a raw operation (no OpenPGPContext&
 * argument).
 *
 * Same as GF_DEF_OP_IMPL_TRAITS but uses DispatchOpTraitsRaw and dispatches
 * by OpenPGPEngine enum rather than by context.
 */
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
