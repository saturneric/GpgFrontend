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

#include <QThread>
#include <cstring>

#include "GFModule.h"
#include "GFModuleRuntimeBoot.h"
#include "GFSDKBuildInfo.h"
#include "ModuleRuntimeStubs.h"

/**
 * @file ModuleRuntimeTest.cpp
 * @brief The module-side framework, tested without a host.
 *
 * gf_module_runtime and gf_sdk reach the host only through the GFHostApi
 * table handed to activate(), so they can be linked here against a recording
 * stand-in for that table and exercised directly. That is the whole reason
 * the modules no longer each carry their own copy of this logic: there was
 * previously nowhere to test it.
 */

namespace {

using stubs::Rec;

// ------------------------------------------------------------ test hooks

GFEventResult g_last_result = GFEventResult::Ok();
int g_activate_calls = 0;
int g_deactivate_calls = 0;
int g_unload_calls = 0;
GFEvent g_captured;

auto OnAlpha(const GFEvent& event) -> GFEventResult {
  g_captured = event;
  return g_last_result;
}

auto OnBeta(const GFEvent& /*event*/) -> GFEventResult {
  return GFEventResult::Ok({{"extra", "yes"}});
}

auto OnActivate() -> GFResult {
  ++g_activate_calls;
  return GFResult::Ok();
}
auto OnDeactivate() -> GFResult {
  ++g_deactivate_calls;
  return GFResult::Ok();
}
void OnUnload() { ++g_unload_calls; }

constexpr GFEventBinding kEvents[] = {
    {"ALPHA", &OnAlpha},
    {"BETA", &OnBeta},
};

auto MakeHooks() -> GFModuleHooks {
  return GFModuleHooks{sizeof(GFModuleHooks),
                       "com.bktus.gpgfrontend.module.test",
                       "1.0.0",
                       "ModuleTest",
                       &OnActivate,
                       &OnDeactivate,
                       &OnUnload,
                       kEvents,
                       std::size(kEvents)};
}

// ------------------------------------------------------- host-side fakes

/// Build the payload the host hands over at activate.
class Payload {
 public:
  Payload(QStringList events, bool verified)
      : events_(std::move(events)), verified_(verified) {
    id_ = "com.bktus.gpgfrontend.module.test";
    version_ = "1.0.0";
    context_ = "ModuleTest";
    locale_ = "en_US";
    for (const auto& e : events_) event_utf8_.append(e.toUtf8());
    for (const auto& e : event_utf8_) event_ptrs_.append(e.constData());

    info_.struct_size = sizeof(GFModuleBootstrapInfo);
    info_.abi_version = GF_SDK_ABI_VERSION;
    info_.flags = verified_ ? GF_MODULE_BOOT_VERIFIED : 0U;
    info_.module_id = id_.constData();
    info_.module_version = version_.constData();
    info_.translation_context = context_.constData();
    info_.locale = locale_.constData();
    info_.events = event_ptrs_.constData();
    info_.events_size = static_cast<size_t>(event_ptrs_.size());
  }

  auto Get() -> GFModuleBootstrapInfo* { return &info_; }

 private:
  QStringList events_;
  bool verified_;
  QByteArray id_, version_, context_, locale_;
  QList<QByteArray> event_utf8_;
  QVector<const char*> event_ptrs_;
  GFModuleBootstrapInfo info_{};
};

/// A delivered event, allocated the way the host allocates one.
auto MakeEvent(const QString& id, const QMap<QString, QByteArray>& params)
    -> GFModuleEvent* {
  auto* e = static_cast<GFModuleEvent*>(GFMemAlloc(
      gf::runtime::SdkContext(), GF_ARENA_NORMAL, sizeof(GFModuleEvent)));
  e->id = GFMemStrDup(gf::runtime::SdkContext(), GF_ARENA_NORMAL,
                      id.toUtf8().constData());
  e->trigger_id =
      GFMemStrDup(gf::runtime::SdkContext(), GF_ARENA_NORMAL, "trigger-1");
  e->params = nullptr;

  GFModuleEventParam* prev = nullptr;
  for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
    auto* p = static_cast<GFModuleEventParam*>(
        GFMemAlloc(gf::runtime::SdkContext(), GF_ARENA_NORMAL,
                   sizeof(GFModuleEventParam)));
    p->name = GFMemStrDup(gf::runtime::SdkContext(), GF_ARENA_NORMAL,
                          it.key().toUtf8().constData());
    p->value = GFMemStrDup(gf::runtime::SdkContext(), GF_ARENA_NORMAL,
                           it.value().constData());
    p->next = nullptr;
    if (prev == nullptr) {
      e->params = p;
    } else {
      prev->next = p;
    }
    prev = p;
  }
  return e;
}

class ModuleRuntimeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Rec().Reset();
    g_last_result = GFEventResult::Ok();
    g_activate_calls = g_deactivate_calls = g_unload_calls = 0;
    hooks_ = MakeHooks();
    // The UI group is granted because the runtime itself reaches it, through
    // GFEvent::RequireGui. Everything else is withheld, which is what the
    // capability tests below are for.
    host_ = stubs::MakeHostApi(GF_HOST_CAP_UI);
  }

  /// Activate with a default payload, for the tests whose subject is not
  /// activation. A wrapper has no host to call before this, by design.
  void Activate() {
    Payload payload({"ALPHA", "BETA"}, true);
    ASSERT_EQ(Api()->activate(&host_, payload.Get()), 0);
    Rec().Reset();
  }

  auto Api() -> const GFModuleApi* {
    return GFModuleRuntimeGetApi(GF_SDK_ABI_VERSION, &hooks_);
  }

  GFModuleHooks hooks_{};
  GFHostApi host_{};
};

}  // namespace

namespace GpgFrontend::Test {

// ------------------------------------------------------------ entry point

TEST_F(ModuleRuntimeTest, TheStreamFormBuildsOneLineRatherThanSeveral) {
  Activate();
  LOG_W() << "open" << QString("/tmp/x") << "failed:" << 42;

  ASSERT_EQ(Rec().warnings.size(), 1);
  EXPECT_EQ(Rec().warnings.first(), "open \"/tmp/x\" failed: 42");
}

TEST_F(ModuleRuntimeTest, TheStreamFormTakesWhatArgCannotRatherThanRefusing) {
  Activate();
  // The reason this exists beside FLOG_*: QString::arg has no overload for a
  // bool, a QByteArray or a container, so a caller had to convert by hand at
  // every site. QDebug prints all of them.
  LOG_E() << true << QByteArray("bytes") << QStringList{"a", "b"};

  ASSERT_EQ(Rec().errors.size(), 1);
  EXPECT_TRUE(Rec().errors.first().contains("true"));
  EXPECT_TRUE(Rec().errors.first().contains("bytes"));
  EXPECT_TRUE(Rec().errors.first().contains("a"));
}

TEST_F(ModuleRuntimeTest, TheStreamFormEmitsNothingExtraRatherThanAnEmptyLine) {
  Activate();
  // A statement with nothing streamed is still one message, not zero and not
  // a crash -- somebody will write it.
  LOG_W();

  EXPECT_EQ(Rec().warnings.size(), 1);
}

TEST_F(ModuleRuntimeTest, TheApiTableDescribesThisModule) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);

  EXPECT_EQ(api->struct_size, sizeof(GFModuleApi));
  EXPECT_EQ(api->abi_version, GF_SDK_ABI_VERSION);
  EXPECT_STREQ(api->module_id, "com.bktus.gpgfrontend.module.test");
  EXPECT_STREQ(api->version, "1.0.0");
  EXPECT_NE(api->activate, nullptr);
  EXPECT_NE(api->execute, nullptr);
  EXPECT_NE(api->deactivate, nullptr);
  EXPECT_NE(api->unregister, nullptr);
}

// Negotiation, not a crash on the first mismatched call.
TEST_F(ModuleRuntimeTest, AHostOutsideTheSupportedRangeIsDeclined) {
  EXPECT_EQ(GFModuleRuntimeGetApi(GF_SDK_ABI_VERSION + 1, &hooks_), nullptr);
  EXPECT_EQ(GFModuleRuntimeGetApi(GF_SDK_ABI_MIN_SUPPORTED - 1, &hooks_),
            nullptr);
}

// The hook table grows by appending, so a short one must be detected rather
// than read past -- the same rule the ABI tables follow.
TEST_F(ModuleRuntimeTest, AHookTableTooShortToDescribeItselfIsRefused) {
  auto stunted = MakeHooks();
  stunted.struct_size = offsetof(GFModuleHooks, events);
  EXPECT_EQ(GFModuleRuntimeGetApi(GF_SDK_ABI_VERSION, &stunted), nullptr);

  EXPECT_EQ(GFModuleRuntimeGetApi(GF_SDK_ABI_VERSION, nullptr), nullptr);
}

