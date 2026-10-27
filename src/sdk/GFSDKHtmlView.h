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

#include <stddef.h>
#include <stdint.h>

#include "GFSDKContext.h"

/**
 * @file GFSDKHtmlView.h
 * @brief Read-only HTML views, rendered by whichever engine is present.
 *
 * Two halves. A CONSUMER (capability "html.view") embeds a view into a
 * widget of its own and hands it a body and its parts. A RENDERER
 * (capability "html.provider") supplies the engine. The Host sits between
 * them and owns everything that decides safety: the policy, the document
 * the body is wrapped in, which slot a view may occupy, and which parts a
 * document may ask for.
 *
 * The consumer never chooses an engine and never sees one. Without a
 * renderer every call answers GF_HTML_VIEW_E_UNAVAILABLE, and a consumer
 * shows its content another way.
 *
 * This is a VIEW. Editing rich text is a separate interface, if it ever
 * exists, and is never added here as flags.
 *
 * Ownership. Every GFBufferRef handed to a function here is TRANSFERRED to
 * the Host, on success and failure alike; part arrays are borrowed and their
 * buffers are not. Every call is GUI-thread only.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* The status codes, policy, part, event and renderer types are in
 * GFSDKTypes.h, because GFSDKHostApi.h needs them too. */

/* --- consumers (capability "html.view") --------------------------------- */

/**
 * @brief Whether a view under @p preset could be created now.
 *
 * GF_HTML_VIEW_OK, GF_HTML_VIEW_E_UNAVAILABLE (no renderer),
 * GF_HTML_VIEW_E_RESTART_REQUIRED (one is installed but not usable until
 * the application restarts) or GF_HTML_VIEW_E_POLICY (none enforces it).
 */
int GFHtmlViewAvailable(GFSDKContext* ctx, int preset);

/**
 * @brief Put a view into @p slot.
 *
 * @p instance is one of this module's live native widget instances, and
 * @p slot that widget or one of its descendants, empty. The view fills it.
 * @p fn receives the view's events, with @p user.
 */
int GFHtmlViewCreate(GFSDKContext* ctx, uint64_t instance, void* slot,
                     int preset, GFHtmlViewEventFn fn, void* user,
                     uint64_t* out_view);

/**
 * @brief Show @p html_utf8, a decoded body, with @p parts available to it.
 *
 * The Host wraps the body in a document of its own and never parses it. A
 * part outside the allowlist or over a limit is dropped and counted in
 * @p out_dropped; the body over the document limit is refused.
 */
int GFHtmlViewLoad(GFSDKContext* ctx, uint64_t view, GFBufferRef html_utf8,
                   const GFHtmlPart* parts, size_t part_count,
                   size_t* out_dropped);

/** Show nothing, and drop the body and parts. */
int GFHtmlViewClear(GFSDKContext* ctx, uint64_t view);

/** Remove the view. Its events stop before this returns. */
int GFHtmlViewDestroy(GFSDKContext* ctx, uint64_t view);

/* --- renderers (capability "html.provider") ----------------------------- */

/** Provide a renderer. Withdrawn automatically at deactivation. */
int GFHtmlProviderRegister(GFSDKContext* ctx,
                           const GFHtmlViewProviderSpec* spec);
int GFHtmlProviderUnregister(GFSDKContext* ctx, const char* name);

/**
 * @brief Ask for one part of the document shown in @p view.
 *
 * @p session must be the session that document was loaded under, and the
 * view one this renderer serves. @p done reports the outcome later, never
 * inside this call.
 */
int GFHtmlProviderRequestPart(GFSDKContext* ctx, uint64_t view,
                              const char* session, const char* cid,
                              GFHtmlPartDoneFn done, void* user,
                              uint64_t* out_request);

/** Cancel a part request. After it returns, its `done` never runs. */
int GFHtmlProviderCancelPart(GFSDKContext* ctx, uint64_t request);

/** The user activated a link in @p view. The Host decides what happens. */
int GFHtmlProviderLinkActivated(GFSDKContext* ctx, uint64_t view,
                                const char* url);

/** The document in @p view finished loading: @p ok 1, or 0 for failed. */
int GFHtmlProviderLoaded(GFSDKContext* ctx, uint64_t view, int ok);

#ifdef __cplusplus
}
#endif
