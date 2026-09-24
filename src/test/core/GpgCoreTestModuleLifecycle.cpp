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
#include <QElapsedTimer>
#include <atomic>
#include <future>
#include <optional>

#include "GpgFrontendTest.h"
#include "core/module/Module.h"
#include "core/module/ModuleDispatchGate.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleManifest.h"
#include "core/thread/Task.h"
#include "core/thread/TaskRunnerGetter.h"
#include "core/utils/MemoryUtils.h"
#include "sdk/GFSDKBuildInfo.h"
#include "sdk/GFSDKHostApi.h"
#include "sdk/GFSDKModuleApi.h"
#include "sdk/GFSDKHostCommands.hpp"
#include "ui/command/CodecStage.h"
#include "ui/command/CommandRegistry.h"

/**
 * @file GpgCoreTestModuleLifecycle.cpp
 * @brief The module lifecycle, through the real manager, gates and grant.
 *
 * A module here is a table of C functions -- the same GFModuleApi a library
 * exports -- driven through ModuleManager exactly as a loaded package is. What
 * is pinned is what used to break: a failed activation left the module marked
 * active with its subscriptions live; a failed deactivation left it half
 * torn down; events went to modules that had gone; answers were accepted from
 * anyone, any number of times, and every event was kept for the life of the
 * process.
 *
 * Each test uses a module id of its own: the manager keeps every module it
 * ever registered.
 */

namespace GpgFrontend::Test {

namespace {

/// The event probe N subscribes to. One of the Host's generated family, and
/// one per probe: an event the bundled modules listen to would reach them,
/// and one shared by the probes would have them answering each other.
auto EventOf(int n) -> QString {
  return QStringLiteral("EDIT_TAB_TYPE_PROBE%1_OP_DECRYPT").arg(n);
}

/// What one probe module saw, and what it is told to do.
struct Probe {
  std::atomic<int> activates{0};
  std::atomic<int> deactivates{0};
  std::atomic<int> executes{0};
  std::atomic<int> unregisters{0};

  std::atomic<int> activate_rc{0};
  std::atomic<int> deactivate_rc{0};
  std::atomic<bool> answer_events{true};
  std::atomic<bool> register_command{true};
  std::atomic<bool> register_decoder{false};  ///< also provide `<id>.decode`
  std::atomic<int> decodes{0};                ///< times the decoder was entered

  const GFHostApi* host = nullptr;
  QByteArray id;
  QByteArray event;
  QByteArray last_trigger;
  QMutex mutex;
};

void FreeEvent(GFModuleEvent* event) {
  if (event == nullptr) return;
  SMAFree(const_cast<char*>(event->id));
  SMAFree(const_cast<char*>(event->trigger_id));
  SMAFree(event);
}

void Answer(Probe& p, const QByteArray& trigger) {
  GFModuleEventAnswer answer{};
  answer.struct_size = sizeof(answer);
  answer.event_id = p.event.constData();
  answer.trigger_id = trigger.constData();
  answer.params = nullptr;
  p.host->event->answer(p.host->context, &answer);
}

void Handler(void*, uint64_t, GFBufferView, GFBufferRef, GFBufferRef*, size_t) {
}

/// The codec shape a decoder must declare.
struct DecoderShape {
  static constexpr gf::cmd::Meta kMeta{"", "", "", "", 0,
                                       gf::cmd::kInputDecoder};
  using Args = gf::cmd::host::CodecArgs;
  using Result = gf::cmd::host::CodecResult;
};

/// A module of its own for each N: C function pointers cannot capture.
template <int N>
struct ProbeModule {
  static auto P() -> Probe& {
    static Probe probe;
    return probe;
  }

  static auto Id() -> QString {
    return QStringLiteral("com.example.lifecycle.probe%1").arg(N);
  }

