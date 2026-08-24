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
#include "ui/function/FileSystemItemRules.h"

namespace GpgFrontend::Test {

using UI::FileSystemItemNameStatus;

namespace {

auto Creating(const QString& name, bool target_exists = false,
              bool reserved = false) -> FileSystemItemNameStatus {
  return UI::ValidateFileSystemItemName(name, {}, target_exists, reserved);
}

auto Renaming(const QString& name, const QString& original_name,
              bool target_exists = false, bool reserved = false)
    -> FileSystemItemNameStatus {
  return UI::ValidateFileSystemItemName(name, original_name, target_exists,
                                        reserved);
}

}  // namespace

TEST(FileSystemItemRulesTest, ANameOfNothingButSpaceIsNoName) {
  for (const auto* s : {"", " ", "\t", "   "}) {
    EXPECT_EQ(Creating(s), FileSystemItemNameStatus::kEMPTY);
  }
}

TEST(FileSystemItemRulesTest, TheDotNamesAreRefusedButALeadingDotIsFine) {
  EXPECT_EQ(Creating("."), FileSystemItemNameStatus::kDOT_NAME);
  EXPECT_EQ(Creating(".."), FileSystemItemNameStatus::kDOT_NAME);

  EXPECT_EQ(Creating(".gitignore"), FileSystemItemNameStatus::kOK);
  EXPECT_EQ(Creating("..."), FileSystemItemNameStatus::kOK);
}

TEST(FileSystemItemRulesTest, ANameIsNotAPath) {
  // both separators, on every platform: the name may end up on a share or in
  // an archive read elsewhere.
  EXPECT_EQ(Creating("a/b"), FileSystemItemNameStatus::kPATH_SEPARATOR);
  EXPECT_EQ(Creating("a\\b"), FileSystemItemNameStatus::kPATH_SEPARATOR);
  EXPECT_EQ(Creating("/abs"), FileSystemItemNameStatus::kPATH_SEPARATOR);
}

TEST(FileSystemItemRulesTest, WindowsDeviceNamesAreOnlyRefusedWhereTheyBite) {
  for (const auto* s : {"CON", "con", "com1.txt", "LPT9", "NUL.tar.gz"}) {
    EXPECT_TRUE(UI::IsOSReservedName(s)) << s;
    EXPECT_EQ(Creating(s, false, true), FileSystemItemNameStatus::kOS_RESERVED);
    EXPECT_EQ(Creating(s, false, false), FileSystemItemNameStatus::kOK);
  }

  // a longer name that merely starts with one is not reserved.
  for (const auto* s : {"CONS", "COM10", "console.log"}) {
    EXPECT_FALSE(UI::IsOSReservedName(s)) << s;
    EXPECT_EQ(Creating(s, false, true), FileSystemItemNameStatus::kOK);
  }
}

TEST(FileSystemItemRulesTest, RenamingToTheCurrentNameIsNotACollision) {
  // the item itself occupies the target path, so the existence check would
  // otherwise report the item colliding with itself.
  EXPECT_EQ(Renaming("notes.txt", "notes.txt", true),
            FileSystemItemNameStatus::kUNCHANGED);

  // only rename has an original name to be unchanged from.
  EXPECT_EQ(Creating("notes.txt", true),
            FileSystemItemNameStatus::kALREADY_EXISTS);
}

TEST(FileSystemItemRulesTest, ATakenNameIsRefusedInBothModes) {
  EXPECT_EQ(Creating("taken", true), FileSystemItemNameStatus::kALREADY_EXISTS);
  EXPECT_EQ(Renaming("taken", "before", true),
            FileSystemItemNameStatus::kALREADY_EXISTS);

  EXPECT_EQ(Renaming("free", "before", false), FileSystemItemNameStatus::kOK);
}

TEST(FileSystemItemRulesTest, TheShapeOfANameIsJudgedBeforeItsAvailability) {
  // a taken path must not mask the more useful complaint.
  EXPECT_EQ(Renaming("..", "before", true),
            FileSystemItemNameStatus::kDOT_NAME);
  EXPECT_EQ(Renaming("a/b", "before", true),
            FileSystemItemNameStatus::kPATH_SEPARATOR);
  EXPECT_EQ(Renaming("CON", "before", true, true),
            FileSystemItemNameStatus::kOS_RESERVED);
}

TEST(FileSystemItemRulesTest, DroppingAnItemOnItselfGoesNowhere) {
  EXPECT_TRUE(UI::IsMoveIntoItselfOrChild("/x/a", "/x/a", true));
  EXPECT_TRUE(UI::IsMoveIntoItselfOrChild("/x/f.txt", "/x/f.txt", false));
}

TEST(FileSystemItemRulesTest, AFolderCannotBeDroppedIntoItsOwnSubtree) {
  EXPECT_TRUE(UI::IsMoveIntoItselfOrChild("/x/a", "/x/a/b", true));
  EXPECT_TRUE(UI::IsMoveIntoItselfOrChild("/x/a", "/x/a/b/c/d", true));

  // a sibling whose name merely starts the same way is a different folder.
  EXPECT_FALSE(UI::IsMoveIntoItselfOrChild("/x/a", "/x/ab", true));
  EXPECT_FALSE(UI::IsMoveIntoItselfOrChild("/x/a", "/x/b", true));

  // only a directory can contain anything.
  EXPECT_FALSE(UI::IsMoveIntoItselfOrChild("/x/f.txt", "/x/f.txt/y", false));
}

TEST(FileSystemItemRulesTest, DropTargetsAreComparedAfterCleaning) {
  EXPECT_TRUE(UI::IsMoveIntoItselfOrChild("/x/./a", "/x/a", true));
  EXPECT_TRUE(UI::IsMoveIntoItselfOrChild("/x/a", "/x/b/../a/c", true));
  EXPECT_TRUE(UI::IsMoveIntoItselfOrChild("/x/a/", "/x/a", true));
}

TEST(FileSystemItemRulesTest, ADropIsANoOpOnlyWhenEverySourceIsAlreadyThere) {
  EXPECT_TRUE(UI::IsSameDirectoryOperation({"/x/a", "/x/b"}, "/x"));
  EXPECT_TRUE(UI::IsSameDirectoryOperation({"/x/./a"}, "/x"));

  EXPECT_FALSE(UI::IsSameDirectoryOperation({"/x/a", "/y/b"}, "/x"));
  EXPECT_FALSE(UI::IsSameDirectoryOperation({"/y/b"}, "/x"));

  // nothing to move is, vacuously, nothing to do.
  EXPECT_TRUE(UI::IsSameDirectoryOperation({}, "/x"));
}

}  // namespace GpgFrontend::Test
