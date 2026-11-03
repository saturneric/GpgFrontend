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

#include <QTemporaryDir>

#include "GpgFrontendTest.h"
#include "ui/function/TabManagement.h"

namespace GpgFrontend::Test {

using UI::TabCloseScope;
using UI::TabsToClose;

TEST(TabManagementTest, EachCloseScopeTakesItsTabsInVisualOrder) {
  const QList<bool> modified{false, true, false, false, true};

  EXPECT_EQ(TabsToClose(TabCloseScope::kTHIS, 2, modified), QList<int>{2});
  EXPECT_EQ(TabsToClose(TabCloseScope::kOTHERS, 2, modified),
            (QList<int>{0, 1, 3, 4}));
  EXPECT_EQ(TabsToClose(TabCloseScope::kRIGHT, 2, modified),
            (QList<int>{3, 4}));
  EXPECT_EQ(TabsToClose(TabCloseScope::kLEFT, 2, modified), (QList<int>{0, 1}));
  EXPECT_EQ(TabsToClose(TabCloseScope::kALL, 2, modified),
            (QList<int>{0, 1, 2, 3, 4}));
}

TEST(TabManagementTest, CloseUnmodifiedNeverTakesATabWithUnsavedChanges) {
  const QList<bool> modified{false, true, false, true};
  EXPECT_EQ(TabsToClose(TabCloseScope::kUNMODIFIED, 1, modified),
            (QList<int>{0, 2}));
  EXPECT_TRUE(
      TabsToClose(TabCloseScope::kUNMODIFIED, 0, {true, true}).isEmpty());
}

TEST(TabManagementTest, ScopesWithNothingToCloseAreEmpty) {
  const QList<bool> three{false, false, false};
  // At the ends: nothing to one side.
  EXPECT_TRUE(TabsToClose(TabCloseScope::kLEFT, 0, three).isEmpty());
  EXPECT_TRUE(TabsToClose(TabCloseScope::kRIGHT, 2, three).isEmpty());
  // One tab: no others.
  EXPECT_TRUE(TabsToClose(TabCloseScope::kOTHERS, 0, {false}).isEmpty());
  // An anchor that is not a tab closes nothing anchored to it.
  EXPECT_TRUE(TabsToClose(TabCloseScope::kTHIS, 3, three).isEmpty());
  EXPECT_TRUE(TabsToClose(TabCloseScope::kOTHERS, -1, three).isEmpty());
  EXPECT_TRUE(TabsToClose(TabCloseScope::kALL, 0, {}).isEmpty());
}

TEST(TabManagementTest, MruOrderPutsTheLastUsedFirst) {
  UI::TabMruOrder mru;
  mru.Touch(1);
  mru.Touch(2);
  mru.Touch(3);
  mru.Touch(1);
  EXPECT_EQ(mru.Order(), (QList<quint64>{1, 3, 2}));

  mru.Remove(3);
  EXPECT_EQ(mru.Order(), (QList<quint64>{1, 2}));

  mru.Touch(4);
  mru.Retain({2, 4, 9});
  EXPECT_EQ(mru.Order(), (QList<quint64>{4, 2})) << "9 was never used";
}

TEST(TabManagementTest, ClosingTheActiveTabShowsTheMostRecentSurvivor) {
  const QList<quint64> visual{1, 2, 3, 4};
  // Recently used: 3, then 1; 2 is the visual neighbour Qt would pick.
  const QList<quint64> mru{3, 1, 2, 4};

  EXPECT_EQ(UI::TabToShowAfterClose(mru, visual, {3}, 3), 1U);
  EXPECT_EQ(UI::TabToShowAfterClose(mru, visual, {3, 1}, 3), 2U);
}

TEST(TabManagementTest, ClosingBackgroundTabsKeepsTheActiveOne) {
  EXPECT_EQ(UI::TabToShowAfterClose({2, 1}, {1, 2, 3}, {1, 3}, 2), 2U);
}

TEST(TabManagementTest, WithoutHistoryTheVisualNeighbourIsShown) {
  // The right-hand neighbour first, then the left one.
  EXPECT_EQ(UI::TabToShowAfterClose({}, {1, 2, 3}, {2}, 2), 3U);
  EXPECT_EQ(UI::TabToShowAfterClose({}, {1, 2, 3}, {2, 3}, 2), 1U);
  EXPECT_FALSE(UI::TabToShowAfterClose({1}, {1}, {1}, 1).has_value());
}

TEST(TabManagementTest, ASwitchSessionWalksTheSnapshotAndWraps) {
  const auto all = [](quint64) { return true; };
  UI::TabSwitchSession s;
  s.Begin({30, 20, 10});
  EXPECT_TRUE(s.Active());
  EXPECT_EQ(s.Step(1, all), 20U) << "a quick tap: the previous document";
  EXPECT_EQ(s.Step(1, all), 10U);
  EXPECT_EQ(s.Step(1, all), 30U) << "wraps";
  EXPECT_EQ(s.Step(-1, all), 10U);
  EXPECT_EQ(s.End(), 10U);
  EXPECT_FALSE(s.Active());
}

TEST(TabManagementTest, ASwitchSessionStepsOverClosedTabs) {
  UI::TabSwitchSession s;
  s.Begin({30, 20, 10});
  EXPECT_EQ(s.Step(1, [](quint64 id) { return id != 20; }), 10U);
  EXPECT_FALSE(s.Step(1, [](quint64) { return false; }).has_value())
      << "nowhere to go";

  UI::TabSwitchSession lone;
  lone.Begin({7});
  EXPECT_FALSE(lone.Step(1, [](quint64) { return true; }).has_value());
  EXPECT_FALSE(lone.End().has_value()) << "it never moved";
}

TEST(TabManagementTest, SearchRanksNamesBeforePaths) {
  const QList<UI::TabListEntry> mru{
      {1, u"readme.txt"_s, u"/home/u/notes/readme.txt"_s},
      {2, u"my-notes.md"_s, u"/tmp/my-notes.md"_s},
      {3, u"Notes.txt"_s, u"/srv/Notes.txt"_s},
      {4, u"other.txt"_s, u"/srv/other.txt"_s},
      {5, u"notes-2.txt"_s, {}},
  };

  QList<quint64> ids;
  for (const auto& e : UI::FilterTabEntries(mru, u"  NOTES "_s)) {
    ids.append(e.id);
  }
  // Name starts with it (in MRU order), name contains it, path contains it.
  EXPECT_EQ(ids, (QList<quint64>{3, 5, 2, 1}));
}

TEST(TabManagementTest, AnEmptySearchListsEveryTabInMruOrder) {
  const QList<UI::TabListEntry> mru{{9, u"b"_s, {}}, {4, u"a"_s, {}}};
  const auto shown = UI::FilterTabEntries(mru, u" "_s);
  ASSERT_EQ(shown.size(), 2);
  EXPECT_EQ(shown[0].id, 9U);
  EXPECT_TRUE(UI::FilterTabEntries(mru, u"zz"_s).isEmpty());
}

TEST(TabManagementTest, SameNamedTabsGetTheShortestFolderThatTellsThemApart) {
  const auto out = UI::DisambiguatingSuffixes(
      {u"a.txt"_s, u"a.txt"_s, u"b.txt"_s},
      {u"/x/one/a.txt"_s, u"/x/two/a.txt"_s, u"/x/one/b.txt"_s});
  EXPECT_EQ(out, (QStringList{u"one"_s, u"two"_s, QString()}));

  const auto deeper = UI::DisambiguatingSuffixes(
      {u"a.txt"_s, u"a.txt"_s}, {u"/p/q/docs/a.txt"_s, u"/p/r/docs/a.txt"_s});
  EXPECT_EQ(deeper, (QStringList{u"q/docs"_s, u"r/docs"_s}));

  // An untitled namesake has no folder to show; the other still gets one.
  const auto partial =
      UI::DisambiguatingSuffixes({u"a"_s, u"a"_s}, {u"/x/one/a"_s, QString()});
  EXPECT_EQ(partial, (QStringList{u"one"_s, QString()}));
}

TEST(TabManagementTest, AFileIsTheSameFileByAnyPath) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto file = dir.filePath(u"f.txt"_s);
  {
    QFile f(file);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
  }
  const auto link = dir.filePath(u"link.txt"_s);
  ASSERT_TRUE(QFile::link(file, link));
  const auto other = dir.filePath(u"g.txt"_s);

  const auto key = UI::FileIdentityKey(file);
  EXPECT_FALSE(key.isEmpty());
  EXPECT_EQ(UI::FileIdentityKey(link), key);
  EXPECT_EQ(UI::FileIdentityKey(dir.path() + u"/./sub/../f.txt"_s), key);
  EXPECT_NE(UI::FileIdentityKey(other), key);
}

TEST(TabManagementTest, UntitledNamesAreUniqueAndNeverReused) {
  int counter = 0;
  const QSet<QString> restored{u"untitled1.txt"_s};
  EXPECT_EQ(UI::NextUntitledTitle(counter, u"untitled"_s, u"txt"_s, restored),
            u"untitled2.txt"_s);
  EXPECT_EQ(UI::NextUntitledTitle(counter, u"untitled"_s, u"txt"_s, {}),
            u"untitled3.txt"_s)
      << "untitled2.txt was handed out already, even if closed since";
  EXPECT_EQ(UI::NextUntitledTitle(counter, u"untitled"_s, {}, {}),
            u"untitled4"_s);
}

}  // namespace GpgFrontend::Test
