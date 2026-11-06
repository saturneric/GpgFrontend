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
#include "core/utils/GpgUtils.h"
#include "shell/rules/FileNameRules.h"

namespace GpgFrontend::Test {

TEST(FileTypeUtilsTest, MessageContainersAreRecognised) {
  for (const auto* n : {"a.gpg", "a.pgp", "a.asc"}) {
    EXPECT_TRUE(IsOpenPGPMessageFile(QFileInfo(n))) << n;
    EXPECT_TRUE(IsOpenPGPRelatedFile(QFileInfo(n))) << n;
  }
}

TEST(FileTypeUtilsTest, ADetachedSignatureIsRelatedButNotAMessage) {
  // The distinction is what keeps a .sig out of the decrypt/verify-inline
  // path while still keeping it out of the encrypt/sign menu.
  const QFileInfo sig("a.sig");
  EXPECT_TRUE(IsOpenPGPSignatureFile(sig));
  EXPECT_TRUE(IsOpenPGPRelatedFile(sig));
  EXPECT_FALSE(IsOpenPGPMessageFile(sig));
}

// A folder encrypted is named <folder>.tar.gpg: an archive by its name,
// whatever else is in the name; a plain .tar or .gpg is not one.
TEST(FileTypeUtilsTest, AnEncryptedArchiveIsKnownByItsName) {
  for (const auto* n :
       {"photos.tar.gpg", "Photos 2026.TAR.PGP", "a.b.tar.asc"}) {
    EXPECT_TRUE(IsOpenPGPArchiveFile(QFileInfo(n))) << n;
    EXPECT_TRUE(IsOpenPGPMessageFile(QFileInfo(n))) << n;
  }
  for (const auto* n :
       {"photos.tar", "photos.gpg", "tar.gpg", "x.tar.sig", "x.tar.gz.gpg"}) {
    EXPECT_FALSE(IsOpenPGPArchiveFile(QFileInfo(n))) << n;
  }
}

TEST(FileTypeUtilsTest, PlainFilesMatchNothing) {
  for (const auto* n : {"a.txt", "a", "a.gpg.txt", "archive.tar"}) {
    EXPECT_FALSE(IsOpenPGPMessageFile(QFileInfo(n))) << n;
    EXPECT_FALSE(IsOpenPGPSignatureFile(QFileInfo(n))) << n;
    EXPECT_FALSE(IsOpenPGPRelatedFile(QFileInfo(n))) << n;
  }
}

}  // namespace GpgFrontend::Test
