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

#include "GFSDKHtmlView.h"

#include "GFSDKBuffer.h"
#include "GFSdkInternal.h"

/**
 * @file GFSdkHtmlView.cpp
 * @brief HTML views and their renderers, module-side.
 *
 * A module without the capability gets E_DENIED, with the reason logged,
 * except from GFHtmlViewAvailable(): "is there a view for me" is honestly
 * answered "no" by a module that may not have one.
 */

auto GFHtmlViewAvailable(GFSDKContext* ctx, int preset) -> int {
  GF_SDK_REQUIRE(ctx, html_view, "GFHtmlViewAvailable",
                 GF_HTML_VIEW_E_UNAVAILABLE);
  return g->available(hctx, preset);
}

auto GFHtmlViewCreate(GFSDKContext* ctx, uint64_t instance, void* slot,
                      int preset, GFHtmlViewEventFn fn, void* user,
                      uint64_t* out_view) -> int {
  GF_SDK_REQUIRE(ctx, html_view, "GFHtmlViewCreate", GF_HTML_VIEW_E_DENIED);
  return g->create_view(hctx, instance, slot, preset, fn, user, out_view);
}

auto GFHtmlViewLoad(GFSDKContext* ctx, uint64_t view, GFBufferRef html_utf8,
                    const GFHtmlPart* parts, size_t part_count,
                    size_t* out_dropped) -> int {
  if (ctx == nullptr || ctx->host == nullptr ||
      !GF_SDK_GROUP_HAS(ctx->host, html_view)) {
    // Transferred on failure too: refused buffers must not leak, and the
    // module has no other way to know they were not taken.
    GFBufferRelease(ctx, html_utf8);
    for (size_t i = 0; parts != nullptr && i < part_count; ++i) {
      GFBufferRelease(ctx, parts[i].data);
    }
  }
  GF_SDK_REQUIRE(ctx, html_view, "GFHtmlViewLoad", GF_HTML_VIEW_E_DENIED);
  return g->load(hctx, view, html_utf8, parts, part_count, out_dropped);
}

auto GFHtmlViewClear(GFSDKContext* ctx, uint64_t view) -> int {
  GF_SDK_REQUIRE(ctx, html_view, "GFHtmlViewClear", GF_HTML_VIEW_E_DENIED);
  return g->clear(hctx, view);
}

auto GFHtmlViewDestroy(GFSDKContext* ctx, uint64_t view) -> int {
  GF_SDK_REQUIRE(ctx, html_view, "GFHtmlViewDestroy", GF_HTML_VIEW_E_DENIED);
  return g->destroy_view(hctx, view);
}

auto GFHtmlProviderRegister(GFSDKContext* ctx,
                            const GFHtmlViewProviderSpec* spec) -> int {
  GF_SDK_REQUIRE(ctx, html_provider, "GFHtmlProviderRegister",
                 GF_HTML_VIEW_E_DENIED);
  return g->register_provider(hctx, spec);
}

auto GFHtmlProviderUnregister(GFSDKContext* ctx, const char* name) -> int {
  GF_SDK_REQUIRE(ctx, html_provider, "GFHtmlProviderUnregister",
                 GF_HTML_VIEW_E_DENIED);
  return g->unregister_provider(hctx, name);
}

auto GFHtmlProviderRequestPart(GFSDKContext* ctx, uint64_t view,
                               const char* session, const char* cid,
                               GFHtmlPartDoneFn done, void* user,
                               uint64_t* out_request) -> int {
  GF_SDK_REQUIRE(ctx, html_provider, "GFHtmlProviderRequestPart",
                 GF_HTML_VIEW_E_DENIED);
  return g->request_part(hctx, view, session, cid, done, user, out_request);
}

auto GFHtmlProviderCancelPart(GFSDKContext* ctx, uint64_t request) -> int {
  GF_SDK_REQUIRE(ctx, html_provider, "GFHtmlProviderCancelPart",
                 GF_HTML_VIEW_E_DENIED);
  return g->cancel_part(hctx, request);
}

auto GFHtmlProviderLinkActivated(GFSDKContext* ctx, uint64_t view,
                                 const char* url) -> int {
  GF_SDK_REQUIRE(ctx, html_provider, "GFHtmlProviderLinkActivated",
                 GF_HTML_VIEW_E_DENIED);
  return g->link_activated(hctx, view, url);
}

auto GFHtmlProviderLoaded(GFSDKContext* ctx, uint64_t view, int ok) -> int {
  GF_SDK_REQUIRE(ctx, html_provider, "GFHtmlProviderLoaded",
                 GF_HTML_VIEW_E_DENIED);
  return g->loaded(hctx, view, ok);
}
