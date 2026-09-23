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

#include "LuaState.h"

#include <cstdlib>
#include <cstring>

#include "LuaBinding.h"

extern "C" {
#include "lualib.h"
}

namespace GpgFrontend::UI::Lua {

namespace {

/// Instructions between two budget checks. Small enough that an exhausted
/// budget is noticed promptly, large enough to cost nothing measurable.
constexpr int kHookStep = 1000;

auto Panic(lua_State* L) -> int {
  // Unreachable by construction: every entry is protected. Reaching it means
  // an unprotected call slipped in, and continuing would be undefined.
  const char* msg = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "?";
  qFatal("unprotected Lua error: %s", msg);
  return 0;
}

/// `print`, to the module's log line rather than to stdout.
void PrintImpl(lua_State* L, BindingOutcome& out) {
  QString line;
  const bool ok = Protected(L, [&line](lua_State* S) -> int {
    const int n = lua_gettop(S);
    for (int i = 1; i <= n; ++i) {
      size_t len = 0;
      // luaL_tolstring honours __tostring; it may raise, which is why this
      // is inside Protected and assigns straight into the captured string.
      const char* s = luaL_tolstring(S, i, &len);
      if (i > 1) line += QLatin1Char('\t');
      line += QString::fromUtf8(s, static_cast<int>(len));
      lua_pop(S, 1);
    }
    return 0;
  }, &out);
  if (!ok) return;
  const auto* state = LuaState::Of(L);
  LOG_I() << (state != nullptr ? state->LogPrefix() : QString()) << line;
  out.nret = 0;
}

/// collectgarbage, reduced to the one question that changes nothing.
void CollectGarbageImpl(lua_State* L, BindingOutcome& out) {
  int kb = 0;
  bool is_count = false;
  const bool ok = Protected(L, [&](lua_State* S) -> int {
    size_t n = 0;
    const char* opt =
        lua_type(S, 1) == LUA_TSTRING ? lua_tolstring(S, 1, &n) : "count";
    is_count = std::strcmp(opt, "count") == 0;
    kb = lua_gc(S, LUA_GCCOUNT);
    return 0;
  }, &out);
  if (!ok) return;
  if (!is_count) {
    out.Fail("collectgarbage: only \"count\" is available");
    return;
  }
  Protected(L, [kb](lua_State* S) -> int {
    lua_pushnumber(S, static_cast<lua_Number>(kb));
    return 1;
  }, &out);
  out.nret = out.failed ? 0 : 1;
}

/// For the pcall/xpcall wrappers: has this entry run out?
void BudgetExhaustedImpl(lua_State* L, BindingOutcome& out) {
  const auto* state = LuaState::Of(L);
  const bool exhausted = state != nullptr && state->BudgetExhausted();
  Protected(L, [exhausted](lua_State* S) -> int {
    lua_pushboolean(S, exhausted ? 1 : 0);
    return 1;
  }, &out);
  out.nret = out.failed ? 0 : 1;
}

/**
 * pcall and xpcall, made unable to swallow an exhausted budget. The count
 * hook raises an ordinary Lua error, and a script that wrapped its loop in
 * pcall would catch it, loop again, and be caught again -- for ever, since
 * the hook's errors land almost always inside the protected part. Once the
 * budget is gone, the wrappers re-raise on the way out, so the error reaches
 * the Host whatever the script does.
 */
constexpr const char* kGuardedPcall = R"lua(
local raw_pcall, raw_xpcall, exhausted = pcall, xpcall, ...
local pack, unpack, err = table.pack, table.unpack, error
local function settle(r)
  if exhausted() then err("instruction budget exhausted", 0) end
  return unpack(r, 1, r.n)
end
pcall = function(...) return settle(pack(raw_pcall(...))) end
xpcall = function(...) return settle(pack(raw_xpcall(...))) end
)lua";

}  // namespace

LuaState::LuaState() : LuaState(Limits{}) {}

LuaState::LuaState(Limits limits) : limits_(limits) {
  alloc_.limit = limits_.memory_bytes;
  l_ = lua_newstate(&LuaState::Allocate, &alloc_);
  if (l_ == nullptr) return;

  *static_cast<LuaState**>(lua_getextraspace(l_)) = this;
  lua_atpanic(l_, &Panic);
  lua_sethook(l_, &LuaState::BudgetHook, LUA_MASKCOUNT, kHookStep);

  if (!OpenSandbox()) {
    lua_close(l_);
    l_ = nullptr;
  }
}

