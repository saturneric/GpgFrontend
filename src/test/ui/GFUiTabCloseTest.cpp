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
// Closing editor tabs end to end: the one close path, driven on the GUI thread
// with the dialogs replaced, over plain-text tabs and a module's own document.
//
// Nothing may be closed or wiped before every question has been answered and
// every save has succeeded; that is a property of the whole path, which is why
// these tests build a real TextEdit rather than checking a helper.

#include <gtest/gtest.h>

#include <QTemporaryDir>

#include "GpgFrontendTest.h"
#include "host/SdkTestContext.h"
#include "sdk/GFSDKUI.h"
#include "ui/function/TabManagement.h"
#include "ui/widgets/EditorEmptyView.h"
#include "ui/widgets/OpenTabsPopup.h"
#include "ui/widgets/PlainTextEditorPage.h"
#include "ui/widgets/TextEdit.h"
#include "ui/widgets/TextEditTabWidget.h"

namespace GpgFrontend::Test {

namespace {

constexpr auto kOwner = "com.example.tabclose.owner";

template <typename F>
void OnGui(F&& work) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), std::forward<F>(work),
                            Qt::BlockingQueuedConnection);
}

/// The user's answers, given without a dialog, and what was asked.
struct Answers {
  UI::SaveChoice one = UI::SaveChoice::kDISCARD;
  std::optional<QSet<quint64>> many = QSet<quint64>{};
  QString save_path;
  std::function<void()> while_asking;

  int asked_one = 0;
  int asked_many = 0;
  int save_failed = 0;
  QList<quint64> listed;
};

class FakePrompter : public UI::TabClosePrompter {
 public:
  explicit FakePrompter(std::shared_ptr<Answers> a) : a_(std::move(a)) {}

  auto AskSaveOne(QWidget*, const UI::UnsavedTab& tab)
      -> UI::SaveChoice override {
    ++a_->asked_one;
    a_->listed = {tab.id};
    if (a_->while_asking) a_->while_asking();
    return a_->one;
  }

  auto AskSaveMany(QWidget*, const QList<UI::UnsavedTab>& tabs)
      -> std::optional<QSet<quint64>> override {
    ++a_->asked_many;
    a_->listed.clear();
    for (const auto& t : tabs) a_->listed.append(t.id);
    if (a_->while_asking) a_->while_asking();
    return a_->many;
  }

  auto ChooseSavePath(QWidget*, const QString&, const QString&)
      -> QString override {
    return a_->save_path;
  }

  void SaveFailed(QWidget*, const QString&, const QString&) override {
    ++a_->save_failed;
  }

 private:
  std::shared_ptr<Answers> a_;
};

/// What the module's document view was asked, and how it answers.
struct ViewLog {
  int decided_save = 0;
  int decided_discard = 0;
  int requested = 0;
  int wiped = 0;
  /// close_decided(save): 0 closes; 1 keeps (the save left changes unsaved).
  int save_result = 0;
};
ViewLog g_view;

auto ViewCreate(void*, uint64_t, GFBufferView) -> void* {
  return new QWidget();
}
void ViewLoad(void*, uint64_t, GFBufferView) {}
auto ViewCloseRequested(void*, uint64_t) -> int {
  ++g_view.requested;
  return 0;
}
auto ViewCloseDecided(void*, uint64_t, int save) -> int {
  if (save != 0) {
    ++g_view.decided_save;
    return g_view.save_result;
  }
  ++g_view.decided_discard;
  return 0;
}
void ViewWipe(void*, uint64_t) { ++g_view.wiped; }

auto ViewOps() -> GFDocumentWidgetOps {
  GFDocumentWidgetOps ops{};
  ops.struct_size = sizeof(ops);
  ops.load = &ViewLoad;
  ops.wipe_content = &ViewWipe;
  ops.close_requested = &ViewCloseRequested;
  ops.close_decided = &ViewCloseDecided;
  return ops;
}

