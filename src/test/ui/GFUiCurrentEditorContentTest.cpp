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
#include <QCryptographicHash>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QThread>

#include "GpgFrontendTest.h"
#include "ui/UIModuleManager.h"
#include "ui/widgets/PlainTextEditorPage.h"
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
    OnGuiThread(
        [&] { bytes = UI::CurrentEditorContent().value_or(QByteArray()); });
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
    bytes = UI::CurrentEditorContent().value_or(QByteArray());
  });

  EXPECT_TRUE(bytes.contains("BEGIN PGP MESSAGE"))
      << "got: " << bytes.toStdString();

  OnGuiThread([&] { delete edit; });
}

TEST(GFUiCurrentEditorContentTest, NoEditorIsReportedAsAbsentNotACrash) {
  // A module dialog opened before any editor exists is an ordinary case, and
  // "nothing open" must be distinguishable from an empty document.
  UI::RegisterNamedQObject("main_window_edit", nullptr);
  EXPECT_FALSE(UI::CurrentEditorContent().has_value());
}

TEST(GFUiCurrentEditorContentTest, NoTextTabIsReportedAsAbsent) {
  UI::TextEdit* edit = nullptr;
  std::optional<QByteArray> bytes = QByteArray("unset");
  OnGuiThread([&] {
    edit = new UI::TextEdit(nullptr);
    UI::RegisterNamedQObject("main_window_edit", edit);
    // Only the default workspace tab, which is not a plain-text page.
    bytes = UI::CurrentEditorContent();
  });

  EXPECT_FALSE(bytes.has_value());

  OnGuiThread([&] { delete edit; });
}

namespace {

/// A plain-text tab in a fresh editor, built on the GUI thread.
auto NewTextTab() -> UI::TextEdit* {
  UI::TextEdit* edit = nullptr;
  OnGuiThread([&] {
    edit = new UI::TextEdit(nullptr);
    UI::RegisterNamedQObject("main_window_edit", edit);
    edit->SlotNewTab();
  });
  return edit;
}

/// The current tab's text widget. Call on the GUI thread.
auto CurrentTextWidget(UI::TextEdit* edit) -> QPlainTextEdit* {
  QWidget* page = edit->CurTextPage();
  return page == nullptr ? nullptr : page->findChild<QPlainTextEdit*>();
}

}  // namespace

// The text an operation signs, encrypts or verifies is the text in the
// editor, character for character: no OpenPGP rule asks for a no-break space
// or a U+2028 to be rewritten.
TEST(GFUiCurrentEditorContentTest, OperationsReadTheCharactersAsTyped) {
  auto* edit = NewTextTab();
  const auto typed = QStringLiteral("a\u00A0b\u2028c\nd");
  QString text;
  QByteArray bytes;
  OnGuiThread([&] {
    CurrentTextWidget(edit)->insertPlainText(typed);
    text = edit->CurPlainText();
    bytes = edit->CurDocumentBytesForOperation();
  });

  EXPECT_EQ(text, typed);
  EXPECT_EQ(bytes, typed.toUtf8());

  OnGuiThread([&] { delete edit; });
}

// Shift+Enter is a line break to the person typing it. Qt stores it as
// U+2028, which would now reach the signature as that character; the editor
// must store an ordinary newline instead.
TEST(GFUiCurrentEditorContentTest, ShiftEnterTypesAnOrdinaryNewline) {
  auto* edit = NewTextTab();
  QString text;
  OnGuiThread([&] {
    auto* page = CurrentTextWidget(edit);
    page->insertPlainText(QStringLiteral("a"));
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::ShiftModifier,
                    QStringLiteral("\r"));
    QCoreApplication::sendEvent(page, &press);
    page->insertPlainText(QStringLiteral("b"));
    text = edit->CurPlainText();
  });

  EXPECT_EQ(text, QStringLiteral("a\nb"));

  OnGuiThread([&] { delete edit; });
}

// A document that came in with CRLF is encrypted with CRLF, the same bytes
// Save would write, not silently rewritten to LF.
TEST(GFUiCurrentEditorContentTest, ACrlfDocumentKeepsItsLineEndings) {
  auto* edit = NewTextTab();
  QByteArray bytes;
  OnGuiThread([&] {
    edit->SlotFillTextEditWithText(GFBuffer(QByteArray("one\r\ntwo\r\n")));
    bytes = edit->CurDocumentBytesForOperation();
  });

  EXPECT_EQ(bytes, QByteArray("one\r\ntwo\r\n"));

  // The SHA-256 shown for the document is that of the same bytes.
  const auto want =
      QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
  bool shown = false;
  for (int i = 0; i < 300 && !shown; ++i) {
    QThread::msleep(10);
    OnGuiThread([&] {
      for (auto* label : edit->findChildren<QLabel*>("sha256Label")) {
        shown = shown || label->toolTip().contains(want);
      }
    });
  }
  EXPECT_TRUE(shown);

  OnGuiThread([&] { delete edit; });
}

// A file is read in 8 KiB chunks. A UTF-8 character split across two of them
// must arrive whole, not as two replacement characters.
TEST(GFUiCurrentEditorContentTest, AFileReadInChunksKeepsSplitCharacters) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto path = dir.filePath("split.txt");
  QByteArray content(8191, 'x');
  content += "\xC3\xA9 and \xE2\x82\xAC\n";  // "é" straddles 8192
  {
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(content);
  }

  auto* edit = NewTextTab();
  OnGuiThread([&] { edit->SlotOpenFile(path); });

  // Loading is asynchronous; wait until the whole file is in.
  // The editor may also restore cached tabs, so select the file's own tab.
  QByteArray bytes;
  for (int i = 0; i < 500 && bytes.size() < content.size(); ++i) {
    // The read task runs on the Default runner, the thread this test is on.
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    QThread::msleep(10);
    OnGuiThread([&] {
      auto* tabs = edit->findChild<QTabWidget*>();
      for (int t = 0; tabs != nullptr && t < tabs->count(); ++t) {
        if (tabs->tabText(t).contains("split.txt")) tabs->setCurrentIndex(t);
      }
      bytes = edit->CurDocumentBytesForOperation();
    });
  }
  EXPECT_EQ(bytes, content);

  OnGuiThread([&] { delete edit; });
}

}  // namespace GpgFrontend::Test
