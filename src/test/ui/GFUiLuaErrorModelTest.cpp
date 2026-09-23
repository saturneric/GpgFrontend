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

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>

#include "GpgFrontendTest.h"
#include "ui/lua/LuaState.h"
#include "ui/lua/LuaTestSeam.h"

/**
 * @file GFUiLuaErrorModelTest.cpp
 * @brief No Lua error ever skips a C++ destructor.
 *
 * Every binding in the test seam owns a sentinel -- an object whose
 * destructor must run -- while it fails in one particular way. After each
 * failure no sentinel may be left alive, and the state must still work. Run
 * under ASan too (scripts/run_tests.sh --asan), where a skipped destructor
 * also shows up as a leak.
 */

namespace GpgFrontend::Test {

namespace {

using UI::Lua::LuaState;

auto Lua(LuaState& s, const char* code, QString* error = nullptr) -> bool {
  QString local;
  return s.LoadAndRun(QByteArray(code), QStringLiteral("test"),
                      error != nullptr ? error : &local);
}

auto StillWorks(LuaState& s) -> bool {
  return Lua(s, "local x = {} for i = 1, 10 do x[i] = i end "
                "assert(#x == 10)");
}

}  // namespace

TEST(LuaErrorModelTest, AFailingBindingRunsItsDestructors) {
  LuaState s;
  ASSERT_TRUE(s.Ok());
  UI::Lua::InstallErrorModelTestBindings(s);

  QString error;
  EXPECT_FALSE(Lua(s, "t_fail()", &error));
  EXPECT_TRUE(error.contains("failing on purpose")) << error.toStdString();
  EXPECT_EQ(UI::Lua::LiveErrorModelSentinels(), 0);

  // Caught inside Lua, too: the destructor has run before pcall sees it.
  EXPECT_TRUE(Lua(s, "local ok, e = pcall(t_fail) assert(not ok)"));
  EXPECT_EQ(UI::Lua::LiveErrorModelSentinels(), 0);
  EXPECT_TRUE(StillWorks(s));
}

TEST(LuaErrorModelTest, ACppExceptionBecomesALuaError) {
  LuaState s;
  UI::Lua::InstallErrorModelTestBindings(s);

  QString error;
  EXPECT_FALSE(Lua(s, "t_throw()", &error));
  EXPECT_TRUE(error.contains("throwing on purpose")) << error.toStdString();
  EXPECT_EQ(UI::Lua::LiveErrorModelSentinels(), 0);
  EXPECT_TRUE(StillWorks(s));
}

TEST(LuaErrorModelTest, ALuaErrorInsideABindingUnwindsCleanly) {
  LuaState s;
  UI::Lua::InstallErrorModelTestBindings(s);

  QString error;
  EXPECT_FALSE(Lua(s, "t_call(function() error('from inside') end)", &error));
  EXPECT_TRUE(error.contains("from inside")) << error.toStdString();
  EXPECT_EQ(UI::Lua::LiveErrorModelSentinels(), 0);

  // Nested: a binding calling Lua calling a binding that fails.
  EXPECT_FALSE(Lua(s, "t_call(function() t_call(t_fail) end)"));
  EXPECT_EQ(UI::Lua::LiveErrorModelSentinels(), 0);

  EXPECT_TRUE(Lua(s, "assert(select('#', t_call(t_values, 3)) == 3)"));
  EXPECT_EQ(UI::Lua::LiveErrorModelSentinels(), 0);
  EXPECT_TRUE(StillWorks(s));
}

TEST(LuaErrorModelTest, AnExhaustedBudgetUnwindsCleanly) {
  LuaState s;
  UI::Lua::InstallErrorModelTestBindings(s);

  QString error;
  EXPECT_FALSE(Lua(s, "while true do end", &error));
  EXPECT_TRUE(error.contains("budget")) << error.toStdString();

  // The loop is inside a binding: the abort crosses that binding's frame.
  EXPECT_FALSE(Lua(s, "t_call(function() while true do end end)", &error));
  EXPECT_TRUE(error.contains("budget")) << error.toStdString();
  EXPECT_EQ(UI::Lua::LiveErrorModelSentinels(), 0);

  // And pcall cannot swallow it for ever: the budget is per entry.
  EXPECT_FALSE(Lua(s, "while true do pcall(function() while true do end end) "
                      "end"));
  EXPECT_TRUE(StillWorks(s)) << "the next entry has a fresh budget";
}

// Fail the n-th allocation, for every n a small script makes, while a
// binding is building a table. Whatever point the failure lands at, no
// destructor is skipped and the state survives.
TEST(LuaErrorModelTest, AllocationFailureAtEveryPointUnwindsCleanly) {
  int failures = 0;
  for (size_t n = 1; n <= 400; ++n) {
    LuaState s;
    ASSERT_TRUE(s.Ok());
    UI::Lua::InstallErrorModelTestBindings(s);
    s.FailAllocationAfter(n);
    if (!Lua(s, "local t = t_alloc(40) registered = #t")) ++failures;
    EXPECT_EQ(UI::Lua::LiveErrorModelSentinels(), 0) << "at allocation " << n;
    s.FailAllocationAfter(0);
    EXPECT_TRUE(StillWorks(s)) << "at allocation " << n;
  }
  EXPECT_GT(failures, 0) << "the injection never landed";
}

TEST(LuaErrorModelTest, TheMemoryCapIsAnErrorNotACrash) {
  LuaState::Limits limits;
  limits.memory_bytes = 512U * 1024U;
  LuaState s(limits);
  ASSERT_TRUE(s.Ok());

  QString error;
  EXPECT_FALSE(Lua(s,
                   "local t = {} for i = 1, 1e7 do t[i] = string.rep('x', 64) "
                   ".. i end",
                   &error));
  EXPECT_LE(s.MemoryUsed(), s.MemoryLimit());
  EXPECT_TRUE(StillWorks(s));
}

// The invariant, as a rule about source text: outside the two files that
// implement the protected boundary, nothing in the Lua runtime calls a
// function that raises on its own.
TEST(LuaErrorModelTest, NoBindingRaisesOutsideTheBoundary) {
  const QString dir = QString(GF_TEST_SOURCE_DIR) + "/src/ui/lua";
  ASSERT_TRUE(QDir(dir).exists());

  // luaL_check*/luaL_opt*/luaL_arg* raise on a bad argument; lua_error and
  // luaL_error raise by definition. luaL_checkstack is excluded: it is only
  // ever called inside Protected(), where raising is the point.
  const QRegularExpression raising(
      R"(\b(luaL_check(?!stack)\w*|luaL_opt\w*|luaL_arg\w*)\s*\()");
  const QRegularExpression raises_directly(R"(\b(lua_error|luaL_error)\s*\()");
  const QStringList boundary = {"LuaBinding.h", "LuaState.cpp"};

  QDirIterator it(dir, {"*.cpp", "*.h"}, QDir::Files);
  int files = 0;
  while (it.hasNext()) {
    const auto path = it.next();
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    const auto text = QString::fromUtf8(f.readAll());
    ++files;
    EXPECT_FALSE(raising.match(text).hasMatch())
        << path.toStdString() << " calls a raising argument checker";
    if (!boundary.contains(QFileInfo(path).fileName())) {
      EXPECT_FALSE(raises_directly.match(text).hasMatch())
          << path.toStdString()
          << " raises directly; bindings report through BindingOutcome";
    }
  }
  EXPECT_GE(files, 4);
}

}  // namespace GpgFrontend::Test
