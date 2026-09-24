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

#include <functional>

#include "ui/lua/LuaModuleRuntime.h"

class QMenu;
class QBoxLayout;

namespace GpgFrontend::UI {

class TextEdit;
class PlainTextEditorPage;
class KeyList;

namespace Lua {

using ContextFn = std::function<UiContext()>;

/**
 * @file LuaPlacements.h
 * @brief Where anchors become widgets. The only code that sees both.
 *
 * Every widget here is the Host's own: a QAction in a Host menu, a button in
 * a Host dialog. It remembers which module's action it stands for, by id,
 * and nothing else -- so an action whose module has gone finds nothing to
 * run rather than a dangling pointer, and a script never touches the widget
 * at all. Titles, tooltips and categories come from the command registry.
 */
class GF_UI_EXPORT LuaPlacements {
 public:
  /**
   * @brief Keep @p menu's module actions for @p anchor current.
   *
   * Inserted after a separator of their own, rebuilt when any module's
   * registrations change, and evaluated on aboutToShow. Eager rather than
   * lazy, because some platform menu bars do not deliver aboutToShow.
   */
  static void AttachMenu(const QString& anchor, QMenu* menu, ContextFn ctx);

  /// One-shot, for a menu built per popup: the editor's context menu.
  static void PopulateMenu(const QString& anchor, QMenu* menu,
                           const ContextFn& ctx);

  /// Buttons in @p layout: one per command category, a menu for several.
  static void BuildButtons(const QString& anchor, QBoxLayout* layout,
                           ContextFn ctx);

  /// The context the main window's anchors see: the current document.
  static auto EditorContext(TextEdit* edit) -> UiContext;

  /// The context for one document page.
  static auto PageContext(PlainTextEditorPage* page) -> UiContext;

  /// The context a key list's menus see: its selected keys (never groups).
  static auto KeyListContext(KeyList* list) -> UiContext;

  /// Tell every module's script that @p event happened on @p page.
  static void Notify(const QString& event, PlainTextEditorPage* page);
};

}  // namespace Lua
}  // namespace GpgFrontend::UI