/// A TextEdit on the GUI thread, its prompter replaced, and the fake module
/// document view registered.
class TabCloseTest : public ::testing::Test {
 protected:
  void SetUp() override {
    g_view = {};
    answers_ = std::make_shared<Answers>();
    owner_ = std::make_unique<SdkTestContext>(
        kOwner, GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
    static const auto kOps = ViewOps();
    GFNativeWidgetSpec spec{};
    spec.struct_size = sizeof(spec);
    spec.name = "note";
    spec.kind = GF_NATIVE_DOCUMENT;
    spec.create = &ViewCreate;
    spec.document = &kOps;
    ASSERT_EQ(GFNativeWidgetRegister((*owner_)(), &spec), 0);

    OnGui([this] {
      edit_ = new UI::TextEdit(nullptr);
      edit_->SetTabClosePrompter(std::make_unique<FakePrompter>(answers_));
      // Only what each test opens: the default workspace tab goes.
      Tabs()->clear();
    });
  }

  void TearDown() override {
    OnGui([this] { delete edit_; });
    GFNativeWidgetUnregister((*owner_)(), "note");
    owner_.reset();
  }

  auto Tabs() -> UI::TextEditTabWidget* { return edit_->TabWidget(); }

  /// A plain-text tab holding @p text, modified unless @p clean.
  auto Plain(const QString& title, const QString& text, bool clean = false,
             const QString& path = {}) -> quint64 {
    Tabs()->SlotNewTabWithContent(title, text);
    auto* page = Tabs()->CurTextPage();
    if (!path.isEmpty()) page->SetFilePath(path);
    page->GetTextPage()->document()->setModified(!clean);
    Tabs()->RefreshTabLabels();
    return UI::TextEditTabWidget::TabIdOf(page);
  }

  /// A document the module persists itself, modified unless @p clean.
  auto Note(const QString& title, bool clean = false) -> quint64 {
    auto* page = qobject_cast<UI::PlainTextEditorPage*>(
        edit_->SlotNewCustomTab(u"tabclose_note"_s, title, QIcon(), {}));
    EXPECT_TRUE(page->MountNativeView(QString(kOwner) + u".note"_s));
    page->SetModulePersisted(true);
    page->GetTextPage()->document()->setModified(!clean);
    return UI::TextEditTabWidget::TabIdOf(page);
  }

  auto Page(quint64 id) -> UI::PlainTextEditorPage* {
    return qobject_cast<UI::PlainTextEditorPage*>(Tabs()->PageForTab(id));
  }

  auto TextOf(quint64 id) -> QString {
    auto* p = Page(id);
    return p == nullptr ? QString() : p->GetTextPage()->toPlainText();
  }

  UI::TextEdit* edit_ = nullptr;
  std::shared_ptr<Answers> answers_;
  std::unique_ptr<SdkTestContext> owner_;
};

}  // namespace

TEST_F(TabCloseTest, CancelLeavesEveryTargetOpenAndUntouched) {
  OnGui([this] {
    const auto a = Plain(u"a.txt"_s, u"alpha"_s);
    const auto b = Plain(u"b.txt"_s, u"beta"_s);
    const auto n = Note(u"n.textpack"_s);
    const auto c = Plain(u"c.txt"_s, u"gamma"_s, true);
    answers_->many = std::nullopt;

    const auto outcome = edit_->CloseTabs({a, b, n, c});

    EXPECT_TRUE(outcome.cancelled);
    EXPECT_TRUE(outcome.closed.isEmpty());
    EXPECT_EQ(answers_->asked_many, 1);
    EXPECT_EQ(answers_->listed, (QList<quint64>{a, b, n}))
        << "the unsaved ones, in the order of the strip";
    EXPECT_EQ(TextOf(a), u"alpha"_s);
    EXPECT_EQ(TextOf(b), u"beta"_s);
    EXPECT_NE(Page(n), nullptr);
    EXPECT_EQ(TextOf(c), u"gamma"_s) << "an unmodified target stays too";
  });
  EXPECT_EQ(g_view.decided_save + g_view.decided_discard, 0)
      << "the module is not asked to do anything before the user decides";
  EXPECT_EQ(g_view.requested, 0);
  EXPECT_EQ(g_view.wiped, 0);
}

