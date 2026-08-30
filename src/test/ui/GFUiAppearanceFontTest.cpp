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

#include <QFontDatabase>

#include "GpgFrontendTest.h"
#include "ui/function/AppearanceFont.h"

namespace GpgFrontend::Test {

namespace {

/// Matched as a substring rather than in full, so switching between the
/// ligature and no-ligature cuts (they differ by a trailing " NL") does not
/// break every assertion here.
constexpr auto kBundledFamilyMarker = "JetBrains Mono";

auto InstalledFamilies() -> QStringList {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return QFontDatabase::families();
#else
  QFontDatabase font_database;
  return font_database.families();
#endif
}

auto StylesOf(const QString& family) -> QStringList {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return QFontDatabase::styles(family);
#else
  QFontDatabase font_database;
  return font_database.styles(family);
#endif
}

auto ContainsFamily(const QStringList& families, const QString& family)
    -> bool {
  return std::any_of(families.cbegin(), families.cend(),
                     [&family](const QString& item) {
                       return item.compare(family, Qt::CaseInsensitive) == 0;
                     });
}

}  // namespace

// These only ever query the font database, never register anything: test
// bodies run on a worker thread, and registration has already happened in the
// GpgFrontendApplication constructor long before any of them starts.

TEST(AppearanceFontTest, TheBundledMonospaceFamilyIsRegisteredAtStartup) {
  const auto family = UI::BundledMonospaceFamily();

  // Fails first, and loudest, if Q_INIT_RESOURCE(fonts) went missing or a qrc
  // alias was mistyped: everything else in this suite depends on it.
  ASSERT_FALSE(family.isEmpty());
  EXPECT_TRUE(family.contains(QLatin1String(kBundledFamilyMarker)));
  EXPECT_TRUE(ContainsFamily(InstalledFamilies(), family));
}

TEST(AppearanceFontTest, TheBundledFamilyIsFixedPitch) {
  const auto family = UI::BundledMonospaceFamily();
  ASSERT_FALSE(family.isEmpty());

  // Both the aligned hash columns and the "monospaced fonts only" filter of
  // the settings combo boxes are decided by this one predicate.
  EXPECT_TRUE(UI::IsFixedPitchFontFamily(family));
}

TEST(AppearanceFontTest, BothWeightsOfTheBundledFamilyAreAvailable) {
  const auto family = UI::BundledMonospaceFamily();
  ASSERT_FALSE(family.isEmpty());

  // Regular and Bold are shipped as separate files. With only one of them Qt
  // silently synthesises the other, which is why this counts rather than
  // trusting the family to be complete.
  EXPECT_GE(StylesOf(family).size(), 2);
}

TEST(AppearanceFontTest, ADefaultMonospaceFontUsesTheBundledFamily) {
  const auto family = UI::BundledMonospaceFamily();
  ASSERT_FALSE(family.isEmpty());

  const auto font = UI::DefaultMonospaceFont();
  EXPECT_EQ(font.family(), family);
  EXPECT_TRUE(font.fixedPitch());

  // -1 means "leave the size alone" for the callers that inherit a size from
  // the widget around them.
  EXPECT_EQ(UI::DefaultMonospaceFont(13).pointSize(), 13);
}

TEST(AppearanceFontTest, ASizelessDefaultMonospaceFontKeepsTheSystemFixedSize) {
  // DefaultMonospaceFont() replaced systemFont(FixedFont) at call sites that
  // then clamp the size with std::max(). Only the family may change: a bare
  // QFont(family) would take the *application* default size, which is larger
  // than the system fixed font's, and the clamp would preserve the larger
  // value -- visibly inflating the editor status bar.
  const auto system_fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  EXPECT_EQ(UI::DefaultMonospaceFont().pointSize(), system_fixed.pointSize());
}

TEST(AppearanceFontTest, ApplyingTheMonospaceFamilyKeepsSizeAndWeight) {
  const auto family = UI::BundledMonospaceFamily();
  ASSERT_FALSE(family.isEmpty());

  QFont font;
  font.setPointSize(17);
  font.setBold(true);
  UI::ApplyMonospaceFamily(font);

  EXPECT_EQ(font.family(), family);
  EXPECT_EQ(font.pointSize(), 17);
  EXPECT_TRUE(font.bold());
  EXPECT_TRUE(font.fixedPitch());
}

TEST(AppearanceFontTest, AnEmptyStoredFamilyResolvesToTheBundledFont) {
  const auto family = UI::BundledMonospaceFamily();
  ASSERT_FALSE(family.isEmpty());

  // The core of the change: an unset appearance setting no longer lands on
  // whatever each platform happens to call its fixed font.
  const auto font = UI::ResolveAppearanceFont({}, 12);
  EXPECT_EQ(font.family(), family);
  EXPECT_EQ(font.pointSize(), 12);
  EXPECT_TRUE(font.fixedPitch());
}

TEST(AppearanceFontTest, AnUninstalledStoredFamilyFallsBackToTheBundledFont) {
  const auto family = UI::BundledMonospaceFamily();
  ASSERT_FALSE(family.isEmpty());

  // A family that was uninstalled after it was chosen must not be handed to
  // Qt, which would substitute an arbitrary -- often proportional -- font.
  const auto missing =
      QStringLiteral("NoSuchFamily-") + GenerateRandomString(12);
  EXPECT_EQ(UI::ResolveAppearanceFont(missing, 11).family(), family);
}

TEST(AppearanceFontTest, AnInstalledStoredFamilyWinsOverTheBundledOne) {
  const auto bundled = UI::BundledMonospaceFamily();
  ASSERT_FALSE(bundled.isEmpty());

  const auto families = InstalledFamilies();
  const auto other = std::find_if(
      families.cbegin(), families.cend(), [&bundled](const QString& item) {
        return item.compare(bundled, Qt::CaseInsensitive) != 0;
      });
  if (other == families.cend()) {
    GTEST_SKIP() << "no second font family installed on this system";
  }

  // Bundling a default must not take the choice away from the user.
  EXPECT_EQ(UI::ResolveAppearanceFont(*other, 11).family(), *other);
}

TEST(AppearanceFontTest, TheReportScaleStepsFromItsBaseAndFloors) {
  EXPECT_EQ(UI::ReportFontPixelSize(0), UI::kReportBaseFontPx);
  EXPECT_EQ(UI::ReportFontPixelSize(-1), UI::kReportBaseFontPx - 1);
  EXPECT_EQ(UI::ReportFontPixelSize(3), UI::kReportBaseFontPx + 3);

  // The floor is what keeps a deep step from disappearing. It used to be
  // spelled inline at every call site, with two different values (7 and 8).
  EXPECT_EQ(UI::ReportFontPixelSize(-100), UI::kReportMinFontPx);
}

TEST(AppearanceFontTest, TheReportFontIsSizedInPixelsNotPoints) {
  const auto family = UI::BundledMonospaceFamily();
  ASSERT_FALSE(family.isEmpty());

  const auto font = UI::ReportFont();
  EXPECT_EQ(font.family(), family);
  EXPECT_TRUE(font.fixedPitch());
  EXPECT_EQ(font.pixelSize(), UI::kReportBaseFontPx);

  // The regression guard. Points are not comparable across platforms here: Qt
  // reports 72 logical DPI on macOS and 96 everywhere else, which is how the
  // report document ended up visibly larger on macOS. A pointSize() other than
  // -1 means a point-size path survived somewhere in the scale.
  EXPECT_EQ(font.pointSize(), -1);
}

TEST(AppearanceFontTest, TheReportFontCarriesItsStepAndWeight) {
  ASSERT_FALSE(UI::BundledMonospaceFamily().isEmpty());

  const auto key_font = UI::ReportFont(-1, true);
  EXPECT_EQ(key_font.pixelSize(), UI::kReportBaseFontPx - 1);
  EXPECT_TRUE(key_font.bold());

  EXPECT_FALSE(UI::ReportFont(-1).bold());
}

TEST(AppearanceFontTest, TheBundledFontRendersEveryCharacterAtTheSameWidth) {
  const auto family = UI::BundledMonospaceFamily();
  ASSERT_FALSE(family.isEmpty());

  // The one that reproduces the original bug. isFixedPitch() only reports what
  // the family claims; this measures what the font engine actually does, so it
  // fails if the family resolves to a substitute -- exactly what "Monospace"
  // used to do off Linux, leaving the hash columns ragged.
  const QFontMetrics metrics(UI::DefaultMonospaceFont(12));
  const auto narrow = metrics.horizontalAdvance(QChar('i'));

  EXPECT_GT(narrow, 0);
  for (const auto c : {QChar('W'), QChar('0'), QChar('.'), QChar('F')}) {
    EXPECT_EQ(metrics.horizontalAdvance(c), narrow)
        << "character " << c.toLatin1() << " is not the same width as 'i'";
  }
}

TEST(AppearanceFontTest, RegisteringTheBundledFontsIsIdempotent) {
  const auto before = InstalledFamilies().size();
  const auto family = UI::BundledMonospaceFamily();

  UI::RegisterBundledFonts();

  // A second addApplicationFont() on the same file returns a fresh id and adds
  // the family a second time, which would then show up twice in every font
  // combo box. The once-flag is what prevents that.
  EXPECT_EQ(InstalledFamilies().size(), before);
  EXPECT_EQ(UI::BundledMonospaceFamily(), family);
}

}  // namespace GpgFrontend::Test
