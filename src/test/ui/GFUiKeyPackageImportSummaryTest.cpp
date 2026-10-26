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
#include "ui/dialog/import_export/KeyPackageImportSummary.h"

namespace GpgFrontend::Test {

namespace {

using UI::BuildKeyPackageImportKeyRows;
using UI::BuildKeyPackageImportReviewRows;
using UI::CanAdvanceKeyPackageImport;
using UI::DescribeKeyPackageImportError;
using UI::DescribeKeyPackageImportProblem;
using UI::DescribeKeyPackageImportSelection;
using UI::KeyPackageImportChoice;
using UI::KeyPackageImportFile;
using UI::KeyPackageImportKey;
using UI::KeyPackageImportKeyColumns;
using UI::KeyPackageImportKeyFileFor;
using UI::KeyPackageImportProblem;
using UI::KeyPackageImportProblems;
using UI::KeyPackageImportStep;

auto Good(const QString& path, qint64 bytes) -> KeyPackageImportFile {
  return {path, true, true, bytes};
}

/// Both files present and small, the PIN typed.
auto Ready() -> KeyPackageImportChoice {
  KeyPackageImportChoice choice;
  choice.package = Good("/a/pkg.gfpack", 900);
  choice.key_file = Good("/b/pkg.key", 300);
  choice.pin_entered = true;
  choice.keys_selected = 1;
  return choice;
}

auto Found(bool has_secret, bool present, bool present_private)
    -> KeyPackageImportKey {
  KeyPackageImportKey found;
  found.key = {"FPR", "KEYID", "Alice", "alice@example.org", has_secret};
  found.present = present;
  found.present_private = present_private;
  return found;
}

}  // namespace

TEST(KeyPackageImportSummaryTest, TheKeyFileIsLookedForBesideThePackage) {
  EXPECT_EQ(KeyPackageImportKeyFileFor("/a/pkg.gfpack"), "/a/pkg.key");
  EXPECT_EQ(KeyPackageImportKeyFileFor("/a/pkg.GFPACK"), "/a/pkg.key");
  // A package without the usual extension still gets a sibling to look for.
  EXPECT_EQ(KeyPackageImportKeyFileFor("/a/pkg"), "/a/pkg.key");
  EXPECT_TRUE(KeyPackageImportKeyFileFor({}).isEmpty());
}

TEST(KeyPackageImportSummaryTest, TheFilesStepNeedsBothFiles) {
  auto choice = Ready();
  EXPECT_TRUE(CanAdvanceKeyPackageImport(KeyPackageImportStep::kFiles, choice));

  choice.pin_entered = false;
  // The PIN is the last step's business.
  EXPECT_TRUE(CanAdvanceKeyPackageImport(KeyPackageImportStep::kFiles, choice));

  choice.key_file = {};
  EXPECT_EQ(KeyPackageImportProblems(KeyPackageImportStep::kFiles, choice),
            QList{KeyPackageImportProblem::kNoKeyFile});
}

TEST(KeyPackageImportSummaryTest, UnreadableFilesAreNamed) {
  auto choice = Ready();
  choice.package.readable = false;
  EXPECT_EQ(KeyPackageImportProblems(KeyPackageImportStep::kFiles, choice),
            QList{KeyPackageImportProblem::kPackageUnreadable});

  choice = Ready();
  choice.key_file.is_file = false;
  EXPECT_EQ(KeyPackageImportProblems(KeyPackageImportStep::kFiles, choice),
            QList{KeyPackageImportProblem::kKeyFileUnreadable});
}

TEST(KeyPackageImportSummaryTest, TheSizeLimitsAreTheOldPromptsLimits) {
  auto choice = Ready();
  choice.package.bytes = UI::kMaxKeyPackageBytes;
  choice.key_file.bytes = UI::kMaxKeyPackageKeyFileBytes;
  EXPECT_TRUE(CanAdvanceKeyPackageImport(KeyPackageImportStep::kFiles, choice));

  choice.package.bytes = UI::kMaxKeyPackageBytes + 1;
  choice.key_file.bytes = UI::kMaxKeyPackageKeyFileBytes + 1;
  const QList expected = {KeyPackageImportProblem::kPackageTooLarge,
                          KeyPackageImportProblem::kKeyFileTooLarge};
  EXPECT_EQ(KeyPackageImportProblems(KeyPackageImportStep::kFiles, choice),
            expected);

  EXPECT_EQ(UI::kMaxKeyPackageBytes, 32LL * 1024 * 1024);
  EXPECT_EQ(UI::kMaxKeyPackageKeyFileBytes, 1024LL * 1024);
}

TEST(KeyPackageImportSummaryTest, OneFileCannotBeBothHalves) {
  auto choice = Ready();
  choice.key_file = choice.package;
  EXPECT_TRUE(KeyPackageImportProblems(KeyPackageImportStep::kFiles, choice)
                  .contains(KeyPackageImportProblem::kSameFile));
}

TEST(KeyPackageImportSummaryTest, TheUnlockStepNeedsThePinAndTheFiles) {
  auto choice = Ready();
  EXPECT_TRUE(
      CanAdvanceKeyPackageImport(KeyPackageImportStep::kUnlock, choice));

  choice.pin_entered = false;
  EXPECT_EQ(KeyPackageImportProblems(KeyPackageImportStep::kUnlock, choice),
            QList{KeyPackageImportProblem::kNoPin});

  choice = Ready();
  choice.package = {};
  EXPECT_FALSE(
      CanAdvanceKeyPackageImport(KeyPackageImportStep::kUnlock, choice));
}

TEST(KeyPackageImportSummaryTest, ProblemsComeInTheOrderTheStepsAsk) {
  const QList expected = {KeyPackageImportProblem::kNoPackage,
                          KeyPackageImportProblem::kNoKeyFile,
                          KeyPackageImportProblem::kNoPin};
  EXPECT_EQ(KeyPackageImportProblems(KeyPackageImportStep::kUnlock, {}),
            expected);
}

TEST(KeyPackageImportSummaryTest, AWrongPinIsAnsweredWithAnotherTry) {
  // The operator's message for a key file that does not open with the PIN.
  const auto failure =
      DescribeKeyPackageImportError("Decrypt the Key of Key Package Failed");
  EXPECT_TRUE(failure.retry_pin);
  EXPECT_TRUE(failure.message.contains("PIN"));
}

TEST(KeyPackageImportSummaryTest, OtherFailuresAreNotBlamedOnThePin) {
  for (const auto* error :
       {"Decrypt the Key Package Failed", "Read Key Package Failed",
        "Read the Key of Key Package Failed",
        "Import Key Data in the Key Package Failed", "something new"}) {
    const auto failure = DescribeKeyPackageImportError(error);
    EXPECT_FALSE(failure.retry_pin) << error;
    EXPECT_FALSE(failure.message.isEmpty()) << error;
  }
}

TEST(KeyPackageImportSummaryTest, TheReviewNamesBothFiles) {
  const auto rows = BuildKeyPackageImportReviewRows(Ready());
  ASSERT_EQ(rows.size(), 2);
  EXPECT_EQ(rows[0].value, QDir::toNativeSeparators("/a/pkg.gfpack"));
  EXPECT_EQ(rows[1].value, QDir::toNativeSeparators("/b/pkg.key"));
  EXPECT_TRUE(rows[0].path);

  const auto empty = BuildKeyPackageImportReviewRows({});
  EXPECT_TRUE(empty[0].dimmed);
  EXPECT_FALSE(empty[0].path);
}

TEST(KeyPackageImportSummaryTest, TheKeysStepNeedsATickedKey) {
  auto choice = Ready();
  EXPECT_TRUE(CanAdvanceKeyPackageImport(KeyPackageImportStep::kKeys, choice));

  choice.keys_selected = 0;
  EXPECT_EQ(KeyPackageImportProblems(KeyPackageImportStep::kKeys, choice),
            QList{KeyPackageImportProblem::kNoKeysSelected});
  // Ticking is the last step's business.
  EXPECT_TRUE(
      CanAdvanceKeyPackageImport(KeyPackageImportStep::kUnlock, choice));
}

TEST(KeyPackageImportSummaryTest, EachKeySaysWhatImportingItBrings) {
  const auto rows = BuildKeyPackageImportKeyRows({
      Found(true, false, false),   // new, private key included
      Found(false, false, false),  // new, public only
      Found(true, true, false),    // brings the private key
      Found(true, true, true),     // already here in full
      Found(false, true, true),    // public only, already here
  });
  ASSERT_EQ(rows.size(), 5);

  EXPECT_EQ(rows[0].status, "New, with private key");
  EXPECT_EQ(rows[1].status, "New");
  EXPECT_EQ(rows[2].status, "Adds the private key");
  EXPECT_EQ(rows[3].status, "Already here");
  EXPECT_EQ(rows[4].status, "Already here");

  EXPECT_FALSE(rows[0].already_present);
  EXPECT_FALSE(rows[2].already_present);
  EXPECT_TRUE(rows[3].already_present);
  EXPECT_TRUE(rows[4].already_present);

  EXPECT_EQ(rows[0].name, "Alice");
  EXPECT_EQ(rows[0].email, "alice@example.org");
  EXPECT_EQ(rows[0].id, "KEYID");
}

TEST(KeyPackageImportSummaryTest, TheTableHasOneHeadingPerColumn) {
  EXPECT_EQ(KeyPackageImportKeyColumns().size(), 4);
}

TEST(KeyPackageImportSummaryTest, TheSelectionLineCountsTicks) {
  EXPECT_EQ(DescribeKeyPackageImportSelection(2, 3), "2 of 3 keys ticked.");
  EXPECT_FALSE(DescribeKeyPackageImportSelection(1, 1).contains("(s)"));
  EXPECT_NE(DescribeKeyPackageImportSelection(1, 1),
            DescribeKeyPackageImportSelection(0, 1));
}

TEST(KeyPackageImportSummaryTest, AnEmptySelectionInThePackageIsNamed) {
  const auto failure =
      DescribeKeyPackageImportError("No Selected Key Found in the Key Package");
  EXPECT_FALSE(failure.retry_pin);
  EXPECT_NE(failure.message,
            DescribeKeyPackageImportError("something new").message);
}

TEST(KeyPackageImportSummaryTest, EveryProblemHasWords) {
  for (const auto problem :
       {KeyPackageImportProblem::kNoPackage,
        KeyPackageImportProblem::kPackageUnreadable,
        KeyPackageImportProblem::kPackageTooLarge,
        KeyPackageImportProblem::kNoKeyFile,
        KeyPackageImportProblem::kKeyFileUnreadable,
        KeyPackageImportProblem::kKeyFileTooLarge,
        KeyPackageImportProblem::kSameFile, KeyPackageImportProblem::kNoPin,
        KeyPackageImportProblem::kNoKeysSelected}) {
    EXPECT_FALSE(DescribeKeyPackageImportProblem(problem).isEmpty());
  }
}

}  // namespace GpgFrontend::Test
