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

#include <QCoreApplication>
#include <QSemaphore>
#include <QThread>
#include <atomic>
#include <mutex>

#include "GpgFrontendTest.h"
#include "sdk/GFSDKHostApi.h"
#include "ui/command/CommandRegistry.h"

/**
 * @file GFUiCommandRegistryTest.cpp
 * @brief The registry's policy, pinned. Every caller goes through it, so
 *        what it refuses here is refused everywhere.
 *
 * The registry is a process-wide singleton; each test uses ids of its own
 * and removes what it registered.
 */

namespace GpgFrontend::Test {

namespace {

using UI::CommandCaller;
using UI::CommandProvider;
using UI::CommandRegistry;

auto Reg() -> CommandRegistry& { return CommandRegistry::Instance(); }

auto Module(const QString& id, uint32_t caps = 0) -> CommandCaller {
  return CommandCaller{id, caps, QStringLiteral("module")};
}

auto Host() -> CommandCaller { return CommandCaller{{}, 0, "host"}; }

/// A provider that answers at once with {"ok": true}.
auto Immediate(const QString& id, const QString& owner, uint32_t caps = 0,
               uint32_t flags = 0) -> CommandProvider {
  CommandProvider p;
  p.id = id;
  p.owner = owner;
  p.required_caps = caps;
  p.flags = flags;
  p.run = [](const gf::cmd::CommandContext&, QCborMap,
             std::vector<gf::cmd::Blob>, gf::cmd::Completer done) {
    done({GF_CMD_OK, 0, QCborMap{{QStringLiteral("ok"), true}}, {}, {}});
  };
  return p;
}

/// A provider that parks its completer for the test to fire later.
struct Parked {
  std::mutex mutex;
  gf::cmd::Completer done;
  QSemaphore arrived;
};

auto Parking(const QString& id, const QString& owner, Parked* parked)
    -> CommandProvider {
  CommandProvider p;
  p.id = id;
  p.owner = owner;
  p.run = [parked](const gf::cmd::CommandContext&, QCborMap,
                   std::vector<gf::cmd::Blob>, gf::cmd::Completer done) {
    {
      std::lock_guard<std::mutex> lock(parked->mutex);
      parked->done = std::move(done);
    }
    parked->arrived.release();
  };
  return p;
}

}  // namespace

TEST(CommandRegistryTest, RegistrationStaysInsideTheOwnersNamespace) {
  EXPECT_EQ(Reg().Register(Immediate("org.gpgfrontend.test.reg.host", {})),
            GF_CMD_OK);
  EXPECT_EQ(Reg().Register(Immediate("com.example.other.x", {})),
            GF_CMD_E_DENIED)
      << "the Host registers only under org.gpgfrontend.";

  EXPECT_EQ(
      Reg().Register(Immediate("com.example.reg.a.x", "com.example.reg.a")),
      GF_CMD_OK);
  EXPECT_EQ(
      Reg().Register(Immediate("com.example.reg.b.x", "com.example.reg.a")),
      GF_CMD_E_DENIED)
      << "a module cannot claim another module's namespace";
  EXPECT_EQ(Reg().Register(Immediate("org.gpgfrontend.test.reg.steal",
                                     "com.example.reg.a")),
            GF_CMD_E_DENIED)
      << "nor the Host's";
  EXPECT_EQ(
      Reg().Register(Immediate("com.example.reg.a.x", "com.example.reg.a")),
      GF_CMD_E_DENIED)
      << "an id is registered once";
  EXPECT_EQ(
      Reg().Register(Immediate("com.example.reg.a.hostonly",
                               "com.example.reg.a", 0, gf::cmd::kHostOnly)),
      GF_CMD_E_DENIED);

  // A verified module registers only what its signed manifest lists.
  EXPECT_EQ(Reg().Register(
                Immediate("com.example.reg.a.unlisted", "com.example.reg.a"),
                QStringList{"com.example.reg.a.listed"}),
            GF_CMD_E_DENIED);
  EXPECT_EQ(
      Reg().Register(Immediate("com.example.reg.a.listed", "com.example.reg.a"),
                     QStringList{"com.example.reg.a.listed"}),
      GF_CMD_OK);

  EXPECT_EQ(Reg().Unregister("com.example.reg.a.x", "com.example.reg.b"),
            GF_CMD_E_DENIED)
      << "only the owner withdraws";
  Reg().RemoveAllFor("com.example.reg.a");
  EXPECT_FALSE(Reg().Contains("com.example.reg.a.listed"));
  EXPECT_EQ(Reg().Unregister("org.gpgfrontend.test.reg.host", {}), GF_CMD_OK);
}

TEST(CommandRegistryTest, InvocationChecksExistenceOwnershipAndCapabilities) {
  ASSERT_EQ(Reg().Register(
                Immediate("org.gpgfrontend.test.inv.gpg", {}, GF_HOST_CAP_GPG)),
            GF_CMD_OK);
  ASSERT_EQ(Reg().Register(Immediate("org.gpgfrontend.test.inv.hostonly", {}, 0,
                                     gf::cmd::kHostOnly)),
            GF_CMD_OK);

  int delivered = 0;
  auto done = [&](gf::cmd::RawResult) { ++delivered; };

  EXPECT_EQ(Reg()
                .Invoke("org.gpgfrontend.test.inv.none", {}, {},
                        Module("com.example.inv"), {}, done)
                .status,
            GF_CMD_E_UNKNOWN);
  EXPECT_EQ(Reg()
                .Invoke("org.gpgfrontend.test.inv.gpg", {}, {},
                        Module("com.example.inv", GF_HOST_CAP_UI), {}, done)
                .status,
            GF_CMD_E_DENIED)
      << "the caller lacks the capability the command requires";
  EXPECT_EQ(Reg()
                .Invoke("org.gpgfrontend.test.inv.hostonly", {}, {},
                        Module("com.example.inv", ~0U), {}, done)
                .status,
            GF_CMD_E_DENIED);
  EXPECT_EQ(delivered, 0) << "a refused call never calls back";

  EXPECT_EQ(Reg()
                .Invoke("org.gpgfrontend.test.inv.gpg", {}, {},
                        Module("com.example.inv", GF_HOST_CAP_GPG), {}, done)
                .status,
            GF_CMD_OK);
  EXPECT_EQ(
      Reg()
          .Invoke("org.gpgfrontend.test.inv.hostonly", {}, {}, Host(), {}, done)
          .status,
      GF_CMD_OK);
  EXPECT_EQ(delivered, 2);

  Reg().Unregister("org.gpgfrontend.test.inv.gpg", {});
  Reg().Unregister("org.gpgfrontend.test.inv.hostonly", {});
}

TEST(CommandRegistryTest, StateHidesWhatTheCallerMayNotUse) {
  auto p = Immediate("org.gpgfrontend.test.state", {}, GF_HOST_CAP_GPG);
  p.state = [](const gf::cmd::CommandContext& ctx) -> uint32_t {
    return ctx.has_selection ? GF_CMD_STATE_ENABLED | GF_CMD_STATE_VISIBLE
                             : GF_CMD_STATE_VISIBLE;
  };
  ASSERT_EQ(Reg().Register(std::move(p)), GF_CMD_OK);

  gf::cmd::CommandContext ctx;
  EXPECT_EQ(Reg().State("org.gpgfrontend.test.state", Host(), ctx),
            GF_CMD_STATE_VISIBLE);
  ctx.has_selection = true;
  EXPECT_EQ(Reg().State("org.gpgfrontend.test.state", Host(), ctx),
            GF_CMD_STATE_ENABLED | GF_CMD_STATE_VISIBLE);
  EXPECT_EQ(
      Reg().State("org.gpgfrontend.test.state", Module("com.example.s"), ctx),
      0U)
      << "no capability, no state: not even visible";
  EXPECT_EQ(Reg().State("org.gpgfrontend.test.nothing", Host(), ctx), 0U);

  Reg().Unregister("org.gpgfrontend.test.state", {});
}

// Commands that touch the UI run on the GUI thread, wherever they are
// invoked from. This test body is itself off the GUI thread.
TEST(CommandRegistryTest, AGuiCommandRunsOnTheGuiThread) {
  ASSERT_NE(QThread::currentThread(), QCoreApplication::instance()->thread());

  std::atomic<bool> on_gui{false};
  CommandProvider p =
      Immediate("org.gpgfrontend.test.gui", {}, 0, gf::cmd::kNeedsGuiThread);
  p.run = [&](const gf::cmd::CommandContext&, QCborMap,
              std::vector<gf::cmd::Blob>, gf::cmd::Completer done) {
    on_gui = QThread::currentThread() == QCoreApplication::instance()->thread();
    done({});
  };
  ASSERT_EQ(Reg().Register(std::move(p)), GF_CMD_OK);

  QSemaphore finished;
  ASSERT_EQ(Reg()
                .Invoke("org.gpgfrontend.test.gui", {}, {}, Host(), {},
                        [&](gf::cmd::RawResult) { finished.release(); })
                .status,
            GF_CMD_OK);
  ASSERT_TRUE(finished.tryAcquire(1, 10000));
  EXPECT_TRUE(on_gui.load());

  Reg().Unregister("org.gpgfrontend.test.gui", {});
}

TEST(CommandRegistryTest, NoCallbackAfterCancelReturns) {
  Parked parked;
  ASSERT_EQ(Reg().Register(Parking("com.example.cancel.p.cmd",
                                   "com.example.cancel.p", &parked)),
            GF_CMD_OK);

  std::atomic<int> delivered{0};
  const auto ticket = Reg().Invoke("com.example.cancel.p.cmd", {}, {},
                                   Module("com.example.cancel.c"), {},
                                   [&](gf::cmd::RawResult) { ++delivered; });
  ASSERT_EQ(ticket.status, GF_CMD_OK);
  ASSERT_TRUE(parked.arrived.tryAcquire(1, 10000));

  EXPECT_EQ(Reg().Cancel(ticket.call_id, "com.example.cancel.other"),
            GF_CMD_E_DENIED)
      << "only the caller cancels its own call";
  EXPECT_FALSE(Reg().IsCancelled(ticket.call_id));
  EXPECT_EQ(Reg().Cancel(ticket.call_id, "com.example.cancel.c"), GF_CMD_OK);
  EXPECT_TRUE(Reg().IsCancelled(ticket.call_id));

  // The provider finishes anyway; nobody is told.
  gf::cmd::Completer done;
  {
    std::lock_guard<std::mutex> lock(parked.mutex);
    done = std::move(parked.done);
  }
  done({GF_CMD_OK, 0, {}, {}, {}});
  EXPECT_EQ(delivered.load(), 0);

  Reg().RemoveAllFor("com.example.cancel.p");
}

TEST(CommandRegistryTest, OnlyTheProviderFinishesACall) {
  Parked parked;
  ASSERT_EQ(Reg().Register(Parking("com.example.finish.p.cmd",
                                   "com.example.finish.p", &parked)),
            GF_CMD_OK);

  std::atomic<int> status{1};
  const auto ticket =
      Reg().Invoke("com.example.finish.p.cmd", {}, {}, Host(), {},
                   [&](gf::cmd::RawResult r) { status = r.status; });
  ASSERT_TRUE(parked.arrived.tryAcquire(1, 10000));

  EXPECT_EQ(Reg().Finish(ticket.call_id, "com.example.finish.other",
                         {GF_CMD_OK, 0, {}, {}, {}}),
            GF_CMD_E_DENIED);
  EXPECT_EQ(status.load(), 1);
  EXPECT_EQ(Reg().Finish(ticket.call_id, "com.example.finish.p",
                         {GF_CMD_E_FAILED, 0, {}, {}, {}}),
            GF_CMD_OK);
  EXPECT_EQ(status.load(), GF_CMD_E_FAILED);
  EXPECT_EQ(Reg().Finish(ticket.call_id, "com.example.finish.p",
                         {GF_CMD_OK, 0, {}, {}, {}}),
            GF_CMD_E_UNKNOWN)
      << "a call finishes once";

  Reg().RemoveAllFor("com.example.finish.p");
}

// What deactivation relies on: afterwards nothing enters the module and
// nothing is delivered to it, and whoever it was serving is told.
TEST(CommandRegistryTest, RemovingAModuleWithdrawsEverythingItTouches) {
  Parked parked;
  ASSERT_EQ(Reg().Register(
                Parking("com.example.rm.p.cmd", "com.example.rm.p", &parked)),
            GF_CMD_OK);

  // A call the module is serving, for the Host.
  std::atomic<int> served_status{1};
  ASSERT_EQ(Reg()
                .Invoke("com.example.rm.p.cmd", {}, {}, Host(), {},
                        [&](gf::cmd::RawResult r) { served_status = r.status; })
                .status,
            GF_CMD_OK);
  ASSERT_TRUE(parked.arrived.tryAcquire(1, 10000));

  // A call the module made, to somebody else.
  Parked other;
  ASSERT_EQ(Reg().Register(
                Parking("com.example.rm.q.cmd", "com.example.rm.q", &other)),
            GF_CMD_OK);
  std::atomic<int> made_delivered{0};
  ASSERT_EQ(
      Reg()
          .Invoke("com.example.rm.q.cmd", {}, {}, Module("com.example.rm.p"),
                  {}, [&](gf::cmd::RawResult) { ++made_delivered; })
          .status,
      GF_CMD_OK);
  ASSERT_TRUE(other.arrived.tryAcquire(1, 10000));
  EXPECT_EQ(Reg().PendingCallsOf("com.example.rm.p"), 2);

  Reg().RemoveAllFor("com.example.rm.p");
  Reg().RemoveAllFor("com.example.rm.p");  // idempotent

  EXPECT_FALSE(Reg().Contains("com.example.rm.p.cmd"));
  EXPECT_EQ(served_status.load(), GF_CMD_E_UNAVAILABLE);
  EXPECT_EQ(Reg().PendingCallsOf("com.example.rm.p"), 0);

  gf::cmd::Completer late;
  {
    std::lock_guard<std::mutex> lock(other.mutex);
    late = std::move(other.done);
  }
  late({GF_CMD_OK, 0, {}, {}, {}});
  EXPECT_EQ(made_delivered.load(), 0)
      << "a result owed to a removed module is never delivered";

  Reg().RemoveAllFor("com.example.rm.q");
}

TEST(CommandRegistryTest, AWithdrawnModuleIsClosedUntilItIsReopened) {
  // Between a module's withdrawal and its reactivation, whatever of it is
  // still running -- its UI script, a worker -- must not be able to reach
  // the registry: it neither invokes nor registers.
  ASSERT_EQ(Reg().Register(Immediate("com.example.closed.host.cmd",
                                     "com.example.closed.host")),
            GF_CMD_OK);
  Reg().RemoveAllFor("com.example.closed.m");

  EXPECT_EQ(Reg()
                .Invoke("com.example.closed.host.cmd", {}, {},
                        Module("com.example.closed.m"), {}, nullptr)
                .status,
            GF_CMD_E_UNAVAILABLE);
  EXPECT_EQ(Reg().Register(
                Immediate("com.example.closed.m.cmd", "com.example.closed.m")),
            GF_CMD_E_UNAVAILABLE);

  Reg().Reopen("com.example.closed.m");
  EXPECT_EQ(Reg()
                .Invoke("com.example.closed.host.cmd", {}, {},
                        Module("com.example.closed.m"), {}, nullptr)
                .status,
            GF_CMD_OK);
  EXPECT_EQ(Reg().Register(
                Immediate("com.example.closed.m.cmd", "com.example.closed.m")),
            GF_CMD_OK);

  Reg().RemoveAllFor("com.example.closed.m");
  Reg().Reopen("com.example.closed.m");
  Reg().RemoveAllFor("com.example.closed.host");
  Reg().Reopen("com.example.closed.host");
}

TEST(CommandRegistryTest, AModuleCannotClaimAChildModulesNamespace) {
  // `a.b` owning `a.b.c.x` would take the command namespace of module `a.b.c`.
  EXPECT_EQ(Reg().Register(
                Immediate("com.example.nest.child.cmd", "com.example.nest")),
            GF_CMD_E_DENIED);
  EXPECT_EQ(
      Reg().Register(Immediate("com.example.nest.cmd", "com.example.nest")),
      GF_CMD_OK);
  Reg().RemoveAllFor("com.example.nest");
  Reg().Reopen("com.example.nest");
}

TEST(CommandRegistryTest, HostBlobsAreSharedNotCopied) {
  // const throughout: GFBuffer is copy-on-write, and a non-const Data()
  // would detach -- which is a copy made by the test, not by the registry.
  const GFBuffer buffer(QByteArray("secret"));
  const auto blob = UI::MakeHostBlob(buffer);
  const auto back = UI::BlobToGFBuffer(blob);
  EXPECT_EQ(back.Data(), buffer.Data()) << "the same storage, not a copy";
  EXPECT_EQ(blob.Data(), buffer.Data());
}

}  // namespace GpgFrontend::Test
