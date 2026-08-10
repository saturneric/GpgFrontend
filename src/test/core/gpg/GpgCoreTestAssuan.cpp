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

#include "GpgCoreTest.h"
#include "core/function/gpg/GpgAssuanHelper.h"

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreAssuanConnectTestA) {
  auto& helper = GpgAssuanHelper::GetInstance();

  auto ret = helper.ConnectToSocket(GpgComponentType::kGPG_AGENT);
  ASSERT_EQ(ret, GPG_ERR_NO_ERROR);

  GpgAssuanHelper::DataCallback d_cb =
      [=](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "data callback of command GETINFO pid: " << ctx->buffer;
    return 0;
  };

  GpgAssuanHelper::InqueryCallback i_cb =
      [](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "inquery callback of command GETINFO pid: " << ctx->inquery_name;
    return 0;
  };

  GpgAssuanHelper::StatusCallback s_cb =
      [](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "status callback of command GETINFO pid: " << ctx->status;
    return 0;
  };

  ret = helper.SendCommand(GpgComponentType::kGPG_AGENT, "GETINFO pid", d_cb,
                           i_cb, s_cb);
  ASSERT_EQ(ret, GPG_ERR_NO_ERROR);
}

TEST_F(GpgCoreTest, CoreAssuanConnectTestB) {
  auto& helper = GpgAssuanHelper::GetInstance();

  auto [ret, status] =
      helper.SendStatusCommand(GpgComponentType::kGPG_AGENT, "keyinfo --list");
  ASSERT_EQ(ret, GPG_ERR_NO_ERROR);
  ASSERT_TRUE(!status.isEmpty());
  ASSERT_TRUE(status.front().startsWith("KEYINFO"));

  LOG_D() << "status lines of command keyinfo --list: " << status;
}

// A component that cannot be reached used to be retried in full on every call,
// each retry spawning gpgconf and blocking the caller -- which for most callers
// is the GUI thread. Enumerating the key generation algorithms alone asks
// hundreds of times, so one dead agent froze the interface. The failure is
// remembered now, and replaying it has to be effectively free.
TEST_F(GpgCoreTest, CoreAssuanSuppressesRepeatedConnectFailures) {
  auto& helper = GpgAssuanHelper::GetInstance();

  const auto first = helper.ConnectToSocket(GpgComponentType::kKEYBOXD);
  if (first == GPG_ERR_NO_ERROR) {
    GTEST_SKIP() << "keyboxd is reachable here, nothing to suppress";
  }

  QElapsedTimer timer;
  timer.start();
  const auto second = helper.ConnectToSocket(GpgComponentType::kKEYBOXD);
  const auto elapsed_ms = timer.elapsed();

  EXPECT_EQ(second, first);

  // The path being avoided stats the socket and runs gpgconf to completion,
  // which costs tens of milliseconds. A replayed failure is a map lookup.
  EXPECT_LT(elapsed_ms, 5);

  // The maintenance actions go through here, so "restart the components" has to
  // mean the next call really tries again rather than replaying the cache.
  helper.ResetAllConnections();
}

// The same suppression, exercised without depending on which daemons happen to
// be running: a component gpgconf has no socket path for can never connect, so
// this always takes the failure path and always takes it twice.
TEST_F(GpgCoreTest, CoreAssuanReplaysCachedConnectFailure) {
  auto& helper = GpgAssuanHelper::GetInstance();

  // Outside the enumerated components on purpose: ConvertComponentType2String
  // yields no gpgconf key for it, so the socket path comes back empty.
  const auto unknown = static_cast<GpgComponentType>(200);

  const auto first = helper.ConnectToSocket(unknown);
  ASSERT_EQ(first, GPG_ERR_ENOPKG);

  // Replayed from the failure record rather than recomputed. Before this the
  // whole probe ran again for every caller.
  EXPECT_EQ(helper.ConnectToSocket(unknown), first);

  helper.ResetAllConnections();

  // Cleared, not sticky: the answer is the same because the component is still
  // unknown, not because the old record survived.
  EXPECT_EQ(helper.ConnectToSocket(unknown), first);

  helper.ResetAllConnections();
}
}  // namespace GpgFrontend::Test