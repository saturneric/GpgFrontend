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

#include <QString>
#include <cstdint>

namespace GpgFrontend::UI::Lua {

/**
 * @file LuaAnchors.h
 * @brief The places a module's UI script may attach to. A stable contract.
 *
 * An anchor is a name for a place in the Host's UI -- "the Help menu", "the
 * editor's context menu", "the key details actions" -- never the Qt object
 * behind it. Lua cannot resolve an anchor to a widget; the Host maps each to
 * its own QMenu, layout, settings page list or tab system, and can change
 * how without any script noticing.
 *
 * Rules for every anchor:
 *  - its id never changes meaning once published;
 *  - `since` is the anchor catalog version that introduced it;
 *  - a deprecated anchor keeps working for at least one release after
 *    `deprecated_since`, with a warning when a script loads; it is removed
 *    only with an SDK major version;
 *  - an unknown id is an error at load time that lists the valid ones.
 */

enum class AnchorKind {
  kMENU,      ///< actions in a menu
  kBUTTONS,   ///< actions as buttons, grouped by command category
  kSETTINGS,  ///< a mounted page in the Settings dialog
  kEDITOR,    ///< a mounted view for a document type
  kDIALOG,    ///< a mounted dialog, opened by org.gpgfrontend.view.open
};

/// What a context carries at an anchor, as bits.
enum AnchorContext : uint32_t {
  kCTX_NONE = 0,
  kCTX_DOCUMENT = 1U << 0,   ///< ctx.document
  kCTX_SELECTION = 1U << 1,  ///< ctx:has_selection()
  kCTX_KEY = 1U << 2,        ///< ctx.key
};

struct AnchorSpec {
  const char* id;
  AnchorKind kind;
  uint32_t context;       ///< AnchorContext bits
  bool multiple_modules;  ///< may more than one module contribute
  int since;              ///< catalog version that introduced it
  int deprecated_since;   ///< 0: not deprecated
  const char* summary;
};

/// The catalog version this Host implements.
constexpr int kAnchorCatalogVersion = 1;

/// The anchor named @p id, or nullptr when there is none.
auto GF_UI_EXPORT FindAnchor(const QString& id) -> const AnchorSpec*;

/// Every anchor, in catalog order.
auto GF_UI_EXPORT AllAnchors() -> QList<const AnchorSpec*>;

/// A plain-text description of the catalog, for docs and the snapshot test.
auto GF_UI_EXPORT AnchorCatalogReference() -> QString;

}  // namespace GpgFrontend::UI::Lua
