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

#include "LuaTestSeam.h"

#include <atomic>
#include <stdexcept>

#include "LuaBinding.h"
#include "LuaState.h"

namespace GpgFrontend::UI::Lua {

namespace {

std::atomic<int> g_live{0};

/// Non-trivially destructible, and owns heap memory ASan would see leak.
class Sentinel {
 public:
  Sentinel() : payload_(QStringLiteral("sentinel payload %1").arg(++g_live)) {}
  ~Sentinel() { --g_live; }
  Sentinel(const Sentinel&) = delete;
  auto operator=(const Sentinel&) -> Sentinel& = delete;

 private:
  QString payload_;
};

void FailImpl(lua_State* /*L*/, BindingOutcome& out) {
  const Sentinel s;
  out.Fail("t_fail: failing on purpose");
}

void ThrowImpl(lua_State* /*L*/, BindingOutcome& /*out*/) {
  const Sentinel s;
  throw std::runtime_error("t_throw: throwing on purpose");
}

void CallImpl(lua_State* L, BindingOutcome& out) {
  const Sentinel s;
  int nret = 0;
  const bool ok = Protected(L, [&nret](lua_State* S) -> int {
    // The binding's arguments -- the function first -- are the lambda's.
    const int n = lua_gettop(S);
    lua_call(S, n - 1, LUA_MULTRET);
    nret = lua_gettop(S);
    return nret;
  }, &out);
  if (ok) out.nret = nret;
}

void AllocImpl(lua_State* L, BindingOutcome& out) {
  const Sentinel s;
  lua_Integer n = 0;
  if (lua_type(L, 1) == LUA_TNUMBER) n = lua_tointegerx(L, 1, nullptr);
  const bool ok = Protected(L, [n](lua_State* S) -> int {
    lua_createtable(S, 0, 0);
    for (lua_Integer i = 1; i <= n; ++i) {
      lua_pushfstring(S, "string number %d", static_cast<int>(i));
      lua_rawseti(S, -2, i);
    }
    return 1;
  }, &out);
  if (ok) out.nret = 1;
}

void ValuesImpl(lua_State* L, BindingOutcome& out) {
  const Sentinel s;
  lua_Integer n = 0;
  if (lua_type(L, 1) == LUA_TNUMBER) n = lua_tointegerx(L, 1, nullptr);
  const bool ok = Protected(L, [n](lua_State* S) -> int {
    luaL_checkstack(S, static_cast<int>(n), "t_values");
    for (lua_Integer i = 1; i <= n; ++i) lua_pushinteger(S, i);
    return static_cast<int>(n);
  }, &out);
  if (ok) out.nret = static_cast<int>(n);
}

}  // namespace

void InstallErrorModelTestBindings(LuaState& state) {
  Protected(state.L(), [](lua_State* L) -> int {
    lua_pushcfunction(L, &LuaBinding<&FailImpl>);
    lua_setglobal(L, "t_fail");
    lua_pushcfunction(L, &LuaBinding<&ThrowImpl>);
    lua_setglobal(L, "t_throw");
    lua_pushcfunction(L, &LuaBinding<&CallImpl>);
    lua_setglobal(L, "t_call");
    lua_pushcfunction(L, &LuaBinding<&AllocImpl>);
    lua_setglobal(L, "t_alloc");
    lua_pushcfunction(L, &LuaBinding<&ValuesImpl>);
    lua_setglobal(L, "t_values");
    return 0;
  });
}

auto LiveErrorModelSentinels() -> int { return g_live.load(); }

}  // namespace GpgFrontend::UI::Lua
