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

#include "ui/function/ProfileController.h"

namespace GpgFrontend::UI::Test {

TEST(ProfileLaunchArgsTest, ProfileOptionsAreStrippedWithTheirValues) {
  const QStringList args = {"gpgfrontend", "--profile", "work", "-l", "info"};

  EXPECT_EQ(StripProfileArgs(args), QStringList({"gpgfrontend", "-l", "info"}));
}

TEST(ProfileLaunchArgsTest, TheEqualsFormIsStrippedToo) {
  const QStringList args = {"gpgfrontend", "--profile=work",
                            "--profile-root=/srv/p", "-l", "info"};

  EXPECT_EQ(StripProfileArgs(args), QStringList({"gpgfrontend", "-l", "info"}));
}

TEST(ProfileLaunchArgsTest, APositionalPackageIsStripped) {
  // the package selected *this* window's profile; a new window opening a
  // different one must not inherit it
  const QStringList args = {"gpgfrontend", "/home/x/work.gfprofile"};
  EXPECT_EQ(StripProfileArgs(args), QStringList({"gpgfrontend"}));
}

TEST(ProfileLaunchArgsTest, UnrelatedArgumentsAreLeftAlone) {
  const QStringList args = {"gpgfrontend", "--self-check", "-l", "debug"};
  EXPECT_EQ(StripProfileArgs(args), args);
}

// The bug this exists to prevent: a window opened from a window opened from a
// window carrying three --profile flags, with the resolver honouring the first
// and the user landing on a profile they left two windows ago.
TEST(ProfileLaunchArgsTest, OpeningRepeatedlyDoesNotAccumulateFlags) {
  QStringList args = {"gpgfrontend", "-l", "info"};

  for (const auto* target : {"one", "two", "three"}) {
    auto launch = BuildProfileLaunchArgs(args, QString::fromUtf8(target));
    args = QStringList{"gpgfrontend"} + launch;
  }

  EXPECT_EQ(args.count("--profile"), 1);
  EXPECT_EQ(args,
            QStringList({"gpgfrontend", "-l", "info", "--profile", "three"}));
}

TEST(ProfileLaunchArgsTest, TheImplicitProfilesCarryNoFlag) {
  // classic and portable are what the resolver falls back to on its own, so
  // naming them on the command line would be a profile id that does not exist
  for (const auto* id : {"classic", "portable", ""}) {
    const auto out = BuildProfileLaunchArgs(
        {"gpgfrontend", "--profile", "work"}, QString::fromUtf8(id));
    EXPECT_FALSE(out.contains("--profile")) << id;
  }
}

TEST(ProfileLaunchArgsTest, Argv0IsNotPartOfThePackageArguments) {
  const auto out = BuildPackageLaunchArgs(
      {"/usr/bin/gpgfrontend", "-l", "info"}, "/home/x/work.gfprofile");
  EXPECT_EQ(out, QStringList({"-l", "info", "/home/x/work.gfprofile"}));
}

// A package names a profile just as `--profile` does, so the two must not both
// reach the new window: the resolver would honour the flag and quietly ignore
// the package the user actually picked.
TEST(ProfileLaunchArgsTest, APackageReplacesAnInheritedProfileFlag) {
  const auto out =
      BuildPackageLaunchArgs({"gpgfrontend", "--profile", "work", "-l", "info"},
                             "/home/x/a.gfprofile");

  EXPECT_FALSE(out.contains("--profile"));
  EXPECT_EQ(out, QStringList({"-l", "info", "/home/x/a.gfprofile"}));
}

TEST(ProfileLaunchArgsTest, OneWindowPerPackageNotOnePerOpen) {
  // opening package B from a window already showing package A leaves exactly
  // one positional argument, not both
  const auto out = BuildPackageLaunchArgs(
      {"gpgfrontend", "/home/x/a.gfprofile"}, "/home/x/b.gfprofile");

  EXPECT_EQ(out, QStringList({"/home/x/b.gfprofile"}));
}

TEST(ProfileLaunchArgsTest, Argv0IsNotPartOfTheRelaunchArguments) {
  const auto out =
      BuildProfileLaunchArgs({"/usr/bin/gpgfrontend", "-l", "info"}, "work");
  EXPECT_FALSE(out.contains("/usr/bin/gpgfrontend"));
  EXPECT_EQ(out, QStringList({"-l", "info", "--profile", "work"}));
}

}  // namespace GpgFrontend::UI::Test
