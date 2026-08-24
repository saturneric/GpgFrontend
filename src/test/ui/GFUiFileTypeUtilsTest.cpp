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
#include "ui/function/FileTypeUtils.h"

namespace GpgFrontend::Test {

TEST(FileTypeUtilsTest, MessageContainersAreRecognised) {
  for (const auto* n : {"a.gpg", "a.pgp", "a.asc"}) {
    EXPECT_TRUE(UI::IsOpenPGPMessageFile(QFileInfo(n))) << n;
    EXPECT_TRUE(UI::IsOpenPGPRelatedFile(QFileInfo(n))) << n;
  }
}

TEST(FileTypeUtilsTest, ADetachedSignatureIsRelatedButNotAMessage) {
  // The distinction is what keeps a .sig out of the decrypt/verify-inline
  // path while still keeping it out of the encrypt/sign menu.
  const QFileInfo sig("a.sig");
  EXPECT_TRUE(UI::IsOpenPGPSignatureFile(sig));
  EXPECT_TRUE(UI::IsOpenPGPRelatedFile(sig));
  EXPECT_FALSE(UI::IsOpenPGPMessageFile(sig));
}

TEST(FileTypeUtilsTest, PlainFilesMatchNothing) {
  for (const auto* n : {"a.txt", "a", "a.gpg.txt", "archive.tar"}) {
    EXPECT_FALSE(UI::IsOpenPGPMessageFile(QFileInfo(n))) << n;
    EXPECT_FALSE(UI::IsOpenPGPSignatureFile(QFileInfo(n))) << n;
    EXPECT_FALSE(UI::IsOpenPGPRelatedFile(QFileInfo(n))) << n;
  }
}

TEST(FileTypeUtilsTest, ProfilePackagesAreRecognised) {
  for (const auto* n : {"a.gfp", "A.GFP", "profile.gfp", "/tmp/work.Gfp"}) {
    EXPECT_TRUE(UI::IsProfilePackageFile(QFileInfo(n))) << n;
  }
}

TEST(FileTypeUtilsTest, NearMissesAreNotProfilePackages) {
  // .gfpack is the key package, an unrelated thing that merely starts the same
  // way; the rest are ordinary files that happen to mention the extension.
  for (const auto* n : {"a.gfpack", "a.gfp.txt", "a.txt", "a", "gfp"}) {
    EXPECT_FALSE(UI::IsProfilePackageFile(QFileInfo(n))) << n;
  }
}

TEST(FileTypeUtilsTest, ProfilePackagesAndOpenPGPFilesAreDisjoint) {
  // What decides the file panel's double-click and what decides its
  // encrypt/decrypt menu must never claim the same file.
  const QFileInfo package("a.gfp");
  EXPECT_FALSE(UI::IsOpenPGPMessageFile(package));
  EXPECT_FALSE(UI::IsOpenPGPSignatureFile(package));
  EXPECT_FALSE(UI::IsOpenPGPRelatedFile(package));

  for (const auto* n : {"a.gpg", "a.pgp", "a.asc", "a.sig"}) {
    EXPECT_FALSE(UI::IsProfilePackageFile(QFileInfo(n))) << n;
  }
}

TEST(FileTypeUtilsTest, SuffixMatchingIgnoresCase) {
  EXPECT_TRUE(UI::IsOpenPGPMessageFile(QFileInfo("KEY.ASC")));
  EXPECT_TRUE(UI::IsOpenPGPMessageFile(QFileInfo("Key.Gpg")));
  EXPECT_TRUE(UI::IsOpenPGPSignatureFile(QFileInfo("a.SIG")));
  EXPECT_TRUE(UI::IsProfilePackageFile(QFileInfo("Work.GFP")));
}

}  // namespace GpgFrontend::Test
