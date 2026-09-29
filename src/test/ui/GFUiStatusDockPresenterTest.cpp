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

// The Status Panel's two separate axes: the dock's own visibility (its X, the
// View menu) and, for a visible dock, Expanded / Collapsed. Collapse is not
// hide and not clear; only a NEW result expands; a compact context is entered
// and left, not re-applied on every tab switch.
//
// gtest bodies run on a worker thread, and widgets live on the GUI thread, so
// every case runs there with a blocking hop. The window is shown (offscreen
// platform) so that geometry and the toggle action's check state are real.
// Heights are compared, never asserted as pixel values.

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QLabel>
#include <QMainWindow>
#include <QStyle>
#include <QTextBrowser>
#include <QThread>
#include <QToolButton>

#include "GpgFrontendTest.h"
#include "core/model/SettingsObject.h"
#include "ui/widgets/InfoBoardWidget.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "ui/widgets/StatusDockPresenter.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::Test {

namespace {

using UI::InfoBoardWidget;
using UI::kINFO_ERROR_CRITICAL;
using UI::kINFO_ERROR_OK;
using UI::StatusDockPresentation;
using UI::StatusDockPresenter;

/// Run @p work on the GUI thread and wait for it.
template <typename F>
void OnGuiThread(F&& work) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), std::forward<F>(work),
                            Qt::BlockingQueuedConnection);
}

/// No upper limit. QWIDGETSIZE_MAX is what the presenter sets; once the
/// dock's own layout has run, it reports the layout's QLAYOUTSIZE_MAX instead,
/// and both mean the same thing.
auto Unconstrained(int max_height) -> bool {
  return max_height >= QLAYOUTSIZE_MAX;
}

/// A main window with an editor in the middle and the board docked at the
/// bottom, wired as MainWindow wires it. GUI thread only.
struct PresenterFixture {
  QMainWindow window;
  QDockWidget* dock = new QDockWidget(&window);
  InfoBoardWidget* board = new InfoBoardWidget(dock);
  StatusDockPresenter* presenter = nullptr;

  explicit PresenterFixture(bool show = true,
                            const QString& settings_name = {}) {
    window.setCentralWidget(new QWidget(&window));
    dock->setObjectName(QStringLiteral("infoBoardDock"));
    dock->setWindowTitle(QStringLiteral("Status Panel"));
    window.addDockWidget(Qt::BottomDockWidgetArea, dock);
    dock->setWidget(board);
    dock->setMinimumHeight(120);
    UI::RevealOnNewResult(board, dock);
    presenter = new StatusDockPresenter(&window, dock, board, settings_name);
    // Instant by default here, so these cases check the layout semantics;
    // the transition tests below turn the animation back on.
    presenter->SetTransitionDuration(0);
    board->AddBarAction(presenter->ToggleAction());

    window.resize(900, 700);
    // Not shown: what MainWindow's constructor sees while it restores the
    // saved layout and the tabs.
    if (!show) return;
    window.show();
    window.resizeDocks({dock}, {260}, Qt::Vertical);
    Settle();
  }

  static void Settle() { QCoreApplication::processEvents(); }

  /// Lets a running transition play out, and says whether the dock's height
  /// was ever released to QWIDGETSIZE_MAX on the way -- which only the end of
  /// an expansion does.
  auto PlayOut() const -> bool {
    bool released = false;
    QElapsedTimer clock;
    clock.start();
    while (presenter->IsTransitioning() && clock.elapsed() < 3000) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
      QThread::msleep(2);
      if (presenter->IsTransitioning() &&
          Unconstrained(dock->maximumHeight())) {
        released = true;
      }
    }
    Settle();
    return released;
  }

  [[nodiscard]] auto Child(const char* name) const -> QWidget* {
    return board->findChild<QWidget*>(QString::fromLatin1(name));
  }
  [[nodiscard]] auto Shows(const char* name) const -> bool {
    auto* w = Child(name);
    return w != nullptr && w->isVisibleTo(dock);
  }
  [[nodiscard]] auto Bar(const char* name) const -> QWidget* {
    return presenter->CompactBar()->findChild<QWidget*>(
        QString::fromLatin1(name));
  }
  /// Visible in the collapsed row, as laid out inside the dock.
  [[nodiscard]] auto BarShows(const char* name) const -> bool {
    auto* w = Bar(name);
    return w != nullptr && w->isVisibleTo(dock);
  }
  [[nodiscard]] auto ExpandButton() const -> QToolButton* {
    return qobject_cast<QToolButton*>(Bar("statusDockBarExpand"));
  }
  [[nodiscard]] auto RawText() const -> QString {
    return board->findChild<QTextBrowser*>(QStringLiteral("infoBoard"))
        ->toPlainText();
  }
  [[nodiscard]] auto IndicatorStyle() const -> QString {
    return Bar("statusDockBarIndicator")->styleSheet();
  }
  [[nodiscard]] auto BoardShown() const -> bool {
    return board->isVisibleTo(dock);
  }
  [[nodiscard]] auto Collapsed() const -> bool {
    return presenter->Presentation() == StatusDockPresentation::kCollapsed;
  }

  void Collapse() const {
    presenter->Collapse();
    Settle();
  }
  void Expand() const {
    presenter->Expand();
    Settle();
  }
  /// What the dock's X does.
  void UserCloses() const {
    dock->close();
    Settle();
  }
};

}  // namespace

