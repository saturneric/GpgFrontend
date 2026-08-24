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

#include <QTextCursor>
#include <QTextDocument>

#include "GpgFrontendTest.h"
#include "ui/function/TextFind.h"

namespace GpgFrontend::Test {

namespace {

// 0123456789...
// "alpha beta gamma alpha delta": alpha at 0 and 17, gamma at 11.
auto SampleText() -> QString {
  return QStringLiteral("alpha beta gamma alpha delta");
}

auto SelectionAt(QTextDocument* doc, int start, int end) -> QTextCursor {
  QTextCursor cursor(doc);
  cursor.setPosition(start);
  cursor.setPosition(end, QTextCursor::KeepAnchor);
  return cursor;
}

}  // namespace

TEST(TextFindTest, AnEmptySearchDropsTheSelection) {
  // The reported bug: emptying the search box left the last match highlighted,
  // because QTextDocument::find() answers an empty string with a null cursor
  // and the widget only applied non-null ones.
  QTextDocument doc;
  doc.setPlainText(SampleText());

  const auto result =
      UI::FindInDocument(&doc, SelectionAt(&doc, 11, 16), QString(), false);

  EXPECT_FALSE(result.hasSelection());
  EXPECT_EQ(result.position(), 11);
}

TEST(TextFindTest, AnEmptySearchLeavesABareCaretWhereItIs) {
  QTextDocument doc;
  doc.setPlainText(SampleText());

  QTextCursor caret(&doc);
  caret.setPosition(7);

  const auto result = UI::FindInDocument(&doc, caret, QString(), false);

  EXPECT_FALSE(result.hasSelection());
  EXPECT_EQ(result.position(), 7);
}

TEST(TextFindTest, AForwardSearchSelectsTheMatch) {
  QTextDocument doc;
  doc.setPlainText(SampleText());

  const auto result = UI::FindInDocument(&doc, QTextCursor(&doc),
                                         QStringLiteral("gamma"), false);

  EXPECT_EQ(result.selectionStart(), 11);
  EXPECT_EQ(result.selectionEnd(), 16);
}

TEST(TextFindTest, AForwardSearchWrapsToTheStart) {
  QTextDocument doc;
  doc.setPlainText(SampleText());

  // Starting past the last "alpha" leaves nothing ahead, so it has to resume
  // from the top rather than report no match.
  const auto result = UI::FindInDocument(&doc, SelectionAt(&doc, 17, 22),
                                         QStringLiteral("alpha"), false);

  EXPECT_EQ(result.selectionStart(), 0);
}

TEST(TextFindTest,
     ABackwardSearchWrapsToTheLastMatchNotToTheEleventhCharacter) {
  // QTextCursor::End is the number 11 once it is passed where an int offset is
  // expected, so the old wrap searched backwards from character 11 and landed
  // on the FIRST "alpha" instead of the last one.
  QTextDocument doc;
  doc.setPlainText(SampleText());

  QTextCursor start(&doc);
  start.setPosition(0);

  const auto result =
      UI::FindInDocument(&doc, start, QStringLiteral("alpha"), true);

  EXPECT_EQ(result.selectionStart(), 17);
  EXPECT_NE(result.selectionStart(), 0);
}

TEST(TextFindTest, ABackwardSearchWalksBackwards) {
  QTextDocument doc;
  doc.setPlainText(SampleText());

  const auto result = UI::FindInDocument(&doc, SelectionAt(&doc, 17, 22),
                                         QStringLiteral("alpha"), true);

  EXPECT_EQ(result.selectionStart(), 0);
}

TEST(TextFindTest, AMissLeavesTheCursorAlone) {
  QTextDocument doc;
  doc.setPlainText(SampleText());

  const auto from = SelectionAt(&doc, 11, 16);
  const auto result =
      UI::FindInDocument(&doc, from, QStringLiteral("epsilon"), false);

  EXPECT_EQ(result.selectionStart(), from.selectionStart());
  EXPECT_EQ(result.selectionEnd(), from.selectionEnd());
}

TEST(TextFindTest, TheSearchIsCaseSensitive) {
  QTextDocument doc;
  doc.setPlainText(SampleText());

  const auto result = UI::FindInDocument(&doc, QTextCursor(&doc),
                                         QStringLiteral("GAMMA"), false);

  EXPECT_FALSE(result.hasSelection());
}

TEST(TextFindTest, TypingOneMoreCharacterExtendsTheMatchInPlace) {
  // What the incremental search does on every keystroke: restart from where the
  // current match began, so "gam" growing into "gamm" stays on the same word
  // instead of skipping past it.
  QTextDocument doc;
  doc.setPlainText(SampleText());

  auto cursor = SelectionAt(&doc, 11, 14);  // "gam"
  cursor.setPosition(cursor.selectionStart());

  const auto result =
      UI::FindInDocument(&doc, cursor, QStringLiteral("gamm"), false);

  EXPECT_EQ(result.selectionStart(), 11);
  EXPECT_EQ(result.selectionEnd(), 15);
}

TEST(TextFindTest, ANullDocumentGivesTheCursorBack) {
  QTextDocument doc;
  doc.setPlainText(SampleText());

  const auto from = SelectionAt(&doc, 11, 16);
  const auto result =
      UI::FindInDocument(nullptr, from, QStringLiteral("alpha"), false);

  EXPECT_EQ(result.selectionStart(), from.selectionStart());
}

}  // namespace GpgFrontend::Test
