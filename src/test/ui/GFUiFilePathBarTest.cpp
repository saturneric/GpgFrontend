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

#include <QAbstractButton>
#include <QApplication>
#include <QLabel>
#include <algorithm>

#include "GpgFrontendTest.h"
#include "ui/widgets/FilePathBar.h"

namespace GpgFrontend::Test {

namespace {

const auto kDeepPath =
    u"/srv/screenshots/run/temporary-profile-5c3ee45a0b5c5e55/storage/"
    "workspace"_s;

/// The crumbs, chevrons and buttons the bar shows, left to right.
auto VisibleParts(const UI::FilePathBar& bar) -> QList<QRect> {
  QList<QRect> parts;
  for (auto* w : bar.findChildren<QWidget*>()) {
    if (!w->isVisibleTo(&bar)) continue;
    if (qobject_cast<QAbstractButton*>(w) == nullptr &&
        qobject_cast<QLabel*>(w) == nullptr) {
      continue;
    }
    parts.append(QRect(w->mapTo(&bar, QPoint(0, 0)), w->size()));
  }
  std::sort(parts.begin(), parts.end(),
            [](const QRect& a, const QRect& b) { return a.x() < b.x(); });
  return parts;
}

void ExpectSideBySide(const UI::FilePathBar& bar) {
  const auto parts = VisibleParts(bar);
  ASSERT_GE(parts.size(), 2);
  for (qsizetype i = 1; i < parts.size(); ++i) {
    EXPECT_LE(parts.at(i - 1).right(), parts.at(i).left())
        << "parts " << i - 1 << " and " << i << " overlap at width "
        << bar.width();
  }
  EXPECT_LE(parts.back().right(), bar.width());
}

auto ShowsCrumb(const UI::FilePathBar& bar, const QString& text) -> bool {
  const auto buttons = bar.findChildren<QAbstractButton*>();
  return std::any_of(buttons.cbegin(), buttons.cend(),
                     [&](const QAbstractButton* b) {
                       return b->isVisibleTo(&bar) && b->text() == text;
                     });
}

}  // namespace

TEST(FilePathBarTest, ADeepPathFoldsIntoTheOverflowWithoutOverlap) {
  RunOnMainThread([]() {
    UI::FilePathBar bar;
    bar.SetPath(kDeepPath);
    bar.resize(360, 32);
    bar.show();
    QApplication::processEvents();

    ExpectSideBySide(bar);
    // The folder shown always stays, folded parents go behind the overflow.
    EXPECT_TRUE(ShowsCrumb(bar, u"workspace"_s));
    EXPECT_TRUE(ShowsCrumb(bar, u"…"_s));
    EXPECT_FALSE(ShowsCrumb(bar, u"srv"_s));
  });
}

TEST(FilePathBarTest, ResizingRefitsWithoutLeavingOldCrumbsBehind) {
  RunOnMainThread([]() {
    UI::FilePathBar bar;
    bar.SetPath(kDeepPath);
    bar.resize(360, 32);
    bar.show();
    QApplication::processEvents();

    for (const auto width : {900, 300, 600}) {
      bar.resize(width, 32);
      QApplication::processEvents();
      ExpectSideBySide(bar);
      EXPECT_TRUE(ShowsCrumb(bar, u"workspace"_s)) << width;
    }
    // Wide enough for the whole path: nothing folded.
    bar.resize(1400, 32);
    QApplication::processEvents();
    EXPECT_TRUE(ShowsCrumb(bar, u"srv"_s));
    EXPECT_FALSE(ShowsCrumb(bar, u"…"_s));
  });
}

}  // namespace GpgFrontend::Test
