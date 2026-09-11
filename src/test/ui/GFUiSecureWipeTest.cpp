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
#include "ui/function/SecureWipe.h"

namespace GpgFrontend::Test {

// A bare QTextDocument is safe to build on the worker thread these tests run
// on; a QWidget would not be.

TEST(SecureWipeTest, AWipedDocumentIsEmptyAndUnmodified) {
  QTextDocument doc;
  doc.setPlainText(QStringLiteral("decrypted message"));
  doc.setModified(true);

  UI::WipeTextDocument(&doc);

  EXPECT_TRUE(doc.toRawText().isEmpty());
  EXPECT_FALSE(doc.isModified());
}

TEST(SecureWipeTest, AWipedSecretCannotBeBroughtBackWithUndo) {
  QTextDocument doc;
  doc.setUndoRedoEnabled(true);
  doc.setPlainText(QStringLiteral("correct horse battery staple"));

  UI::WipeTextDocument(&doc);
  doc.undo();

  // The property that actually matters to a user closing a tab: Ctrl+Z must
  // not put the plaintext back on screen.
  EXPECT_TRUE(doc.toRawText().isEmpty());
}

TEST(SecureWipeTest, ARedoCannotBringTheSecretBackEither) {
  QTextDocument doc;
  doc.setUndoRedoEnabled(true);
  doc.setPlainText(QStringLiteral("top secret"));

  UI::WipeTextDocument(&doc);
  doc.undo();
  doc.redo();

  EXPECT_TRUE(doc.toRawText().isEmpty());
}

TEST(SecureWipeTest, TheUndoRedoSettingIsRestored) {
  QTextDocument enabled;
  enabled.setUndoRedoEnabled(true);
  UI::WipeTextDocument(&enabled);
  EXPECT_TRUE(enabled.isUndoRedoEnabled());

  QTextDocument disabled;
  disabled.setUndoRedoEnabled(false);
  UI::WipeTextDocument(&disabled);
  EXPECT_FALSE(disabled.isUndoRedoEnabled());
}

TEST(SecureWipeTest, WipingAnEmptyDocumentIsHarmless) {
  QTextDocument doc;
  UI::WipeTextDocument(&doc);
  EXPECT_TRUE(doc.toRawText().isEmpty());
}

TEST(SecureWipeTest, ANullDocumentIsIgnored) {
  UI::WipeTextDocument(nullptr);
  SUCCEED();
}

}  // namespace GpgFrontend::Test
