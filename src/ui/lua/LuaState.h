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

#include <QByteArray>
#include <QString>
#include <cstddef>

struct lua_State;
struct lua_Debug;

namespace GpgFrontend::UI::Lua {

/**
 * @brief One sandboxed Lua interpreter.
 *
 * What a module's UI script can reach is decided here, once, and not by the
 * script: the libraries opened, the functions removed from them, how much
 * memory it may hold and how many instructions any one entry may run.
 *
 *  - Libraries: base, string, table, math and utf8 only. The io, os,
 *    package, debug and coroutine libraries are not even in the binary.
 *  - Removed: dofile, loadfile, load, require, string.dump (and with it any
 *    route to bytecode), math.random/randomseed (the runtime is
 *    deterministic), and collectgarbage except "count".
 *  - Scripts load as text only. A precompiled chunk is refused.
 *  - Memory: a hard cap enforced by the allocator; a script over it gets a
 *    Lua out-of-memory error, never the process.
 *  - Time: an instruction budget per entry, enforced by a count hook. A loop
 *    that never ends ends with an error.
 *
 * Owned by one thread; every entry must come from it.
 */
class GF_UI_EXPORT LuaState {
 public:
  struct Limits {
    size_t memory_bytes = 8U * 1024U * 1024U;
    long load_budget = 2'000'000;  ///< instructions for loading a script
    long call_budget = 200'000;    ///< instructions for any other entry
  };

  explicit LuaState(Limits limits);
  LuaState();
  ~LuaState();

  LuaState(const LuaState&) = delete;
  auto operator=(const LuaState&) -> LuaState& = delete;

  /// Null when the state could not be created -- out of memory at birth.
  [[nodiscard]] auto L() const -> lua_State* { return l_; }
  [[nodiscard]] auto Ok() const -> bool { return l_ != nullptr; }

  [[nodiscard]] auto MemoryUsed() const -> size_t { return alloc_.used; }
  [[nodiscard]] auto MemoryLimit() const -> size_t { return alloc_.limit; }
  [[nodiscard]] auto Limit() const -> const Limits& { return limits_; }

  /// Arm the instruction budget for the next entry.
  void ArmBudget(long instructions);

  /// Whether the current entry has run out. Sticky until the next ArmBudget:
  /// pcall and xpcall re-raise while it holds, so a script cannot catch its
  /// way past the limit.
  [[nodiscard]] auto BudgetExhausted() const -> bool { return budget_ <= 0; }
  void ArmCallBudget() { ArmBudget(limits_.call_budget); }

  /**
   * @brief Load @p source as text and run it, under the load budget.
   *
   * @return false, with the reason in @p error, on a syntax error, a runtime
   *         error, an exhausted budget or memory cap, or a binary chunk
   */
  auto LoadAndRun(const QByteArray& source, const QString& chunk_name,
                  QString* error) -> bool;

  /// Test seam: fail the @p n-th growing allocation from now on (0: never).
  void FailAllocationAfter(size_t n);

  /// Where `print` goes; the module id is prefixed. Empty: the Host log.
  void SetLogPrefix(const QString& prefix) { log_prefix_ = prefix; }
  [[nodiscard]] auto LogPrefix() const -> const QString& {
    return log_prefix_;
  }

  /// The LuaState owning @p L, from the state's extra space.
  static auto Of(lua_State* L) -> LuaState*;

  /// Whatever owns this state -- a module's runtime -- for its bindings.
  void SetOwner(void* owner) { owner_ = owner; }
  [[nodiscard]] auto Owner() const -> void* { return owner_; }

 private:
  struct Allocator {
    size_t used = 0;
    size_t limit = 0;
    size_t count = 0;    ///< growing allocations since FailAllocationAfter
    size_t fail_at = 0;  ///< 0: never
  };

  static auto Allocate(void* ud, void* ptr, size_t osize, size_t nsize)
      -> void*;
  static void BudgetHook(lua_State* L, lua_Debug* ar);
  auto OpenSandbox() -> bool;

  Limits limits_;
  Allocator alloc_;
  long budget_ = 0;
  lua_State* l_ = nullptr;
  QString log_prefix_;
  void* owner_ = nullptr;
};

}  // namespace GpgFrontend::UI::Lua