TEST_F(ModuleRuntimeTest, AModuleWithoutIdentityCannotBeMatchedToAManifest) {
  auto anonymous = MakeHooks();
  anonymous.module_id = nullptr;
  EXPECT_EQ(GFModuleRuntimeGetApi(GF_SDK_ABI_VERSION, &anonymous), nullptr);
}

// ------------------------------------------------------------- bootstrap

// The payload is borrowed and dies with the activate() call, so anything the
// runtime keeps has to be a copy.
TEST_F(ModuleRuntimeTest, TheBootstrapPayloadIsCopiedNotBorrowed) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);

  {
    Payload payload({"ALPHA", "BETA"}, true);
    ASSERT_EQ(api->activate(&host_, payload.Get()), 0);
  }  // payload destroyed here, exactly as the host's frame would be

  EXPECT_EQ(GFModuleId(), "com.bktus.gpgfrontend.module.test");
  EXPECT_EQ(GFModuleVersion(), "1.0.0");
  EXPECT_TRUE(GFModuleIsVerified());
}

TEST_F(ModuleRuntimeTest, AHostTableTooSmallIsRefused) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);

  auto stunted = stubs::MakeHostApi(0);
  stunted.struct_size = sizeof(size_t);

  Payload payload({"ALPHA", "BETA"}, true);
  EXPECT_NE(api->activate(&stunted, payload.Get()), 0);
  EXPECT_EQ(api->activate(nullptr, payload.Get()), -1);

  // A table big enough but with no context authorizes nothing, so every call
  // through it would be refused. Declining to activate says so once instead
  // of letting the module fail at each call.
  auto contextless = stubs::MakeHostApi(0);
  contextless.context = nullptr;
  EXPECT_NE(api->activate(&contextless, payload.Get()), 0);
}

// An older host passes nothing. The module still has to load, from the only
// facts it has -- its own.
TEST_F(ModuleRuntimeTest, NoPayloadFallsBackToTheModulesOwnTable) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  ASSERT_EQ(api->activate(&host_, nullptr), 0);

  EXPECT_FALSE(GFModuleIsVerified());
  EXPECT_EQ(GFModuleId(), "com.bktus.gpgfrontend.module.test");

  EXPECT_EQ(Rec().listened.size(), 2);
  EXPECT_TRUE(Rec().listened.contains("ALPHA"));
  EXPECT_TRUE(Rec().listened.contains("BETA"));
  EXPECT_FALSE(Rec().warnings.isEmpty())
      << "subscribing without a declaration is a weaker guarantee and must "
         "say so";
}

// ---------------------------------------------------------- subscriptions

TEST_F(ModuleRuntimeTest, ADeclaredEventWithNoHandlerRefusesActivation) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);

  Payload payload({"ALPHA", "BETA", "GAMMA"}, true);
  EXPECT_NE(api->activate(&host_, payload.Get()), 0);
  EXPECT_EQ(g_activate_calls, 0) << "the module must not start";

  const auto joined = Rec().errors.join(" ");
  EXPECT_TRUE(joined.contains("GAMMA")) << joined.toStdString();
}

TEST_F(ModuleRuntimeTest, AHandlerWithNoDeclarationRefusesActivation) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);

  Payload payload({"ALPHA"}, true);
  EXPECT_NE(api->activate(&host_, payload.Get()), 0);
  EXPECT_EQ(g_activate_calls, 0);

  const auto joined = Rec().errors.join(" ");
  EXPECT_TRUE(joined.contains("BETA")) << joined.toStdString();
}

TEST_F(ModuleRuntimeTest, AnExactMatchSubscribesOncePerEvent) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);

  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  EXPECT_EQ(g_activate_calls, 1);
  EXPECT_EQ(Rec().listened, (QStringList{"ALPHA", "BETA"}));
  EXPECT_EQ(Rec().listened_as, "com.bktus.gpgfrontend.module.test");
  EXPECT_EQ(Rec().translator_registered_for,
            "com.bktus.gpgfrontend.module.test");
}

// ------------------------------------------------------------- dispatch

