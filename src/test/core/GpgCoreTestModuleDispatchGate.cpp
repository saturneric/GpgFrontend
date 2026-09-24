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

#include <QElapsedTimer>
#include <atomic>
#include <memory>
#include <thread>

#include "GpgFrontendTest.h"
#include "core/module/ModuleDispatchGate.h"

/**
 * @file GpgCoreTestModuleDispatchGate.cpp
 * @brief The admission gate that makes module teardown safe.
 *
 * The two invariants worth pinning, because they are what actually break:
 * the sweep must observe zero in-flight calls when it runs, and a call
 * arriving after the gate closes must be REFUSED rather than half-executed.
 */

namespace GpgFrontend::Test {

using Module::ModuleDispatchGate;
using Module::ModuleDispatchScope;

TEST(ModuleDispatchGateTest, AnOpenGateAdmitsAndCounts) {
  ModuleDispatchGate gate;
  EXPECT_FALSE(gate.IsClosed());
  EXPECT_EQ(gate.InFlight(), 0);

  ASSERT_TRUE(gate.TryEnter());
  EXPECT_EQ(gate.InFlight(), 1);

  gate.Leave();
  EXPECT_EQ(gate.InFlight(), 0);
}

TEST(ModuleDispatchGateTest, AClosedGateRefusesNewCalls) {
  ModuleDispatchGate gate;
  gate.Close();

  EXPECT_TRUE(gate.IsClosed());
  EXPECT_FALSE(gate.TryEnter()) << "a call arriving after close must be "
                                   "refused, not queued";
  EXPECT_EQ(gate.InFlight(), 0);
}

/// A call held inside the gate by ANOTHER thread, until released. A waiter
/// does not wait for calls on its own thread, so every test about waiting for
/// a call has to put that call somewhere else.
class HeldCall {
 public:
  explicit HeldCall(ModuleDispatchGate& gate) : gate_(gate) {
    thread_ = std::thread([this] {
      entered_ = gate_.TryEnter();
      ready_ = true;
      while (!release_) std::this_thread::yield();
      if (entered_) gate_.Leave();
    });
    while (!ready_) std::this_thread::yield();
  }
  ~HeldCall() {
    Release();
    thread_.join();
  }
  HeldCall(const HeldCall&) = delete;
  auto operator=(const HeldCall&) -> HeldCall& = delete;

  [[nodiscard]] auto Entered() const -> bool { return entered_; }
  void Release() { release_ = true; }

