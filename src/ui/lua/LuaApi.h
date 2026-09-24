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

struct lua_State;

namespace GpgFrontend::UI::Lua {

class LuaModuleRuntime;
struct BindingOutcome;

/**
 * @brief The whole of what a module's UI script can call. Deliberately small.
 *
 *   commands.get(id) / commands.invoke(cmd, args[, fn]) / commands.state(cmd)
 *   ui.anchor(name) / ui.anchor.settings{} / .editor{} / .dialog{}
 *   ui.action{} / ui.mount{} / ui.subscribe{}
 *   native.widget(id) / native.factory(id)         -- ui.custom only
 *   state.get(key[, default]) / state.set(key, v) / state.host(key)
 *   theme.color(role)
 *
 * Nothing here returns a Qt object, a pointer, or a way to call an arbitrary
 * method; every value that refers to something in the Host is a typed handle
 * resolved against the calling module's own runtime. See LuaApiReference()
 * for the full text, which the documentation snapshot is checked against.
 */
struct LuaApi {
  /// Install every table and metatable into @p rt's fresh state.
  static auto Install(LuaModuleRuntime& rt, QString* error) -> bool;

  static void CommandsGet(lua_State* L, BindingOutcome& out);
  static void CommandsState(lua_State* L, BindingOutcome& out);
  static void CommandsInvoke(lua_State* L, BindingOutcome& out);
  static void CommandFlag(lua_State* L, BindingOutcome& out);
  static void CallCancel(lua_State* L, BindingOutcome& out);
  static void UiAnchor(lua_State* L, BindingOutcome& out);
  static void UiMountAnchor(lua_State* L, BindingOutcome& out);
  static void UiAction(lua_State* L, BindingOutcome& out);
  static void UiMount(lua_State* L, BindingOutcome& out);
  static void UiSubscribe(lua_State* L, BindingOutcome& out);
  static void NativeRef(lua_State* L, BindingOutcome& out);
  static void StateGet(lua_State* L, BindingOutcome& out);
  static void StateSet(lua_State* L, BindingOutcome& out);
  static void StateHost(lua_State* L, BindingOutcome& out);
  static void ThemeColor(lua_State* L, BindingOutcome& out);
  static void Index(lua_State* L, BindingOutcome& out);
  static void ContextHasSelection(lua_State* L, BindingOutcome& out);
  static void ContextHasKeyGroup(lua_State* L, BindingOutcome& out);
  static void DocumentHasOpenPgp(lua_State* L, BindingOutcome& out);
  static void MakeRef(lua_State* L, BindingOutcome& out);
};

/// The Lua API as text: every table, function and handle type, with its
/// lifetime. Documentation is checked against this.
auto GF_UI_EXPORT LuaApiReference() -> QString;

}  // namespace GpgFrontend::UI::Lua