TEST(GFUiStatusDockPresenterTest, CollapseKeepsTheDockVisibleAsAThinBar) {
  OnGuiThread([] {
    PresenterFixture f;
    const int expanded = f.dock->height();

    f.Collapse();

    EXPECT_TRUE(f.Collapsed());
    EXPECT_FALSE(f.dock->isHidden()) << "collapse is not hide";
    EXPECT_FALSE(f.BoardShown()) << "the content is hidden";
    EXPECT_EQ(f.dock->titleBarWidget(), f.presenter->CompactBar())
        << "one row: the bar stands in for the title bar";
    EXPECT_LT(f.dock->height(), expanded);
    // No slack left over from the expanded minimum: the row gets what it
    // prefers, and the rest goes to the editor.
    const int frame = 2 * f.dock->style()->pixelMetric(
                              QStyle::PM_DockWidgetFrameWidth, nullptr, f.dock);
    EXPECT_LE(f.dock->height(),
              f.presenter->CompactBar()->sizeHint().height() + frame);
  });
}

TEST(GFUiStatusDockPresenterTest, CollapsedBarKeepsOnlyTheEssentialControls) {
  OnGuiThread([] {
    PresenterFixture f;
    f.Collapse();

    EXPECT_TRUE(f.BarShows("statusDockBarIndicator"));
    EXPECT_TRUE(f.BarShows("statusDockBarTitle"));
    EXPECT_TRUE(f.BarShows("statusDockBarClose"));
    ASSERT_NE(f.ExpandButton(), nullptr);
    EXPECT_TRUE(f.ExpandButton()->isVisibleTo(f.dock));
    EXPECT_EQ(f.ExpandButton()->text(), f.presenter->ToggleAction()->text());
    EXPECT_FALSE(f.BarShows("statusDockBarStatus"))
        << "an idle board has no status word, only the grey dot";

    for (const char* name :
         {"copyToolButton", "saveToolButton", "magnifierToolButton",
          "clearToolButton", "segStatusButton", "segDetailsButton"}) {
      EXPECT_FALSE(f.Shows(name)) << name;
    }
  });
}

TEST(GFUiStatusDockPresenterTest, ExpandBringsBackTheSameResult) {
  OnGuiThread([] {
    PresenterFixture f;
    f.board->SetInfoBoard(QStringLiteral("bad signature"),
                          kINFO_ERROR_CRITICAL);
    const auto text = f.RawText();
    ASSERT_FALSE(text.isEmpty());

    f.Collapse();
    EXPECT_EQ(f.RawText(), text) << "collapse is not clear";
    f.Expand();

    EXPECT_FALSE(f.Collapsed());
    EXPECT_TRUE(f.BoardShown());
    EXPECT_EQ(f.dock->titleBarWidget(), nullptr) << "the native title bar";
    EXPECT_TRUE(f.Shows("stackedWidget"));
    EXPECT_TRUE(f.Shows("copyToolButton"));
    EXPECT_TRUE(f.Shows("segStatusButton"));
    EXPECT_FALSE(f.Shows("line")) << "what the board hides itself stays hidden";
    EXPECT_EQ(f.RawText(), text);
  });
}