TEST_F(ModuleRuntimeTest, AnOkResultAnswersWithZero) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  ASSERT_EQ(api->execute(MakeEvent("BETA", {})), 0);

  ASSERT_EQ(Rec().answers.size(), 1);
  EXPECT_EQ(Rec().answers.first().value("ret"), "0");
  EXPECT_EQ(Rec().answers.first().value("extra"), "yes");
  EXPECT_FALSE(Rec().answers.first().contains("err"));
}

// Every failure looks the same on the wire, because that is all there has
// ever been: 64 call sites all passed -1.
TEST_F(ModuleRuntimeTest, EveryFailureAnswersWithMinusOneAndAReason) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  for (const auto& result :
       {GFEventResult::Bad("bad"), GFEventResult::Fail("failed"),
        GFEventResult::Unavailable("gone")}) {
    Rec().answers.clear();
    g_last_result = result;

    EXPECT_EQ(api->execute(MakeEvent("ALPHA", {})), -1);
    ASSERT_EQ(Rec().answers.size(), 1);
    EXPECT_EQ(Rec().answers.first().value("ret"), "-1");
    EXPECT_EQ(Rec().answers.first().value("err"), result.reason);
  }
}

TEST_F(ModuleRuntimeTest, AnUnknownEventIdIsAnsweredNotDropped) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  EXPECT_EQ(api->execute(MakeEvent("NOT_SUBSCRIBED", {})), -1);
  ASSERT_EQ(Rec().answers.size(), 1);
  EXPECT_EQ(Rec().answers.first().value("ret"), "-1");
  EXPECT_TRUE(Rec().answers.first().value("err").contains("NOT_SUBSCRIBED"));
}

// The property the old QMap<QString,QString> transport destroyed: binary had
// to be base64-encoded by hand at both ends, and anything that forgot was
// silently mangled.
TEST_F(ModuleRuntimeTest, ParameterOctetsSurviveVerbatim) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  const QByteArray binary("\x01\x02\xff\xfe\x7f", 5);
  ASSERT_EQ(api->execute(MakeEvent("ALPHA", {{"blob", binary}})), 0);

  EXPECT_EQ(g_captured.Bytes("blob"), binary);
  EXPECT_EQ(g_captured.Id(), "ALPHA");
  EXPECT_EQ(g_captured.TriggerId(), "trigger-1");
}

TEST_F(ModuleRuntimeTest, RequireReportsWhatIsMissing) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);
  ASSERT_EQ(api->execute(MakeEvent("ALPHA", {{"present", "x"}})), 0);

  QString out;
  EXPECT_TRUE(g_captured.Require("present", out).ok);
  EXPECT_EQ(out, "x");

  const auto missing = g_captured.Require("absent", out);
  EXPECT_FALSE(missing.ok);
  EXPECT_EQ(missing.status, GFEventStatus::kBAD_REQUEST);
  EXPECT_TRUE(missing.reason.contains("absent"));
}

// ------------------------------------------------------ deferred answers

TEST_F(ModuleRuntimeTest, ADeferredHandlerIsNotAnsweredByTheRuntime) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  g_last_result = GFEventResult::Deferred();
  EXPECT_EQ(api->execute(MakeEvent("ALPHA", {})), 0);
  EXPECT_TRUE(Rec().answers.isEmpty())
      << "answering here would answer the same trigger twice";
}

TEST_F(ModuleRuntimeTest, ADeferredAnswerCanBeSentFromAnotherThread) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  g_last_result = GFEventResult::Deferred();
  ASSERT_EQ(api->execute(MakeEvent("ALPHA", {})), 0);

  const auto answer = g_captured.Answer();
  auto* thread = QThread::create([answer] { answer.Ok({{"late", "yes"}}); });
  thread->start();
  ASSERT_TRUE(thread->wait(5000));
  delete thread;

  ASSERT_EQ(Rec().answers.size(), 1);
  EXPECT_EQ(Rec().answers.first().value("ret"), "0");
  EXPECT_EQ(Rec().answers.first().value("late"), "yes");
  EXPECT_TRUE(answer.Answered());
}

// The host frees the parameters it is handed, so a second answer would be a
// second free of a trigger that is already gone.
TEST_F(ModuleRuntimeTest, AnEventIsAnsweredAtMostOnce) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  g_last_result = GFEventResult::Deferred();
  ASSERT_EQ(api->execute(MakeEvent("ALPHA", {})), 0);

  const auto answer = g_captured.Answer();
  answer.Ok();
  answer.Ok();
  answer.Fail("and again");

  EXPECT_EQ(Rec().answers.size(), 1);
  EXPECT_FALSE(Rec().errors.isEmpty()) << "the extra answers must be reported";
}

