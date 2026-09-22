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

#include <QByteArray>
#include <QString>

#include "GpgFrontendTest.h"
#include "core/SdkTestContext.h"
#include "sdk/GFSDK.hpp"

/**
 * @file GpgCoreTestSdkProcess.cpp
 * @brief process.execute borrows what it is given.
 *
 * The host used to free the context array, every context, the command and
 * every argument. gf::sdk::RunCommand passes stack and Qt-owned memory, so
 * that aborted in a debug build and corrupted the heap in a release build.
 */

namespace GpgFrontend::Test {

namespace {

auto Ctx() -> GFSDKContext* {
  static SdkTestContext context("com.example.sdk.process");
  return context.get();
}

struct Outcome {
  int calls = 0;
  int exit_code = -1;
  QString out;
};

void Record(void* data, int exit_code, const char* out, const char* /*err*/) {
  auto* outcome = static_cast<Outcome*>(data);
  outcome->calls++;
  outcome->exit_code = exit_code;
  outcome->out = QString::fromUtf8(out);
}

}  // namespace

#ifndef _WIN32

TEST(SdkProcessTest, RunCommandWithBorrowedMemoryRunsAndReturns) {
  Outcome outcome;
  ASSERT_EQ(gf::sdk::RunCommand(Ctx(), "/bin/sh", {"-c", "printf borrowed"},
                                &Record, &outcome),
            0);
  EXPECT_EQ(outcome.calls, 1);
  EXPECT_EQ(outcome.exit_code, 0);
  EXPECT_EQ(outcome.out, QString("borrowed"));
}

TEST(SdkProcessTest, RunCommandsRunsEveryCommandInTheBatch) {
  Outcome first;
  Outcome second;
  ASSERT_EQ(gf::sdk::RunCommands(
                Ctx(), {{"/bin/sh", {"-c", "printf one"}, &Record, &first},
                        {"/bin/sh", {"-c", "printf two"}, &Record, &second}}),
            0);
  EXPECT_EQ(first.out, QString("one"));
  EXPECT_EQ(second.out, QString("two"));
}

#endif

TEST(SdkProcessTest, AnEmptyBatchIsRefused) {
  EXPECT_NE(gf::sdk::RunCommands(Ctx(), {}), 0);
}

}  // namespace GpgFrontend::Test