TEST(GFUiStatusDockPresenterTest, IndicatorKeepsTheResultColourWhileCollapsed) {
  OnGuiThread([] {
    PresenterFixture f;
    f.board->SetInfoBoard(QStringLiteral("bad"), kINFO_ERROR_CRITICAL);
    f.Collapse();
    EXPECT_TRUE(f.IndicatorStyle().contains(
        f.board->StatusColor(kINFO_ERROR_CRITICAL).name()));
    EXPECT_TRUE(f.BarShows("statusDockBarStatus"));
    EXPECT_EQ(qobject_cast<QLabel*>(f.Bar("statusDockBarStatus"))->text(),
              f.board->StatusTitle(kINFO_ERROR_CRITICAL));

    // A new result expands; collapsing again must still show its colour.
    f.board->SetInfoBoard(QStringLiteral("good"), kINFO_ERROR_OK);
    f.Collapse();
    EXPECT_TRUE(f.IndicatorStyle().contains(
        f.board->StatusColor(kINFO_ERROR_OK).name()));

    // The row mirrors the board while collapsed: a reset greys the dot.
    f.board->SlotReset();
    EXPECT_TRUE(f.Collapsed());
    EXPECT_TRUE(f.IndicatorStyle().contains(
        f.board->IndicatorColor(UI::kINFO_ERROR_NEUTRAL).name()));
    EXPECT_FALSE(f.BarShows("statusDockBarStatus"));
  });
}

TEST(GFUiStatusDockPresenterTest, TheBarsCloseButtonIsTheDocksX) {
  OnGuiThread([] {
    PresenterFixture f;
    f.Collapse();

    qobject_cast<QToolButton*>(f.Bar("statusDockBarClose"))->click();
    PresenterFixture::Settle();

    EXPECT_TRUE(f.dock->isHidden()) << "hide, as the native X does";
    EXPECT_FALSE(f.dock->toggleViewAction()->isChecked());
    EXPECT_TRUE(f.Collapsed()) << "the presentation is only remembered";
  });
}

TEST(GFUiStatusDockPresenterTest, OnlyANewResultExpands) {
  OnGuiThread([] {
    PresenterFixture f;
    int expansions = 0;
    QObject::connect(f.presenter,
                     &StatusDockPresenter::SignalPresentationChanged,
                     f.presenter, [&](StatusDockPresentation p) {
                       if (p == StatusDockPresentation::kExpanded) ++expansions;
                     });

    f.Collapse();
    f.board->SetInfoBoard(QStringLiteral("first"), kINFO_ERROR_OK);
    EXPECT_FALSE(f.Collapsed());
    EXPECT_EQ(expansions, 1);

    // Collapsed again by the user, with the old result still there.
    f.Collapse();
    f.board->UpdateActionButtons();
    f.board->ApplyAppearanceSettings();
    EXPECT_TRUE(f.Collapsed()) << "re-rendered content is not news";

    f.board->SetInfoBoard(QStringLiteral("second"), kINFO_ERROR_CRITICAL);
    EXPECT_FALSE(f.Collapsed());
    EXPECT_EQ(expansions, 2);

    // Already expanded: another result changes nothing.
    f.board->SetInfoBoard(QStringLiteral("third"), kINFO_ERROR_OK);
    EXPECT_EQ(expansions, 2);
  });
}

TEST(GFUiStatusDockPresenterTest, TheXHidesAndDoesNotCollapse) {
  OnGuiThread([] {
    PresenterFixture f;
    f.UserCloses();

    EXPECT_TRUE(f.dock->isHidden());
    EXPECT_FALSE(f.Collapsed()) << "hide is not collapse";
    EXPECT_FALSE(f.dock->toggleViewAction()->isChecked());
  });
}

TEST(GFUiStatusDockPresenterTest, ViewMenuTogglesTheWholeDock) {
  OnGuiThread([] {
    PresenterFixture f;
    f.Collapse();
    auto* view = f.dock->toggleViewAction();
    ASSERT_TRUE(view->isChecked());

    view->trigger();
    PresenterFixture::Settle();
    EXPECT_TRUE(f.dock->isHidden());
    EXPECT_FALSE(view->isChecked());
    EXPECT_TRUE(f.Collapsed()) << "the presentation is remembered";

    view->trigger();
    PresenterFixture::Settle();
    EXPECT_FALSE(f.dock->isHidden());
    EXPECT_TRUE(view->isChecked());
    EXPECT_TRUE(f.Collapsed()) << "and comes back with the dock";
    EXPECT_TRUE(f.BarShows("statusDockBarIndicator"));
  });
}

