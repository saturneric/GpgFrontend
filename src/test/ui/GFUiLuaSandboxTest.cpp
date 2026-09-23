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

#include <gtest/gtest.h>

#include "GpgFrontendTest.h"
#include "ui/lua/LuaState.h"

/**
 * @file GFUiLuaSandboxTest.cpp
 * @brief What a module's UI script cannot reach.
 */

namespace GpgFrontend::Test {

namespace {

using UI::Lua::LuaState;

auto Lua(LuaState& s, const char* code, QString* error = nullptr) -> bool {
  QString local;
  return s.LoadAndRun(QByteArray(code), QStringLiteral("sandbox"),
                      error != nullptr ? error : &local);
}

}  // namespace

TEST(LuaSandboxTest, NoLibraryReachesOutsideTheProcess) {
  LuaState s;
  ASSERT_TRUE(s.Ok());
  for (const char* name :
       {"io", "os", "package", "debug", "coroutine", "dofile", "loadfile",
        "load", "loadstring", "require"}) {
    const auto code = QString("assert(%1 == nil, '%1')").arg(name).toUtf8();
    EXPECT_TRUE(Lua(s, code.constData())) << name << " is reachable";
  }
  EXPECT_TRUE(Lua(s, "assert(string.dump == nil)"));
  EXPECT_TRUE(Lua(s, "assert(math.random == nil and math.randomseed == nil)"));
}

TEST(LuaSandboxTest, WhatIsLeftIsOrdinaryLua) {
  LuaState s;
  EXPECT_TRUE(Lua(s, "assert(string.format('%d', 42) == '42')"));
  EXPECT_TRUE(Lua(s, "local t = {3, 1, 2} table.sort(t) assert(t[1] == 1)"));
  EXPECT_TRUE(Lua(s, "assert(math.floor(2.5) == 2)"));
  EXPECT_TRUE(Lua(s, "assert(utf8.len('héllo') == 5)"));
  EXPECT_TRUE(Lua(s, "assert(type(collectgarbage('count')) == 'number')"));
  EXPECT_FALSE(Lua(s, "collectgarbage('collect')"))
      << "only the question that changes nothing";
  EXPECT_TRUE(Lua(s, "print('hello from', 1, true)"));
}

TEST(LuaSandboxTest, OnlyTextIsLoaded) {
  LuaState s;
  QString error;
  // The binary chunk signature. The parser is what gives the language's
  // guarantees; a precompiled chunk would bypass it.
  EXPECT_FALSE(s.LoadAndRun(QByteArray("\x1bLua\x54\x00", 6), "binary",
                            &error));
  EXPECT_TRUE(error.contains("binary")) << error.toStdString();
  EXPECT_FALSE(Lua(s, "this is not lua", &error));
}

TEST(LuaSandboxTest, StatesAreIndependent) {
  LuaState a;
  LuaState b;
  EXPECT_TRUE(Lua(a, "shared = 1"));
  EXPECT_TRUE(Lua(b, "assert(shared == nil)"));
}

}  // namespace GpgFrontend::Test
