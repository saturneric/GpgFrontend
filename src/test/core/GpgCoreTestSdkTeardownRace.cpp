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

#include <QByteArray>
#include <QThread>
#include <array>
#include <atomic>
#include <memory>

#include "GpgFrontendTest.h"
#include "core/module/ModuleCapability.h"
#include "core/module/ModuleSdkBridge.h"
#include "sdk/GFSDKHostApi.h"

/**
 * @file GpgCoreTestSdkTeardownRace.cpp
 * @brief Teardown racing an in-flight call: the case the never-free design
 * exists for.
 *
 * ## The situation being modelled
 *
 * A module is unloaded while a thread it started, and failed to stop, is
 * still calling into the host. This is not hypothetical tidiness: it is the
 * single failure mode that decided the shape of the whole context mechanism.
 * Nothing on the host side is ever freed -- not the context record, not the
 * `GFHostApi` table it contains -- precisely so that a call arriving from such
 * a thread can be REFUSED rather than followed into memory that has gone away.
 * A bounded leak of a few dozen bytes per module is the price, and it buys the
 * difference between a log line and a crash.
 *
 * ## Four properties, and why each needs a test
 *
 *   1. The context struct and its table stay readable for a stale worker.
 *      Nothing else proves the never-free rule was actually followed; if
 *      somebody "tidied up" the record at release, the table read below would
 *      be the read of a freed object.
 *   2. Authorization is revoked before any host state could become unsafe.
 *      Expressed as: no call that BEGAN after `ReleaseHostApi` returned was
 *      ever served. The test reads the release flag before each call, so a
 *      served-after outcome is unambiguous rather than a scheduling artefact.
 *   3. An in-flight primitive either completes safely or a later call is
 *      refused, with no third outcome. There are exactly two outcomes by
 *      construction -- the primitive returns a handle or it returns null --
 *      so "no third outcome" means: no crash, no hang, no torn state. Those
 *      are what the run itself checks.
 *   4. No use-after-free. This is the property a normal run CANNOT see: every
 *      read above would succeed just as happily against freed memory that
 *      nothing had reused yet. Run under `scripts/run_tests_asan.sh`, whose
 *      default `*Stress*` filter picks these tests up, which is why they are
 *      named for it.
 *
 * The sweep runs in the middle of the race on purpose. Unload does release,
 * then sweep, and the sweep walks the very ledger the workers are still
 * adding to, so it is the sharpest point in the whole sequence.
 */

namespace GpgFrontend::Test {

namespace {

constexpr auto* kRacingModule = "com.example.teardown.race";

auto Iterations() -> int {
  const auto iter = qEnvironmentVariableIntValue("GF_STRESS_ITER");
  // Deliberately modest by default. Every post-revocation call logs its own
  // refusal, which is the correct behaviour and poor reading in bulk; the
  // race is reproduced by the overlap, not by the volume.
  return iter > 0 ? qMin(iter, 200) : 40;
}

/// A worker that calls the host in a loop, exactly as a module's own thread
/// would, with nothing the host did anywhere on its stack.
class Caller : public QThread {
 public:
  Caller(const GFHostApi* host, const std::atomic<bool>& released,
         const std::atomic<bool>& stop)
      : host_(host), released_(released), stop_(stop) {}

  /// Calls whose outcome was decided while the grant was still live.
  std::atomic<int> served_before{0};
  std::atomic<int> refused_before{0};
  /// Calls that BEGAN after ReleaseHostApi() had returned.
  std::atomic<int> served_after{0};
  std::atomic<int> refused_after{0};

 protected:
  void run() override {
    while (!stop_.load(std::memory_order_acquire)) {
      // Read first, call second. If this says the grant is gone then the
      // release genuinely happened-before this call, so a served outcome
      // would be a real violation rather than a lost race.
      const auto after = released_.load(std::memory_order_acquire);

      auto* buf = host_->buffer->new_from_bytes(host_->context, "race", 4);
      const auto served = buf != nullptr;
      if (served) {
        // Touch it, so a handle that was swept out from under us would be
        // used rather than merely counted.
        (void)host_->buffer->size(host_->context, buf);
        host_->buffer->release(host_->context, buf);
      }

      auto& bucket = after ? (served ? served_after : refused_after)
                           : (served ? served_before : refused_before);
      bucket.fetch_add(1, std::memory_order_relaxed);

      QThread::msleep(1);
    }
  }

