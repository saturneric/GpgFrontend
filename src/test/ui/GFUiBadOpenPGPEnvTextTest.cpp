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

#include "GpgFrontendTest.h"
#include "ui/function/OpenPGPEnvGuard.h"

namespace GpgFrontend::Test {

namespace {

const std::array kAllReasons{
    BadOpenPGPEnvReason::kUNKNOWN,
    BadOpenPGPEnvReason::kNO_SUPPORTED_ENGINE,
    BadOpenPGPEnvReason::kBASIC_PATH_INIT_FAILED,
    BadOpenPGPEnvReason::kDEFAULT_CONTEXT_INIT_FAILED,
    BadOpenPGPEnvReason::kKEY_CACHE_INIT_FAILED,
    BadOpenPGPEnvReason::kNO_VALID_KEY_DATABASE,
};

}  // namespace

TEST(BadOpenPGPEnvTextTest, EveryReasonProducesATitleAndABody) {
  for (const auto reason : kAllReasons) {
    const auto text = UI::DescribeBadOpenPGPEnv(reason, "detail");
    EXPECT_FALSE(text.title.isEmpty()) << static_cast<int>(reason);
    EXPECT_FALSE(text.body.isEmpty()) << static_cast<int>(reason);
  }
}

TEST(BadOpenPGPEnvTextTest, AMissingKeyDatabaseIsNotReportedAsAMissingEngine) {
  // The whole point of the split: telling a user with a moved key database
  // that no engine was found sends them looking in the wrong place.
  const auto db = UI::DescribeBadOpenPGPEnv(
      BadOpenPGPEnvReason::kNO_VALID_KEY_DATABASE, "");
  const auto engine =
      UI::DescribeBadOpenPGPEnv(BadOpenPGPEnvReason::kNO_SUPPORTED_ENGINE, "");
  EXPECT_NE(db.title, engine.title);

  const auto paths = UI::DescribeBadOpenPGPEnv(
      BadOpenPGPEnvReason::kBASIC_PATH_INIT_FAILED, "");
  EXPECT_NE(paths.title, engine.title);
  EXPECT_NE(paths.title, db.title);
}

TEST(BadOpenPGPEnvTextTest, RetryIsOfferedOnlyWhereItCouldHelp) {
  // A restart re-reads the same unusable key database configuration, so
  // offering Retry there just invites the user to loop.
  EXPECT_FALSE(
      UI::DescribeBadOpenPGPEnv(BadOpenPGPEnvReason::kNO_VALID_KEY_DATABASE, "")
          .offer_retry);

  for (const auto reason : kAllReasons) {
    if (reason == BadOpenPGPEnvReason::kNO_VALID_KEY_DATABASE) continue;
    EXPECT_TRUE(UI::DescribeBadOpenPGPEnv(reason, "").offer_retry)
        << static_cast<int>(reason);
  }
}

TEST(BadOpenPGPEnvTextTest, TheDetailReachesTheUser) {
  for (const auto reason : kAllReasons) {
    EXPECT_TRUE(UI::DescribeBadOpenPGPEnv(reason, "a-specific-detail")
                    .body.contains("a-specific-detail"))
        << static_cast<int>(reason);
  }
}

TEST(BadOpenPGPEnvTextTest, UnknownAndNoEngineShareTheGenericFallback) {
  EXPECT_EQ(
      UI::DescribeBadOpenPGPEnv(BadOpenPGPEnvReason::kUNKNOWN, "d").title,
      UI::DescribeBadOpenPGPEnv(BadOpenPGPEnvReason::kNO_SUPPORTED_ENGINE, "d")
          .title);
}

}  // namespace GpgFrontend::Test
