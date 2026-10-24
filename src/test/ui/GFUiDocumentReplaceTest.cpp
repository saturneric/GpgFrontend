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

#include <QTextDocument>

#include "GpgFrontendTest.h"
#include "ui/function/DocumentReplace.h"

namespace GpgFrontend::Test {

// A bare QTextDocument is safe to build on the worker thread these tests run
// on; a QWidget would not be.

TEST(DocumentReplaceTest, UndoableReplaceComesBackWithOneUndo) {
  QTextDocument doc;
  doc.setUndoRedoEnabled(true);
  doc.setPlainText(u"hello"_s);

  UI::ReplaceDocumentText(&doc, u"-----BEGIN PGP MESSAGE-----"_s, true);
  ASSERT_EQ(doc.toPlainText(), "-----BEGIN PGP MESSAGE-----");

  doc.undo();
  EXPECT_EQ(doc.toPlainText(), "hello");

  doc.redo();
  EXPECT_EQ(doc.toPlainText(), "-----BEGIN PGP MESSAGE-----");
}

// Typing before the operation must still be undoable after undoing it.
TEST(DocumentReplaceTest, UndoableReplaceKeepsEarlierHistory) {
  QTextDocument doc;
  doc.setUndoRedoEnabled(true);
  QTextCursor cursor(&doc);
  cursor.insertText(u"typed"_s);

  UI::ReplaceDocumentText(&doc, u"result"_s, true);
  doc.undo();
  ASSERT_EQ(doc.toPlainText(), "typed");

  doc.undo();
  EXPECT_TRUE(doc.toPlainText().isEmpty());
}

// The default: the operation drops the history, so the previous text (the
// plaintext, after an encryption) cannot be brought back.
TEST(DocumentReplaceTest, PlainReplaceDropsTheHistory) {
  QTextDocument doc;
  doc.setUndoRedoEnabled(true);
  QTextCursor cursor(&doc);
  cursor.insertText(u"secret"_s);

  UI::ReplaceDocumentText(&doc, u"result"_s, false);
  doc.undo();

  EXPECT_EQ(doc.toPlainText(), "result");
  EXPECT_TRUE(doc.isUndoRedoEnabled());
}

TEST(DocumentReplaceTest, NullDocumentIsIgnored) {
  UI::ReplaceDocumentText(nullptr, u"x"_s, true);
  UI::ReplaceDocumentText(nullptr, u"x"_s, false);
}

// What a crypto operation reads. toPlainText() turns a no-break space into a
// space and U+2028 into a newline; a signature over either is then a
// signature over text the user never had.
TEST(DocumentReplaceTest, DocumentTextKeepsEveryCharacterAsHeld) {
  QTextDocument doc;
  const auto text = u"price:\u00A0100\u2028next\nline\t \n"_s;
  QTextCursor(&doc).insertText(text);

  EXPECT_EQ(UI::DocumentText(&doc), text);
  EXPECT_EQ(UI::DocumentText(&doc).toUtf8(), QByteArray("price:\xC2\xA0"
                                                        "100\xE2\x80\xA8"
                                                        "next\nline\t \n"));
  // The difference this exists for.
  EXPECT_NE(doc.toPlainText(), text);
}

TEST(DocumentReplaceTest, DocumentTextOfNothingIsEmpty) {
  QTextDocument doc;
  EXPECT_TRUE(UI::DocumentText(&doc).isEmpty());
  EXPECT_TRUE(UI::DocumentText(nullptr).isEmpty());
}

}  // namespace GpgFrontend::Test