TEST(GFUiStatusDockPresenterTest, EnteringACompactContextCollapses) {
  OnGuiThread([] {
    PresenterFixture f;
    f.presenter->SetCompactContext(true);
    EXPECT_TRUE(f.presenter->InCompactContext());
    EXPECT_TRUE(f.Collapsed());
    EXPECT_FALSE(f.dock->isHidden());
  });
}

TEST(GFUiStatusDockPresenterTest, AHiddenDockIsNotTurnedIntoACollapsedOne) {
  OnGuiThread([] {
    PresenterFixture f;
    f.UserCloses();

    f.presenter->SetCompactContext(true);

    EXPECT_TRUE(f.dock->isHidden());
    EXPECT_FALSE(f.dock->toggleViewAction()->isChecked());
    EXPECT_FALSE(f.Collapsed());
  });
}

TEST(GFUiStatusDockPresenterTest, CompactToCompactKeepsTheUsersChoice) {
  OnGuiThread([] {
    PresenterFixture f;
    f.presenter->SetCompactContext(true);  // ordinary -> Email A
    f.Expand();                            // the user opens it

    f.presenter->SetCompactContext(true);  // Email A -> Email B

    EXPECT_FALSE(f.Collapsed()) << "no re-collapse inside the context";
  });
}

TEST(GFUiStatusDockPresenterTest, LeavingTheCompactContextRestoresThePrevious) {
  OnGuiThread([] {
    PresenterFixture f;

    // Expanded before; the user expanded and collapsed again inside.
    f.presenter->SetCompactContext(true);
    f.Expand();
    f.Collapse();
    f.presenter->SetCompactContext(false);
    EXPECT_FALSE(f.presenter->InCompactContext());
    EXPECT_FALSE(f.Collapsed());

    // Collapsed before; the user expanded inside.
    f.Collapse();
    f.presenter->SetCompactContext(true);
    f.Expand();
    f.presenter->SetCompactContext(false);
    EXPECT_TRUE(f.Collapsed());
  });
}

TEST(GFUiStatusDockPresenterTest, ResetChangesNeitherAxis) {
  OnGuiThread([] {
    PresenterFixture f;
    f.board->SetInfoBoard(QStringLiteral("x"), kINFO_ERROR_OK);

    f.board->SlotReset();
    EXPECT_FALSE(f.Collapsed());
    EXPECT_FALSE(f.dock->isHidden());

    f.Collapse();
    f.board->SlotReset();
    EXPECT_TRUE(f.Collapsed());
    EXPECT_FALSE(f.dock->isHidden());

    f.UserCloses();
    f.board->SlotReset();
    EXPECT_TRUE(f.Collapsed());
    EXPECT_TRUE(f.dock->isHidden());
  });
}

TEST(GFUiStatusDockPresenterTest, SavedStateCarriesTheExpandedHeight) {
  OnGuiThread([] {
    QByteArray state;
    int bar = 0;
    {
      PresenterFixture f;
      f.Collapse();
      bar = f.dock->height();
      const StatusDockPresenter::ExpandedForSaveGuard guard(f.presenter);
      PresenterFixture::Settle();
      state = f.window.saveState();
    }

    PresenterFixture g;
    g.window.resizeDocks({g.dock}, {140}, Qt::Vertical);
    PresenterFixture::Settle();
    ASSERT_TRUE(g.window.restoreState(state));
    PresenterFixture::Settle();

    EXPECT_FALSE(g.dock->isHidden());
    EXPECT_GT(g.dock->height(), bar) << "not the thin bar's height";
  });
}

TEST(GFUiStatusDockPresenterTest, SavedStateKeepsAHiddenDockHidden) {
  OnGuiThread([] {
    QByteArray state;
    {
      PresenterFixture f;
      f.Collapse();
      f.UserCloses();
      const StatusDockPresenter::ExpandedForSaveGuard guard(f.presenter);
      state = f.window.saveState();
    }

    PresenterFixture g;
    ASSERT_TRUE(g.window.restoreState(state));
    PresenterFixture::Settle();
    EXPECT_TRUE(g.dock->isHidden());
  });
}

