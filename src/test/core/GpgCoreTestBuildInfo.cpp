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
#include "core/utils/BuildInfoUtils.h"

namespace GpgFrontend::Test {

// What the desktop's About dialog and the phone's About page list, and what
// is copied for a bug report: the same facts, in the same order.
TEST(BuildInfoTest, TheCopiedTextIsTheListedFacts) {
  const auto facts = BuildInfoFacts();
  ASSERT_FALSE(facts.isEmpty());
  EXPECT_EQ(facts.front().value, GetProjectVersion());

  const auto lines = BuildInfoText().split(u'\n');
  ASSERT_EQ(lines.size(), facts.size());
  for (qsizetype i = 0; i < facts.size(); ++i) {
    EXPECT_EQ(lines.at(i), QString::fromLatin1(facts.at(i).caption) + u' ' +
                               facts.at(i).value);
    EXPECT_TRUE(QString::fromLatin1(facts.at(i).caption).endsWith(u':'));
  }
}

}  // namespace GpgFrontend::Test