  static auto Activate(const GFHostApi* host, void*) -> int {
    auto& p = P();
    ++p.activates;
    p.host = host;
    // What a real module does first: subscribe, then provide.
    host->event->subscribe(host->context, p.event.constData());
    if (p.register_command) {
      const auto command = (p.id + ".probe");
      const auto descriptor = QCborValue(QCborMap{}).toCbor();
      auto* buf =
          host->buffer->new_from_bytes(host->context, descriptor.constData(),
                                       static_cast<size_t>(descriptor.size()));
      GFCommandSpec spec{};
      spec.struct_size = sizeof(spec);
      spec.id = command.constData();
      spec.descriptor_cbor = buf;
      spec.handler = &Handler;
      host->command->register_command(host->context, &spec);
      host->buffer->release(host->context, buf);
    }
    if (p.register_decoder) {
      const auto command = (p.id + ".decode");
      const auto descriptor =
          QCborValue(gf::cmd::Describe<DecoderShape>()).toCbor();
      auto* buf =
          host->buffer->new_from_bytes(host->context, descriptor.constData(),
                                       static_cast<size_t>(descriptor.size()));
      GFCommandSpec spec{};
      spec.struct_size = sizeof(spec);
      spec.id = command.constData();
      spec.descriptor_cbor = buf;
      spec.handler = &Decode;
      host->command->register_command(host->context, &spec);
      host->buffer->release(host->context, buf);
    }
    return p.activate_rc.load();
  }

  /// Counts the entry, then answers "not mine" at once.
  static void Decode(void*, uint64_t call_id, GFBufferView, GFBufferRef args,
                     GFBufferRef* blobs, size_t blob_count) {
    auto& p = P();
    ++p.decodes;
    p.host->buffer->release(p.host->context, args);
    for (size_t i = 0; i < blob_count; ++i) {
      p.host->buffer->release(p.host->context, blobs[i]);
    }
    gf::cmd::EncodeState st;
    const auto result =
        QCborValue(gf::cmd::EncodeMap(gf::cmd::host::CodecResult{}, st))
            .toCbor();
    auto* buf = p.host->buffer->new_from_bytes(
        p.host->context, result.constData(), static_cast<size_t>(result.size()));
    p.host->command->complete(p.host->context, call_id, GF_CMD_OK, buf, nullptr,
                              0, nullptr);
  }

  static auto Execute(GFModuleEvent* event) -> int {
    auto& p = P();
    ++p.executes;
    const QByteArray trigger(event->trigger_id);
    FreeEvent(event);
    {
      const QMutexLocker lock(&p.mutex);
      p.last_trigger = trigger;
    }
    if (p.answer_events) Answer(p, trigger);
    return 0;
  }

  static auto Deactivate() -> int {
    ++P().deactivates;
    return P().deactivate_rc.load();
  }

  static void Unregister() { ++P().unregisters; }

  static auto Api() -> const GFModuleApi* {
    static const QByteArray kId = Id().toUtf8();
    static const GFModuleApi kApi = {
        sizeof(GFModuleApi), GF_SDK_ABI_VERSION, kId.constData(), "1.0.0",
        &Activate,           &Execute,           &Deactivate,     &Unregister,
    };
    return &kApi;
  }

  /// Register it with the manager, not auto-activated.
  static auto Register(const QStringList& capabilities = {})
      -> Module::ModulePtr {
    P().id = Id().toUtf8();
    P().event = EventOf(N).toUtf8();
    auto module = SecureCreateSharedObject<Module::Module>(Api(), QString());
    Module::ModuleManifest manifest;
    manifest.id = Id();
    manifest.version = "1.0.0";
    manifest.events = {EventOf(N)};
    manifest.commands = {Id() + ".probe", Id() + ".decode"};
    manifest.capabilities = capabilities;
    manifest.translation_context = "GTrC";
    module->SetModuleManifest(manifest);
    module->SetSourcePackagePath(
        QStringLiteral("/nonexistent/%1/module.gfmodule").arg(Id()));
    Module::ModuleManager::GetInstance().RegisterLoadedModule(module, false);
    return module;
  }
};

/// Everything posted to the module runner so far has run.
void DrainModuleRunner() {
  auto done = std::make_shared<std::promise<void>>();
  auto finished = done->get_future();
  Thread::TaskRunnerGetter::GetInstance()
      .GetTaskRunner(Thread::TaskRunnerGetter::kTaskRunnerType_Module)
      ->PostTask(new Thread::Task(
          [done](const DataObjectPtr&) -> int {
            done->set_value();
            return 0;
          },
          "test/drain"));
  ASSERT_EQ(finished.wait_for(std::chrono::seconds(10)),
            std::future_status::ready);
}

auto Manager() -> Module::ModuleManager& {
  return Module::ModuleManager::GetInstance();
}

auto HasCommand(const QString& id) -> bool {
  return UI::CommandRegistry::Instance().Contains(id);
}

/// What an event's callback heard, from which listener.
struct Heard {
  QMutex mutex;
  QList<QPair<QString, Module::Event::Params>> answers;

