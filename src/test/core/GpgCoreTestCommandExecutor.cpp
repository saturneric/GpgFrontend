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

#include "core/function/gpg/GpgCommandExecutor.h"

namespace GpgFrontend::Test {

#ifndef Q_OS_WINDOWS

namespace {

/// The exit code a command reported to its callback, or 1000 if none.
auto RunAndGetExitCode(const QString& cmd, const QStringList& args) -> int {
  int exit_code = 1000;
  GpgCommandExecutor::ExecuteSync(
      {cmd, args,
       [&exit_code](int code, const QString&, const QString&) {
         exit_code = code;
       }});
  return exit_code;
}

}  // namespace

// exitCode() of a process that never started, or did not exit by itself, is
// not the command's answer; it used to arrive as 0, a success.
TEST(GpgCommandExecutorTest, ACommandThatDidNotFinishIsAFailure) {
  EXPECT_NE(RunAndGetExitCode("/nonexistent/gpgfrontend-test-binary", {}), 0)
      << "a command that never started succeeded";
  EXPECT_NE(RunAndGetExitCode("/bin/sh", {"-c", "kill -9 $$"}), 0)
      << "a killed command succeeded";
}

TEST(GpgCommandExecutorTest, ACommandThatFinishesReportsItsExitCode) {
  EXPECT_EQ(RunAndGetExitCode("/bin/sh", {"-c", "exit 0"}), 0);
  EXPECT_EQ(RunAndGetExitCode("/bin/sh", {"-c", "exit 3"}), 3);
}

#endif

}  // namespace GpgFrontend::Test