 private:
  ModuleDispatchGate& gate_;
  std::atomic<bool> entered_{false};
  std::atomic<bool> ready_{false};
  std::atomic<bool> release_{false};
  std::thread thread_;
};

// Closing does not abort what is already running -- it only stops admitting.
// Tearing down around a call that is still inside is the whole hazard.
TEST(ModuleDispatchGateTest, ClosingDoesNotEvictCallsAlreadyInside) {
  ModuleDispatchGate gate;
  auto call = std::make_unique<HeldCall>(gate);
  ASSERT_TRUE(call->Entered());

  gate.Close();
  EXPECT_EQ(gate.InFlight(), 1) << "the in-flight call is still in flight";

  // And it is not quiet until that call leaves.
  EXPECT_FALSE(gate.WaitQuiescent(50));

  call.reset();
  EXPECT_TRUE(gate.WaitQuiescent(50));
}

TEST(ModuleDispatchGateTest, WaitQuiescentReturnsImmediatelyWhenIdle) {
  ModuleDispatchGate gate;
  QElapsedTimer timer;
  timer.start();
  EXPECT_TRUE(gate.WaitQuiescent(5000));
  EXPECT_LT(timer.elapsed(), 1000) << "an idle gate must not wait out its "
                                      "timeout";
}

// A timeout must be REPORTED, not swallowed: the shutdown path skips the rest
// of teardown on false, because freeing a module's resources while its code
// runs is worse than leaking them on the way out.
TEST(ModuleDispatchGateTest, WaitQuiescentReportsATimeoutRatherThanHanging) {
  ModuleDispatchGate gate;
  const HeldCall call(gate);
  ASSERT_TRUE(call.Entered());

  QElapsedTimer timer;
  timer.start();
  EXPECT_FALSE(gate.WaitQuiescent(100));
  EXPECT_GE(timer.elapsed(), 90);
}

TEST(ModuleDispatchGateTest, WaitQuiescentWakesWhenTheLastCallLeaves) {
  ModuleDispatchGate gate;
  HeldCall first(gate);
  HeldCall second(gate);
  ASSERT_EQ(gate.InFlight(), 2);

  std::thread leaver([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    first.Release();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    second.Release();
  });

  // Must observe ZERO in flight when it returns -- that is the precondition
  // every later teardown step relies on.
  EXPECT_TRUE(gate.WaitQuiescent(5000));
  leaver.join();
}

// The module runner can be inside a module's code, spinning a nested event
// loop, when the task that deactivates that same module runs. Waiting for its
// own call would wait for itself.
TEST(ModuleDispatchGateTest, AWaiterDoesNotWaitForItsOwnCalls) {
  ModuleDispatchGate gate;
  ASSERT_TRUE(gate.TryEnter());

  QElapsedTimer timer;
  timer.start();
  EXPECT_TRUE(gate.WaitQuiescent(5000));
  EXPECT_LT(timer.elapsed(), 1000);

  // It still waits for everyone else's.
  const HeldCall other(gate);
  EXPECT_FALSE(gate.WaitQuiescent(50));
  gate.Leave();
}

TEST(ModuleDispatchGateTest, APerModuleGateReopensAndIsStable) {
  auto& gate = Module::ModuleEntryGate("com.example.gate.reopen");
  EXPECT_EQ(&gate, &Module::ModuleEntryGate("com.example.gate.reopen"));
  EXPECT_NE(&gate, &Module::ModuleEntryGate("com.example.gate.other"));

  gate.Close();
  EXPECT_FALSE(gate.TryEnter());
  gate.Open();
  ASSERT_TRUE(gate.TryEnter());
  gate.Leave();
}

TEST(ModuleDispatchGateTest, TheScopeGuardBalancesEvenOnAnEarlyReturn) {
  ModuleDispatchGate gate;
  {
    ModuleDispatchScope scope(gate);
    ASSERT_TRUE(scope.Entered());
    EXPECT_EQ(gate.InFlight(), 1);
    // an early return here is the normal case in the dispatcher
  }
  EXPECT_EQ(gate.InFlight(), 0);
}

TEST(ModuleDispatchGateTest, TheScopeGuardDoesNotLeaveWhatItNeverEntered) {
  ModuleDispatchGate gate;
  gate.Close();
  {
    ModuleDispatchScope scope(gate);
    EXPECT_FALSE(scope.Entered());
  }
  // A refused scope must not decrement on destruction; if it did, the counter
  // would drift negative and quiescence would be reported far too early.
  EXPECT_EQ(gate.InFlight(), 0);
}

TEST(ModuleDispatchGateTest, ConcurrentEntriesAreCountedExactly) {
  ModuleDispatchGate gate;
  constexpr int kThreads = 8;
  constexpr int kIterations = 200;

  std::atomic<int> refused{0};
  std::vector<std::thread> workers;
  workers.reserve(kThreads);

  for (int i = 0; i < kThreads; ++i) {
    workers.emplace_back([&] {
      for (int n = 0; n < kIterations; ++n) {
        ModuleDispatchScope scope(gate);
        if (!scope.Entered()) ++refused;
      }
    });
  }
  for (auto& w : workers) w.join();

  EXPECT_EQ(refused.load(), 0) << "the gate was never closed";
  EXPECT_EQ(gate.InFlight(), 0) << "every entry was balanced";
}

}  // namespace GpgFrontend::Test
