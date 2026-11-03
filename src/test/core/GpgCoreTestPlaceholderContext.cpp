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

#include "GpgFrontendTest.h"
#include "core/GFConstants.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GFEngineSupportIf.h"

namespace GpgFrontend::Test {

// Regression test for the deep-restart crash that prevented the app from
// relaunching after the first key database was added (macOS sandbox / rPGP
// fallback). OpenPGPContext::GetInstance() lazily creates a placeholder context
// for a channel that has no real context (e.g. one already torn down during the
// restart). If that placeholder defaulted to the GnuPG engine, an engine
// support check would call EngineVersion(), route into the live gpg-agent query
// and dynamic_cast the placeholder to GpgContext, throwing and killing the
// process before it could relaunch. The placeholder must instead report the
// always-available rPGP engine and a GnuPG support check must simply fail.
TEST(GpgCorePlaceholderContextTest, PlaceholderChannelIsNotGnuPG) {
  // A channel never configured with a real context in any test.
  constexpr int kUnusedChannel = 4096;

  auto& ctx = OpenPGPContext::GetInstance(kUnusedChannel);
  EXPECT_EQ(ctx.Engine(), OpenPGPEngine::kRPGP);

  // Must not throw, and must report that GnuPG is not supported on this
  // channel.
  bool supported = true;
  EXPECT_NO_THROW(supported = GpgContextSupportIf(
                      kUnusedChannel,
                      {{OpenPGPEngine::kGNUPG, kGpgMinimalSupportVersion}}));
  EXPECT_FALSE(supported);
}

}  // namespace GpgFrontend::Test
