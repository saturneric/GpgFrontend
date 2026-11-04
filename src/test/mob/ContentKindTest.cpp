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

#include "crypto/ContentKind.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::ClassifyFileName;
using GpgFrontend::Mobile::ClassifyText;

TEST(ContentKindTest, EachArmorIsWhatItSays) {
  EXPECT_EQ(ClassifyText(""), "empty");
  EXPECT_EQ(ClassifyText(" \n\t"), "empty");
  EXPECT_EQ(ClassifyText("Dear Bob,\nthe numbers."), "plain");
  EXPECT_EQ(ClassifyText("-----BEGIN PGP MESSAGE-----\n\nhQ==\n"), "message");
  EXPECT_EQ(ClassifyText("-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA512\n"
                         "\ntext\n-----BEGIN PGP SIGNATURE-----\n"),
            "signed");
  EXPECT_EQ(ClassifyText("-----BEGIN PGP SIGNATURE-----\n"), "signature");
  EXPECT_EQ(ClassifyText("-----BEGIN PGP PUBLIC KEY BLOCK-----\n"),
            "public_key");
  EXPECT_EQ(ClassifyText("-----BEGIN PGP PRIVATE KEY BLOCK-----\n"),
            "private_key");
}

TEST(ContentKindTest, TextAroundTheArmorDoesNotChangeIt) {
  // Indented, it does not start a line: not a header.
  EXPECT_EQ(ClassifyText("  -----BEGIN PGP MESSAGE-----\nhQ==\n"), "plain");
  EXPECT_EQ(ClassifyText("> quoted mail\n-----BEGIN PGP MESSAGE-----\nhQ==\n"),
            "message");
  EXPECT_EQ(ClassifyText("\r\n-----BEGIN PGP MESSAGE-----\r\nhQ==\r\n"),
            "message");
  // A header mentioned inside a line is only text.
  EXPECT_EQ(ClassifyText("paste the -----BEGIN PGP MESSAGE----- block"),
            "plain");
  // A truncated header is not one.
  EXPECT_EQ(ClassifyText("-----BEGIN PGP MESS"), "plain");
}

// Files are told by their names, as the desktop's file panel tells them:
// nothing is read.
TEST(ContentKindTest, AFileIsWhatItsNameSays) {
  EXPECT_EQ(ClassifyFileName("letter.txt.gpg"), "message");
  EXPECT_EQ(ClassifyFileName("letter.txt.pgp"), "message");
  EXPECT_EQ(ClassifyFileName("letter.txt.asc"), "message");
  EXPECT_EQ(ClassifyFileName("REPORT.PDF.GPG"), "message");
  EXPECT_EQ(ClassifyFileName("contract.pdf.sig"), "signature");
  EXPECT_EQ(ClassifyFileName("Contract.pdf.SIG"), "signature");
  EXPECT_EQ(ClassifyFileName("photo.png"), "");
  EXPECT_EQ(ClassifyFileName("gpg"), "");
  EXPECT_EQ(ClassifyFileName("notes.gpg.txt"), "");
  EXPECT_EQ(ClassifyFileName(""), "");
}

}  // namespace GpgFrontend::Test::Mobile