TEST_F(TabCloseTest, AMixedBulkCloseAsksOnceAndCarriesOutEachAnswer) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto path = dir.filePath(u"a.txt"_s);

  OnGui([&] {
    const auto a = Plain(u"a.txt"_s, u"saved text"_s, false, path);
    const auto b = Plain(u"b.txt"_s, u"thrown away"_s);
    const auto n = Note(u"n.textpack"_s);
    const auto c = Plain(u"c.txt"_s, u"clean"_s, true);
    answers_->many = QSet<quint64>{a, n};

    const auto outcome = edit_->CloseTabs({a, b, n, c});

    EXPECT_FALSE(outcome.cancelled);
    EXPECT_TRUE(outcome.kept.isEmpty());
    EXPECT_EQ(QSet<quint64>(outcome.closed.cbegin(), outcome.closed.cend()),
              (QSet<quint64>{a, b, n, c}));
    EXPECT_EQ(answers_->asked_many, 1);
    EXPECT_EQ(answers_->asked_one, 0);
    EXPECT_EQ(Tabs()->count(), 0);
  });

  QFile f(path);
  ASSERT_TRUE(f.open(QIODevice::ReadOnly));
  EXPECT_EQ(f.readAll(), QByteArray("saved text"));
  EXPECT_EQ(g_view.decided_save, 1);
  EXPECT_EQ(g_view.decided_discard, 0);
  EXPECT_EQ(g_view.requested, 0)
      << "a view that takes the Host's decision is not asked again";
}

TEST_F(TabCloseTest, AFailedSaveKeepsThatTabWithItsContent) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  // A folder that does not exist: the write fails.
  const auto path = dir.filePath(u"missing/a.txt"_s);

  OnGui([&] {
    const auto a = Plain(u"a.txt"_s, u"precious"_s, false, path);
    const auto c = Plain(u"c.txt"_s, u"clean"_s, true);
    answers_->one = UI::SaveChoice::kSAVE;

    const auto outcome = edit_->CloseTabs({a, c});

    EXPECT_FALSE(outcome.cancelled);
    EXPECT_EQ(outcome.kept, QList<quint64>{a});
    EXPECT_EQ(outcome.closed, QList<quint64>{c});
    EXPECT_EQ(answers_->save_failed, 1);
    EXPECT_EQ(TextOf(a), u"precious"_s);
    EXPECT_TRUE(UI::TextEditTabWidget::IsPageModified(Page(a)));
  });
}

TEST_F(TabCloseTest, ACancelledSaveAsKeepsAnUntitledTab) {
  OnGui([&] {
    const auto a = Plain(u"untitled9.txt"_s, u"draft"_s);
    answers_->one = UI::SaveChoice::kSAVE;
    answers_->save_path.clear();  // the user cancels the file dialog

    const auto outcome = edit_->CloseTabs({a});

    EXPECT_EQ(outcome.kept, QList<quint64>{a});
    EXPECT_EQ(TextOf(a), u"draft"_s);
  });
}

TEST_F(TabCloseTest, ANoteSavedFromAnOlderCiphertextStaysOpenAndModified) {
  // Its encrypted snapshot is older than its text: saving writes that
  // snapshot, and the newer edits are still unsaved.
  g_view.save_result = 1;

  OnGui([&] {
    const auto n = Note(u"n.textpack"_s);
    const auto c = Plain(u"c.txt"_s, u"clean"_s, true);

    // Close Unmodified never offers it.
    QList<bool> modified;
    for (const auto id : Tabs()->TabIds()) {
      modified.append(
          UI::TextEditTabWidget::IsPageModified(Tabs()->PageForTab(id)));
    }
    const auto unmodified =
        UI::TabsToClose(UI::TabCloseScope::kUNMODIFIED, 0, modified);
    ASSERT_EQ(unmodified.size(), 1);
    EXPECT_EQ(Tabs()->TabIds().at(unmodified.front()), c);

    answers_->one = UI::SaveChoice::kSAVE;
    const auto outcome = edit_->CloseTabs({n, c});

    EXPECT_EQ(outcome.kept, QList<quint64>{n});
    EXPECT_EQ(outcome.closed, QList<quint64>{c});
    ASSERT_NE(Page(n), nullptr);
    EXPECT_TRUE(UI::TextEditTabWidget::IsPageModified(Page(n)));
  });
  EXPECT_EQ(g_view.decided_save, 1);
  EXPECT_EQ(g_view.wiped, 0) << "kept, so never wiped";
}

