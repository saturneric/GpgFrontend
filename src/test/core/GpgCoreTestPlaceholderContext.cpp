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
#include "core/function/openpgp/ChannelHandle.h"
#include "core/function/openpgp/OpenPGPContext.h"
#include "core/model/GFEngineSupportIf.h"
#include "core/model/GpgKeyGenerateInfo.h"

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

// A subkey algorithm is looked up on the engine in use, never on a retired
// channel: one retired by a key database switch was freed under the lookup (a
// crash in the phone app's tests, after a test that switched databases).
TEST(GpgCorePlaceholderContextTest,
     SubkeyAlgoIsNeverLookedUpOnARetiredChannel) {
  constexpr int kGeneration = 0x7ffd0000;
  const auto retired = AllocateChannelHandle();
  ASSERT_TRUE(RegisterChannelHandle(retired, "id-retired", true, kGeneration));
  ASSERT_TRUE(RetireChannelHandle(retired));
  auto [found_retired, none] =
      KeyGenerateInfo::SearchSubKeyAlgo("cv25519", retired);
  EXPECT_FALSE(found_retired);

  // The channel in use offers what its engine can make.
  auto [found, algo] = KeyGenerateInfo::SearchSubKeyAlgo(
      "cv25519", UsableDefaultChannelHandle());
  EXPECT_TRUE(found);
  EXPECT_EQ(algo.Id(), "cv25519");
}

}  // namespace GpgFrontend::Test
