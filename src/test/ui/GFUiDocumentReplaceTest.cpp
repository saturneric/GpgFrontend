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
  doc.setPlainText(QStringLiteral("hello"));

  UI::ReplaceDocumentText(&doc, QStringLiteral("-----BEGIN PGP MESSAGE-----"),
                          true);
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
  cursor.insertText(QStringLiteral("typed"));

  UI::ReplaceDocumentText(&doc, QStringLiteral("result"), true);
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
  cursor.insertText(QStringLiteral("secret"));

  UI::ReplaceDocumentText(&doc, QStringLiteral("result"), false);
  doc.undo();

  EXPECT_EQ(doc.toPlainText(), "result");
  EXPECT_TRUE(doc.isUndoRedoEnabled());
}

TEST(DocumentReplaceTest, NullDocumentIsIgnored) {
  UI::ReplaceDocumentText(nullptr, QStringLiteral("x"), true);
  UI::ReplaceDocumentText(nullptr, QStringLiteral("x"), false);
}

}  // namespace GpgFrontend::Test
