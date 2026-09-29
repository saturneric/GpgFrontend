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

// The Status Panel's one automatic visibility change: a NEW result reveals a
// closed panel. Content that is merely still on the board never does, and a
// reset never hides or shows anything.
//
// gtest bodies run on a worker thread here, and a widget may only be built on
// the GUI thread, so every widget case runs there with a blocking hop.

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QTabWidget>
#include <QTextBrowser>

#include "GpgFrontendTest.h"
#include "ui/widgets/InfoBoardWidget.h"

namespace GpgFrontend::Test {

namespace {

using UI::InfoBoardCard;
using UI::InfoBoardWidget;
using UI::kINFO_ERROR_CRITICAL;
using UI::kINFO_ERROR_NEUTRAL;
using UI::kINFO_ERROR_OK;
using UI::kINFO_ERROR_WARN;

/// Run @p work on the GUI thread and wait for it.
template <typename F>
void OnGuiThread(F&& work) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), std::forward<F>(work),
                            Qt::BlockingQueuedConnection);
}

/// A never-shown main window with the board in a dock, wired as MainWindow
/// wires it. Build and use it on the GUI thread only.
struct RevealFixture {
  QMainWindow window;
  QDockWidget* dock = new QDockWidget(&window);
  InfoBoardWidget* board = new InfoBoardWidget(dock);
  int posted = 0;

  RevealFixture() {
    window.addDockWidget(Qt::BottomDockWidgetArea, dock);
    dock->setWidget(board);
    UI::RevealOnNewResult(board, dock);
    QObject::connect(board, &InfoBoardWidget::SignalResultPosted, board,
                     [this](UI::InfoBoardStatus) { ++posted; });
  }

  /// What the dock's close button does. (Not the toggle action: its checked
  /// state follows real visibility, which a never-shown window does not have.)
  void UserCloses() const { dock->close(); }
};

}  // namespace

TEST(GFUiInfoBoardRevealTest, EmptyPostingsAreNotResults) {
  EXPECT_FALSE(UI::IsMeaningfulInfoBoardResult(QString(), 0));
  EXPECT_FALSE(UI::IsMeaningfulInfoBoardResult(QStringLiteral(" \n\t "), 0));
  EXPECT_TRUE(UI::IsMeaningfulInfoBoardResult(QStringLiteral("done"), 0));
  EXPECT_TRUE(UI::IsMeaningfulInfoBoardResult(QString(), 1));
}

TEST(GFUiInfoBoardRevealTest, NewResultRevealsAClosedPanel) {
  OnGuiThread([] {
    RevealFixture f;
    f.UserCloses();
    ASSERT_TRUE(f.dock->isHidden());

    f.board->SetInfoBoard(QStringLiteral("ok"), kINFO_ERROR_OK);

    EXPECT_FALSE(f.dock->isHidden());
    EXPECT_EQ(f.posted, 1);
  });
}

TEST(GFUiInfoBoardRevealTest, NewResultLeavesAnOpenPanelOpen) {
  OnGuiThread([] {
    RevealFixture f;
    ASSERT_FALSE(f.dock->isHidden());

    f.board->SetInfoBoard(QStringLiteral("ok"), kINFO_ERROR_OK);

    EXPECT_FALSE(f.dock->isHidden());
  });
}

TEST(GFUiInfoBoardRevealTest, OldContentNeverOverridesAUserClose) {
  OnGuiThread([] {
    RevealFixture f;
    f.board->SetInfoBoard(QStringLiteral("first"), kINFO_ERROR_WARN);
    f.UserCloses();

    // Re-rendering what is already there is not news.
    f.board->UpdateActionButtons();
    f.board->ApplyAppearanceSettings();

    EXPECT_TRUE(f.dock->isHidden());
    EXPECT_EQ(f.posted, 1);

    // The next result is.
    f.board->SetInfoBoard(QStringLiteral("second"), kINFO_ERROR_CRITICAL);
    EXPECT_FALSE(f.dock->isHidden());
    EXPECT_EQ(f.posted, 2);
  });
}

TEST(GFUiInfoBoardRevealTest, ResetNeitherHidesNorShows) {
  OnGuiThread([] {
    RevealFixture f;
    f.board->SetInfoBoard(QStringLiteral("ok"), kINFO_ERROR_OK);
    const int posted = f.posted;

    f.board->SlotReset();
    EXPECT_FALSE(f.dock->isHidden());

    f.UserCloses();
    f.board->SlotReset();
    EXPECT_TRUE(f.dock->isHidden());
    EXPECT_EQ(f.posted, posted);
  });
}

TEST(GFUiInfoBoardRevealTest, EmptyUpdatesDoNotReveal) {
  OnGuiThread([] {
    RevealFixture f;
    f.UserCloses();

    f.board->SetInfoBoard(QString(), kINFO_ERROR_OK);
    f.board->SetInfoBoard(QStringLiteral("   "), kINFO_ERROR_WARN);
    f.board->SetInfoBoardCards(QString(), kINFO_ERROR_NEUTRAL, {});
    f.board->SlotRefresh(QString(), kINFO_ERROR_CRITICAL);

    EXPECT_TRUE(f.dock->isHidden());
    EXPECT_EQ(f.posted, 0);
  });
}

TEST(GFUiInfoBoardRevealTest, CardsAloneAreAResult) {
  OnGuiThread([] {
    RevealFixture f;
    f.UserCloses();

    InfoBoardCard card;
    card.title = QStringLiteral("SHA-256");
    card.fields.append({QStringLiteral("File"), QStringLiteral("a.txt")});
    f.board->SetInfoBoardCards(QString(), kINFO_ERROR_NEUTRAL, {card});

    EXPECT_FALSE(f.dock->isHidden());
    EXPECT_EQ(f.posted, 1);
  });
}

TEST(GFUiInfoBoardRevealTest, StructuredPostingsAnnounceExactlyOnce) {
  OnGuiThread([] {
    RevealFixture f;

    GpgOpResultInfo info;
    info.operation = QStringLiteral("Verify");
    f.board->SetInfoBoardWithOpInfo(QStringLiteral("verified"), kINFO_ERROR_OK,
                                    info);
    EXPECT_EQ(f.posted, 1);

    f.UserCloses();
    f.board->SetInfoBoardFromResults(
        QStringLiteral("2 files"), kINFO_ERROR_WARN,
        {UI::GpgOperaResult(info), UI::GpgOperaResult(info)});
    EXPECT_EQ(f.posted, 2);
    EXPECT_FALSE(f.dock->isHidden());
  });
}

TEST(GFUiInfoBoardRevealTest, AnotherCurrentTabClearsTheLastTabsResult) {
  OnGuiThread([] {
    RevealFixture f;
    QTabWidget tabs;
    tabs.addTab(new QWidget(), QStringLiteral("a"));
    f.board->AssociateTabWidget(&tabs);
    auto raw = [&] {
      return f.board->findChild<QTextBrowser*>(QStringLiteral("infoBoard"))
          ->toPlainText();
    };

    f.board->SetInfoBoard(QStringLiteral("decrypted"), kINFO_ERROR_OK);
    ASSERT_FALSE(raw().isEmpty());

    // Opening a tab makes it current without any click on the tab bar.
    tabs.setCurrentIndex(tabs.addTab(new QWidget(), QStringLiteral("b")));

    EXPECT_TRUE(raw().isEmpty()) << "the old result is not the new tab's";
    EXPECT_EQ(f.board->CurrentStatus(), kINFO_ERROR_NEUTRAL);
    EXPECT_FALSE(f.dock->isHidden()) << "a reset never hides";
  });
}

}  // namespace GpgFrontend::Test
