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

#include "shell/rules/RevocationReasons.h"

namespace GpgFrontend::Test {

using Shell::DefaultRevocationReason;
using Shell::RevocationReasonsFor;
using Shell::RevocationTarget;

namespace {

auto CodesOf(RevocationTarget target) -> QList<int> {
  QList<int> codes;
  for (const auto& reason : RevocationReasonsFor(target)) {
    codes << reason.code;
    EXPECT_FALSE(reason.text.isEmpty());
  }
  return codes;
}

}  // namespace

// A key or subkey gives one of OpenPGP's four, a user ID only none or "no
// longer valid": the core refuses any other for it.
TEST(ShellRevocationReasonsTest, EachTargetGetsOpenPgpsCodes) {
  EXPECT_EQ(CodesOf(RevocationTarget::kKey), (QList<int>{0, 1, 2, 3}));
  EXPECT_EQ(CodesOf(RevocationTarget::kSubkey), (QList<int>{0, 1, 2, 3}));
  EXPECT_EQ(CodesOf(RevocationTarget::kUserId), (QList<int>{0, 4}));
}

// The usual reason is one the target offers.
TEST(ShellRevocationReasonsTest, TheUsualReasonIsOffered) {
  for (const auto target : {RevocationTarget::kKey, RevocationTarget::kSubkey,
                            RevocationTarget::kUserId}) {
    EXPECT_TRUE(CodesOf(target).contains(DefaultRevocationReason(target)));
  }
  EXPECT_EQ(DefaultRevocationReason(RevocationTarget::kKey), 3);
  EXPECT_EQ(DefaultRevocationReason(RevocationTarget::kUserId), 4);
}

// A whole key is superseded by a newer key, a subkey by a newer subkey.
TEST(ShellRevocationReasonsTest, SupersededNamesWhatReplacesIt) {
  const auto key = RevocationReasonsFor(RevocationTarget::kKey).at(2).text;
  const auto subkey =
      RevocationReasonsFor(RevocationTarget::kSubkey).at(2).text;
  EXPECT_EQ(key, QStringLiteral("A newer key replaces it"));
  EXPECT_EQ(subkey, QStringLiteral("A newer subkey replaces it"));
}

}  // namespace GpgFrontend::Test
