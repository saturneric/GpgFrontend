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

#include "GFSDKContext.h"

/**
 * @file GFSDKWebEditor.h
 * @brief An editable page of the module's own, inside one of its widgets.
 *
 * Unlike an HTML view, script runs -- but only the module's OWN script: the
 * page is served from the module's embedded WEB_ASSETS (gfasset://<session>/)
 * and nothing else, under a Content-Security-Policy the Host puts first in
 * index.html (`script-src gfasset:`, no eval, no inline script, no network).
 * Resources the module puts (images) are served from Host memory under
 * gfres://<session>/<path>. Nothing is persisted by the engine.
 *
 * Page and module talk in JSON objects only. The Host checks each message --
 * a JSON object, a string "type", at most GF_WEB_EDITOR_MAX_MESSAGE_BYTES --
 * before the consumer hears it, and never logs its content.
 *
 * Every call: GUI thread.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* --- consumers (capability "web.editor") --------------------------------- */

/** @return GF_HTML_VIEW_OK, or why an editor cannot be made now. */
int GFWebEditorAvailable(GFSDKContext* ctx);

/**
 * @brief Make an editor in @p slot, an empty widget inside native widget
 *        @p instance, and load the module's index.html into it.
 */
int GFWebEditorCreate(GFSDKContext* ctx, uint64_t instance, void* slot,
                      GFWebEditorEventFn fn, void* user, uint64_t* out_editor);

/** @brief Send one JSON object to the page. */
int GFWebEditorPost(GFSDKContext* ctx, uint64_t editor, const char* json,
                    size_t len);

/**
 * @brief Serve @p data at gfres://<session>/@p path. @p data is TRANSFERRED.
 *        Images only (png, jpeg, gif, webp).
 */
int GFWebEditorPutResource(GFSDKContext* ctx, uint64_t editor, const char* path,
                           const char* mime, GFBufferRef data);

int GFWebEditorRemoveResource(GFSDKContext* ctx, uint64_t editor,
                              const char* path);

/** @brief Gone, with every resource; the module hears no LOST for it. */
int GFWebEditorDestroy(GFSDKContext* ctx, uint64_t editor);

/* --- engines (capability "web.editor.provider") -------------------------- */

int GFWebEditorProviderRegister(GFSDKContext* ctx,
                                const GFWebEditorProviderSpec* spec);
int GFWebEditorProviderUnregister(GFSDKContext* ctx, const char* name);
int GFWebEditorProviderResolve(GFSDKContext* ctx, uint64_t editor,
                               const char* url, GFBufferRef* out_data,
                               GFBufferRef* out_mime);
int GFWebEditorProviderMessage(GFSDKContext* ctx, uint64_t editor,
                               const char* json, size_t len);
int GFWebEditorProviderLinkActivated(GFSDKContext* ctx, uint64_t editor,
                                     const char* url);
int GFWebEditorProviderReady(GFSDKContext* ctx, uint64_t editor);

#ifdef __cplusplus
}
#endif
