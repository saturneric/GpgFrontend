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
 * @file WebEditorDocument.h
 * @brief The pure half of web editors: what a page may load, what it may
 * say, and the policy put in front of it. No engine, no widget.
 */

namespace GpgFrontend::UI::WebEditor {

/// Asset and resource schemes; the session is the URL's host.
inline constexpr auto kAssetScheme = "gfasset";
inline constexpr auto kResourceScheme = "gfres";

/// The page's whole permission: its own script and styles, images the module
/// put, nothing on the network, no eval, no inline script, no frames, no
/// forms, no plugins. `'unsafe-inline'` styles only, which ProseMirror needs.
GF_UI_EXPORT auto ContentSecurityPolicy() -> QString;

/// @p html with the policy as the first thing in its <head> (one is made
/// when there is none). A policy the page brings itself can only tighten it.
GF_UI_EXPORT auto InjectPolicy(const QByteArray& html) -> QByteArray;

/// The type an asset is served as, by extension; empty: not served.
GF_UI_EXPORT auto AssetMime(const QString& path) -> QString;

/// Whether a consumer may serve @p mime as a resource: still images only.
GF_UI_EXPORT auto IsAllowedResourceMime(const QString& mime) -> bool;

/// A URL the page asked for, taken apart; nullopt for anything that is not
/// one of the two schemes with a clean relative path.
struct GF_UI_EXPORT Request {
  bool asset = false;  ///< gfasset (else gfres)
  QString session;
  QString path;  ///< relative, '/'-separated, no "." or ".." segment
};
GF_UI_EXPORT auto ParseRequest(const QString& url) -> std::optional<Request>;

/// A path a consumer may put a resource at: the same rules as a request's.
GF_UI_EXPORT auto IsCleanPath(const QString& path) -> bool;

/// Whether @p json may cross the bridge: one JSON object, under the size
/// limit, with a non-empty string "type" of at most 64 characters.
GF_UI_EXPORT auto IsValidMessage(const QByteArray& json) -> bool;

inline constexpr qint64 kMaxMessageBytes = 48LL * 1024 * 1024;
inline constexpr qint64 kMaxResourceBytes = 20LL * 1024 * 1024;
inline constexpr qint64 kMaxTotalResourceBytes = 256LL * 1024 * 1024;
inline constexpr qint64 kMaxAssetBytes = 16LL * 1024 * 1024;

}  // namespace GpgFrontend::UI::WebEditor