TEST(GFUiStatusDockPresenterTest, ACancelledCloseKeepsTheBar) {
  OnGuiThread([] {
    PresenterFixture f;
    f.Collapse();
    const int bar = f.dock->height();

    {
      const StatusDockPresenter::ExpandedForSaveGuard guard(f.presenter);
      PresenterFixture::Settle();
      EXPECT_FALSE(f.BoardShown())
          << "the save changes geometry, not what is shown";
    }  // the close was cancelled: the window lives on
    PresenterFixture::Settle();

    EXPECT_TRUE(f.Collapsed());
    EXPECT_FALSE(f.BoardShown());
    EXPECT_EQ(f.dock->titleBarWidget(), f.presenter->CompactBar());
    EXPECT_EQ(f.dock->maximumHeight(), f.dock->minimumHeight());
    EXPECT_EQ(f.dock->height(), bar);
  });
}

TEST(GFUiStatusDockPresenterTest, PagesDoNotPreferCompactUnlessTheyOptIn) {
  OnGuiThread([] {
    auto* edit = new UI::TextEdit(nullptr);
    edit->SlotNewTab();
    auto* page = edit->CurPageTextEdit();
    ASSERT_NE(page, nullptr);
    EXPECT_FALSE(page->PrefersCompactStatusDock());
    delete edit;
  });
}

// ------------------------------------------------------------ transitions

TEST(GFUiStatusDockPresenterTest, AnimatedCollapseEndsAsTheBar) {
  OnGuiThread([] {
    PresenterFixture f;
    f.presenter->SetTransitionDuration(StatusDockPresenter::kTransitionMs);
    const int expanded = f.dock->height();

    f.presenter->Collapse();
    EXPECT_TRUE(f.presenter->IsTransitioning());
    EXPECT_TRUE(f.Collapsed()) << "the target is the state at once";
    EXPECT_FALSE(f.BoardShown()) << "content goes first, never squeezed";
    EXPECT_EQ(f.dock->titleBarWidget(), f.presenter->CompactBar());

    f.PlayOut();
    EXPECT_FALSE(f.presenter->IsTransitioning());
    EXPECT_LT(f.dock->height(), expanded);
    EXPECT_EQ(f.dock->minimumHeight(), f.dock->maximumHeight())
        << "the bar keeps its compact constraint";
    EXPECT_EQ(f.dock->height(), f.dock->minimumHeight());
  });
}

TEST(GFUiStatusDockPresenterTest, AnimatedExpandRestoresTheRememberedPanel) {
  OnGuiThread([] {
    PresenterFixture f;
    const int expanded = f.dock->height();
    const int min_height = f.dock->minimumHeight();
    f.Collapse();  // instant
    f.presenter->SetTransitionDuration(StatusDockPresenter::kTransitionMs);

    f.presenter->Expand();
    EXPECT_TRUE(f.presenter->IsTransitioning());
    EXPECT_FALSE(f.BoardShown()) << "not revealed into a sliver";

    f.PlayOut();
    EXPECT_FALSE(f.Collapsed());
    EXPECT_TRUE(f.BoardShown());
    EXPECT_EQ(f.dock->titleBarWidget(), nullptr);
    EXPECT_EQ(f.dock->minimumHeight(), min_height);
    EXPECT_TRUE(Unconstrained(f.dock->maximumHeight()));
    EXPECT_EQ(f.dock->height(), expanded) << "the remembered height";
  });
}

TEST(GFUiStatusDockPresenterTest, ACollapseMidExpansionTurnsAround) {
  OnGuiThread([] {
    PresenterFixture f;
    f.Collapse();
    const int bar = f.dock->height();
    f.presenter->SetTransitionDuration(StatusDockPresenter::kTransitionMs);

    f.presenter->Expand();
    QThread::msleep(40);
    QCoreApplication::processEvents();
    f.presenter->Collapse();

    EXPECT_FALSE(f.PlayOut())
        << "the expansion was never completed before turning back";
    EXPECT_TRUE(f.Collapsed());
    EXPECT_FALSE(f.BoardShown());
    EXPECT_EQ(f.dock->height(), bar);
    EXPECT_EQ(f.dock->minimumHeight(), f.dock->maximumHeight());
  });
}

