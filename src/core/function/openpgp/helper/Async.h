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

#include "core/function/openpgp/OpenPGPContext.h"
#include "core/function/openpgp/helper/Op.h"
#include "core/utils/AsyncUtils.h"

namespace GpgFrontend {

/**
 * @brief Schedule an OpTraits<OpTag> operation on the GPG task runner and
 * invoke @p cb on completion.
 *
 * Checks engine/version support via OpTraits<OpTag>::Versions(), then posts the
 * operation to the GPG task runner. The callback is delivered to the calling
 * thread when done.
 *
 * @tparam OpTag operation tag whose OpTraits specialization provides Call() and
 * Versions()
 * @tparam Args  argument types forwarded to OpTraits<OpTag>::Call()
 * @param ctx  OpenPGP context to execute the operation on
 * @param cb   callback invoked on the calling thread with (GpgError,
 * DataObjectPtr)
 * @param args arguments forwarded to the operation implementation
 */
template <typename OpTag, typename... Args>
void RunRegisteredAsync(OpenPGPContext& ctx, const GpgOperationCallback& cb,
                        Args&&... args) {
  auto stored_args =
      std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);

  RunGpgOperaAsync(
      ctx.GetChannel(),
      [ctx_ptr = &ctx, stored_args = std::move(stored_args)](
          const DataObjectPtr& data_object) -> GpgError {
        return std::apply(
            [&](auto&&... unpacked) -> GpgError {
              return OpTraits<OpTag>::Call(*ctx_ptr, unpacked..., data_object);
            },
            stored_args);
      },
      cb, OpTraits<OpTag>::kOpName, OpTraits<OpTag>::Versions());
}

/**
 * @brief Run an OpTraits<OpTag> operation synchronously on the calling thread.
 *
 * Checks engine/version support via OpTraits<OpTag>::Versions() and executes
 * the operation immediately without posting to a task runner.
 *
 * @tparam OpTag operation tag whose OpTraits specialization provides Call() and
 * Versions()
 * @tparam Args  argument types forwarded to OpTraits<OpTag>::Call()
 * @param ctx  OpenPGP context to execute the operation on
 * @param args arguments forwarded to the operation implementation
 * @return tuple of (GpgError, DataObjectPtr) with the operation result
 */
template <typename OpTag, typename... Args>
auto RunRegisteredSync(OpenPGPContext& ctx, Args&&... args)
    -> std::tuple<GpgError, DataObjectPtr> {
  auto stored_args =
      std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);

  return RunGpgOperaSync(
      ctx.GetChannel(),
      [ctx_ptr = &ctx, stored_args = std::move(stored_args)](
          const DataObjectPtr& data_object) -> GpgError {
        return std::apply(
            [&](auto&&... unpacked) -> GpgError {
              return OpTraits<OpTag>::Call(*ctx_ptr, unpacked..., data_object);
            },
            stored_args);
      },
      OpTraits<OpTag>::kOpName, OpTraits<OpTag>::Versions());
}

/**
 * @brief Call OpTraits<OpTag>::Call() directly, forwarding all arguments.
 *
 * Invokes the operation synchronously without any async dispatch or support
 * check. Asserts that an implementation exists for the context's engine.
 *
 * @tparam OpTag operation tag whose OpTraits specialization provides Call()
 * @tparam Args  argument types forwarded to OpTraits<OpTag>::Call()
 * @param ctx  OpenPGP context passed as the first argument to Call()
 * @param args remaining arguments forwarded to Call()
 * @return whatever OpTraits<OpTag>::Call() returns
 */
template <typename OpTag, typename... Args>
auto RunRegisteredForward(OpenPGPContext& ctx, Args&&... args) {
  auto stored_args =
      std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);

  return std::apply(
      [&](auto&&... unpacked) {
        return OpTraits<OpTag>::Call(ctx, unpacked...);
      },
      stored_args);
}

/**
 * @brief Call OpTraits<OpTag>::RoutableCall() directly, forwarding all
 * arguments.
 *
 * Like RunRegisteredForward() but uses RoutableCall(), which only asserts that
 * an implementation exists for the engine — it does not check version support.
 * Use when you want to dispatch to any available implementation regardless of
 * version constraints.
 *
 * @tparam OpTag operation tag whose OpTraits specialization provides
 * RoutableCall()
 * @tparam Args  argument types forwarded to RoutableCall()
 * @param ctx  OpenPGP context passed as the first argument to RoutableCall()
 * @param args remaining arguments forwarded to RoutableCall()
 * @return whatever OpTraits<OpTag>::RoutableCall() returns
 */
template <typename OpTag, typename... Args>
auto RunRegisteredRoutableForward(OpenPGPContext& ctx, Args&&... args) {
  auto stored_args =
      std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);

  return std::apply(
      [&](auto&&... unpacked) {
        return OpTraits<OpTag>::RoutableCall(ctx, unpacked...);
      },
      stored_args);
}

/**
 * @brief Call OpTraits<OpTag>::Call() dispatching by raw OpenPGPEngine enum.
 *
 * Used for operations whose implementation does not take an OpenPGPContext&
 * (defined with GF_DEF_OP_IMPL_TRAITS_RAW). The engine is passed directly
 * without a context object.
 *
 * @tparam OpTag operation tag whose OpTraits specialization provides a raw
 * Call()
 * @tparam Args  argument types forwarded to Call()
 * @param engine engine enum value used for dispatch
 * @param args   arguments forwarded to the selected implementation
 * @return whatever OpTraits<OpTag>::Call() returns
 */
template <typename OpTag, typename... Args>
auto RunRegisteredRawForward(OpenPGPEngine& engine, Args&&... args) {
  auto stored_args =
      std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);

  return std::apply(
      [&](auto&&... unpacked) {
        return OpTraits<OpTag>::Call(engine, unpacked...);
      },
      stored_args);
}

}  // namespace GpgFrontend
