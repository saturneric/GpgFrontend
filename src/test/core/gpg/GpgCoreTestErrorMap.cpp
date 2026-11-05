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

// GnuPG's error codes and GpgFrontend's: the mapping at the GnuPG backend.

#include <type_traits>

#include "GpgCoreTest.h"
#include "GpgFrontendTest.h"
#include "core/function/gpg/GpgErrorMap.h"
#include "core/typedef/GFError.h"
#include "core/utils/AsyncUtils.h"

namespace GpgFrontend::Test {

// An error is never a GnuPG number: neither way round, and not by accident.
static_assert(!std::is_convertible_v<GFError, gpgme_error_t>);
static_assert(!std::is_convertible_v<gpgme_error_t, GFError>);

TEST(GFErrorTest, EveryCodeHasItsOwnGnuPGCode) {
  QSet<gpgme_error_t> seen;
  for (auto i = static_cast<int>(GFErrorCode::kOK);
       i <= static_cast<int>(GFErrorCode::kBUG); i++) {
    const auto code = static_cast<GFErrorCode>(i);
    const auto gpg = ToGpgError(GFError::Application(code));
    EXPECT_FALSE(seen.contains(gpg)) << "code " << i << " shares " << gpg;
    seen.insert(gpg);
    EXPECT_EQ(FromGpgError(gpg).code, code) << "code " << i;
  }
}

TEST(GFErrorTest, AGnuPGErrorKeepsWhatGnuPGSaid) {
  const auto raw = gpgme_err_make(GPG_ERR_SOURCE_GPGME, GPG_ERR_BAD_PASSPHRASE);
  const auto err = FromGpgError(raw);
  EXPECT_EQ(err.code, GFErrorCode::kBAD_PASSPHRASE);
  EXPECT_EQ(err.origin, GFErrorOrigin::kGNUPG);
  EXPECT_EQ(static_cast<gpgme_error_t>(err.native), raw);
  EXPECT_EQ(ToGpgError(err), raw) << "the source must survive the trip";

  EXPECT_TRUE(FromGpgError(GPG_ERR_NO_ERROR).Ok());
  EXPECT_TRUE(FromGpgError(GPG_ERR_CANCELED).Canceled());
}

TEST(GFErrorTest, AGnuPGCodeWithNoCounterpartIsAGeneralFailure) {
  const auto err = FromGpgError(GPG_ERR_ENOPKG);
  EXPECT_EQ(err.code, GFErrorCode::kGENERAL);
  EXPECT_TRUE(err.Failed());
  EXPECT_EQ(ToGpgError(err), GPG_ERR_ENOPKG);
}

// What the runner reports for a channel reads as that channel's engine's.
TEST_F(GpgCoreTest, AnEngineErrorOnAGnuPGChannelIsGnuPGs) {
  EXPECT_EQ(EngineErrorOf(kGpgChannelForUnitTest, GFErrorCode::kCANCELED),
            FromGpgError(GPG_ERR_CANCELED));
  EXPECT_EQ(
      EngineErrorOf(kGpgChannelForUnitTest, GFErrorCode::kNOT_OPERATIONAL),
      FromGpgError(GPG_ERR_NOT_OPERATIONAL));
}

}  // namespace GpgFrontend::Test