TEST_F(TabCloseTest, ATabThatDisappearsDuringTheQuestionIsLeftAlone) {
  OnGui([&] {
    const auto a = Plain(u"a.txt"_s, u"one"_s);
    const auto b = Plain(u"b.txt"_s, u"two"_s);
    const auto c = Plain(u"c.txt"_s, u"three"_s, true);
    answers_->while_asking = [&] {
      auto* gone = Tabs()->PageForTab(b);
      Tabs()->removeTab(Tabs()->indexOf(gone));
      delete gone;
    };

    const auto outcome = edit_->CloseTabs({a, b, c});

    EXPECT_FALSE(outcome.cancelled);
    EXPECT_EQ(QSet<quint64>(outcome.closed.cbegin(), outcome.closed.cend()),
              (QSet<quint64>{a, c}));
    EXPECT_EQ(Tabs()->count(), 0);
  });
}

TEST_F(TabCloseTest, ClosingABackgroundTabKeepsTheActiveDocument) {
  OnGui([&] {
    const auto a = Plain(u"a.txt"_s, u"a"_s, true);
    const auto b = Plain(u"b.txt"_s, u"b"_s, true);
    const auto c = Plain(u"c.txt"_s, u"c"_s, true);
    Tabs()->setCurrentWidget(Tabs()->PageForTab(b));

    edit_->CloseTabs({a, c});

    EXPECT_EQ(UI::TextEditTabWidget::TabIdOf(Tabs()->currentWidget()), b);
  });
}

TEST_F(TabCloseTest, ClosingTheActiveTabShowsTheOneUsedBeforeIt) {
  OnGui([&] {
    const auto a = Plain(u"a.txt"_s, u"a"_s, true);
    const auto b = Plain(u"b.txt"_s, u"b"_s, true);
    const auto c = Plain(u"c.txt"_s, u"c"_s, true);
    Q_UNUSED(b);
    // Used: c, then a; b is c's visual neighbour, which Qt would show.
    Tabs()->setCurrentWidget(Tabs()->PageForTab(a));
    Tabs()->setCurrentWidget(Tabs()->PageForTab(c));

    edit_->CloseTabs({c});

    EXPECT_EQ(UI::TextEditTabWidget::TabIdOf(Tabs()->currentWidget()), a);
  });
}

TEST_F(TabCloseTest, AModuleDocumentIsAskedAboutInTheHostsDialog) {
  OnGui([&] {
    const auto n = Note(u"n.textpack"_s);
    answers_->one = UI::SaveChoice::kDISCARD;

    const auto outcome = edit_->CloseTabs({n});

    EXPECT_EQ(outcome.closed, QList<quint64>{n});
    EXPECT_EQ(answers_->asked_one, 1);
  });
  EXPECT_EQ(g_view.decided_discard, 1);
  // Wiped by the close itself and again by the page's close event, as before.
  EXPECT_GE(g_view.wiped, 1) << "wiped once it is really closing";
}

TEST_F(TabCloseTest, QuittingWithAModifiedNoteAndCancelKeepsEverything) {
  OnGui([&] {
    const auto n = Note(u"n.textpack"_s);
    const auto a = Plain(u"a.txt"_s, u"alpha"_s, true);
    answers_->one = UI::SaveChoice::kCANCEL;

    EXPECT_FALSE(edit_->MaybeSaveAnyTab());
    EXPECT_NE(Page(n), nullptr);
    EXPECT_EQ(TextOf(a), u"alpha"_s);
  });
  EXPECT_EQ(g_view.decided_save + g_view.decided_discard, 0);
}