 private:
  const GFHostApi* host_;
  const std::atomic<bool>& released_;
  const std::atomic<bool>& stop_;
};

}  // namespace

TEST(SdkTeardownRaceStress,
     AStaleWorkerIsRefusedAndNeverFollowedIntoFreedMemory) {
  const auto* host = static_cast<const GFHostApi*>(Module::ModuleSdkMintHostApi(
      kRacingModule, Module::ModuleCapabilityMask({"storage"})));
  ASSERT_NE(host, nullptr);
  ASSERT_NE(host->buffer, nullptr);

  // What the table said while the module was live, to compare with after.
  const auto struct_size = host->struct_size;
  const auto abi_version = host->abi_version;
  const QByteArray module_id = host->module_id;

  std::atomic<bool> released{false};
  std::atomic<bool> stop{false};

  constexpr int kWorkers = 3;
  std::array<std::unique_ptr<Caller>, kWorkers> workers{};
  for (auto& worker : workers) {
    worker = std::make_unique<Caller>(host, released, stop);
    worker->start();
  }

  // Let them get genuinely into the loop before pulling the rug out. Without
  // this the release could land before the first call and the served side of
  // the test would never be exercised at all.
  const auto iterations = Iterations();
  QThread::msleep(static_cast<unsigned long>(iterations));

  // Unload, in the order the loader does it: the grant goes first, and only
  // then is the ledger swept. The workers are mid-loop throughout.
  Module::ModuleSdkReleaseHostApi(kRacingModule);
  released.store(true, std::memory_order_release);
  const auto swept = Module::ModuleSdkSweepHandles(kRacingModule);

  QThread::msleep(static_cast<unsigned long>(iterations));
  stop.store(true, std::memory_order_release);

  int served_before = 0;
  int refused_before = 0;
  int served_after = 0;
  int refused_after = 0;
  for (auto& worker : workers) {
    ASSERT_TRUE(worker->wait(60000));
    served_before += worker->served_before.load();
    refused_before += worker->refused_before.load();
    served_after += worker->served_after.load();
    refused_after += worker->refused_after.load();
  }

  // (3) The race actually happened. A test that never served anything, or
  // never refused anything, would pass while proving nothing.
  EXPECT_GT(served_before, 0)
      << "no call was served while the grant was live, so the teardown was "
         "not raced against anything";
  EXPECT_GT(refused_after, 0)
      << "no call arrived after revocation, so nothing was refused and the "
         "interesting half of the sequence never ran";
  EXPECT_EQ(refused_before, 0)
      << "a live grant refused a call, which is the opposite failure";

  // (2) The invariant. Revocation precedes any unsafe host state because
  // there is no unsafe host state to reach: nothing is freed, and the gate
  // closes first.
  EXPECT_EQ(served_after, 0)
      << served_after
      << " call(s) began after ReleaseHostApi() returned and were still "
         "served; authorization must be revoked before anything else happens";

  // The sweep ran against a ledger that was still being written to. It may
  // legitimately have found nothing, since these workers release what they
  // take, so its count is not the assertion; that it neither crashed nor
  // double-freed is, and that is ASan's judgement. What IS assertable is that
  // the ledger is consistent afterwards: a second pass finds nothing left.
  EXPECT_EQ(Module::ModuleSdkSweepHandles(kRacingModule), 0U)
      << "a sweep that raced the workers left " << swept
      << " reclaimed and something still outstanding";

  // (1) The never-free rule, read back. Under ASan this is the load that
  // would report a use-after-free if the record had been deleted at release.
  EXPECT_EQ(host->struct_size, struct_size);
  EXPECT_EQ(host->abi_version, abi_version);
  EXPECT_EQ(QByteArray(host->module_id), module_id);
  EXPECT_NE(host->buffer, nullptr)
      << "the table is abandoned at unload, not dismantled: a stale worker "
         "must find a function pointer to be refused by, not a null one to "
         "crash on";
}

/**
 * @brief A handle the sweep reclaimed is refused, not followed.
 *
 * The other half of the same reasoning, one level down. Buffer handles ARE
 * freed at unload -- the sweep is what reclaims a module's leaks -- so a
 * stale worker holding one is holding a pointer to freed memory. It stays
 * safe only because validity is decided by looking the pointer UP in a
 * registry rather than by dereferencing it to read a magic word.
 *
 * The context is minted again first, which is what a reload does, so that the
 * gate is open and the handle registry is the thing actually under test. With
 * a revoked context the call would be refused at the gate and this would
 * prove nothing.
 */
TEST(SdkTeardownRaceStress, ASweptHandleIsRefusedRatherThanDereferenced) {
  const auto mask = Module::ModuleCapabilityMask({"storage"});
  const auto* host = static_cast<const GFHostApi*>(
      Module::ModuleSdkMintHostApi(kRacingModule, mask));
  ASSERT_NE(host, nullptr);

  // Leaked on purpose: this is what the sweep exists to reclaim.
  auto* leaked = host->buffer->new_from_bytes(host->context, "stale", 5);
  ASSERT_NE(leaked, nullptr);
  ASSERT_EQ(host->buffer->size(host->context, leaked), 5U);

  Module::ModuleSdkReleaseHostApi(kRacingModule);
  EXPECT_EQ(Module::ModuleSdkSweepHandles(kRacingModule), 1U);

  // Reloaded. Same id, same record, live grant again.
  const auto* reloaded = static_cast<const GFHostApi*>(
      Module::ModuleSdkMintHostApi(kRacingModule, mask));
  ASSERT_NE(reloaded, nullptr);

  // The stale handle, presented to a live context. ASan would report the
  // use-after-free here if validity were decided by dereferencing.
  EXPECT_EQ(reloaded->buffer->size(reloaded->context, leaked), 0U);
  EXPECT_EQ(reloaded->buffer->data(reloaded->context, leaked), nullptr);
  // Releasing it twice must also be harmless, for the same reason.
  reloaded->buffer->release(reloaded->context, leaked);

  Module::ModuleSdkReleaseHostApi(kRacingModule);
}

}  // namespace GpgFrontend::Test