// ------------------------------------------------------------- ownership

// The host allocates the event and every parameter and hands them over; the
// module side frees them. Every path, including the ones that fail.
TEST_F(ModuleRuntimeTest, EveryDeliveredEventIsFreedExactlyOnce) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  for (const auto& id : {QString("ALPHA"), QString("NOT_SUBSCRIBED")}) {
    for (const auto deferred : {false, true}) {
      g_last_result =
          deferred ? GFEventResult::Deferred() : GFEventResult::Ok();

      Rec().Reset();
      auto* event = MakeEvent(id, {{"a", "1"}, {"b", "2"}});
      api->execute(event);

      // Across the whole exchange -- the delivered event the module side must
      // free, and the answer the host side must free -- every allocation is
      // matched. Counting only the delivery would miss a leaked answer, and
      // counting only the answer would miss a leaked event.
      EXPECT_EQ(Rec().allocations, Rec().frees)
          << "id=" << id.toStdString() << " deferred=" << deferred;
      EXPECT_GT(Rec().allocations, 0);
    }
  }
}

// ------------------------------------------------------------- lifecycle

TEST_F(ModuleRuntimeTest, LifecycleHooksRunAndMissingOnesAreNoOps) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  EXPECT_EQ(api->deactivate(), 0);
  EXPECT_EQ(g_deactivate_calls, 1);

  api->unregister();
  EXPECT_EQ(g_unload_calls, 1);

  // A module that implements nothing must still load and tear down.
  auto bare = MakeHooks();
  bare.on_activate = nullptr;
  bare.on_deactivate = nullptr;
  bare.on_unload = nullptr;

  const auto* bare_api = GFModuleRuntimeGetApi(GF_SDK_ABI_VERSION, &bare);
  ASSERT_NE(bare_api, nullptr);
  Payload p2({"ALPHA", "BETA"}, true);
  EXPECT_EQ(bare_api->activate(&host_, p2.Get()), 0);
  EXPECT_EQ(bare_api->deactivate(), 0);
  bare_api->unregister();
}

// ----------------------------------------------------------- translations

TEST_F(ModuleRuntimeTest, TheTranslatorReaderIsRegisteredAndSurvivesNoQmFile) {
  const auto* api = Api();
  ASSERT_NE(api, nullptr);
  Payload payload({"ALPHA", "BETA"}, true);
  ASSERT_EQ(api->activate(&host_, payload.Get()), 0);

  ASSERT_NE(Rec().translator_reader, nullptr);

  // No .qm is compiled into this test binary, so the reader must report
  // "nothing" rather than hand the host a buffer it would then free.
  char* data = reinterpret_cast<char*>(1);
  const auto size = Rec().translator_reader("en_US", &data);
  EXPECT_EQ(size, 0);
  EXPECT_EQ(data, nullptr);
}

// ------------------------------------------------- the module-side sdk
//
// These exercise the wrappers a module actually calls -- the GFSDK* names --
// running against the capability table the fake host handed over. Before the
// SDK moved module-side there was nothing here to test: the names resolved
// to the host's shared library and a test binary could only stub them out.

TEST_F(ModuleRuntimeTest, AWrapperWhoseGroupWasWithheldRefusesTheCall) {
  Activate();  // the fake grants ui and nothing else

  // gpg was never granted, so host->gpg is null. The wrapper must return the
  // failure value for its own signature rather than call through a null
  // pointer -- and it must say why, once.
  GFGpgResultRef result = nullptr;
  EXPECT_NE(GFGpgDecrypt(gf::runtime::SdkContext(), 0, nullptr, &result), 0);
  EXPECT_EQ(result, nullptr);

  ASSERT_FALSE(Rec().errors.isEmpty());
  const auto joined = Rec().errors.join(" ");
  EXPECT_TRUE(joined.contains("gpg"))
      << "the denial must name the capability that was missing: "
      << joined.toStdString();
  EXPECT_TRUE(joined.contains("GFGpgDecrypt"))
      << "and the entry point that wanted it: " << joined.toStdString();
}

