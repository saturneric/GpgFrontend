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

#include "GpgFrontendTest.h"
#include "ui/function/GpgOperaHelper.h"

namespace GpgFrontend::Test {

TEST(WaitForOperaTest, TheQueuedStartOfAnOperationNeverHasANullReceiver) {
  // Callers pass qApp->activeWindow(), which is null whenever no window of
  // this application is active — on X11 that includes the moment just after a
  // modal dialog closes, which is when a deep restart asks to write a packaged
  // profile back. A queued call to a null receiver is dropped without a word,
  // so the operation never started and the waiting dialog spun on it forever.
  EXPECT_EQ(UI::OperaStartContext(nullptr), qApp);
  EXPECT_NE(UI::OperaStartContext(nullptr), nullptr);
}

// --- GpgOperaContextHolderTest -------------------------------------------
//
// GpgOperaContextBasement owns a reference cycle: the operas it holds capture a
// GpgOperaContext that points strongly back at it. Nothing below constructs a
// widget, so these are safe on the worker thread the tests run on.

namespace {

// Builds the real cycle: a context from the basement, captured by an opera
// stored back inside that same basement.
void MakeOperaCycle(const QSharedPointer<UI::GpgOperaContextBasement>& base) {
  base->GetContextBuffer(0).append(GFBuffer(QString("plaintext")));

  auto context = UI::GetGpgOperaContextFromBasement(base, 0);
  ASSERT_NE(context, nullptr);

  base->operas.push_back(
      [context](const UI::OperaWaitingHd& /*hd*/) { (void)context; });
}

}  // namespace

TEST(GpgOperaContextHolderTest,
     TheBasementIsReleasedEvenWhenTheOperasNeverRan) {
  QWeakPointer<UI::GpgOperaContextBasement> weak;

  {
    UI::GpgOperaContextHolder holder;
    weak = holder.Base();
    MakeOperaCycle(holder.Base());
    ASSERT_FALSE(weak.isNull());
  }

  // Leaving the scope broke the cycle, so the keys and buffers are gone even
  // though nobody cleared operas by hand.
  EXPECT_TRUE(weak.isNull());
}

TEST(GpgOperaContextHolderTest, WithoutTheHolderTheSameCycleHoldsTheBasement) {
  QWeakPointer<UI::GpgOperaContextBasement> weak;

  {
    auto base = SecureCreateSharedObject<UI::GpgOperaContextBasement>();
    weak = base;
    MakeOperaCycle(base);
  }

  // This is the bug the holder exists to prevent: the last external reference
  // is gone and the basement is still alive.
  ASSERT_FALSE(weak.isNull());

  // Clearing by hand is what every exec helper has to remember to do.
  weak.toStrongRef()->operas.clear();
  EXPECT_TRUE(weak.isNull());
}

TEST(GpgOperaContextHolderTest, AnOperaStillInFlightKeepsTheBasementAlive) {
  QWeakPointer<UI::GpgOperaContextBasement> weak;
  UI::OperaWaitingCb queued;

  {
    UI::GpgOperaContextHolder holder;
    weak = holder.Base();
    MakeOperaCycle(holder.Base());

    // What WaitForMultipleOperas does when it queues a start: it captures by
    // value, so the queued copy owns its own strong reference.
    queued = holder->operas.front();
  }

  // The holder is gone, but the in-flight copy must still be able to write its
  // result. This is why clearing the container is safe mid-operation, and it
  // would fail loudly if context->base were ever weakened.
  EXPECT_FALSE(weak.isNull());

  queued = nullptr;
  EXPECT_TRUE(weak.isNull());
}

TEST(GpgOperaContextHolderTest,
     TheHolderToleratesOperasThatWereAlreadyCleared) {
  QWeakPointer<UI::GpgOperaContextBasement> weak;

  {
    UI::GpgOperaContextHolder holder;
    weak = holder.Base();
    MakeOperaCycle(holder.Base());

    // The exec helpers still clear explicitly, to release the buffers when the
    // work is done rather than when the slot happens to return. The holder's
    // backstop must not mind.
    holder->operas.clear();
    EXPECT_TRUE(holder->operas.isEmpty());
  }

  EXPECT_TRUE(weak.isNull());
}

TEST(GpgOperaContextHolderTest, ArrowAccessReachesTheOwnedBasement) {
  UI::GpgOperaContextHolder holder;

  holder->ascii = true;
  EXPECT_TRUE(holder.Base()->ascii);
  EXPECT_EQ(holder.operator->(), holder.Base().data());
}

}  // namespace GpgFrontend::Test
