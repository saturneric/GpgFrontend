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

#include "core/profile/ProfileLoader.h"
#include "shell/present/ProfileLoadErrorText.h"

namespace GpgFrontend::Test {

// The words for a profile that would not open are core's, so every interface
// says the same; one the person backed out of needs none.
TEST(ProfileLoadErrorTest, EveryFailureButBackingOutIsPutIntoWords) {
  ProfileLoadError cancelled;
  cancelled.failure = ProfileLoadFailure::kCANCELLED;
  EXPECT_FALSE(DescribeProfileLoadError(cancelled).has_value());

  ProfileLoadError open;
  open.failure = ProfileLoadFailure::kALREADY_OPEN;
  open.subject = QStringLiteral("/profiles/work");
  const auto text = DescribeProfileLoadError(open);
  ASSERT_TRUE(text.has_value());
  EXPECT_FALSE(text->title.isEmpty());
  EXPECT_TRUE(text->text.contains(QDir::toNativeSeparators(open.subject)));
}

// A store that would not answer is not a lost key: the words say so, carry the
// platform's reason, and never speak of resetting anything.
TEST(ProfileLoadErrorTest, AnUnreachableStoreIsNotDescribedAsALostKey) {
  ProfileLoadError locked;
  locked.failure = ProfileLoadFailure::kKEY_STORE_LOCKED;
  locked.subject = QStringLiteral("/profiles/work/secure/app.key");
  locked.detail = QStringLiteral("store said: locked");

  const auto text = DescribeProfileLoadError(locked);
  ASSERT_TRUE(text.has_value());
  EXPECT_FALSE(text->title.isEmpty());
  EXPECT_TRUE(text->text.contains(locked.detail));
  EXPECT_FALSE(text->text.contains(QStringLiteral("reset"),
                                   Qt::CaseInsensitive));
}

}  // namespace GpgFrontend::Test
