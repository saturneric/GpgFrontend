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

#include <cstddef>
#include <type_traits>

#include "GFSDKContext.h"

/**
 * @file GFSdkInternal.h
 * @brief The shape every public SDK function has, and nothing else.
 *
 * ## Stateless, and what that costs
 *
 * There is no file-scope variable anywhere in this directory. No bound table,
 * no current module, no thread-local, not even a "have I already complained"
 * flag. Everything a call needs arrives in its @ref GFSDKContext argument.
 *
 * The one visible consequence is that a capability denial is reported on
 * every call rather than once, because remembering would be state. A module
 * that ignores a denial inside a loop will say so in the log a great many
 * times. That is the right trade: a module is expected to ask once, at
 * activation, whether it holds what it needs, and the facade's Has() is there
 * for exactly that.
 *
 * ## Two failure shapes, deliberately distinguished
 *
 * A NULL group means the module was never granted this capability, and the
 * host never sees the call at all; that is what @ref GFSdkReportUnavailable
 * exists for. A refusal INSIDE a primitive means the host looked at the
 * context and said no, and the host logs that itself, with the reason it
 * knows and this layer does not: whether the module was torn down or simply
 * never had the grant.
 */

namespace gf_sdk_api {

/**
 * @brief Report that @p entry_point needs a group this module does not have.
 *
 * Written through the host's log group, which is always granted, so a denial
 * is visible even for a module that holds nothing else. Silent when there is
 * no usable context at all, because there is then nowhere to write to.
 */
void GFSdkReportUnavailable(const GFSDKContext* ctx, const char* group,
                            const char* entry_point);

}  // namespace gf_sdk_api

/**
 * @brief Open a public SDK function that needs capability group @p member.
 *
 * Declares `g` as the group and `hctx` as the authorization token, read from
 * the same table `g` came from, so the two cannot be a mismatched pair.
 */
#define GF_SDK_REQUIRE(ctx, member, name, failure)                \
  if ((ctx) == nullptr || (ctx)->host == nullptr ||               \
      (ctx)->host->member == nullptr) {                           \
    ::gf_sdk_api::GFSdkReportUnavailable((ctx), #member, (name)); \
    return failure;                                               \
  }                                                               \
  const auto* g = (ctx)->host->member;                            \
  auto* hctx = (ctx)->host->context

/**
 * @brief Whether group @p g is long enough to contain member @p field.
 *
 * Group structs only grow by appending, and each begins with the
 * `struct_size` the HOST compiled it with. Today every host this SDK accepts
 * (GF_SDK_ABI_MIN_SUPPORTED) has every member, so no wrapper needs this. A
 * member appended later does: its wrapper must check it before calling,
 * because an older host's table simply ends before it.
 *
 *   GF_SDK_REQUIRE(ctx, app, "GFAppNewThing", -1);
 *   if (!GF_SDK_GROUP_HAS(g, new_thing)) return -1;
 */
#define GF_SDK_GROUP_HAS(g, field)                                             \
  ((g)->struct_size >=                                                         \
       offsetof(std::remove_cv_t<std::remove_pointer_t<decltype(g)>>, field) + \
           sizeof((g)->field) &&                                               \
   (g)->field != nullptr)

/// The void-returning form.
#define GF_SDK_REQUIRE_VOID(ctx, member, name)                    \
  if ((ctx) == nullptr || (ctx)->host == nullptr ||               \
      (ctx)->host->member == nullptr) {                           \
    ::gf_sdk_api::GFSdkReportUnavailable((ctx), #member, (name)); \
    return;                                                       \
  }                                                               \
  const auto* g = (ctx)->host->member;                            \
  auto* hctx = (ctx)->host->context