TEST(GFUiStatusDockPresenterTest, AnExpandMidCollapseTurnsAround) {
  OnGuiThread([] {
    PresenterFixture f;
    const int expanded = f.dock->height();
    f.presenter->SetTransitionDuration(StatusDockPresenter::kTransitionMs);

    f.presenter->Collapse();
    QThread::msleep(40);
    QCoreApplication::processEvents();
    f.presenter->Expand();
    f.PlayOut();

    EXPECT_FALSE(f.Collapsed());
    EXPECT_TRUE(f.BoardShown());
    EXPECT_TRUE(Unconstrained(f.dock->maximumHeight()));
    EXPECT_EQ(f.dock->height(), expanded)
        << "the height from before the collapse, not from halfway down";
  });
}

TEST(GFUiStatusDockPresenterTest, ANewResultExpandsSmoothlyToo) {
  OnGuiThread([] {
    PresenterFixture f;
    f.Collapse();
    f.presenter->SetTransitionDuration(StatusDockPresenter::kTransitionMs);

    f.board->SetInfoBoard(QStringLiteral("verified"), kINFO_ERROR_OK);
    EXPECT_TRUE(f.presenter->IsTransitioning());
    f.PlayOut();
    EXPECT_FALSE(f.Collapsed());
    EXPECT_TRUE(f.BoardShown());
  });
}

TEST(GFUiStatusDockPresenterTest, AnimatedAndInstantEndInTheSameLayout) {
  OnGuiThread([] {
    struct Final {
      int height, min, max;
      bool collapsed, board;
    };
    const auto run = [](int ms) {
      PresenterFixture f;
      f.presenter->SetTransitionDuration(ms);
      f.presenter->Collapse();
      f.PlayOut();
      const Final collapsed{f.dock->height(), f.dock->minimumHeight(),
                            f.dock->maximumHeight(), f.Collapsed(),
                            f.BoardShown()};
      f.presenter->Expand();
      f.PlayOut();
      const Final expanded{f.dock->height(), f.dock->minimumHeight(),
                           f.dock->maximumHeight(), f.Collapsed(),
                           f.BoardShown()};
      return std::pair{collapsed, expanded};
    };
    const auto a = run(StatusDockPresenter::kTransitionMs);
    const auto b = run(0);
    for (const auto& [x, y] :
         {std::pair{a.first, b.first}, std::pair{a.second, b.second}}) {
      EXPECT_EQ(x.height, y.height);
      EXPECT_EQ(x.min, y.min);
      EXPECT_EQ(x.max, y.max);
      EXPECT_EQ(x.collapsed, y.collapsed);
      EXPECT_EQ(x.board, y.board);
    }
  });
}

TEST(GFUiStatusDockPresenterTest, SavingMidTransitionSavesARealState) {
  OnGuiThread([] {
    PresenterFixture f;
    f.presenter->SetTransitionDuration(StatusDockPresenter::kTransitionMs);
    f.presenter->Collapse();
    {
      const StatusDockPresenter::ExpandedForSaveGuard guard(f.presenter);
      EXPECT_FALSE(f.presenter->IsTransitioning()) << "finished, not frozen";
      EXPECT_TRUE(Unconstrained(f.dock->maximumHeight()))
          << "the expanded geometry is what gets saved";
    }
    EXPECT_TRUE(f.Collapsed());
    EXPECT_EQ(f.dock->minimumHeight(), f.dock->maximumHeight());
  });
}

TEST(GFUiStatusDockPresenterTest, DestroyingMidTransitionLeavesNothingRunning) {
  OnGuiThread([] {
    {
      PresenterFixture f;
      f.presenter->SetTransitionDuration(StatusDockPresenter::kTransitionMs);
      f.presenter->Collapse();
      ASSERT_TRUE(f.presenter->IsTransitioning());
    }  // window, dock, board and presenter go, mid-flight
    // Frames that were due must find nothing to write to.
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < 250) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
      QThread::msleep(5);
    }
    SUCCEED();
  });
}

// ------------------------------------------------------------ startup

/// A layout saved with the panel at @p height, as the last run left it.
auto SavedLayout(int height, int* actual) -> QByteArray {
  PresenterFixture f;
  f.window.resizeDocks({f.dock}, {height}, Qt::Vertical);
  PresenterFixture::Settle();
  *actual = f.dock->height();
  return f.window.saveState();
}

