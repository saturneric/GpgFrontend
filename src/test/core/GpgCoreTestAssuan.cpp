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
  ASSERT_TRUE(ret);

  GpgAssuanHelper::DataCallback d_cb =
      [=](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "data callback of command GETINFO pid: " << ctx->buffer;
    return 0;
  };

  GpgAssuanHelper::InqueryCallback i_cb =
      [](const QSharedPointer<GpgAssuanHelper::AssuanCallbackContext>& ctx)
      -> gpg_error_t {
    LOG_D() << "inquery callback of command GETINFO pid: " << ctx->inquery;
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
  ASSERT_TRUE(ret);
}

TEST_F(GpgCoreTest, CoreAssuanConnectTestB) {
  auto& helper = GpgAssuanHelper::GetInstance();

  auto [ret, status] =
      helper.SendStatusCommand(GpgComponentType::kGPG_AGENT, "keyinfo --list");
  ASSERT_TRUE(ret);
  ASSERT_TRUE(!status.isEmpty());
  ASSERT_TRUE(status.front().startsWith("KEYINFO"));

  LOG_D() << "status lines of command keyinfo --list: " << status;
}
}  // namespace GpgFrontend::Test