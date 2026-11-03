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

#include <thread>

#include "MobileTest.h"
#include "startup/InteractionBridge.h"

namespace GpgFrontend::Test::Mobile {

using GpgFrontend::Mobile::InteractionBridge;

namespace {

// Ask from a thread of its own: the bridge belongs to this one.
auto AskAsync(InteractionBridge& bridge, InteractionBridge::Kind kind,
              QVariantMap details) -> std::future<QVariant> {
  return std::async(std::launch::async, [&bridge, kind, details]() {
    return bridge.Ask(kind, details);
  });
}

auto WaitForQuestion(InteractionBridge& bridge) -> QVariantMap {
  WaitFor([&bridge]() { return !bridge.Current().isEmpty(); });
  return bridge.Current();
}

}  // namespace

TEST(InteractionBridgeTest, AnAnswerReachesTheThreadThatAsked) {
  InteractionBridge bridge(nullptr);
  auto reply = AskAsync(bridge, InteractionBridge::Kind::kCONFIRM_FORCE_UNLOCK,
                        {{"pid", 42}});

  const auto question = WaitForQuestion(bridge);
  EXPECT_EQ(question.value("kind").toString(), "confirm_force_unlock");
  EXPECT_EQ(question.value("pid").toInt(), 42);

  bridge.answer(question.value("id").toInt(), true);
  EXPECT_TRUE(reply.get().toBool());
  EXPECT_TRUE(bridge.Current().isEmpty());
}

TEST(InteractionBridgeTest, QuestionsAreShownOneAtATimeInOrder) {
  InteractionBridge bridge(nullptr);
  auto first = AskAsync(bridge, InteractionBridge::Kind::kCONFIRM_KEY_RESET,
                        {{"reason", "first"}});
  WaitForQuestion(bridge);
  auto second = AskAsync(bridge, InteractionBridge::Kind::kCONFIRM_KEY_RESET,
                         {{"reason", "second"}});
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto question = bridge.Current();
  EXPECT_EQ(question.value("reason").toString(), "first");
  bridge.decline(question.value("id").toInt());
  EXPECT_FALSE(first.get().isValid());

  question = WaitForQuestion(bridge);
  EXPECT_EQ(question.value("reason").toString(), "second");
  bridge.answer(question.value("id").toInt(), true);
  EXPECT_TRUE(second.get().toBool());
}

TEST(InteractionBridgeTest, CancellingAnswersEverythingSafely) {
  InteractionBridge bridge(nullptr);
  auto pending = AskAsync(bridge, InteractionBridge::Kind::kAPP_KEY_PIN, {});
  WaitForQuestion(bridge);

  bridge.CancelAll();
  EXPECT_FALSE(pending.get().isValid());
  EXPECT_TRUE(bridge.Current().isEmpty());

  // Anything asked afterwards is answered at once, without a person.
  auto later = AskAsync(bridge, InteractionBridge::Kind::kAPP_KEY_PIN, {});
  ASSERT_EQ(later.wait_for(std::chrono::seconds(2)), std::future_status::ready);
  EXPECT_FALSE(later.get().isValid());
}

TEST(InteractionBridgeTest, AskingOnItsOwnThreadDoesNotWait) {
  InteractionBridge bridge(nullptr);
  // This thread is the bridge's: waiting here would deadlock.
  EXPECT_FALSE(
      bridge.Ask(InteractionBridge::Kind::kCONFIRM_KEY_RESET, {}).isValid());
}

}  // namespace GpgFrontend::Test::Mobile
