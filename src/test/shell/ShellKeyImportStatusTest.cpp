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

#include "shell/present/KeyImportStatus.h"

namespace GpgFrontend::Test {

using Shell::DescribeKeyImportStatus;
using Shell::KeyImportChange;
using Shell::KeyImportChangeOf;

TEST(ShellKeyImportStatusTest, EveryChangeToAKeyIsSaid) {
  // A user ID and a signature together: both, not only the larger bit.
  EXPECT_EQ(DescribeKeyImportStatus(2 | 4),
            u"Public, New UID, New Signature"_s);
  EXPECT_EQ(DescribeKeyImportStatus(16 | 2 | 8),
            u"Private, New UID, New Subkey"_s);
  EXPECT_EQ(KeyImportChangeOf(2 | 4), KeyImportChange::kUpdated);
}

TEST(ShellKeyImportStatusTest, ANewKeyIsNewWhateverElseCameWithIt) {
  EXPECT_EQ(DescribeKeyImportStatus(1), u"Public, New Key"_s);
  EXPECT_EQ(DescribeKeyImportStatus(16 | 1 | 2), u"Private, New Key"_s);
  EXPECT_EQ(KeyImportChangeOf(17), KeyImportChange::kNewKey);
}

TEST(ShellKeyImportStatusTest, NothingNewIsUnchanged) {
  EXPECT_EQ(DescribeKeyImportStatus(0), u"Public, Unchanged"_s);
  EXPECT_EQ(DescribeKeyImportStatus(16), u"Private, Unchanged"_s);
  EXPECT_EQ(KeyImportChangeOf(16), KeyImportChange::kUnchanged);
}

}  // namespace GpgFrontend::Test
