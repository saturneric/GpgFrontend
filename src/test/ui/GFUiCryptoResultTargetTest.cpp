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

// Where the result of a crypto operation is allowed to land.
//
// A crypto operation runs on the module task runner and its callback arrives
// an unbounded time later. Every one of these operations used to write through
// "the current text page", so the answer to "which document does this belong
// to?" was whichever tab happened to be in front when the result came back --
// and for decrypt, which ran with no modal over it at all, switching tabs
// during the operation put the plaintext into an unrelated document.
//
// The rule is now: a result may only modify the exact page that started the
// operation, and is discarded otherwise. Never the current tab, and never a
// freshly created one.
//
// These cases run off the GUI thread and cannot build a QWidget, so the policy
// is exercised through TextEdit::ClassifyResultTarget. QObject lifetime IS
// real here: the destroyed-page case below deletes an actual object and reads
// the QPointer that the production code reads.

#include <gtest/gtest.h>

#include <QObject>
#include <QPointer>

#include "GpgFrontendTest.h"
#include "ui/widgets/TextEdit.h"

namespace GpgFrontend::Test {

namespace {

using ResultTarget = UI::TextEdit::ResultTarget;

auto Classify(bool alive, int index, bool text) -> ResultTarget {
  return UI::TextEdit::ClassifyResultTarget(alive, index, text);
}

}  // namespace

TEST(GFUiCryptoResultTargetTest, AResultGoesToThePageThatAskedForIt) {
  EXPECT_EQ(Classify(true, 0, true), ResultTarget::kDeliver);
  EXPECT_EQ(Classify(true, 7, true), ResultTarget::kDeliver);
}

TEST(GFUiCryptoResultTargetTest, AClosedPageDiscardsItsResult) {
  // The tab was closed while the operation was in flight. The bytes have
  // nowhere legitimate to go, and putting them anywhere else would disclose
  // one document's plaintext inside another.
  EXPECT_EQ(Classify(false, -1, true), ResultTarget::kPageDestroyed);

  // A destroyed page is never rescued by looking plausible in other respects.
  EXPECT_EQ(Classify(false, 0, true), ResultTarget::kPageDestroyed);
}

TEST(GFUiCryptoResultTargetTest, ADetachedPageDiscardsItsResult) {
  // Alive, but no longer one of the tab widget's pages: removed and pending
  // deletion. Writing to it would be writing into a document the user can no
  // longer see.
  EXPECT_EQ(Classify(true, -1, true), ResultTarget::kPageDetached);
}

TEST(GFUiCryptoResultTargetTest, APageThatCannotHoldTextDiscardsItsResult) {
  EXPECT_EQ(Classify(true, 0, false), ResultTarget::kNotATextPage);
}

TEST(GFUiCryptoResultTargetTest, OnlyOneOutcomeEverWrites) {
  // The property that matters is not which reasons exist but that exactly one
  // of them permits a write. A future edit that adds a fallback -- "deliver to
  // the current tab instead", "open a new tab for it" -- has to fail here.
  int deliverable = 0;
  for (const bool alive : {false, true}) {
    for (const int index : {-1, 0, 3}) {
      for (const bool text : {false, true}) {
        if (Classify(alive, index, text) == ResultTarget::kDeliver) {
          ++deliverable;
          EXPECT_TRUE(alive) << "a destroyed page was judged deliverable";
          EXPECT_GE(index, 0) << "a detached page was judged deliverable";
          EXPECT_TRUE(text) << "a non-text page was judged deliverable";
        }
      }
    }
  }
  // alive x {0,3} x text
  EXPECT_EQ(deliverable, 2);
}

TEST(GFUiCryptoResultTargetTest, QPointerReportsARealDeletionAsNotAlive) {
  // Not a tautology: this is the mechanism the production code depends on to
  // tell a closed tab from a live one. A raw pointer would still read as
  // non-null here and the result would be written into freed memory.
  auto* page = new QObject;
  QPointer<QObject> tracked(page);
  ASSERT_FALSE(tracked.isNull());
  EXPECT_EQ(Classify(!tracked.isNull(), 0, true), ResultTarget::kDeliver);

  delete page;

  ASSERT_TRUE(tracked.isNull());
  EXPECT_EQ(Classify(!tracked.isNull(), 0, true), ResultTarget::kPageDestroyed);
}

}  // namespace GpgFrontend::Test