TEST_F(TabCloseTest, ACtrlTabSessionStepsOverATabClosedDuringIt) {
  OnGui([&] {
    const auto a = Plain(u"a.txt"_s, u"a"_s, true);
    const auto b = Plain(u"b.txt"_s, u"b"_s, true);
    const auto c = Plain(u"c.txt"_s, u"c"_s, true);
    // Most recent first: c, b, a.

    Tabs()->StepSwitchSession(1);
    EXPECT_EQ(UI::TextEditTabWidget::TabIdOf(Tabs()->currentWidget()), b);
    EXPECT_TRUE(Tabs()->SwitchSessionActive());

    edit_->CloseTabs({a});
    Tabs()->StepSwitchSession(1);
    EXPECT_EQ(UI::TextEditTabWidget::TabIdOf(Tabs()->currentWidget()), c)
        << "a is gone; the walk wraps to c";

    Tabs()->EndSwitchSession();
    EXPECT_EQ(Tabs()->MruTabIds().front(), c);

    // A quick tap: to the previous document, which becomes the most recent.
    Tabs()->StepSwitchSession(1);
    Tabs()->EndSwitchSession();
    EXPECT_EQ(UI::TextEditTabWidget::TabIdOf(Tabs()->currentWidget()), b);
    EXPECT_EQ(Tabs()->MruTabIds().front(), b);
  });
}

TEST_F(TabCloseTest, TabIdsFollowTheTabsWhenDragged) {
  OnGui([&] {
    const auto a = Plain(u"a.txt"_s, u"a"_s, true);
    const auto b = Plain(u"b.txt"_s, u"b"_s, true);
    const auto c = Plain(u"c.txt"_s, u"c"_s, true);
    Tabs()->tabBar()->moveTab(0, 2);
    EXPECT_EQ(Tabs()->TabIds(), (QList<quint64>{b, c, a}));

    // "Close to the Right" of b, in the order the strip now shows.
    const auto right =
        UI::TabsToClose(UI::TabCloseScope::kRIGHT, 0, {false, false, false});
    QList<quint64> ids;
    for (const auto i : right) ids.append(Tabs()->TabIds().at(i));
    EXPECT_EQ(ids, (QList<quint64>{c, a}));
  });
}

TEST_F(TabCloseTest, TitlesKeepTheirOwnMarksAndSameNamesAreToldApart) {
  OnGui([&] {
    const auto star = Plain(u"*draft"_s, u"x"_s);
    const auto dot = Plain(u"● notes"_s, u"y"_s);
    auto* star_page = Tabs()->PageForTab(star);
    EXPECT_EQ(UI::TextEditTabWidget::TabTitleOf(star_page), u"*draft"_s);
    EXPECT_EQ(Tabs()->TabLabel(Tabs()->indexOf(star_page)), u"*draft"_s)
        << "modified, and still its own title";
    EXPECT_EQ(UI::TextEditTabWidget::TabTitleOf(Tabs()->PageForTab(dot)),
              u"● notes"_s);

    Tabs()->SetTabTitle(star_page, u"a&b"_s);
    EXPECT_EQ(Tabs()->TabLabel(Tabs()->indexOf(star_page)), u"a&b"_s);

    const auto one = Plain(u"a.txt"_s, u"1"_s, true, u"/x/one/a.txt"_s);
    const auto two = Plain(u"a.txt"_s, u"2"_s, true, u"/x/two/a.txt"_s);
    EXPECT_EQ(Tabs()->TabLabel(Tabs()->indexOf(Tabs()->PageForTab(one))),
              u"a.txt · one"_s);
    EXPECT_EQ(Tabs()->TabLabel(Tabs()->indexOf(Tabs()->PageForTab(two))),
              u"a.txt · two"_s);
  });
}

TEST_F(TabCloseTest, OpeningAnOpenFileByAnotherPathActivatesItsTab) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto file = dir.filePath(u"f.txt"_s);
  {
    QFile f(file);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("text");
  }
  const auto link = dir.filePath(u"link.txt"_s);
  ASSERT_TRUE(QFile::link(file, link));

  OnGui([&] {
    Tabs()->SlotOpenFile(file);
    ASSERT_EQ(Tabs()->count(), 1);
    const auto first = UI::TextEditTabWidget::TabIdOf(Tabs()->currentWidget());
    Plain(u"other.txt"_s, u"o"_s, true);

    Tabs()->SlotOpenFile(link);
    EXPECT_EQ(Tabs()->count(), 2);
    EXPECT_EQ(UI::TextEditTabWidget::TabIdOf(Tabs()->currentWidget()), first);

    Tabs()->setCurrentIndex(1);
    Tabs()->SlotOpenFile(dir.path() + u"/./f.txt"_s);
    EXPECT_EQ(Tabs()->count(), 2);
    EXPECT_EQ(UI::TextEditTabWidget::TabIdOf(Tabs()->currentWidget()), first);
  });
}

