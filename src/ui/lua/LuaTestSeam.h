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

namespace GpgFrontend::UI::Lua {

class LuaState;

/**
 * @file LuaTestSeam.h
 * @brief Bindings that exist to prove the error model, for tests only.
 *
 * Each owns a counting sentinel -- an object with a non-trivial destructor --
 * while it fails in a different way. A sentinel still alive afterwards is a
 * destructor Lua's longjmp skipped. They are never installed into a module's
 * state; only a test calls InstallErrorModelTestBindings().
 *
 * Installed as globals:
 *   t_fail()           fails from the binding's own frame
 *   t_throw()          throws a C++ exception inside the binding
 *   t_call(f, ...)     calls a Lua function from inside a binding
 *   t_alloc(n)         builds a table of n fresh strings inside a binding
 *   t_values(n)        returns 1..n
 */
void GF_UI_EXPORT InstallErrorModelTestBindings(LuaState& state);

/// Sentinels constructed and not yet destroyed. 0 when every one ran.
auto GF_UI_EXPORT LiveErrorModelSentinels() -> int;

}  // namespace GpgFrontend::UI::Lua