TEST(GFUiStatusDockPresenterTest, ARestoredPanelKeepsItsSavedHeight) {
  OnGuiThread([] {
    int saved = 0;
    const auto state = SavedLayout(330, &saved);
    ASSERT_GT(saved, 200);

    PresenterFixture f(false);
    ASSERT_TRUE(f.window.restoreState(state));
    // Restoring the tabs: an e-mail tab is current for a moment, then the
    // text tab the user left in front.
    f.presenter->SetCompactContext(true);
    f.presenter->SetCompactContext(false);
    f.window.show();
    PresenterFixture::Settle();

    EXPECT_FALSE(f.Collapsed());
    EXPECT_NEAR(f.dock->height(), saved, 2)
        << "the height the user left it at, not the minimum";
  });
}

TEST(GFUiStatusDockPresenterTest, AStartupInCompactExpandsToTheSavedHeight) {
  OnGuiThread([] {
    int saved = 0;
    const auto state = SavedLayout(330, &saved);

    PresenterFixture f(false);
    ASSERT_TRUE(f.window.restoreState(state));
    f.presenter->SetCompactContext(true);  // the front tab is an e-mail
    f.window.show();
    PresenterFixture::Settle();
    EXPECT_TRUE(f.Collapsed());
    EXPECT_EQ(f.dock->titleBarWidget(), f.presenter->CompactBar());
    EXPECT_LT(f.dock->height(), saved);

    f.Expand();
    EXPECT_NEAR(f.dock->height(), saved, 2);
  });
}

TEST(GFUiStatusDockPresenterTest, AnExpandWhileTheWindowIsHiddenLandsOnShow) {
  OnGuiThread([] {
    PresenterFixture f;
    const int expanded = f.dock->height();
    f.Collapse();
    f.window.hide();  // to the tray, say

    f.presenter->Expand();
    f.window.show();
    PresenterFixture::Settle();

    EXPECT_FALSE(f.Collapsed());
    EXPECT_TRUE(f.BoardShown());
    EXPECT_TRUE(Unconstrained(f.dock->maximumHeight()));
    EXPECT_NEAR(f.dock->height(), expanded, 2);
  });
}

TEST(GFUiStatusDockPresenterTest, TheSavedHeightIsTheOneTheUserDragged) {
  OnGuiThread([] {
    QByteArray state;
    int dragged = 0;
    {
      PresenterFixture f;
      // Collapsed and expanded once, so an expanded height is remembered...
      f.Collapse();
      f.Expand();
      // ...and then the user drags the splitter somewhere else entirely.
      f.window.resizeDocks({f.dock}, {420}, Qt::Vertical);
      PresenterFixture::Settle();
      dragged = f.dock->height();
      ASSERT_GT(dragged, 300);

      const StatusDockPresenter::ExpandedForSaveGuard guard(f.presenter);
      EXPECT_EQ(f.dock->height(), dragged) << "saving must not resize";
      state = f.window.saveState();
    }

    PresenterFixture g;
    ASSERT_TRUE(g.window.restoreState(state));
    PresenterFixture::Settle();
    EXPECT_NEAR(g.dock->height(), dragged, 2);
  });
}

TEST(GFUiStatusDockPresenterTest,
     QuittingWhileCollapsedSavesTheExpandedHeight) {
  OnGuiThread([] {
    QByteArray state;
    int expanded = 0;
    {
      PresenterFixture f;
      f.window.resizeDocks({f.dock}, {380}, Qt::Vertical);
      PresenterFixture::Settle();
      expanded = f.dock->height();
      f.Collapse();  // the e-mail tab in front when the user quits

      // What closeEvent does: guard, then save, with no event loop between.
      const StatusDockPresenter::ExpandedForSaveGuard guard(f.presenter);
      state = f.window.saveState();
    }

    PresenterFixture g;
    ASSERT_TRUE(g.window.restoreState(state));
    PresenterFixture::Settle();
    EXPECT_NEAR(g.dock->height(), expanded, 2)
        << "the panel's height, not the minimum the bar's content allows";
  });
}

