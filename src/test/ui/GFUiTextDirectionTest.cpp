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

#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextOption>

#include "GpgFrontendTest.h"
#include "ui/function/TextDirection.h"
#include "ui/struct/settings_object/AppearanceSO.h"

namespace GpgFrontend::Test {

TEST(TextDirectionTest, TextWithoutAnyStrongCharacterIsLeftToRight) {
  EXPECT_EQ(UI::DetectTextDirection({}), Qt::LeftToRight);
  EXPECT_EQ(UI::DetectTextDirection(QStringLiteral("\n\n")), Qt::LeftToRight);
  EXPECT_EQ(UI::DetectTextDirection(QStringLiteral("\t\n42.")),
            Qt::LeftToRight);
}

TEST(TextDirectionTest, LatinTextIsLeftToRight) {
  EXPECT_EQ(UI::DetectTextDirection(QStringLiteral("hello")), Qt::LeftToRight);
}

TEST(TextDirectionTest, RightToLeftScriptsAreDetected) {
  // Arabic and Persian are DirAL, Hebrew is DirR: both classes have to count,
  // and only Arabic would be covered by testing one of them.
  EXPECT_EQ(UI::DetectTextDirection(QString::fromUtf8("مرحبا")),
            Qt::RightToLeft);
  EXPECT_EQ(UI::DetectTextDirection(QString::fromUtf8("שלום")),
            Qt::RightToLeft);
  EXPECT_EQ(UI::DetectTextDirection(QString::fromUtf8("سلام")),
            Qt::RightToLeft);
}

TEST(TextDirectionTest, LeadingWeakAndNeutralCharactersAreSkipped) {
  // Digits and punctuation carry no direction of their own, so a message that
  // opens with a quote or a date must not be read as left-to-right.
  EXPECT_EQ(UI::DetectTextDirection(QString::fromUtf8("  123 — \"مرحبا\"")),
            Qt::RightToLeft);
  EXPECT_EQ(UI::DetectTextDirection(QString::fromUtf8("()[]{}<>")),
            Qt::LeftToRight);
}

TEST(TextDirectionTest, TheFirstStrongCharacterDecides) {
  EXPECT_EQ(UI::DetectTextDirection(QString::fromUtf8("Re: مرحبا")),
            Qt::LeftToRight);
  EXPECT_EQ(UI::DetectTextDirection(QString::fromUtf8("مرحبا Re:")),
            Qt::RightToLeft);
}

TEST(TextDirectionTest, ArmoredTextIsLeftToRight) {
  // Deliberate: an armored block is machine-readable ASCII and reads from the
  // left however the message inside it was written.
  EXPECT_EQ(UI::DetectTextDirection(
                QString::fromUtf8("-----BEGIN PGP MESSAGE-----\n\nمرحبا")),
            Qt::LeftToRight);
}

TEST(TextDirectionTest, RightToLeftAboveTheBasicPlaneIsDetected) {
  // U+10900 PHOENICIAN LETTER ALF, a surrogate pair. Classifying its halves
  // separately would report left-to-right.
  const std::array<char32_t, 1> code_points{0x10900};
  const auto text = QString::fromUcs4(code_points.data(), code_points.size());

  ASSERT_EQ(text.size(), 2);
  EXPECT_EQ(UI::DetectTextDirection(text), Qt::RightToLeft);
}

TEST(TextDirectionTest, ALoneSurrogateDoesNotDecide) {
  // A half pair left over from a truncated read is not a strong character.
  QString text;
  text.append(QChar(static_cast<char16_t>(0xD802)));
  text.append(QStringLiteral("a"));

  EXPECT_EQ(UI::DetectTextDirection(text), Qt::LeftToRight);
}

TEST(TextDirectionTest, ExplicitModesIgnoreTheContent) {
  EXPECT_EQ(UI::ResolveTextDirection(UI::kTEXT_DIRECTION_LTR,
                                     QString::fromUtf8("مرحبا")),
            Qt::LeftToRight);
  EXPECT_EQ(UI::ResolveTextDirection(UI::kTEXT_DIRECTION_RTL,
                                     QStringLiteral("hello")),
            Qt::RightToLeft);
}

TEST(TextDirectionTest, AutomaticModeFollowsTheContent) {
  for (const auto& text : {QStringLiteral("hello"), QString::fromUtf8("مرحبا"),
                           QString(), QString::fromUtf8("123 שלום")}) {
    EXPECT_EQ(UI::ResolveTextDirection(UI::kTEXT_DIRECTION_AUTO, text),
              UI::DetectTextDirection(text))
        << text.toStdString();
  }
}

TEST(TextDirectionTest, UnrecognisedStoredValuesReadAsAutomatic) {
  // The settings file is user editable, so an out-of-range value has to land
  // somewhere defined rather than be cast into the enum.
  EXPECT_EQ(UI::TextDirectionModeFromInt(-1), UI::kTEXT_DIRECTION_AUTO);
  EXPECT_EQ(UI::TextDirectionModeFromInt(3), UI::kTEXT_DIRECTION_AUTO);
  EXPECT_EQ(UI::TextDirectionModeFromInt(99), UI::kTEXT_DIRECTION_AUTO);

  EXPECT_EQ(UI::TextDirectionModeFromInt(0), UI::kTEXT_DIRECTION_AUTO);
  EXPECT_EQ(UI::TextDirectionModeFromInt(1), UI::kTEXT_DIRECTION_LTR);
  EXPECT_EQ(UI::TextDirectionModeFromInt(2), UI::kTEXT_DIRECTION_RTL);
}

TEST(TextDirectionTest, APlainTextLayoutIsGivenAnExplicitAlignment) {
  // QPlainTextDocumentLayout reorders the characters for the base direction but
  // leaves every line at the left margin, so the editor only moves when the
  // alignment is spelled out. This failed silently once already.
  QTextDocument doc;
  doc.setDocumentLayout(new QPlainTextDocumentLayout(&doc));

  UI::ApplyTextDirectionToDocument(nullptr, &doc, Qt::RightToLeft);
  EXPECT_EQ(doc.defaultTextOption().textDirection(), Qt::RightToLeft);
  EXPECT_EQ(doc.defaultTextOption().alignment(), Qt::AlignRight);

  UI::ApplyTextDirectionToDocument(nullptr, &doc, Qt::LeftToRight);
  EXPECT_EQ(doc.defaultTextOption().textDirection(), Qt::LeftToRight);
  EXPECT_EQ(doc.defaultTextOption().alignment(), Qt::AlignLeft);
}

TEST(TextDirectionTest, ARichTextLayoutKeepsItsLeadingAlignment) {
  // The other half of the same rule: QTextDocumentLayout resolves the leading
  // alignment against the base direction itself, and an explicit Qt::AlignRight
  // would be reversed back to the left edge.
  QTextDocument doc;
  const auto alignment_before = doc.defaultTextOption().alignment();

  UI::ApplyTextDirectionToDocument(nullptr, &doc, Qt::RightToLeft);
  EXPECT_EQ(doc.defaultTextOption().textDirection(), Qt::RightToLeft);
  EXPECT_EQ(doc.defaultTextOption().alignment(), alignment_before);
}

TEST(TextDirectionTest, TheWrapModeSurvivesADirectionChange) {
  // Direction, wrap mode and tab stop all share one QTextOption, so replacing
  // it wholesale instead of amending it would silently unwrap the editor.
  QTextDocument doc;
  doc.setDocumentLayout(new QPlainTextDocumentLayout(&doc));

  auto option = doc.defaultTextOption();
  option.setWrapMode(QTextOption::WrapAnywhere);
  doc.setDefaultTextOption(option);

  UI::ApplyTextDirectionToDocument(nullptr, &doc, Qt::RightToLeft);
  EXPECT_EQ(doc.defaultTextOption().wrapMode(), QTextOption::WrapAnywhere);
}

TEST(TextDirectionTest, ANullDocumentIsIgnored) {
  UI::ApplyTextDirectionToDocument(nullptr, nullptr, Qt::RightToLeft);
}

TEST(AppearanceSOTest, TextDirectionDefaultsToAutomatic) {
  const UI::AppearanceSO appearance{QJsonObject{}};
  EXPECT_EQ(appearance.text_direction, UI::kTEXT_DIRECTION_AUTO);
}

TEST(AppearanceSOTest, TextDirectionSurvivesARoundTrip) {
  UI::AppearanceSO appearance{QJsonObject{}};
  appearance.text_direction = UI::kTEXT_DIRECTION_RTL;

  const auto json = appearance.ToJson();
  // Pins the stored encoding: the settings page reads this back through
  // findData() on an int, so a string here would silently select nothing.
  EXPECT_TRUE(json["text_direction"].isDouble());

  const UI::AppearanceSO restored{json};
  EXPECT_EQ(restored.text_direction, UI::kTEXT_DIRECTION_RTL);
}

TEST(AppearanceSOTest, ACorruptStoredTextDirectionFallsBackToAutomatic) {
  EXPECT_EQ(
      UI::AppearanceSO(QJsonObject{{"text_direction", 42}}).text_direction,
      UI::kTEXT_DIRECTION_AUTO);
  EXPECT_EQ(
      UI::AppearanceSO(QJsonObject{{"text_direction", "rtl"}}).text_direction,
      UI::kTEXT_DIRECTION_AUTO);
}

}  // namespace GpgFrontend::Test
