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

#include "crypto/EditorHistory.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::EditorHistory;

namespace {

/// Type @p text at the end of @p document, as an edit of its own.
void Type(QTextDocument& document, const QString& text) {
  QTextCursor cursor(&document);
  cursor.movePosition(QTextCursor::End);
  cursor.insertText(text);
}

}  // namespace

// A result put in place of the text is one step: Undo brings the text
// back, Redo the result.
TEST(EditorHistoryTest, AReplacementIsOneUndoStep) {
  QTextDocument document;
  Type(document, QStringLiteral("the message"));
  ASSERT_TRUE(
      EditorHistory::ReplaceAll(&document, QStringLiteral("result\nline")));
  EXPECT_EQ(document.toPlainText(), QStringLiteral("result\nline"));

  document.undo();
  EXPECT_EQ(document.toPlainText(), QStringLiteral("the message"));
  document.redo();
  EXPECT_EQ(document.toPlainText(), QStringLiteral("result\nline"));
}

TEST(EditorHistoryTest, AnEmptyResultIsUndoneToo) {
  QTextDocument document;
  Type(document, QStringLiteral("encrypted"));
  ASSERT_TRUE(EditorHistory::ReplaceAll(&document, QString()));
  EXPECT_TRUE(document.toPlainText().isEmpty());
  document.undo();
  EXPECT_EQ(document.toPlainText(), QStringLiteral("encrypted"));
}

// Typing after an Undo ends what Redo could bring back, as for any edit.
TEST(EditorHistoryTest, AnEditAfterUndoEndsTheRedo) {
  QTextDocument document;
  Type(document, QStringLiteral("abc"));
  EditorHistory::ReplaceAll(&document, QStringLiteral("result"));
  document.undo();
  ASSERT_TRUE(document.isRedoAvailable());
  Type(document, QStringLiteral("d"));
  EXPECT_FALSE(document.isRedoAvailable());
  EXPECT_EQ(document.toPlainText(), QStringLiteral("abcd"));
}

TEST(EditorHistoryTest, NoDocumentNothingDone) {
  EXPECT_FALSE(EditorHistory::ReplaceAll(nullptr, QStringLiteral("x")));
}

}  // namespace GpgFrontend::Test::Mobile