  auto Count() -> int {
    const QMutexLocker lock(&mutex);
    return static_cast<int>(answers.size());
  }
};

auto Trigger(int n, const std::shared_ptr<Heard>& heard)
    -> Module::EventReference {
  auto event = Module::MakeEvent(
      EventOf(n), {},
      [heard](const Module::EventIdentifier&,
              const Module::Event::ListenerIdentifier& listener,
              const Module::Event::Params& params) {
        const QMutexLocker lock(&heard->mutex);
        heard->answers.append({listener, params});
      });
  Manager().TriggerEvent(event);
  return event;
}

}  // namespace

TEST(ModuleLifecycleTest, AnActivatedModuleIsActiveSubscribedAndProviding) {
  using M = ProbeModule<1>;
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();

  EXPECT_EQ(M::P().activates, 1);
  EXPECT_TRUE(Manager().IsModuleActivated(M::Id()));
  EXPECT_TRUE(Manager().GetModuleListening(M::Id()).contains(EventOf(1)));
  EXPECT_TRUE(HasCommand(M::Id() + ".probe"));
  EXPECT_FALSE(Module::ModuleEntryGate(M::Id()).IsClosed());
}

TEST(ModuleLifecycleTest, AFailedActivationLeavesNothingBehind) {
  using M = ProbeModule<2>;
  M::P().activate_rc = -1;
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();

  EXPECT_EQ(M::P().activates, 1);
  // It subscribed and registered before it failed. None of that survives.
  EXPECT_FALSE(Manager().IsModuleActivated(M::Id()));
  EXPECT_TRUE(Manager().GetModuleListening(M::Id()).isEmpty());
  EXPECT_FALSE(HasCommand(M::Id() + ".probe"));
  EXPECT_TRUE(Module::ModuleEntryGate(M::Id()).IsClosed());

  // And the grant it was handed is dead: a thread it left behind is refused.
  auto* buf = M::P().host->buffer->new_from_bytes(M::P().host->context, "x", 1);
  EXPECT_EQ(buf, nullptr);

  // Never activated, so never deactivated.
  EXPECT_EQ(M::P().deactivates, 0);
}

TEST(ModuleLifecycleTest, AFailedDeactivationStillDeactivates) {
  using M = ProbeModule<3>;
  M::P().deactivate_rc = -1;
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();
  ASSERT_TRUE(Manager().IsModuleActivated(M::Id()));

  Manager().DeactivateModule(M::Id());
  DrainModuleRunner();

  EXPECT_EQ(M::P().deactivates, 1);
  EXPECT_FALSE(Manager().IsModuleActivated(M::Id()));
  EXPECT_TRUE(Manager().GetModuleListening(M::Id()).isEmpty());
  EXPECT_FALSE(HasCommand(M::Id() + ".probe"));
  EXPECT_TRUE(Module::ModuleEntryGate(M::Id()).IsClosed());
}

TEST(ModuleLifecycleTest, RepeatedCyclesEndWhereTheyShould) {
  using M = ProbeModule<4>;
  M::Register();
  for (int i = 0; i < 5; ++i) {
    Manager().ActiveModule(M::Id());
    DrainModuleRunner();
    ASSERT_TRUE(Manager().IsModuleActivated(M::Id())) << i;
    ASSERT_TRUE(HasCommand(M::Id() + ".probe")) << i;

    Manager().DeactivateModule(M::Id());
    DrainModuleRunner();
    ASSERT_FALSE(Manager().IsModuleActivated(M::Id())) << i;
    ASSERT_FALSE(HasCommand(M::Id() + ".probe")) << i;
  }
  EXPECT_EQ(M::P().activates, 5);
  EXPECT_EQ(M::P().deactivates, 5);

  // A second deactivation of an inactive module is a no-op, not a hook call.
  Manager().DeactivateModule(M::Id());
  DrainModuleRunner();
  EXPECT_EQ(M::P().deactivates, 5);
}

TEST(ModuleLifecycleTest, ADeactivatedModuleCannotReachTheHostOrBeReached) {
  using M = ProbeModule<5>;
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();
  const auto* host = M::P().host;

  Manager().DeactivateModule(M::Id());
  DrainModuleRunner();

  // Its grant is revoked: a worker it left running is refused.
  EXPECT_EQ(host->buffer->new_from_bytes(host->context, "x", 1), nullptr);
  // And the host will not call into it: its gate is closed.
  EXPECT_FALSE(Module::ModuleEntryGate(M::Id()).TryEnter());
}

TEST(ModuleLifecycleTest, AnEventIsAnsweredOnceAndThenForgotten) {
  using M = ProbeModule<6>;
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();

  const auto pending_before = Manager().PendingTriggerCount();
  auto heard = std::make_shared<Heard>();
  Trigger(6, heard);
  DrainModuleRunner();
  ASSERT_TRUE(WaitFor([&] { return heard->Count() >= 1; }));

  EXPECT_GE(M::P().executes, 1);
  // The event, its parameters and its callback are gone once answered.
  EXPECT_EQ(Manager().PendingTriggerCount(), pending_before);

  // A second answer to the same trigger is refused and heard by nobody.
  QByteArray trigger;
  {
    const QMutexLocker lock(&M::P().mutex);
    trigger = M::P().last_trigger;
  }
  EXPECT_FALSE(Manager().AnswerEvent(QString::fromUtf8(trigger), M::Id(), {}));
}

TEST(ModuleLifecycleTest, OnlyAListenerThatWasAskedMayAnswer) {
  using M = ProbeModule<7>;
  M::P().answer_events = false;  // it will defer, and never answer
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();

  auto heard = std::make_shared<Heard>();
  auto event = Trigger(7, heard);
  DrainModuleRunner();
  const auto trigger = event->GetTriggerIdentifier();

  // Another module presenting the trigger id is refused.
  EXPECT_FALSE(
      Manager().AnswerEvent(trigger, "com.example.lifecycle.intruder", {}));
  // The listener that was asked may, once.
  EXPECT_TRUE(Manager().AnswerEvent(trigger, M::Id(), {}));
  EXPECT_FALSE(Manager().AnswerEvent(trigger, M::Id(), {}));
  ASSERT_TRUE(WaitFor([&] { return heard->Count() == 1; }));
}

TEST(ModuleLifecycleTest, AnAnswerOwedByADeactivatedModuleIsAFailure) {
  using M = ProbeModule<8>;
  M::P().answer_events = false;
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();

  auto heard = std::make_shared<Heard>();
  Trigger(8, heard);
  DrainModuleRunner();
  ASSERT_EQ(heard->Count(), 0);

  // Whoever waits on the callback -- a modal waiting dialog -- is told.
  Manager().DeactivateModule(M::Id());
  DrainModuleRunner();
  ASSERT_TRUE(WaitFor([&] { return heard->Count() == 1; }));
  const QMutexLocker lock(&heard->mutex);
  EXPECT_EQ(heard->answers[0].first, M::Id());
  EXPECT_EQ(heard->answers[0].second.value("ret"), GFBuffer(QString("-1")));
}

TEST(ModuleLifecycleTest, AnEventNobodyIsListeningToIsStillAnswered) {
  auto heard = std::make_shared<Heard>();
  // No active module subscribes to this one in a test process.
  auto event = Module::MakeEvent(
      "EDIT_TAB_TYPE_NOBODY_OP_DECRYPT", {},
      [heard](const Module::EventIdentifier&,
              const Module::Event::ListenerIdentifier& listener,
              const Module::Event::Params& params) {
        const QMutexLocker lock(&heard->mutex);
        heard->answers.append({listener, params});
      });
  EXPECT_FALSE(Manager().IsEventListening("EDIT_TAB_TYPE_NOBODY_OP_DECRYPT"));
  Manager().TriggerEvent(event);
  ASSERT_TRUE(WaitFor([&] { return heard->Count() == 1; }));
  const QMutexLocker lock(&heard->mutex);
  EXPECT_EQ(heard->answers[0].second.value("ret"), GFBuffer(QString("-1")));
}

TEST(ModuleLifecycleTest, ListeningMeansAnActiveListener) {
  using M = ProbeModule<9>;
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();
  ASSERT_TRUE(Manager().GetModuleListening(M::Id()).contains(EventOf(9)));

  Manager().DeactivateModule(M::Id());
  DrainModuleRunner();
  EXPECT_TRUE(Manager().GetModuleListening(M::Id()).isEmpty());
}

TEST(ModuleLifecycleTest,
     AnEventQueuedBeforeDeactivationDoesNotReachTheModule) {
  using M = ProbeModule<10>;
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();

  // Close the module's gate the way deactivation's first step does, with an
  // event already dispatched to it but not yet delivered.
  auto heard = std::make_shared<Heard>();
  Module::ModuleEntryGate(M::Id()).Close();
  Trigger(10, heard);
  DrainModuleRunner();

  EXPECT_EQ(M::P().executes, 0);
  ASSERT_TRUE(WaitFor([&] { return heard->Count() == 1; }));
  {
    const QMutexLocker lock(&heard->mutex);
    EXPECT_EQ(heard->answers[0].second.value("ret"), GFBuffer(QString("-1")));
  }
  Module::ModuleEntryGate(M::Id()).Open();
}

auto SnapshotOf(const QString& id)
    -> std::optional<Module::ModuleLifecycleSnapshot> {
  for (const auto& m : Manager().LifecycleSnapshot()) {
    if (m.id == id) return m;
  }
  return std::nullopt;
}

TEST(ModuleLifecycleTest, TheSnapshotNamesEachModulesState) {
  using Good = ProbeModule<11>;
  using Bad = ProbeModule<12>;
  Bad::P().activate_rc = -1;
  Good::Register();
  Bad::Register();
  Manager().ActiveModule(Good::Id());
  Manager().ActiveModule(Bad::Id());
  DrainModuleRunner();

  const auto good = SnapshotOf(Good::Id());
  ASSERT_TRUE(good.has_value());
  EXPECT_EQ(good->state, Module::ModuleLifecycleState::kACTIVE);
  EXPECT_FALSE(good->integrated);
  EXPECT_TRUE(good->listening.contains(EventOf(11)));

  const auto bad = SnapshotOf(Bad::Id());
  ASSERT_TRUE(bad.has_value());
  EXPECT_EQ(bad->state, Module::ModuleLifecycleState::kFAILED);
  EXPECT_TRUE(bad->listening.isEmpty());

  Manager().DeactivateModule(Good::Id());
  DrainModuleRunner();
  EXPECT_EQ(SnapshotOf(Good::Id())->state,
            Module::ModuleLifecycleState::kINACTIVE);
  EXPECT_EQ(
      Module::ModuleLifecycleStateName(Module::ModuleLifecycleState::kINACTIVE),
      "Inactive");
}

TEST(ModuleLifecycleTest, TheSnapshotCountsTheAnswersAModuleOwes) {
  using M = ProbeModule<13>;
  M::P().answer_events = false;  // defers
  M::Register();
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();
  EXPECT_EQ(Manager().ListenersOf(EventOf(13)), QStringList{M::Id()});

  auto heard = std::make_shared<Heard>();
  auto event = Trigger(13, heard);
  DrainModuleRunner();
  EXPECT_EQ(SnapshotOf(M::Id())->owed_answers, 1);

  ASSERT_TRUE(
      Manager().AnswerEvent(event->GetTriggerIdentifier(), M::Id(), {}));
  EXPECT_EQ(SnapshotOf(M::Id())->owed_answers, 0);

  Manager().DeactivateModule(M::Id());
  DrainModuleRunner();
  EXPECT_TRUE(Manager().ListenersOf(EventOf(13)).isEmpty());
}

TEST(ModuleLifecycleTest, AModuleWithoutASignedManifestIsNotActivated) {
  static const QByteArray kId = "com.example.lifecycle.unsigned";
  static std::atomic<int> activates{0};
  static const GFModuleApi kApi = {
      sizeof(GFModuleApi),
      GF_SDK_ABI_VERSION,
      kId.constData(),
      "1.0.0",
      [](const GFHostApi*, void*) -> int { return ++activates, 0; },
      [](GFModuleEvent* e) -> int { return FreeEvent(e), 0; },
      []() -> int { return 0; },
      []() {},
  };
  auto module = SecureCreateSharedObject<Module::Module>(&kApi, QString());
  ASSERT_TRUE(module->IsGood());
  Manager().RegisterLoadedModule(module, false);
  Manager().ActiveModule(QString::fromUtf8(kId));
  DrainModuleRunner();

  EXPECT_EQ(activates, 0);
  EXPECT_FALSE(Manager().IsModuleActivated(QString::fromUtf8(kId)));
}

TEST(ModuleLifecycleTest, AModuleIdMustHaveTheOneIdentityShape) {
  static const GFModuleApi kApi = {
      sizeof(GFModuleApi),
      GF_SDK_ABI_VERSION,
      "com.Example.MixedCase",
      "1.0.0",
      [](const GFHostApi*, void*) -> int { return 0; },
      [](GFModuleEvent*) -> int { return 0; },
      []() -> int { return 0; },
      []() {},
  };
  EXPECT_FALSE(Module::Module(&kApi, QString()).IsGood());
}

// The pre-decrypt decoder, through the real gates. Once its module starts to
// deactivate, a decoder is never entered again -- and the Host's decrypt goes
// on as if it had never been there.
TEST(ModuleLifecycleTest, ADeactivatedDecoderIsNeverEnteredAndDecryptGoesOn) {
  using M = ProbeModule<20>;
  M::P().register_decoder = true;
  M::Register({"editor"});
  Manager().ActiveModule(M::Id());
  DrainModuleRunner();
  ASSERT_TRUE(Manager().IsModuleActivated(M::Id()));
  ASSERT_TRUE(UI::CommandRegistry::Instance()
                  .ProvidersWithFlag(gf::cmd::kInputDecoder)
                  .contains(M::Id() + ".decode"));

  const auto run = []() {
    auto done = std::make_shared<std::atomic<int>>(0);
    auto kind = std::make_shared<UI::CodecStageResult::Kind>();
    UI::CodecStage::RunDecoders(GFBuffer(QByteArray("plain words, no token")),
                                [done, kind](UI::CodecStageResult r) {
                                  *kind = r.kind;
                                  ++*done;
                                });
    QElapsedTimer t;
    t.start();
    while (done->load() == 0 && t.elapsed() < 10000) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    EXPECT_EQ(done->load(), 1);
    return *kind;
  };

  EXPECT_EQ(run(), UI::CodecStageResult::Kind::kNotHandled);
  EXPECT_EQ(M::P().decodes.load(), 1) << "an active decoder is asked";

  Manager().DeactivateModule(M::Id());
  DrainModuleRunner();
  EXPECT_TRUE(Module::ModuleEntryGate(M::Id()).IsClosed());

  EXPECT_EQ(run(), UI::CodecStageResult::Kind::kNotHandled);
  EXPECT_EQ(M::P().decodes.load(), 1) << "a deactivated decoder is not";
}

}  // namespace GpgFrontend::Test
