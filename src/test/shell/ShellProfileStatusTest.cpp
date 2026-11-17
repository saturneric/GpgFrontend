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

#include "shell/present/ProfileStatus.h"
#include "shell/present/SecurityDisplayNames.h"

namespace GpgFrontend::Test {

namespace {

/// The character the whole restructure exists to remove from these readings.
constexpr auto kEmDash = QChar(0x2014);

auto AllReadings() -> QList<AboutStatusValue> {
  return {
      DescribeSessionStorage(true, false),
      DescribeSessionStorage(false, true),
      DescribeSessionStorage(false, false),
      DescribeAppKeyProtection(AppKeyProtection::kNONE, true),
      DescribeAppKeyProtection(AppKeyProtection::kPIN, false),
      DescribeAppKeyProtection(AppKeyProtection::kKEYCHAIN, true),
  };
}

/// A healthy installed profile on a desktop with both engines.
auto DesktopFacts() -> ProfileStatusFacts {
  ProfileStatusFacts facts;
  facts.secure_level = 2;
  facts.kind = ProfileKind::kINSTALLED_ROOT;
  facts.display_name = QStringLiteral("Default");
  facts.profile_id = QStringLiteral("1b4e28ba-2fa1-11d2-883f-0016d3cca427");
  facts.root = QStringLiteral("/home/user/.local/share/GpgFrontend");
  facts.protection = AppKeyProtection::kKEYCHAIN;
  facts.secret_store_available = true;
  facts.secret_store_name = QStringLiteral("Secret Service");
  facts.engines = {QStringLiteral("GnuPG"), QStringLiteral("rPGP")};
  facts.gnupg_supported = true;
  return facts;
}

/// The same profile on a phone: rPGP only, no running mode, no workspace.
auto PhoneFacts() -> ProfileStatusFacts {
  auto facts = DesktopFacts();
  facts.show_running_mode = false;
  facts.secret_store_name = QStringLiteral("Android Keystore");
  facts.engines = {QStringLiteral("rPGP")};
  facts.gnupg_supported = false;
  return facts;
}

auto Titles(const QList<StatusSection>& sections) -> QStringList {
  QStringList titles;
  for (const auto& section : sections) titles << section.title;
  return titles;
}

auto Captions(const QList<StatusRow>& rows) -> QStringList {
  QStringList captions;
  for (const auto& row : rows) captions << row.caption;
  return captions;
}

auto Find(const QList<StatusSection>& sections, const QString& caption)
    -> std::optional<StatusRow> {
  for (const auto& section : sections) {
    for (const auto& row : section.rows) {
      if (row.caption == caption) return row;
    }
  }
  return std::nullopt;
}

}  // namespace

TEST(ShellProfileStatusTest, AHealthyProfileHasNothingNeedingAttention) {
  const auto report = BuildProfileStatus(DesktopFacts());
  EXPECT_TRUE(report.attention.isEmpty());

  // Glance first; every details section after it, each its own.
  const auto sections = ProfileStatusSections(report);
  EXPECT_EQ(Titles(sections),
            QStringList({"At a Glance", "Application", "Profile", "Folders",
                         "OpenPGP Engines"}));
}

TEST(ShellProfileStatusTest, TheGlanceTellsProfileProtectionAndLevel) {
  const auto report = BuildProfileStatus(DesktopFacts());
  EXPECT_EQ(Captions(report.glance),
            QStringList(
                {"Profile:", "Application Key Protection:", "Secure Level:"}));
  EXPECT_EQ(report.glance.last().value, SecureLevelDisplayName(2));
}

TEST(ShellProfileStatusTest, AMissingCredentialStoreComesFirst) {
  auto facts = DesktopFacts();
  facts.secret_store_available = false;
  facts.secret_store_reason = QStringLiteral("libsecret-1.so.0: not found");

  const auto sections = ProfileStatusSections(BuildProfileStatus(facts));
  ASSERT_FALSE(sections.isEmpty());
  EXPECT_EQ(sections.first().title, "Needs Attention");
  ASSERT_EQ(sections.first().rows.size(), 1);
  const auto& row = sections.first().rows.first();
  EXPECT_EQ(row.caption, "System Credential Store:");
  EXPECT_TRUE(row.degraded);
  // The loader's reason, as produced, for a bug report.
  EXPECT_EQ(row.detail, facts.secret_store_reason);

  // Said once: not again among the application's rows.
  const auto application = sections.at(2);
  EXPECT_EQ(application.title, "Application");
  EXPECT_FALSE(Captions(application.rows).contains("System Credential Store:"));
}

TEST(ShellProfileStatusTest, AnAvailableStoreIsNamed) {
  const auto row = Find(ProfileStatusSections(BuildProfileStatus(PhoneFacts())),
                        "System Credential Store:");
  ASSERT_TRUE(row.has_value());
  EXPECT_EQ(row->value, "Android Keystore");
  EXPECT_FALSE(row->degraded);
}

TEST(ShellProfileStatusTest, AFallbackStorageNeedsAttention) {
  auto facts = DesktopFacts();
  facts.kind = ProfileKind::kEPHEMERAL;
  facts.transient = true;

  const auto report = BuildProfileStatus(facts);
  ASSERT_EQ(report.attention.size(), 1);
  EXPECT_EQ(report.attention.first().caption, "Session Storage:");
  EXPECT_TRUE(report.attention.first().degraded);

  // Memory is no fallback, so it stays with the profile.
  facts.storage_volatile = true;
  EXPECT_TRUE(BuildProfileStatus(facts).attention.isEmpty());
}

TEST(ShellProfileStatusTest, APhoneHasOnlyItsEngineAndNoRunningMode) {
  const auto sections = ProfileStatusSections(BuildProfileStatus(PhoneFacts()));

  EXPECT_FALSE(Find(sections, "Running Mode:").has_value());
  EXPECT_FALSE(Find(sections, "GnuPG Offline Mode:").has_value());
  EXPECT_FALSE(Find(sections, "Pinentry Program Path:").has_value());

  const auto engines = sections.last();
  EXPECT_EQ(engines.title, "OpenPGP Engines");
  EXPECT_EQ(Captions(engines.rows), QStringList({"rPGP"}));
}

TEST(ShellProfileStatusTest, ADesktopKeepsItsGnuPGSettings) {
  const auto sections =
      ProfileStatusSections(BuildProfileStatus(DesktopFacts()));

  const auto mode = Find(sections, "Running Mode:");
  ASSERT_TRUE(mode.has_value());
  EXPECT_EQ(mode->value, "Installed Mode");

  const auto pinentry = Find(sections, "Pinentry Program Path:");
  ASSERT_TRUE(pinentry.has_value());
  EXPECT_TRUE(pinentry->dimmed);
  EXPECT_FALSE(pinentry->path);
}

TEST(ShellProfileStatusTest, OptionalProfileRowsAppearOnlyWhenTheyApply) {
  auto facts = DesktopFacts();
  auto sections = ProfileStatusSections(BuildProfileStatus(facts));
  EXPECT_FALSE(Find(sections, "Profile Key Rotation:").has_value());
  EXPECT_FALSE(Find(sections, "Profile Format Version:").has_value());
  EXPECT_FALSE(Find(sections, "Profiles Folder:").has_value());

  facts.rotating_keys = true;
  facts.schema_version = 3;
  facts.profiles_root = QStringLiteral("/home/user/.local/share/profiles");
  sections = ProfileStatusSections(BuildProfileStatus(facts));
  EXPECT_TRUE(Find(sections, "Profile Key Rotation:").has_value());
  EXPECT_EQ(Find(sections, "Profile Format Version:")->value, "3");
  EXPECT_TRUE(Find(sections, "Profiles Folder:")->path);
}

TEST(ShellProfileStatusTest, TheTextCopiesEverySectionAndDetail) {
  auto facts = DesktopFacts();
  facts.secret_store_available = false;
  facts.secret_store_reason = QStringLiteral("no D-Bus session");

  const auto text =
      ProfileStatusText(ProfileStatusSections(BuildProfileStatus(facts)));
  EXPECT_TRUE(
      text.startsWith("[Needs Attention]\nSystem Credential Store: "
                      "Unavailable\n    no D-Bus session\n"))
      << text.toStdString();
  EXPECT_TRUE(text.contains("\n\n[OpenPGP Engines]\nGnuPG Available"));
}

TEST(ShellProfileStatusTest, NoStatusRowCarriesAnEmDash) {
  auto facts = DesktopFacts();
  facts.kind = ProfileKind::kPACKAGED;
  facts.transient = true;
  facts.secure_area_resident = true;
  facts.rotating_keys = true;
  facts.allows_keychain = false;
  facts.secret_store_available = false;

  for (const auto& section : ProfileStatusSections(BuildProfileStatus(facts))) {
    for (const auto& row : section.rows) {
      EXPECT_FALSE(row.value.contains(kEmDash)) << row.value.toStdString();
      EXPECT_FALSE(row.detail.contains(kEmDash)) << row.detail.toStdString();
    }
  }
}

TEST(ShellProfileStatusTest, EachStorageOutcomeReadsDifferently) {
  const auto memory = DescribeSessionStorage(true, false);
  const auto encrypted = DescribeSessionStorage(false, true);
  const auto plain = DescribeSessionStorage(false, false);

  EXPECT_NE(memory.value, encrypted.value);
  EXPECT_NE(encrypted.value, plain.value);
  EXPECT_NE(memory.value, plain.value);

  // Every storage outcome has something to add, so none of them is a bare
  // value: that is what the second line is for.
  EXPECT_FALSE(memory.detail.isEmpty());
  EXPECT_FALSE(encrypted.detail.isEmpty());
  EXPECT_FALSE(plain.detail.isEmpty());
}

TEST(ShellProfileStatusTest, VolatileWinsOverEncryptedAtRest) {
  // The two axes are independent, and an accessor can report both. Memory is
  // the stronger statement, so it is the one shown.
  const auto both = DescribeSessionStorage(true, true);

  EXPECT_EQ(both.value, DescribeSessionStorage(true, false).value);
  EXPECT_FALSE(both.degraded);
}

TEST(ShellProfileStatusTest, OnlyTheOrdinaryFolderCountsAsAFallback) {
  // A silent downgrade is the one thing this mechanism must not be, so the
  // outcome nobody asked for is the only one marked.
  EXPECT_FALSE(DescribeSessionStorage(true, false).degraded);
  EXPECT_FALSE(DescribeSessionStorage(false, true).degraded);
  EXPECT_TRUE(DescribeSessionStorage(false, false).degraded);
}

TEST(ShellProfileStatusTest, ARootProfileDoesNotPrintItsNameTwice) {
  // The defect: CurrentProfileDisplayName() answers with the kind for a profile
  // that has no name of its own, and the type row under it then said the same
  // word again.
  for (const auto kind :
       {ProfileKind::kINSTALLED_ROOT, ProfileKind::kPORTABLE_ROOT}) {
    const auto rows = BuildProfileIdentityRows(kind, "Default", false);

    ASSERT_EQ(rows.size(), 1) << ProfileKindToString(kind).toStdString();
    EXPECT_EQ(rows.first().value, "Default");
    // Nothing is lost by the merge: what the second row would have said is now
    // the sentence under the first.
    EXPECT_FALSE(rows.first().detail.isEmpty());
  }
}

TEST(ShellProfileStatusTest, ANamedProfileKeepsItsTypeRow) {
  // Where the two rows say different things, they both earn their place.
  for (const auto kind : {ProfileKind::kPERSIST, ProfileKind::kPACKAGED}) {
    const auto rows = BuildProfileIdentityRows(kind, "Work", false);

    ASSERT_EQ(rows.size(), 2) << ProfileKindToString(kind).toStdString();
    EXPECT_EQ(rows.first().value, "Work");
    EXPECT_NE(rows.at(1).value, rows.first().value);
    EXPECT_FALSE(rows.at(1).value.isEmpty());
  }
}

TEST(ShellProfileStatusTest, OnlyATransientSessionIsCalledTemporary) {
  // Asked of the profile rather than of the kind, so a shape added later cannot
  // inherit the sentence just by being packaged.
  const auto transient =
      BuildProfileIdentityRows(ProfileKind::kPACKAGED, "Work", true);
  const auto kept =
      BuildProfileIdentityRows(ProfileKind::kPACKAGED, "Work", false);

  ASSERT_EQ(transient.size(), 2);
  ASSERT_EQ(kept.size(), 2);
  EXPECT_FALSE(transient.at(1).detail.isEmpty());
  EXPECT_TRUE(kept.at(1).detail.isEmpty());
}

TEST(ShellProfileStatusTest, ATemporaryProfileSaysItWillBeDestroyed) {
  const auto rows =
      BuildProfileIdentityRows(ProfileKind::kEPHEMERAL, "Temporary", true);

  ASSERT_EQ(rows.size(), 2);
  EXPECT_FALSE(rows.at(1).value.isEmpty());
  EXPECT_FALSE(rows.at(1).detail.isEmpty());
}

TEST(ShellProfileStatusTest, AForcedProtectionSaysWhyItWasForced) {
  // The settings page greys the keychain out for anything that travels and does
  // not explain itself; this row is where that explanation lives.
  const auto forced = DescribeAppKeyProtection(AppKeyProtection::kNONE, false);
  const auto chosen = DescribeAppKeyProtection(AppKeyProtection::kNONE, true);

  EXPECT_EQ(forced.value, chosen.value);
  EXPECT_FALSE(forced.detail.isEmpty());
  EXPECT_TRUE(chosen.detail.isEmpty());

  // A rule, not a fallback: nothing here went wrong.
  EXPECT_FALSE(forced.degraded);
}

TEST(ShellProfileStatusTest, NoReadingCarriesAnEmDash) {
  // The regression this whole structure replaces: a value, an em dash, and an
  // explanatory clause written as one string, which turned the value column
  // into a paragraph.
  for (const auto& reading : AllReadings()) {
    EXPECT_FALSE(reading.value.contains(kEmDash))
        << reading.value.toStdString();
    EXPECT_FALSE(reading.detail.contains(kEmDash))
        << reading.detail.toStdString();
  }
}

TEST(ShellProfileStatusTest, EveryValueStillReadsAsAValue) {
  for (const auto& reading : AllReadings()) {
    EXPECT_FALSE(reading.value.isEmpty());

    // A value is a phrase, not a sentence: no terminating period, and short
    // enough to scan in a form column.
    EXPECT_FALSE(reading.value.trimmed().endsWith(QLatin1Char('.')))
        << reading.value.toStdString();
    EXPECT_EQ(reading.value.trimmed(), reading.value);

    // The sentence, where there is one, is where the length went.
    if (!reading.detail.isEmpty()) {
      EXPECT_LT(reading.value.length(), reading.detail.length())
          << reading.value.toStdString();
      EXPECT_TRUE(reading.detail.trimmed().endsWith(QLatin1Char('.')))
          << reading.detail.toStdString();
    }
  }
}

TEST(ShellProfileStatusTest, ADetailWithoutADegradedStateIsAHover) {
  // Elaboration does not have to cost the page a line. Three sentences written
  // under their values is what pushed the Profile card off the bottom of the
  // dialog; behind a tooltip they are still there for anyone who looks.
  const auto detail = DescribeSessionStorage(true, false).detail;

  ASSERT_FALSE(detail.isEmpty());
  EXPECT_FALSE(ShowsDetailInline(detail, false));
}

TEST(ShellProfileStatusTest, ADegradedStateKeepsItsReasonOnScreen) {
  // The point of marking a state degraded is that nobody asked for it and
  // nobody would otherwise notice, so its reason is the one sentence that
  // cannot be left to a hover.
  const auto detail = DescribeSessionStorage(false, false).detail;

  ASSERT_FALSE(detail.isEmpty());
  EXPECT_TRUE(ShowsDetailInline(detail, true));
}

TEST(ShellProfileStatusTest, NothingIsShownInlineWithoutADetail) {
  EXPECT_FALSE(ShowsDetailInline({}, false));
  EXPECT_FALSE(ShowsDetailInline({}, true));
}

TEST(ShellProfileStatusTest, OnlyTheFallbackStorageOutcomeStaysOnScreen) {
  // The regression guard: this fails the moment a reading is marked degraded
  // without anyone deciding whether its sentence should be visible.
  auto inline_count = 0;
  for (const auto& reading : AllReadings()) {
    if (ShowsDetailInline(reading.detail, reading.degraded)) inline_count++;
  }

  EXPECT_EQ(inline_count, 1);

  const auto plain = DescribeSessionStorage(false, false);
  EXPECT_TRUE(ShowsDetailInline(plain.detail, plain.degraded));
}

}  // namespace GpgFrontend::Test