LuaState::~LuaState() {
  if (l_ != nullptr) lua_close(l_);
}

auto LuaState::Of(lua_State* L) -> LuaState* {
  return *static_cast<LuaState**>(lua_getextraspace(L));
}

auto LuaState::Allocate(void* ud, void* ptr, size_t osize, size_t nsize)
    -> void* {
  auto* a = static_cast<Allocator*>(ud);
  // For a new block Lua passes a type tag in osize, not a size.
  const size_t old_size = ptr == nullptr ? 0 : osize;

  if (nsize == 0) {
    std::free(ptr);
    a->used -= old_size;
    return nullptr;
  }

  // Lua requires that shrinking never fails; only growth is refused.
  if (nsize > old_size) {
    if (a->fail_at != 0 && ++a->count >= a->fail_at) return nullptr;
    if (a->used - old_size + nsize > a->limit) return nullptr;
  }

  void* block = std::realloc(ptr, nsize);
  if (block == nullptr) return nullptr;
  a->used = a->used - old_size + nsize;
  return block;
}

void LuaState::BudgetHook(lua_State* L, lua_Debug* /*ar*/) {
  // Plain data only in this frame: luaL_error longjmps from here.
  auto* self = Of(L);
  self->budget_ -= kHookStep;
  if (self->budget_ <= 0) {
    luaL_error(L, "instruction budget exhausted");
  }
}

void LuaState::ArmBudget(long instructions) { budget_ = instructions; }

void LuaState::FailAllocationAfter(size_t n) {
  alloc_.count = 0;
  alloc_.fail_at = n;
}

auto LuaState::OpenSandbox() -> bool {
  ArmBudget(limits_.load_budget);
  return Protected(l_, [](lua_State* L) -> int {
    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
    lua_settop(L, 0);

    // Everything that loads code from elsewhere, or reaches bytecode.
    for (const char* name : {"dofile", "loadfile", "load", "loadstring",
                             "require"}) {
      lua_pushnil(L);
      lua_setglobal(L, name);
    }
    lua_getglobal(L, LUA_STRLIBNAME);
    lua_pushnil(L);
    lua_setfield(L, -2, "dump");
    lua_pop(L, 1);

    // Deterministic: no source of randomness.
    lua_getglobal(L, LUA_MATHLIBNAME);
    lua_pushnil(L);
    lua_setfield(L, -2, "random");
    lua_pushnil(L);
    lua_setfield(L, -2, "randomseed");
    lua_pop(L, 1);

    lua_pushcfunction(L, &LuaBinding<&CollectGarbageImpl>);
    lua_setglobal(L, "collectgarbage");
    lua_pushcfunction(L, &LuaBinding<&PrintImpl>);
    lua_setglobal(L, "print");

    if (luaL_loadbufferx(L, kGuardedPcall, std::strlen(kGuardedPcall),
                         "=sandbox", "t") != LUA_OK) {
      return lua_error(L);
    }
    lua_pushcfunction(L, &LuaBinding<&BudgetExhaustedImpl>);
    lua_call(L, 1, 0);
    return 0;
  });
}

auto LuaState::LoadAndRun(const QByteArray& source, const QString& chunk_name,
                          QString* error) -> bool {
  if (l_ == nullptr) {
    if (error != nullptr) *error = QStringLiteral("no Lua state");
    return false;
  }
  const auto name = ("=" + chunk_name).toUtf8();
  BindingOutcome out;
  ArmBudget(limits_.load_budget);
  const bool ok = Protected(l_, [&source, &name](lua_State* L) -> int {
    // "t": text only. A binary chunk would bypass the parser, and with it
    // every guarantee the language gives about what a script can do.
    if (luaL_loadbufferx(L, source.constData(),
                         static_cast<size_t>(source.size()),
                         name.constData(), "t") != LUA_OK) {
      return lua_error(L);
    }
    lua_call(L, 0, 0);
    return 0;
  }, &out);
  if (!ok && error != nullptr) *error = QString::fromUtf8(out.message.data());
  return ok;
}

}  // namespace GpgFrontend::UI::Lua
