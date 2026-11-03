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

#include <QSignalSpy>
#include <future>

#include "MobileTest.h"
#include "lifecycle/AppLifecycle.h"
#include "startup/InteractionBridge.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::AppLifecycle;
using GpgFrontend::Mobile::InteractionBridge;

TEST(AppLifecycleTest, GoingToTheBackgroundForgetsSecrets) {
  InteractionBridge bridge(nullptr);
  AppLifecycle lifecycle(&bridge);
  QSignalSpy suspended(&lifecycle, &AppLifecycle::SignalSuspended);
  QSignalSpy resumed(&lifecycle, &AppLifecycle::SignalResumed);
  QSignalSpy wiped(&bridge, &InteractionBridge::SignalWipeSecretInputs);

  lifecycle.OnApplicationStateChanged(Qt::ApplicationInactive);
  EXPECT_FALSE(lifecycle.IsSuspended());
  EXPECT_EQ(suspended.count(), 0);

  lifecycle.OnApplicationStateChanged(Qt::ApplicationSuspended);
  EXPECT_TRUE(lifecycle.IsSuspended());
  EXPECT_EQ(suspended.count(), 1);
  EXPECT_TRUE(wiped.wait(2000));

  // A second Suspended is the same background stay.
  lifecycle.OnApplicationStateChanged(Qt::ApplicationSuspended);
  EXPECT_EQ(suspended.count(), 1);

  lifecycle.OnApplicationStateChanged(Qt::ApplicationActive);
  EXPECT_FALSE(lifecycle.IsSuspended());
  EXPECT_EQ(resumed.count(), 1);
}

TEST(AppLifecycleTest, AQuestionWaitingForAnAnswerSurvivesTheBackground) {
  InteractionBridge bridge(nullptr);
  AppLifecycle lifecycle(&bridge);

  auto pending = std::async(std::launch::async, [&bridge]() {
    return bridge.Ask(InteractionBridge::Kind::kAPP_KEY_PIN, {});
  });
  ASSERT_TRUE(WaitFor([&bridge]() { return !bridge.Current().isEmpty(); }));

  lifecycle.OnApplicationStateChanged(Qt::ApplicationSuspended);
  lifecycle.OnApplicationStateChanged(Qt::ApplicationActive);

  // Still asked, nothing answered on the person's behalf.
  EXPECT_FALSE(bridge.Current().isEmpty());
  EXPECT_EQ(pending.wait_for(std::chrono::milliseconds(100)),
            std::future_status::timeout);
  bridge.CancelAll();
  EXPECT_FALSE(pending.get().isValid());
}

}  // namespace GpgFrontend::Test::Mobile
