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

#include <type_traits>

#include "GpgFrontendTest.h"
#include "core/function/openpgp/helper/Op.h"
#include "core/function/result_analyse/GpgResultAnalyse.h"
#include "core/typedef/GFError.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

// An error is never a number: neither way round, and not by accident.
static_assert(!std::is_convertible_v<GFError, int>);

TEST(GFErrorTest, TellsSuccessCancellationAbortionAndFailureApart) {
  EXPECT_TRUE(GFError{}.Ok());
  EXPECT_FALSE(GFError{}.Failed());

  const auto canceled = GFError::Application(GFErrorCode::kCANCELED);
  EXPECT_TRUE(canceled.Canceled());
  EXPECT_FALSE(canceled.Failed());

  const auto aborted = GFError::Application(GFErrorCode::kABORTED);
  EXPECT_TRUE(aborted.Aborted());
  EXPECT_FALSE(aborted.Failed());

  const auto failed = GFError::Application(GFErrorCode::kBAD_PASSPHRASE);
  EXPECT_TRUE(failed.Failed());
  EXPECT_EQ(failed.origin, GFErrorOrigin::kAPPLICATION);
  EXPECT_EQ(GFError::Application(GFErrorCode::kOK).origin,
            GFErrorOrigin::kNONE);
}

TEST(GFErrorTest, AnEngineErrorOnAnRpgpChannelHasNoNativeValue) {
  // A channel never configured with a real context: an rPGP placeholder.
  constexpr int kUnusedChannel = 4098;

  const auto err = EngineErrorOf(kUnusedChannel, GFErrorCode::kNOT_SUPPORTED);
  EXPECT_EQ(err,
            (GFError{GFErrorCode::kNOT_SUPPORTED, GFErrorOrigin::kRPGP, 0}));
}

// Every outcome can be put into words, whichever engine's texts the build has.
TEST(GFErrorTest, EveryCodeCanBeDescribed) {
  for (auto i = static_cast<int>(GFErrorCode::kOK);
       i <= static_cast<int>(GFErrorCode::kBUG); i++) {
    const auto [source, description] =
        DescribeGFError(GFError::Application(static_cast<GFErrorCode>(i)));
    EXPECT_FALSE(description.isEmpty()) << "code " << i;
  }
}

// An operation no engine may run used to return a default-constructed result,
// and a default GFError, alone or first in a tuple, reads as success.
TEST(GFErrorTest, AnUnsupportedOperationNeverReadsAsSuccess) {
  EXPECT_EQ(NotSupportedResult<GFError>().code, GFErrorCode::kNOT_SUPPORTED);

  const auto [err, buffer] = NotSupportedResult<std::tuple<GFError, int>>();
  EXPECT_EQ(err.code, GFErrorCode::kNOT_SUPPORTED);
  EXPECT_EQ(buffer, 0);

  EXPECT_FALSE(NotSupportedResult<bool>());
}

}  // namespace GpgFrontend::Test
