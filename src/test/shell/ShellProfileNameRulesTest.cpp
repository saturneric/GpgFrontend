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

#include "shell/rules/ProfileNameRules.h"

namespace GpgFrontend::Test {

using Shell::CheckProfileName;
using Shell::kMaxProfileNameLength;
using Shell::ProfileNameProblem;

TEST(ShellProfileNameRulesTest, ANameIsNeededAndNotTooLong) {
  EXPECT_EQ(CheckProfileName({}), ProfileNameProblem::kEmpty);
  EXPECT_EQ(CheckProfileName(QStringLiteral("   ")),
            ProfileNameProblem::kEmpty);
  EXPECT_EQ(CheckProfileName(QStringLiteral(" Work ")),
            ProfileNameProblem::kNone);
  EXPECT_EQ(CheckProfileName(QString(kMaxProfileNameLength, u'a')),
            ProfileNameProblem::kNone);
  EXPECT_EQ(CheckProfileName(QString(kMaxProfileNameLength + 1, u'a')),
            ProfileNameProblem::kTooLong);
  // Measured trimmed: spaces around a name of the longest length are fine.
  EXPECT_EQ(CheckProfileName(QStringLiteral("  ") +
                             QString(kMaxProfileNameLength, u'a') +
                             QStringLiteral("  ")),
            ProfileNameProblem::kNone);
}

TEST(ShellProfileNameRulesTest, ATakenNameIsRefusedWhateverItsCase) {
  const QStringList taken{QStringLiteral("Default"), QStringLiteral("Work")};
  EXPECT_EQ(CheckProfileName(QStringLiteral("work"), taken),
            ProfileNameProblem::kTaken);
  EXPECT_EQ(CheckProfileName(QStringLiteral(" DEFAULT "), taken),
            ProfileNameProblem::kTaken);
  EXPECT_EQ(CheckProfileName(QStringLiteral("Home"), taken),
            ProfileNameProblem::kNone);
}

}  // namespace GpgFrontend::Test
