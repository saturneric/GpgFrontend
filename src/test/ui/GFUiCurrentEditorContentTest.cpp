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

// The SDK's route to "what the user is looking at".
//
// CurrentEditorContent() hops to the GUI thread when it is not already on it.
// The hop used to be unconditional, and a Qt::BlockingQueuedConnection to
// one's OWN thread is a deadlock: Qt refuses it with "Dead lock detected" and
// hands back nothing. That is the common case, not the exotic one -- a module
// dialog's button handler runs on the GUI thread -- so this test makes the
// call FROM the GUI thread, which is where the bug lived.
//
// Note gtest bodies run on a worker thread here, so "on the GUI thread" has
// to be arranged explicitly.

#include <gtest/gtest.h>

#include <QApplication>
#include <QThread>

#include "GpgFrontendTest.h"
#include "ui/UIModuleManager.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::Test {

namespace {

/// Run @p work on the GUI thread and wait for it.
template <typename F>
void OnGuiThread(F&& work) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), std::forward<F>(work),
                            Qt::BlockingQueuedConnection);
}

struct CapturedMessages {
  static inline QStringList lines;
  static inline QtMessageHandler previous = nullptr;

  static void Handler(QtMsgType type, const QMessageLogContext& context,
                      const QString& message) {
    lines.append(message);
    if (previous != nullptr) previous(type, context, message);
  }

  CapturedMessages() {
    lines.clear();
    previous = qInstallMessageHandler(&Handler);
  }
  ~CapturedMessages() { qInstallMessageHandler(previous); }
};

}  // namespace

TEST(GFUiCurrentEditorContentTest, ReadingFromTheGuiThreadDoesNotDeadlock) {
  UI::TextEdit* edit = nullptr;
  OnGuiThread([&] {
    edit = new UI::TextEdit(nullptr);
    UI::RegisterNamedQObject("main_window_edit", edit);
  });
  ASSERT_NE(edit, nullptr);

  QByteArray bytes;
  bool deadlocked = false;
  {
    const CapturedMessages captured;
    OnGuiThread([&] { bytes = UI::CurrentEditorContent(); });
    deadlocked = std::any_of(
        CapturedMessages::lines.cbegin(), CapturedMessages::lines.cend(),
        [](const QString& line) { return line.contains("Dead lock"); });
  }

  EXPECT_FALSE(deadlocked)
      << "a blocking queued call to the caller's own thread is a deadlock";

  OnGuiThread([&] { delete edit; });
}

TEST(GFUiCurrentEditorContentTest, ReturnsWhatTheCurrentTabHolds) {
  UI::TextEdit* edit = nullptr;
  QByteArray bytes;
  OnGuiThread([&] {
    edit = new UI::TextEdit(nullptr);
    UI::RegisterNamedQObject("main_window_edit", edit);
    // The default workspace tab is not a plain-text page, and the fill slot
    // dereferences CurTextPage() unconditionally.
    edit->SlotNewTab();
    edit->SlotFillTextEditWithText(QString("-----BEGIN PGP MESSAGE-----"));
    bytes = UI::CurrentEditorContent();
  });

  EXPECT_TRUE(bytes.contains("BEGIN PGP MESSAGE"))
      << "got: " << bytes.toStdString();

  OnGuiThread([&] { delete edit; });
}

TEST(GFUiCurrentEditorContentTest, NoEditorIsAnEmptyAnswerNotACrash) {
  // The SDK promises empty rather than a failure when nothing is open, and a
  // module dialog opened before any tab exists is an ordinary case.
  UI::RegisterNamedQObject("main_window_edit", nullptr);
  EXPECT_TRUE(UI::CurrentEditorContent().isEmpty());
}

}  // namespace GpgFrontend::Test
