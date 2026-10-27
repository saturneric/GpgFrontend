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

/**
 * @file HtmlViewDocument.h
 * @brief The document an HTML view is given, and the policy it is under.
 *
 * Pure: no widget, no engine, no thread affinity. Everything here decides
 * what a renderer is ALLOWED to see, so it is kept where a test can reach
 * every branch of it without a GUI.
 *
 * The body is never parsed. It is made inert as one attribute value -- the
 * srcdoc of a sandboxed iframe inside a document the Host writes -- so there
 * is no sanitizer to be wrong, and nothing in the sender's markup can reach
 * the outer document's <head>, where the policy is.
 */

namespace GpgFrontend::UI::HtmlView {

/// The GF_HTML_POLICY_* bits, spelled here so gf_ui need not include the
/// SDK. GFHostHtmlView.cpp asserts that the two agree.
enum PolicyBit : uint32_t {
  kNO_NETWORK = 1U << 0,
  kPARTS_ONLY = 1U << 1,
  kNO_SCRIPT = 1U << 2,
  kNO_NAVIGATION = 1U << 3,
  kNO_POPUPS = 1U << 4,
  kNO_DOWNLOADS = 1U << 5,
  kNO_PERSISTENCE = 1U << 6,
  kNO_PERMISSIONS = 1U << 7,
  kNO_DNS_PREFETCH = 1U << 8,
  kNO_PLUGINS = 1U << 9,
};

/// GF_HTML_VIEW_PRESET_MAIL.
constexpr int kPresetMail = 1;

/// The scheme every part is loaded from; GF_HTML_PART_SCHEME.
constexpr auto kPartScheme = "gfpart";

/// A fixed policy, as the Host fills it in from a preset.
struct GF_UI_EXPORT Policy {
  int preset = 0;
  uint32_t required = 0;
  qint64 max_document_bytes = 0;
  qint64 max_part_bytes = 0;
  qint64 max_total_part_bytes = 0;
};

/// The policy @p preset names, or nullopt for a preset this Host lacks.
auto GF_UI_EXPORT PolicyFor(int preset) -> std::optional<Policy>;

/// A fresh session name: 32 lower-case hex digits from the system RNG.
auto GF_UI_EXPORT NewSession() -> QString;

/// Whether @p session is shaped like one NewSession() could have made.
auto GF_UI_EXPORT IsSessionName(const QString& session) -> bool;

/**
 * @brief A Content-ID, or the cid: URL naming one, as one comparable key.
 *
 * RFC 2392: the URL form percent-encodes the addr-spec and drops the angle
 * brackets the header form has. Both arrive here; both become the bare,
 * decoded addr-spec. Empty when nothing is left.
 */
auto GF_UI_EXPORT NormalizeCid(const QString& cid) -> QString;

/// The URL a renderer loads @p cid from, inside @p session.
auto GF_UI_EXPORT PartUrl(const QString& session, const QString& cid)
    -> QString;

/**
 * @brief Point the body's cid: references at @p session.
 *
 * Only where an image can be named: the src, background and poster
 * attributes, each candidate of a srcset, and CSS url(). Anything else is
 * left exactly as written and, under PARTS_ONLY, loads nothing.
 */
auto GF_UI_EXPORT RewriteCidReferences(const QString& body,
                                       const QString& session) -> QString;

/// @p text as the value of a double-quoted attribute: & " < > escaped, and
/// NUL, which no HTML parser keeps, replaced.
auto GF_UI_EXPORT EscapeAttribute(const QString& text) -> QString;

/// The Content-Security-Policy the outer document carries.
auto GF_UI_EXPORT ContentSecurityPolicy() -> QString;

/// The sandbox the body's frame runs in. Never allow-scripts, allow-forms or
/// allow-downloads.
///
/// @p frame_same_origin grants the frame allow-same-origin: it then shares the
/// document's own origin (the part scheme) instead of an opaque one. A renderer
/// asks for this only when its engine will not otherwise load the part scheme
/// into the frame -- WebView2 refuses a custom scheme from an opaque origin,
/// where WebKit and QtWebEngine allow it. It stays safe because no script can
/// ever run (no allow-scripts, scripting off in the engine, no CSP script-src),
/// so the only thing same-origin unlocks is loading the view's own parts, each
/// still gated to the current session by the host.
auto GF_UI_EXPORT SandboxTokens(bool frame_same_origin = false) -> QString;

/**
 * @brief The whole document a renderer is handed, as UTF-8.
 *
 * The policy first in <head>, then the body -- references rewritten,
 * escaped -- as the srcdoc of a sandboxed iframe: no script, forms or
 * downloads, an opaque origin, and navigation only on a user's click,
 * before the engine's own enforcement is even reached.
 */
auto GF_UI_EXPORT BuildDocument(const QString& body, const QString& session,
                                bool frame_same_origin = false) -> QByteArray;

/// Whether a part of type @p mime may be shown at all. Images only, and
/// not SVG, which is a document rather than a picture.
auto GF_UI_EXPORT IsAllowedPartMime(const QString& mime) -> bool;

}  // namespace GpgFrontend::UI::HtmlView