TEST_F(ModuleRuntimeTest, ADenialIsReportedEveryTimeBecauseTheSdkIsStateless) {
  Activate();

  // Deliberately NOT "once". Remembering that a denial has already been
  // reported would be state, and gf_sdk holds none: no bound table, no
  // current module, not even a flag. That is the whole property, and this is
  // the visible cost of it.
  //
  // A module is expected to ask once, at activation, whether it holds what it
  // needs, rather than discover it in a loop.
  GFGpgResultRef result = nullptr;
  for (int i = 0; i < 5; ++i) {
    GFGpgSign(gf::runtime::SdkContext(), 0, nullptr, 0, nullptr, 0, 0, &result);
  }

  EXPECT_EQ(Rec().errors.size(), 5);
  EXPECT_TRUE(Rec().errors.first().contains("gpg"));
}

TEST_F(ModuleRuntimeTest, TheColourHelpersDifferOnlyByTheRoleTheyAskFor) {
  Activate();

  // Five names on this side, one primitive on the other. The fake returns
  // the role it was given, so this asserts that each name asks for its own
  // role -- the property that replaced five host entry points.
  EXPECT_EQ(GFUIThemeColor(gf::runtime::SdkContext(), GF_UI_COLOR_MUTED_TEXT,
                           nullptr),
            0xFF000000U | GF_UI_COLOR_MUTED_TEXT);
  EXPECT_EQ(
      GFUIThemeColor(gf::runtime::SdkContext(), GF_UI_COLOR_BORDER, nullptr),
      0xFF000000U | GF_UI_COLOR_BORDER);
  EXPECT_EQ(
      GFUIThemeColor(gf::runtime::SdkContext(), GF_UI_COLOR_WARNING, nullptr),
      0xFF000000U | GF_UI_COLOR_WARNING);
  EXPECT_EQ(
      GFUIThemeColor(gf::runtime::SdkContext(), GF_UI_COLOR_DANGER, nullptr),
      0xFF000000U | GF_UI_COLOR_DANGER);
  EXPECT_EQ(GFUIThemeColor(gf::runtime::SdkContext(),
                           GF_UI_COLOR_ACCENT_POSITIVE, nullptr),
            0xFF000000U | GF_UI_COLOR_ACCENT_POSITIVE);
  EXPECT_EQ(GFUIThemeColor(gf::runtime::SdkContext(),
                           GF_UI_COLOR_ACCENT_NEGATIVE, nullptr),
            0xFF000000U | GF_UI_COLOR_ACCENT_NEGATIVE);
}

TEST_F(ModuleRuntimeTest, AHelperThatNeedsNothingFromTheHostCallsNothing) {
  Activate();
  const auto before = Rec().allocations;

  // Version comparison crosses no boundary at all -- two strings in, an
  // ordering out -- so it is computed here and costs no ABI. The quirks are
  // the host's own, faithfully: a leading "v" is dropped, and a version with
  // more components sorts after one with fewer.
  EXPECT_LT(GFCompareSoftwareVersion("2.1.0", "2.1.1"), 0);
  EXPECT_GT(GFCompareSoftwareVersion("2.10.0", "2.9.0"), 0);
  EXPECT_EQ(GFCompareSoftwareVersion("v2.1.0", "2.1.0"), 0);
  EXPECT_GT(GFCompareSoftwareVersion("2.1.0", "2.1"), 0);

  EXPECT_EQ(Rec().allocations, before)
      << "GFCompareSoftwareVersion must not reach the host for anything";
}

TEST_F(ModuleRuntimeTest, AWrapperWorksFromAThreadTheModuleStarted) {
  Activate();

  // The case the context exists for. Nothing the host did is on this
  // thread's stack, so an implementation that read the host's thread-local
  // record of "whose code is running" would find nothing and refuse.
  QString copied;
  QThread* worker = QThread::create([&copied]() {
    auto* s = GFMemStrDup(gf::runtime::SdkContext(), GF_ARENA_NORMAL,
                          "from a worker");
    if (s != nullptr) {
      copied = QString::fromUtf8(s);
      GFMemFree(gf::runtime::SdkContext(), GF_ARENA_NORMAL, s);
    }
  });
  worker->start();
  ASSERT_TRUE(worker->wait(30000));
  worker->deleteLater();

  EXPECT_EQ(copied.toStdString(), "from a worker");
  EXPECT_GT(Rec().calls_off_thread, 0)
      << "the call did not actually happen on another thread";
}

}  // namespace GpgFrontend::Test
