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

#include <QScopeGuard>
#include <QSignalSpy>
#include <future>

#include "MobileTest.h"
#include "core/function/GlobalSettingStation.h"
#include "core/function/SettingCatalog.h"
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
  // At once: in the background the event loop is held, and a wipe left for
  // it would wait for the return.
  EXPECT_EQ(wiped.count(), 1);

  // A second Suspended is the same background stay.
  lifecycle.OnApplicationStateChanged(Qt::ApplicationSuspended);
  EXPECT_EQ(suspended.count(), 1);

  lifecycle.OnApplicationStateChanged(Qt::ApplicationActive);
  EXPECT_FALSE(lifecycle.IsSuspended());
  EXPECT_EQ(resumed.count(), 1);
}

// Decrypted text goes once the application was away as long as Settings
// says: at once when going, otherwise told on the return; never, if asked.
TEST(AppLifecycleTest, DecryptedTextIsClearedAfterTheTimeAway) {
  const auto forget = qScopeGuard([]() {
    auto settings = GetSettings();
    settings.remove(SettingItems::kClearTextInBackground.Key());
  });
  const auto set = [](int seconds) {
    auto settings = GetSettings();
    SettingItems::kClearTextInBackground.Write(settings, seconds);
  };

  InteractionBridge bridge(nullptr);
  AppLifecycle lifecycle(&bridge);
  qint64 now = 1'000'000;
  lifecycle.SetClock([&now]() { return now; });
  QSignalSpy wiped(&lifecycle, &AppLifecycle::SignalWipeSensitiveText);
  const auto away = [&lifecycle, &now](qint64 ms) {
    lifecycle.OnApplicationStateChanged(Qt::ApplicationSuspended);
    now += ms;
    lifecycle.OnApplicationStateChanged(Qt::ApplicationActive);
  };

  // The default: five minutes.
  EXPECT_EQ(AppLifecycle::ClearTextAfter(), 300);
  away(299'999);
  EXPECT_EQ(wiped.count(), 0);
  away(300'000);
  EXPECT_EQ(wiped.count(), 1);

  // A clock that went back counts as long enough.
  away(-60'000);
  EXPECT_EQ(wiped.count(), 2);

  // At once: when going, not again on the return.
  set(0);
  lifecycle.OnApplicationStateChanged(Qt::ApplicationSuspended);
  EXPECT_EQ(wiped.count(), 3);
  now += 10'000'000;
  lifecycle.OnApplicationStateChanged(Qt::ApplicationActive);
  EXPECT_EQ(wiped.count(), 3);

  // Never.
  set(-1);
  away(10'000'000);
  EXPECT_EQ(wiped.count(), 3);

  // Inactive (a notification shade) is not away at all.
  set(60);
  lifecycle.OnApplicationStateChanged(Qt::ApplicationInactive);
  now += 120'000;
  lifecycle.OnApplicationStateChanged(Qt::ApplicationActive);
  EXPECT_EQ(wiped.count(), 3);
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
