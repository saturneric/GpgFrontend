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

#include "shell/rules/FileOperationRules.h"

namespace GpgFrontend::Test {

using Shell::FileOperand;
using Shell::FileOperation;
using Shell::FileOperationAllowed;

namespace {

auto File(const QString& name) -> FileOperand { return {.name = name}; }

auto Dir(const QString& name) -> FileOperand {
  return {.name = name, .is_file = false, .is_dir = true};
}

}  // namespace

TEST(ShellFileOperationRulesTest, PlainFilesAreEncryptedAndSigned) {
  const QList<FileOperand> plain{File(u"notes.txt"_s), File(u"photo.jpg"_s)};
  EXPECT_TRUE(FileOperationAllowed(FileOperation::kEncrypt, plain));
  EXPECT_TRUE(FileOperationAllowed(FileOperation::kSign, plain));
  EXPECT_FALSE(FileOperationAllowed(FileOperation::kDecrypt, plain));
  EXPECT_FALSE(FileOperationAllowed(FileOperation::kVerify, plain));
}

TEST(ShellFileOperationRulesTest, AFolderIsOnlyEncrypted) {
  const QList<FileOperand> folder{Dir(u"photos"_s)};
  EXPECT_TRUE(FileOperationAllowed(FileOperation::kEncrypt, folder));
  EXPECT_FALSE(FileOperationAllowed(FileOperation::kSign, folder));
}

TEST(ShellFileOperationRulesTest, OpenPGPDataIsDecryptedOrVerified) {
  const QList<FileOperand> messages{File(u"a.txt.gpg"_s), File(u"b.asc"_s)};
  EXPECT_TRUE(FileOperationAllowed(FileOperation::kDecrypt, messages));
  EXPECT_TRUE(FileOperationAllowed(FileOperation::kVerify, messages));
  EXPECT_FALSE(FileOperationAllowed(FileOperation::kEncrypt, messages));

  // A detached signature verifies, and decrypts nothing.
  const QList<FileOperand> signature{File(u"a.txt.sig"_s)};
  EXPECT_TRUE(FileOperationAllowed(FileOperation::kVerify, signature));
  EXPECT_FALSE(FileOperationAllowed(FileOperation::kDecrypt, signature));
}

TEST(ShellFileOperationRulesTest, AMixedChoiceAllowsOnlyWhatFitsAll) {
  const QList<FileOperand> mixed{File(u"notes.txt"_s), File(u"a.txt.gpg"_s)};
  for (const auto op : {FileOperation::kEncrypt, FileOperation::kSign,
                        FileOperation::kDecrypt, FileOperation::kVerify}) {
    EXPECT_FALSE(FileOperationAllowed(op, mixed));
  }
  EXPECT_FALSE(FileOperationAllowed(FileOperation::kEncrypt, {}));
}

TEST(ShellFileOperationRulesTest, ASignatureIsPairedWithTheFileItNames) {
  const auto sig = Shell::MatchVerifyFile(u"report.pdf.SIG"_s);
  EXPECT_EQ(sig.role, Shell::VerifyFileRole::kSignature);
  EXPECT_EQ(sig.partner_names, QStringList{u"report.pdf"_s});

  const auto data = Shell::MatchVerifyFile(u"report.pdf"_s);
  EXPECT_EQ(data.role, Shell::VerifyFileRole::kData);
  EXPECT_EQ(data.partner_names,
            (QStringList{u"report.pdf.sig"_s, u"report.pdf.asc"_s}));

  const auto both = Shell::MatchVerifyFile(u"report.pdf.gpg"_s);
  EXPECT_EQ(both.role, Shell::VerifyFileRole::kCombined);
  EXPECT_TRUE(both.partner_names.isEmpty());
}

}  // namespace GpgFrontend::Test
