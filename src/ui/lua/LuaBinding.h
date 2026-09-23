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

#include <array>
#include <cstring>
#include <exception>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
}

/**
 * @file LuaBinding.h
 * @brief How C++ and Lua meet without undefined behaviour.
 *
 * ## The invariant
 *
 * Lua reports an error with longjmp. Jumping over a C++ frame that owns an
 * object with a non-trivial destructor is undefined behaviour -- the
 * destructor is skipped, or worse. So:
 *
 *   **A frame that calls a Lua API function which can raise holds no object
 *   with a non-trivial destructor.**
 *
 * Everything below exists to make that true by construction:
 *
 *  - Every entry from C++ into Lua goes through Protected(): a lua_pcall
 *    around a C function that runs the caller's lambda. The setjmp of that
 *    pcall is NEWER than the C++ frame that called Protected(), so an error,
 *    an exhausted instruction budget or a failed allocation unwinds only the
 *    lambda and Lua's own frames, and Protected() returns false.
 *
 *  - A Protected() lambda therefore obeys the rule itself: its body holds
 *    only trivially-destructible locals -- pointers, integers, lengths -- and
 *    writes its results through captured references into objects that live
 *    in the enclosing C++ frame. A QString may be *assigned* in the lambda
 *    (`out = QString::fromUtf8(p, n);`): the temporary is gone before the
 *    next Lua call, so nothing is alive when a longjmp could happen.
 *
 *  - A binding called FROM Lua is a LuaBinding<&Impl>: a trampoline whose
 *    own frame is trivially destructible. It runs Impl in a separate,
 *    noexcept frame that may own anything, and raises -- the only call in
 *    the trampoline that can -- after that frame has returned. Impl reads
 *    and writes Lua only through Protected().
 *
 *  - A C++ exception never crosses a Lua frame: the lambda runner and the
 *    binding runner both catch everything and turn it into a Lua error or a
 *    failure status.
 *
 * Lua is built as C (third_party/CMakeLists.txt) precisely so that none of
 * this depends on how a particular compiler unwinds exceptions through C.
 *
 * Tests (GFUiLuaErrorModelTest) count destructor runs across runtime errors,
 * budget aborts, allocation failures and errors inside bindings, and run
 * under ASan; a source test refuses a raising call in a binding file outside
 * this header.
 */

namespace GpgFrontend::UI::Lua {

/// What a binding hands back to its trampoline. Plain data only.
struct BindingOutcome {
  bool failed = false;
  int nret = 0;
  std::array<char, 256> message{};

  void Fail(const char* why) {
    failed = true;
    std::strncpy(message.data(), why == nullptr ? "error" : why,
                 message.size() - 1);
    message.back() = '\0';
  }
};

namespace detail {

template <typename F>
struct ProtectedCall {
  F* fn;
  int nret;
  bool threw;
  std::array<char, 256> message;
};

/// The C function Protected() runs under lua_pcall. No C++ object lives in
/// this frame across a raising call; the one lua_error is after the try.
template <typename F>
auto ProtectedThunk(lua_State* L) -> int {
  auto* call = static_cast<ProtectedCall<F>*>(lua_touserdata(L, 1));
  lua_remove(L, 1);
  call->threw = false;
  try {
    call->nret = (*call->fn)(L);
  } catch (const std::exception& e) {
    call->threw = true;
    std::strncpy(call->message.data(), e.what(), call->message.size() - 1);
  } catch (...) {
    call->threw = true;
    std::strncpy(call->message.data(), "a C++ exception",
                 call->message.size() - 1);
  }
  if (call->threw) return luaL_error(L, "%s", call->message.data());
  return call->nret;
}

}  // namespace detail

/**
 * @brief Run @p fn(L) under lua_pcall.
 *
 * The lambda sees the caller's stack: every value on it is passed along as an
 * argument, so index 1 in the lambda is index 1 where Protected() was called
 * -- a binding's first argument is still its first argument. They are copies
 * (lua_pushvalue, which never allocates); the caller's own values stay put.
 *
 * @p fn returns how many values it left on top of the stack; on success they
 * stay there, above whatever was there before. On failure nothing is left,
 * the error text goes to @p error, and false is returned.
 *
 * @p fn must follow the invariant above: no non-trivially-destructible local
 * alive across a Lua call.
 */
template <typename F>
auto Protected(lua_State* L, F&& fn, BindingOutcome* error = nullptr) -> bool {
  using Fn = std::remove_reference_t<F>;
  detail::ProtectedCall<Fn> call{&fn, 0, false, {}};
  const int base = lua_gettop(L);
  // Room for the function, its context and a copy of every value. Reports
  // failure rather than raising, so this line cannot longjmp.
  if (lua_checkstack(L, base + 2) == 0) {
    if (error != nullptr) error->Fail("Lua stack exhausted");
    return false;
  }
  lua_pushcfunction(L, &detail::ProtectedThunk<Fn>);  // no allocation
  lua_pushlightuserdata(L, &call);                    // no allocation
  for (int i = 1; i <= base; ++i) lua_pushvalue(L, i);  // no allocation
  const int status = lua_pcall(L, base + 1, LUA_MULTRET, 0);
  if (status == LUA_OK) return true;

  if (error != nullptr) {
    size_t n = 0;
    // The error value is a string for everything the Host raises; anything
    // else is not converted, because converting could itself raise.
    const char* msg = lua_type(L, -1) == LUA_TSTRING
                          ? lua_tolstring(L, -1, &n)
                          : (status == LUA_ERRMEM ? "out of memory"
                                                  : "a non-string Lua error");
    error->Fail(msg);
  }
  lua_settop(L, base);
  return false;
}

/// The frame that may own C++ objects. Never lets an exception escape.
inline void RunBinding(lua_State* L, void (*impl)(lua_State*, BindingOutcome&),
                       BindingOutcome& out) noexcept {
  try {
    impl(L, out);
  } catch (const std::exception& e) {
    out.Fail(e.what());
  } catch (...) {
    out.Fail("a C++ exception");
  }
}


/**
 * @brief The trampoline every C++ function callable from Lua goes through.
 *
 * `Impl` is `void(lua_State*, BindingOutcome&)`: it may own C++ objects,
 * touches Lua only through Protected(), and sets `nret` to the number of
 * values it left on the stack for Lua, or calls Fail().
 */
template <void (*Impl)(lua_State*, BindingOutcome&)>
auto LuaBinding(lua_State* L) -> int {
  BindingOutcome out;  // trivially destructible: safe under the raise below
  RunBinding(L, Impl, out);
  if (out.failed) return luaL_error(L, "%s", out.message.data());
  return out.nret;
}

}  // namespace GpgFrontend::UI::Lua