// The bar draws its own close buttons, so QTabWidget does not relay its close
// requests by itself; a middle click and a close button both depend on that.
TEST_F(TabCloseTest, AMiddleClickOrTheCloseButtonClosesThatTab) {
  OnGui([&] {
    const auto a = Plain(u"a.txt"_s, u"a"_s, true);
    const auto b = Plain(u"b.txt"_s, u"b"_s, true);
    auto* bar = Tabs()->tabBar();
    const auto at =
        QPointF(bar->tabRect(Tabs()->indexOf(Tabs()->PageForTab(a))).center());
    const auto global = QPointF(bar->mapToGlobal(at.toPoint()));

    QMouseEvent press(QEvent::MouseButtonPress, at, global, Qt::MiddleButton,
                      Qt::MiddleButton, Qt::NoModifier);
    QApplication::sendEvent(bar, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, at, global,
                        Qt::MiddleButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(bar, &release);

    EXPECT_EQ(Tabs()->PageForTab(a), nullptr);
    ASSERT_NE(Tabs()->PageForTab(b), nullptr);

    // The tab's own close button.
    const int index = Tabs()->indexOf(Tabs()->PageForTab(b));
    auto* button = bar->tabButton(index, QTabBar::RightSide);
    if (button == nullptr) button = bar->tabButton(index, QTabBar::LeftSide);
    ASSERT_NE(button, nullptr);
    qobject_cast<QAbstractButton*>(button)->click();
    EXPECT_EQ(Tabs()->PageForTab(b), nullptr);
  });
}

TEST_F(TabCloseTest, TheEmptyViewReplacesTheTabsWhenTheLastOneCloses) {
  OnGui([&] {
    auto* empty = edit_->findChild<UI::EditorEmptyView*>();
    ASSERT_NE(empty, nullptr);
    const auto a = Plain(u"a.txt"_s, u"a"_s, true);
    EXPECT_TRUE(empty->isHidden());

    edit_->CloseTabs({a});
    EXPECT_EQ(Tabs()->count(), 0);
    EXPECT_FALSE(empty->isHidden());
    EXPECT_TRUE(Tabs()->isHidden());

    Plain(u"b.txt"_s, u"b"_s, true);
    EXPECT_TRUE(empty->isHidden());
    EXPECT_FALSE(Tabs()->isHidden());
  });
}

TEST_F(TabCloseTest, TheOpenTabsListSearchesAndClosesWithoutLeaving) {
  OnGui([&] {
    const auto alpha = Plain(u"alpha.txt"_s, u"a"_s, true);
    const auto beta = Plain(u"beta.txt"_s, u"b"_s, true);
    const auto alphabet = Plain(u"alphabet.txt"_s, u"c"_s, true);

    UI::OpenTabsPopup popup(edit_);
    popup.Open();
    EXPECT_EQ(popup.ListedIds(), (QList<quint64>{alphabet, beta, alpha}))
        << "most recently used first";
    EXPECT_EQ(popup.SelectedId(), alphabet) << "the current tab";

    popup.SetQuery(u"zzz"_s);
    EXPECT_TRUE(popup.ListedIds().isEmpty());

    popup.SetQuery(u"ALPHA"_s);
    EXPECT_EQ(popup.ListedIds(), (QList<quint64>{alphabet, alpha}));

    popup.CloseSelected();  // alphabet
    EXPECT_EQ(Tabs()->PageForTab(alphabet), nullptr);
    EXPECT_EQ(popup.ListedIds(), QList<quint64>{alpha}) << "the query stays";
    EXPECT_EQ(popup.SelectedId(), alpha) << "the neighbour is selected";
  });
}

}  // namespace GpgFrontend::Test
