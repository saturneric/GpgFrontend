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

#include "GFSDKWebEditor.h"

#include "GFSDKBuffer.h"
#include "GFSdkInternal.h"

/**
 * @file GFSdkWebEditor.cpp
 * @brief Web editors and their engines, module-side; the rules of
 * GFSdkHtmlView.cpp: E_DENIED without the capability, except from
 * GFWebEditorAvailable(), which honestly answers "no".
 */

auto GFWebEditorAvailable(GFSDKContext* ctx) -> int {
  GF_SDK_REQUIRE(ctx, web_editor, "GFWebEditorAvailable",
                 GF_HTML_VIEW_E_UNAVAILABLE);
  return g->available(hctx);
}

auto GFWebEditorCreate(GFSDKContext* ctx, uint64_t instance, void* slot,
                       GFWebEditorEventFn fn, void* user, uint64_t* out_editor)
    -> int {
  GF_SDK_REQUIRE(ctx, web_editor, "GFWebEditorCreate", GF_HTML_VIEW_E_DENIED);
  return g->create(hctx, instance, slot, fn, user, out_editor);
}

auto GFWebEditorPost(GFSDKContext* ctx, uint64_t editor, const char* json,
                     size_t len) -> int {
  GF_SDK_REQUIRE(ctx, web_editor, "GFWebEditorPost", GF_HTML_VIEW_E_DENIED);
  return g->post(hctx, editor, json, len);
}

auto GFWebEditorPutResource(GFSDKContext* ctx, uint64_t editor,
                            const char* path, const char* mime,
                            GFBufferRef data) -> int {
  if (ctx == nullptr || ctx->host == nullptr ||
      !GF_SDK_GROUP_HAS(ctx->host, web_editor)) {
    // Transferred whatever happens: refused here, it is still released.
    GFBufferRelease(ctx, data);
  }
  GF_SDK_REQUIRE(ctx, web_editor, "GFWebEditorPutResource",
                 GF_HTML_VIEW_E_DENIED);
  return g->put_resource(hctx, editor, path, mime, data);
}

auto GFWebEditorRemoveResource(GFSDKContext* ctx, uint64_t editor,
                               const char* path) -> int {
  GF_SDK_REQUIRE(ctx, web_editor, "GFWebEditorRemoveResource",
                 GF_HTML_VIEW_E_DENIED);
  return g->remove_resource(hctx, editor, path);
}

auto GFWebEditorDestroy(GFSDKContext* ctx, uint64_t editor) -> int {
  GF_SDK_REQUIRE(ctx, web_editor, "GFWebEditorDestroy", GF_HTML_VIEW_E_DENIED);
  return g->destroy(hctx, editor);
}

auto GFWebEditorProviderRegister(GFSDKContext* ctx,
                                 const GFWebEditorProviderSpec* spec) -> int {
  GF_SDK_REQUIRE(ctx, web_editor_provider, "GFWebEditorProviderRegister",
                 GF_HTML_VIEW_E_DENIED);
  return g->register_provider(hctx, spec);
}

auto GFWebEditorProviderUnregister(GFSDKContext* ctx, const char* name) -> int {
  GF_SDK_REQUIRE(ctx, web_editor_provider, "GFWebEditorProviderUnregister",
                 GF_HTML_VIEW_E_DENIED);
  return g->unregister_provider(hctx, name);
}

auto GFWebEditorProviderResolve(GFSDKContext* ctx, uint64_t editor,
                                const char* url, GFBufferRef* out_data,
                                GFBufferRef* out_mime) -> int {
  GF_SDK_REQUIRE(ctx, web_editor_provider, "GFWebEditorProviderResolve",
                 GF_HTML_VIEW_E_DENIED);
  return g->resolve(hctx, editor, url, out_data, out_mime);
}

auto GFWebEditorProviderMessage(GFSDKContext* ctx, uint64_t editor,
                                const char* json, size_t len) -> int {
  GF_SDK_REQUIRE(ctx, web_editor_provider, "GFWebEditorProviderMessage",
                 GF_HTML_VIEW_E_DENIED);
  return g->message(hctx, editor, json, len);
}

auto GFWebEditorProviderLinkActivated(GFSDKContext* ctx, uint64_t editor,
                                      const char* url) -> int {
  GF_SDK_REQUIRE(ctx, web_editor_provider, "GFWebEditorProviderLinkActivated",
                 GF_HTML_VIEW_E_DENIED);
  return g->link_activated(hctx, editor, url);
}

auto GFWebEditorProviderReady(GFSDKContext* ctx, uint64_t editor) -> int {
  GF_SDK_REQUIRE(ctx, web_editor_provider, "GFWebEditorProviderReady",
                 GF_HTML_VIEW_E_DENIED);
  return g->ready(hctx, editor);
}
