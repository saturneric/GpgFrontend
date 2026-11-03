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

#include <QDir>
#include <QLockFile>
#include <QTemporaryDir>
#include <future>

#include "MobileTest.h"
#include "core/profile/ProfileLock.h"
#include "startup/InteractionBridge.h"
#include "startup/MobileProfileLoaderDelegate.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::InteractionBridge;
using GpgFrontend::Mobile::MobileProfileLoaderDelegate;

namespace {

auto HeldBy(const QString& root) -> ProfileLockResult {
  ProfileLockResult held;
  held.status = ProfileLockStatus::kHELD_ELSEWHERE;
  held.path = root + "/profile.lock";
  held.pid = 4242;
  held.host = "elsewhere";
  return held;
}

}  // namespace

TEST(MobileProfileLoaderDelegateTest, ForcingIsAlwaysAsked) {
  // A "yes" is a force; a holder that has gone is the loader's to find, by
  // trying once more, never the delegate's to assume.
  QTemporaryDir root;
  ASSERT_TRUE(root.isValid());
  InteractionBridge bridge(nullptr);
  MobileProfileLoaderDelegate delegate(&bridge);

  auto decided = std::async(std::launch::async, [&]() {
    return delegate.ConfirmForceUnlock(HeldBy(root.path()));
  });
  ASSERT_TRUE(WaitFor([&bridge]() { return !bridge.Current().isEmpty(); }));
  bridge.CancelAll();
  EXPECT_FALSE(decided.get());
}

TEST(MobileProfileLoaderDelegateTest, ALiveLockIsOnlyForcedByAPerson) {
  QTemporaryDir root;
  ASSERT_TRUE(root.isValid());
  QLockFile live(root.path() + "/profile.lock");
  ASSERT_TRUE(live.tryLock(0));

  InteractionBridge bridge(nullptr);
  MobileProfileLoaderDelegate delegate(&bridge);

  for (const bool choice : {false, true}) {
    auto decided = std::async(std::launch::async, [&]() {
      return delegate.ConfirmForceUnlock(HeldBy(root.path()));
    });
    ASSERT_TRUE(WaitFor([&bridge]() { return !bridge.Current().isEmpty(); }));
    const auto question = bridge.Current();
    EXPECT_EQ(question.value("kind").toString(), "confirm_force_unlock");
    EXPECT_EQ(question.value("host").toString(), "elsewhere");
    bridge.answer(question.value("id").toInt(), choice);
    EXPECT_EQ(decided.get(), choice);
  }

  // Declined, or cancelled by a shutdown: never forced.
  auto declined = std::async(std::launch::async, [&]() {
    return delegate.ConfirmForceUnlock(HeldBy(root.path()));
  });
  ASSERT_TRUE(WaitFor([&bridge]() { return !bridge.Current().isEmpty(); }));
  bridge.CancelAll();
  EXPECT_FALSE(declined.get());
}

TEST(MobileProfileLoaderDelegateTest, ThePinIsWhatThePersonTyped) {
  InteractionBridge bridge(nullptr);
  MobileProfileLoaderDelegate delegate(&bridge);

  auto answered = std::async(std::launch::async, [&]() {
    return delegate.AskAppKeyPin({.creating = false, .failures = 1});
  });
  ASSERT_TRUE(WaitFor([&bridge]() { return !bridge.Current().isEmpty(); }));
  const auto question = bridge.Current();
  EXPECT_EQ(question.value("kind").toString(), "app_key_pin");
  EXPECT_EQ(question.value("failures").toInt(), 1);
  bridge.answer(question.value("id").toInt(),
                QVariantMap{{"action", "pin"}, {"pin", "2468"}});

  const auto answer = answered.get();
  EXPECT_EQ(answer.action, AppKeyPinAnswer::Action::kUsePin);
  EXPECT_EQ(answer.pin.ConvertToQByteArray(), QByteArray("2468"));
}

TEST(MobileProfileLoaderDelegateTest, ADeclinedPinQuits) {
  InteractionBridge bridge(nullptr);
  MobileProfileLoaderDelegate delegate(&bridge);

  auto answered = std::async(std::launch::async,
                             [&]() { return delegate.AskAppKeyPin({}); });
  ASSERT_TRUE(WaitFor([&bridge]() { return !bridge.Current().isEmpty(); }));
  bridge.decline(bridge.Current().value("id").toInt());
  EXPECT_EQ(answered.get().action, AppKeyPinAnswer::Action::kQuit);
}

TEST(MobileProfileLoaderDelegateTest, AForgottenPinIsResetOnlyWhenConfirmed) {
  InteractionBridge bridge(nullptr);
  MobileProfileLoaderDelegate delegate(&bridge);

  for (const bool confirm : {false, true}) {
    auto answered = std::async(std::launch::async, [&]() {
      return delegate.AskAppKeyPin({.creating = false, .failures = 3});
    });
    ASSERT_TRUE(WaitFor([&bridge]() { return !bridge.Current().isEmpty(); }));
    bridge.answer(bridge.Current().value("id").toInt(),
                  QVariantMap{{"action", "reset"}});

    // The loader takes a reset as decided, so it is asked here.
    ASSERT_TRUE(WaitFor([&bridge]() {
      return bridge.Current().value("kind").toString() == "confirm_key_reset";
    }));
    EXPECT_EQ(bridge.Current().value("reason").toString(), "pin_forgotten");
    bridge.answer(bridge.Current().value("id").toInt(), confirm);

    // Declined: one more wrong attempt, the PIN is asked again.
    EXPECT_EQ(answered.get().action, confirm
                                         ? AppKeyPinAnswer::Action::kResetKey
                                         : AppKeyPinAnswer::Action::kUsePin);
  }
}

}  // namespace GpgFrontend::Test::Mobile
