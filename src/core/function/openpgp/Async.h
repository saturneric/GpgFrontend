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

#include "core/function/gpg/GpgContext.h"
#include "core/function/openpgp/Op.h"
#include "core/utils/AsyncUtils.h"

namespace GpgFrontend {

/**
 * @brief
 *
 * @tparam OpTag
 * @tparam Args
 * @param channel
 * @param ctx
 * @param cb
 * @param args
 */
template <typename OpTag, typename... Args>
void RunRegisteredAsync(GpgContext& ctx, const GpgOperationCallback& cb,
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
 * @brief
 *
 * @tparam OpTag
 * @tparam Args
 * @param channel
 * @param ctx
 * @param args
 * @return std::tuple<GpgError, DataObjectPtr>
 */
template <typename OpTag, typename... Args>
auto RunRegisteredSync(GpgContext& ctx, Args&&... args)
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
 * @brief
 *
 * @tparam OpTag
 * @tparam Args
 * @param ctx
 * @param args
 * @return std::tuple<GpgError, DataObjectPtr>
 */
template <typename OpTag, typename... Args>
auto RunRegisteredDirect(GpgContext& ctx, Args&&... args) -> GpgError {
  auto stored_args =
      std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);

  return std::apply(
      [&](auto&&... unpacked) -> GpgError {
        return OpTraits<OpTag>::Call(ctx, unpacked...);
      },
      stored_args);
}
}  // namespace GpgFrontend