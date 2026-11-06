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

// The version shown everywhere is the full SemVer cmake/GitVersion.cmake
// derived from git; the core is its numeric X.Y.Z, which every numeric
// comparison (module min_host_version, the engine version) takes.
TEST(BuildInfoTest, TheVersionIsSemVerAroundItsCore) {
  const auto full = GetProjectVersion();
  const auto core = GetProjectVersionCore();

  static const QRegularExpression kCore(uR"(^v\d+\.\d+\.\d+$)"_s);
  static const QRegularExpression kFull(
      uR"(^v\d+\.\d+\.\d+(-[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?)"
      uR"((\+[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?$)"_s);
  EXPECT_TRUE(kCore.match(core).hasMatch()) << core.toStdString();
  EXPECT_TRUE(kFull.match(full).hasMatch()) << full.toStdString();

  // The full version is the core plus, at most, a prerelease and build.
  ASSERT_TRUE(full.startsWith(core)) << full.toStdString();
  const auto rest = full.mid(core.size());
  EXPECT_TRUE(rest.isEmpty() || rest.startsWith(u'-') || rest.startsWith(u'+'))
      << full.toStdString();
}

}  // namespace GpgFrontend::Test
