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

// The text editor wraps at its viewport's width, so a vertical scrollbar that
// appears only when the content overflows would reflow every line sideways
// whenever the editor loses height -- the Status Panel expanding below it is
// the everyday case. Its gutter is reserved instead: the wrap width must not
// depend on whether there is anything to scroll.
//
// gtest bodies run off the GUI thread, so the widget work hops there.

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QPlainTextEdit>
#include <QScrollBar>

#include "GpgFrontendTest.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::Test {

namespace {

template <typename F>
void OnGuiThread(F&& work) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), std::forward<F>(work),
                            Qt::BlockingQueuedConnection);
}

void Settle() { QCoreApplication::processEvents(); }

}  // namespace

TEST(GFUiEditorScrollGutterTest, TheWrapWidthDoesNotDependOnScrolling) {
  OnGuiThread([] {
    UI::TextEdit edit(nullptr);
    edit.resize(700, 600);
    edit.show();
    edit.SlotNewTab();
    Settle();

    QPlainTextEdit* editor = nullptr;
    for (auto* e : edit.findChildren<QPlainTextEdit*>(
             QStringLiteral("PlainTextEditor"))) {
      if (e->isVisible()) editor = e;
    }
    ASSERT_NE(editor, nullptr);
    EXPECT_EQ(editor->verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOn);

    // A short document: nothing to scroll, and the gutter already there.
    editor->setPlainText(QStringLiteral("short"));
    Settle();
    EXPECT_EQ(editor->verticalScrollBar()->maximum(), 0);

    // The same content throughout from here on (the line-number gutter
    // follows the digit count, which is the content's business, not the
    // panel's): enough lines to fit the tall editor but not the short one.
    QStringList lines;
    for (int i = 0; i < 20; ++i) {
      lines << QStringLiteral("line %1 of a document").arg(i + 10);
    }
    editor->setPlainText(lines.join('\n'));
    Settle();
    auto* bar = editor->verticalScrollBar();
    ASSERT_EQ(bar->maximum(), 0) << "fits: nothing to scroll yet";
    const int width = editor->viewport()->width();

    // Less height, as when the Status Panel expands: now it scrolls, and
    // scrolling works, at exactly the width it had.
    edit.resize(700, 200);
    Settle();
    ASSERT_GT(bar->maximum(), 0) << "the threshold was actually crossed";
    EXPECT_EQ(editor->viewport()->width(), width);
    bar->setValue(bar->maximum());
    EXPECT_EQ(bar->value(), bar->maximum());

    // And back, as when the panel collapses again: no shift the other way.
    edit.resize(700, 600);
    Settle();
    EXPECT_EQ(bar->maximum(), 0);
    EXPECT_EQ(editor->viewport()->width(), width);
  });
}

}  // namespace GpgFrontend::Test
