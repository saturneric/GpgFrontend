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
#include "ui/dialog/import_export/KeyPackageExportSummary.h"

namespace GpgFrontend::Test {

namespace {

using UI::BuildKeyPackageExportDoneRows;
using UI::BuildKeyPackageKeyRows;
using UI::BuildKeyPackageReviewRows;
using UI::CanAdvanceKeyPackageExport;
using UI::CanFilterToPrivateKeys;
using UI::CanIncludePrivateKeys;
using UI::DefaultKeyPackageKeyFilePath;
using UI::DescribeKeyPackageContentLimits;
using UI::DescribeKeyPackageContents;
using UI::DescribeKeyPackageExportProblem;
using UI::DescribeKeyPackageProtection;
using UI::IsValidKeyPackageName;
using UI::KeyPackageExportChoice;
using UI::KeyPackageExportCount;
using UI::KeyPackageExportProblem;
using UI::KeyPackageExportProblems;
using UI::KeyPackageExportStep;
using UI::KeyPackageIncludesPrivate;
using UI::KeyPackageKeepsKey;
using UI::KeyPackageKeyColumns;
using UI::KeyPackageKeyEntry;
using UI::KeyPackagePath;
using UI::WithKeyPackageExtension;

/// Three keys, two of them with a private part: every option has something to
/// change.
auto Mixed() -> KeyPackageExportChoice {
  KeyPackageExportChoice choice;
  choice.selected = 3;
  choice.with_private = 2;
  return choice;
}

/// Everything a finished wizard would hold, with the files in two folders.
auto Ready() -> KeyPackageExportChoice {
  auto choice = Mixed();
  choice.pin_acceptable = true;
  choice.key_file_path = QDir::cleanPath(QDir::tempPath() + "/usb/pkg.key");
  choice.name = "pkg";
  choice.location = QDir::cleanPath(QDir::tempPath() + "/out");
  choice.location_writable = true;
  return choice;
}

auto Keys() -> QList<KeyPackageKeyEntry> {
  return {
      {"Alice", "alice@example.org", "AAAA1111", true},
      {"Bob", "bob@example.org", "BBBB2222", false},
      {"", "carol@example.org", "CCCC3333", true},
  };
}

auto Values(const QList<UI::MetaListRow>& rows) -> QString {
  QStringList values;
  for (const auto& row : rows) values << row.value << row.detail;
  return values.join(" ");
}

}  // namespace

TEST(KeyPackageExportSummaryTest, TheFourContentCombinations) {
  // The filter narrows the set; "include private" decides what of each key is
  // written. Neither is the other's opposite.
  auto choice = Mixed();

  EXPECT_EQ(KeyPackageExportCount(choice), 3);
  EXPECT_FALSE(KeyPackageIncludesPrivate(choice));

  choice.only_with_private = true;
  EXPECT_EQ(KeyPackageExportCount(choice), 2);
  EXPECT_FALSE(KeyPackageIncludesPrivate(choice));

  choice.only_with_private = false;
  choice.include_private = true;
  EXPECT_EQ(KeyPackageExportCount(choice), 3);
  EXPECT_TRUE(KeyPackageIncludesPrivate(choice));

  choice.only_with_private = true;
  EXPECT_EQ(KeyPackageExportCount(choice), 2);
  EXPECT_TRUE(KeyPackageIncludesPrivate(choice));
}

TEST(KeyPackageExportSummaryTest, TheFilterOnlyDropsKeysWithoutAPrivatePart) {
  auto choice = Mixed();
  EXPECT_TRUE(KeyPackageKeepsKey(false, choice));
  EXPECT_TRUE(KeyPackageKeepsKey(true, choice));

  choice.only_with_private = true;
  EXPECT_FALSE(KeyPackageKeepsKey(false, choice));
  EXPECT_TRUE(KeyPackageKeepsKey(true, choice));
}

TEST(KeyPackageExportSummaryTest, PrivateKeysAreNeverClaimedWhenThereAreNone) {
  KeyPackageExportChoice choice;
  choice.selected = 2;
  choice.include_private = true;

  EXPECT_FALSE(KeyPackageIncludesPrivate(choice));
  EXPECT_FALSE(CanIncludePrivateKeys(choice));
  EXPECT_FALSE(DescribeKeyPackageContentLimits(choice).isEmpty());
}

TEST(KeyPackageExportSummaryTest,
     TheFilterIsOnlyOfferedWhereItChangesSomething) {
  auto choice = Mixed();
  EXPECT_TRUE(CanFilterToPrivateKeys(choice));

  // Would empty the set.
  choice.with_private = 0;
  EXPECT_FALSE(CanFilterToPrivateKeys(choice));

  // Would change nothing.
  choice.with_private = choice.selected;
  EXPECT_FALSE(CanFilterToPrivateKeys(choice));
  EXPECT_TRUE(CanIncludePrivateKeys(choice));
  EXPECT_TRUE(DescribeKeyPackageContentLimits(choice).isEmpty());
}

TEST(KeyPackageExportSummaryTest, AnEmptySetCannotLeaveTheFirstStep) {
  KeyPackageExportChoice choice;
  choice.selected = 1;
  choice.only_with_private = true;

  EXPECT_FALSE(
      CanAdvanceKeyPackageExport(KeyPackageExportStep::kContents, choice));
  EXPECT_EQ(
      KeyPackageExportProblems(KeyPackageExportStep::kContents, choice).front(),
      KeyPackageExportProblem::kNoKeys);
  EXPECT_TRUE(
      CanAdvanceKeyPackageExport(KeyPackageExportStep::kContents, Mixed()));
}

TEST(KeyPackageExportSummaryTest, LocationNeedsANameAFolderAndAKeyFile) {
  auto choice = Ready();
  choice.pin_acceptable = false;
  // The PIN is the last step's business.
  EXPECT_TRUE(
      CanAdvanceKeyPackageExport(KeyPackageExportStep::kLocation, choice));

  choice.key_file_path.clear();
  EXPECT_EQ(KeyPackageExportProblems(KeyPackageExportStep::kLocation, choice),
            QList{KeyPackageExportProblem::kNoKeyFile});

  choice = Ready();
  choice.name = "bad/name";
  EXPECT_EQ(KeyPackageExportProblems(KeyPackageExportStep::kLocation, choice),
            QList{KeyPackageExportProblem::kInvalidName});
}

TEST(KeyPackageExportSummaryTest, TheLastStepNeedsThePin) {
  auto choice = Ready();
  EXPECT_TRUE(
      CanAdvanceKeyPackageExport(KeyPackageExportStep::kProtection, choice));

  choice.pin_acceptable = false;
  EXPECT_EQ(KeyPackageExportProblems(KeyPackageExportStep::kProtection, choice),
            QList{KeyPackageExportProblem::kNoPin});
}

TEST(KeyPackageExportSummaryTest, TheLastStepIsNoMoreLenientThanTheFirst) {
  auto choice = Ready();
  choice.location.clear();
  EXPECT_FALSE(
      CanAdvanceKeyPackageExport(KeyPackageExportStep::kProtection, choice));

  choice = Ready();
  choice.selected = 0;
  choice.with_private = 0;
  EXPECT_FALSE(
      CanAdvanceKeyPackageExport(KeyPackageExportStep::kProtection, choice));
}

TEST(KeyPackageExportSummaryTest, ProblemsComeInTheOrderTheStepsAsk) {
  KeyPackageExportChoice choice;
  const auto problems =
      KeyPackageExportProblems(KeyPackageExportStep::kProtection, choice);

  const QList expected = {
      KeyPackageExportProblem::kNoKeys,
      KeyPackageExportProblem::kInvalidName,
      KeyPackageExportProblem::kNoLocation,
      KeyPackageExportProblem::kNoKeyFile,
      KeyPackageExportProblem::kNoPin,
  };
  EXPECT_EQ(problems, expected);
}

TEST(KeyPackageExportSummaryTest, TheKeyFileGoesBesideThePackageByDefault) {
  auto choice = Ready();
  EXPECT_EQ(DefaultKeyPackageKeyFilePath(choice),
            QDir(choice.location).filePath("pkg.key"));

  choice.name = "bad/name";
  EXPECT_TRUE(DefaultKeyPackageKeyFilePath(choice).isEmpty());

  choice = Ready();
  choice.location.clear();
  EXPECT_TRUE(DefaultKeyPackageKeyFilePath(choice).isEmpty());
}

TEST(KeyPackageExportSummaryTest,
     TheDefaultKeyFileNeverCollidesWithThePackage) {
  auto choice = Ready();
  choice.key_file_path = DefaultKeyPackageKeyFilePath(choice);
  EXPECT_TRUE(
      CanAdvanceKeyPackageExport(KeyPackageExportStep::kProtection, choice));
}

TEST(KeyPackageExportSummaryTest, AnUnwritableFolderBlocks) {
  auto choice = Ready();
  choice.location_writable = false;
  EXPECT_TRUE(
      KeyPackageExportProblems(KeyPackageExportStep::kProtection, choice)
          .contains(KeyPackageExportProblem::kLocationNotWritable));
}

TEST(KeyPackageExportSummaryTest, ThePackageAndTheKeyFileCannotBeOneFile) {
  auto choice = Ready();
  choice.key_file_path = KeyPackagePath(choice);
  EXPECT_TRUE(
      KeyPackageExportProblems(KeyPackageExportStep::kProtection, choice)
          .contains(KeyPackageExportProblem::kSameFile));
}

TEST(KeyPackageExportSummaryTest, NamesThatCannotBeFileNamesAreRefused) {
  EXPECT_TRUE(IsValidKeyPackageName("KeyPackage_ab12cd34"));
  EXPECT_TRUE(IsValidKeyPackageName("my keys"));

  for (const auto* name :
       {"", ".", "..", "a/b", "a\\b", "a:b", "a*b", "a?b", "a\"b", "a<b", "a>b",
        "a|b", " lead", "trail ", "dot."}) {
    EXPECT_FALSE(IsValidKeyPackageName(name)) << name;
  }
  EXPECT_FALSE(IsValidKeyPackageName(QString(201, 'a')));
  EXPECT_FALSE(IsValidKeyPackageName(QString("a") + QChar(0x07)));
}

TEST(KeyPackageExportSummaryTest, TheExtensionIsAddedOnlyWhenMissing) {
  EXPECT_EQ(WithKeyPackageExtension("/x/pkg", UI::kKeyPackageExtension),
            "/x/pkg.gfpack");
  EXPECT_EQ(WithKeyPackageExtension("/x/pkg.gfpack", UI::kKeyPackageExtension),
            "/x/pkg.gfpack");
  EXPECT_EQ(WithKeyPackageExtension("/x/pkg.GFPACK", UI::kKeyPackageExtension),
            "/x/pkg.GFPACK");
  EXPECT_EQ(WithKeyPackageExtension("/x/pkg", UI::kKeyPackageKeyFileExtension),
            "/x/pkg.key");
  EXPECT_TRUE(WithKeyPackageExtension({}, UI::kKeyPackageExtension).isEmpty());
}

TEST(KeyPackageExportSummaryTest, ThePackagePathJoinsFolderAndName) {
  auto choice = Ready();
  EXPECT_EQ(KeyPackagePath(choice),
            QDir(choice.location).filePath("pkg.gfpack"));

  choice.name = "bad/name";
  EXPECT_TRUE(KeyPackagePath(choice).isEmpty());

  choice = Ready();
  choice.location.clear();
  EXPECT_TRUE(KeyPackagePath(choice).isEmpty());
}

TEST(KeyPackageExportSummaryTest, EveryKeyIsListedWithWhatOfItGoesIn) {
  auto choice = Mixed();
  choice.only_with_private = true;
  choice.include_private = true;

  const auto rows = BuildKeyPackageKeyRows(Keys(), choice);
  ASSERT_EQ(rows.size(), 3);

  EXPECT_EQ(rows[0].name, "Alice");
  EXPECT_EQ(rows[0].email, "alice@example.org");
  EXPECT_EQ(rows[0].id, "AAAA1111");
  EXPECT_EQ(rows[0].exported, "Public and private");
  EXPECT_FALSE(rows[0].left_out);

  // Left out, but still listed.
  EXPECT_EQ(rows[1].name, "Bob");
  EXPECT_TRUE(rows[1].left_out);
  EXPECT_TRUE(rows[1].exported.contains("no private key"));

  EXPECT_EQ(rows[2].email, "carol@example.org");
  EXPECT_EQ(rows[2].exported, "Public and private");

  choice.include_private = false;
  EXPECT_EQ(BuildKeyPackageKeyRows(Keys(), choice)[0].exported, "Public");
}

TEST(KeyPackageExportSummaryTest, TheTableHasOneHeadingPerColumn) {
  // Name, email, key ID and what is exported: enough to tell two keys with
  // the same name apart before anything is written.
  EXPECT_EQ(KeyPackageKeyColumns().size(), 4);
}

TEST(KeyPackageExportSummaryTest, TheContentsLineCountsBoth) {
  EXPECT_EQ(DescribeKeyPackageContents(Mixed()),
            "3 keys selected, 2 with a private key.");

  KeyPackageExportChoice one;
  one.selected = 1;
  EXPECT_EQ(DescribeKeyPackageContents(one),
            "1 key selected, none with a private key.");

  one.with_private = 1;
  EXPECT_EQ(DescribeKeyPackageContents(one),
            "1 key selected, with its private key.");
}

TEST(KeyPackageExportSummaryTest, TheReviewSaysWhatIsAboutToHappen) {
  auto choice = Ready();
  choice.include_private = true;

  const auto all = Values(BuildKeyPackageReviewRows(choice));
  EXPECT_TRUE(all.contains("3 keys"));
  EXPECT_TRUE(all.contains("Included"));
  EXPECT_TRUE(all.contains("Key file protected by your PIN"));
  EXPECT_TRUE(all.contains(QDir::toNativeSeparators(KeyPackagePath(choice))));
  EXPECT_TRUE(all.contains(QDir::toNativeSeparators(choice.key_file_path)));
}

TEST(KeyPackageExportSummaryTest, TheReviewNeverClaimsProtectionItDoesNotHave) {
  auto choice = Ready();
  choice.pin_acceptable = false;
  for (const auto& row : BuildKeyPackageReviewRows(choice)) {
    EXPECT_FALSE(row.value.contains("protected"));
  }

  choice = Ready();
  choice.key_file_path.clear();
  for (const auto& row : BuildKeyPackageReviewRows(choice)) {
    EXPECT_FALSE(row.value.contains("protected"));
  }
}

TEST(KeyPackageExportSummaryTest, TheDoneRowsNameBothFiles) {
  const auto rows =
      BuildKeyPackageExportDoneRows("/a/pkg.gfpack", "/b/pkg.key");
  ASSERT_EQ(rows.size(), 2);
  EXPECT_TRUE(rows[0].path);
  EXPECT_TRUE(rows[1].path);
  EXPECT_EQ(rows[0].value, QDir::toNativeSeparators("/a/pkg.gfpack"));
  EXPECT_EQ(rows[1].value, QDir::toNativeSeparators("/b/pkg.key"));
}

TEST(KeyPackageExportSummaryTest, TheMechanismNamesTheRealCipher) {
  // The old success message claimed AES-256-GCM; the files are sealed with
  // XChaCha20-Poly1305 behind Argon2id.
  const auto text = DescribeKeyPackageProtection();
  EXPECT_TRUE(text.contains("XChaCha20-Poly1305"));
  EXPECT_TRUE(text.contains("Argon2id"));
  EXPECT_FALSE(text.contains("AES"));
}

TEST(KeyPackageExportSummaryTest, EveryProblemHasWords) {
  for (const auto problem :
       {KeyPackageExportProblem::kNoKeys, KeyPackageExportProblem::kNoPin,
        KeyPackageExportProblem::kNoKeyFile,
        KeyPackageExportProblem::kInvalidName,
        KeyPackageExportProblem::kNoLocation,
        KeyPackageExportProblem::kLocationNotWritable,
        KeyPackageExportProblem::kSameFile}) {
    EXPECT_FALSE(DescribeKeyPackageExportProblem(problem).isEmpty());
  }
}

}  // namespace GpgFrontend::Test