TEST(GFUiStatusDockPresenterTest, ATallFrontTabDoesNotShrinkTheSavedPanel) {
  OnGuiThread([] {
    // Its own settings name, emptied first, so no other run leaks in.
    const auto name = QStringLiteral("test_status_dock_tall_front_tab");
    {
      SettingsObject so(name);
      so["expanded_height"] = 0;
      so.Store(so);
    }

    QByteArray state;
    int saved = 0;
    {
      PresenterFixture f(true, name);
      f.window.resizeDocks({f.dock}, {330}, Qt::Vertical);
      PresenterFixture::Settle();
      saved = f.dock->height();
      // As closeEvent saves: under the guard.
      const StatusDockPresenter::ExpandedForSaveGuard guard(f.presenter);
      state = f.window.saveState();
    }
    ASSERT_GT(saved, 200);

    PresenterFixture f(false, name);
    // The e-mail tab in front at startup needs most of the height (a locked
    // message's panel, say), so the restored dock is squeezed when shown.
    auto* central = f.window.centralWidget();
    central->setMinimumHeight(600);
    ASSERT_TRUE(f.window.restoreState(state));
    f.presenter->SetCompactContext(true);
    f.window.show();
    PresenterFixture::Settle();
    EXPECT_TRUE(f.Collapsed());

    // Then the text tab: nothing tall in front any more.
    central->setMinimumHeight(0);
    f.presenter->SetCompactContext(false);
    PresenterFixture::Settle();

    EXPECT_FALSE(f.Collapsed());
    EXPECT_NEAR(f.dock->height(), saved, 2)
        << "the height the user saved, not what the tall tab left room for";
  });
}

/// Sets what a previous run remembered under @p name.
void Remember(const QString& name, int height) {
  SettingsObject so(name);
  so["expanded_height"] = height;
  so.Store(so);
}

auto Remembered(const QString& name) -> int {
  SettingsObject so(name);
  return so["expanded_height"].toInt();
}

TEST(GFUiStatusDockPresenterTest, APanelStuckAtItsMinimumIsHealedOnStart) {
  OnGuiThread([] {
    // What an earlier build left behind: the layout AND the remembered height
    // both at the dock's minimum, so every start restored it there.
    const auto name = QStringLiteral("test_status_dock_stuck_minimum");
    QByteArray state;
    int minimum = 0;
    {
      PresenterFixture f;
      f.window.resizeDocks({f.dock}, {1}, Qt::Vertical);
      PresenterFixture::Settle();
      minimum = f.dock->height();
      ASSERT_LE(minimum, f.dock->minimumHeight() + 2);
      state = f.window.saveState();
    }
    Remember(name, minimum);

    PresenterFixture f(false, name);
    ASSERT_TRUE(f.window.restoreState(state));
    f.window.show();
    // The heal waits for the first layout pass, then lays out once more.
    for (int i = 0; i < 5; ++i) PresenterFixture::Settle();

    EXPECT_GT(f.dock->height(), minimum + 20) << "healed, not restored stuck";
    EXPECT_NEAR(f.dock->height(), f.window.height() * 30 / 100, 4)
        << "the default layout's share of the window";
  });
}

TEST(GFUiStatusDockPresenterTest, AHeightTheDockWasPushedToIsNeverRemembered) {
  OnGuiThread([] {
    const auto name = QStringLiteral("test_status_dock_pushed");
    Remember(name, 0);

    PresenterFixture f(true, name);
    f.window.resizeDocks({f.dock}, {300}, Qt::Vertical);
    PresenterFixture::Settle();
    const int chosen = f.dock->height();
    {
      const StatusDockPresenter::ExpandedForSaveGuard guard(f.presenter);
    }
    ASSERT_EQ(Remembered(name), chosen);

    // A tall tab comes to the front and pushes the dock to its minimum.
    f.window.centralWidget()->setMinimumHeight(f.window.height());
    PresenterFixture::Settle();
    ASSERT_LE(f.dock->height(), f.dock->minimumHeight() + 2);

    // Collapsing and saving now must not learn or store that minimum.
    f.Collapse();
    {
      const StatusDockPresenter::ExpandedForSaveGuard guard(f.presenter);
    }
    EXPECT_EQ(Remembered(name), chosen);

    // And once the tall tab is gone, expanding goes back to the real height.
    f.window.centralWidget()->setMinimumHeight(0);
    PresenterFixture::Settle();
    f.Expand();
    EXPECT_NEAR(f.dock->height(), chosen, 2);
  });
}

}  // namespace GpgFrontend::Test
