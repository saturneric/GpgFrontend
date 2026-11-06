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

#include <cstddef>

#include "GpgFrontendTest.h"
#include "core/utils/BuildInfoUtils.h"
#include "host/SdkTestContext.h"
#include "sdk/GFSDKApp.h"
#include "sdk/GFSDKHostApi.h"

/**
 * @file GpgCoreTestSdkApp.cpp
 * @brief How the build ships, as a module asks it: the install kind and the
 * nightly identity the update check decides by.
 */

namespace GpgFrontend::Test {

TEST(SdkAppTest, TheBuildSaysHowItShipsAndWhichApplicationItIs) {
  SdkTestContext context("com.example.apptest.facts", 0);
  ASSERT_TRUE(context.live());

  EXPECT_FALSE(GetInstallKind().isEmpty());
  EXPECT_EQ(QString::fromUtf8(GFAppInstallKind(context.get())),
            GetInstallKind());
  EXPECT_EQ(GFAppIsNightlyIdentity(context.get()), IsStableBuild() ? 0 : 1);
}

// The two slots were appended to the app table: a host built before them
// ends its table earlier, and the module side must say "unknown" rather than
// read past its end.
TEST(SdkAppTest, AnOlderHostIsUnknownNotMisread) {
  SdkTestContext context("com.example.apptest.older", 0);
  ASSERT_TRUE(context.live());

  GFHostAppApi older = *context.get()->host->app;
  older.struct_size = offsetof(GFHostAppApi, install_kind);
  GFHostApi host = *context.get()->host;
  host.app = &older;
  GFSDKContext ctx = *context.get();
  ctx.host = &host;

  EXPECT_STREQ(GFAppInstallKind(&ctx), "");
  EXPECT_EQ(GFAppIsNightlyIdentity(&ctx), -1);
  // What the older table has still answers.
  EXPECT_EQ(QString::fromUtf8(GFAppVersion(&ctx)), GetProjectVersion());
}

}  // namespace GpgFrontend::Test
