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

TEST(TextDirectionTest, TheDocumentAnchorStopsAtTheFirstStrongCharacter) {
  QTextDocument doc;
  doc.setPlainText(QStringLiteral("\n123\n") + QString::fromUtf8("مرحبا") +
                   QStringLiteral("\nhello"));

  // The blocks disagree, which under automatic is normal and is exactly why the
  // anchor is a separate question from how any one paragraph is laid out.
  EXPECT_EQ(UI::DetectTextDirection(&doc), Qt::RightToLeft);
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

TEST(TextDirectionTest, AutomaticHandsThePlainTextLayoutBackToQt) {
  // Qt::LayoutDirectionAuto is the whole per-paragraph feature: with it in the
  // document option, QTextEngine resolves each block from its own first strong
  // character. Pinning a real direction here is what used to suppress that.
  QTextDocument doc;
  doc.setDocumentLayout(new QPlainTextDocumentLayout(&doc));
  doc.setPlainText(QStringLiteral("hello\n") + QString::fromUtf8("مرحبا"));

  UI::ApplyTextDirectionToDocument(nullptr, &doc, UI::kTEXT_DIRECTION_AUTO);
  EXPECT_EQ(doc.defaultTextOption().textDirection(), Qt::LayoutDirectionAuto);
}

TEST(TextDirectionTest, AutomaticAnchorsThePlainTextAlignmentToTheDocument) {
  // QPlainTextDocumentLayout ignores per-block formats, so the lines cannot be
  // aligned one at a time and share the document's anchor instead. A file that
  // opens in Arabic still has to reach the right edge, as it did before any of
  // this was resolved per paragraph.
  QTextDocument rtl_doc;
  rtl_doc.setDocumentLayout(new QPlainTextDocumentLayout(&rtl_doc));
  rtl_doc.setPlainText(QString::fromUtf8("مرحبا") + QStringLiteral("\nhello"));

  UI::ApplyTextDirectionToDocument(nullptr, &rtl_doc, UI::kTEXT_DIRECTION_AUTO);
  EXPECT_EQ(rtl_doc.defaultTextOption().alignment(), Qt::AlignRight);

  QTextDocument ltr_doc;
  ltr_doc.setDocumentLayout(new QPlainTextDocumentLayout(&ltr_doc));
  ltr_doc.setPlainText(QStringLiteral("hello\n") + QString::fromUtf8("مرحبا"));

  UI::ApplyTextDirectionToDocument(nullptr, &ltr_doc, UI::kTEXT_DIRECTION_AUTO);
  EXPECT_EQ(ltr_doc.defaultTextOption().alignment(), Qt::AlignLeft);
}

TEST(TextDirectionTest, AutomaticLeavesARichTextLayoutAlignmentAlone) {
  // QTextDocumentLayout aligns each block against that block's own direction,
  // so it needs no anchor and an explicit alignment would only get in its way.
  QTextDocument doc;
  doc.setPlainText(QString::fromUtf8("مرحبا"));
  const auto alignment_before = doc.defaultTextOption().alignment();

  UI::ApplyTextDirectionToDocument(nullptr, &doc, UI::kTEXT_DIRECTION_AUTO);
  EXPECT_EQ(doc.defaultTextOption().textDirection(), Qt::LayoutDirectionAuto);
  EXPECT_EQ(doc.defaultTextOption().alignment(), alignment_before);
}

TEST(TextDirectionTest, ExplicitModesStillPinTheWholeDocument) {
  QTextDocument doc;
  doc.setDocumentLayout(new QPlainTextDocumentLayout(&doc));
  doc.setPlainText(QString::fromUtf8("مرحبا"));

  UI::ApplyTextDirectionToDocument(nullptr, &doc, UI::kTEXT_DIRECTION_LTR);
  EXPECT_EQ(doc.defaultTextOption().textDirection(), Qt::LeftToRight);
  EXPECT_EQ(doc.defaultTextOption().alignment(), Qt::AlignLeft);

  UI::ApplyTextDirectionToDocument(nullptr, &doc, UI::kTEXT_DIRECTION_RTL);
  EXPECT_EQ(doc.defaultTextOption().textDirection(), Qt::RightToLeft);
  EXPECT_EQ(doc.defaultTextOption().alignment(), Qt::AlignRight);
}

TEST(TextDirectionTest, TheWrapModeSurvivesAModeChange) {
  QTextDocument doc;
  doc.setDocumentLayout(new QPlainTextDocumentLayout(&doc));

  auto option = doc.defaultTextOption();
  option.setWrapMode(QTextOption::WrapAnywhere);
  doc.setDefaultTextOption(option);

  for (const auto mode : {UI::kTEXT_DIRECTION_AUTO, UI::kTEXT_DIRECTION_RTL,
                          UI::kTEXT_DIRECTION_AUTO}) {
    UI::ApplyTextDirectionToDocument(nullptr, &doc, mode);
    EXPECT_EQ(doc.defaultTextOption().wrapMode(), QTextOption::WrapAnywhere);
  }
}

TEST(TextDirectionTest, ANullDocumentIsIgnoredByTheModeOverload) {
  for (const auto mode : {UI::kTEXT_DIRECTION_AUTO, UI::kTEXT_DIRECTION_LTR,
                          UI::kTEXT_DIRECTION_RTL}) {
    UI::ApplyTextDirectionToDocument(nullptr, nullptr, mode);
  }
}

TEST(AppearanceSOTest, AStoredTextDirectionIsIgnored) {
  // The setting was removed. A profile written by an older build still carries
  // the key, which has to be read past without complaint and dropped on the
  // next write; an older build reading it back then falls back to automatic,
  // which is both its own default and what this one now always does.
  const UI::AppearanceSO appearance{QJsonObject{{"text_direction", 2}}};
  EXPECT_FALSE(appearance.ToJson().contains("text_direction"));
}

}  // namespace GpgFrontend::Test
