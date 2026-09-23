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

#include "GpgFrontendTest.h"
#include "sdk/GFSDKHostApi.h"
#include "ui/command/CommandRegistry.h"
#include "ui/lua/LuaAnchors.h"
#include "ui/lua/LuaApi.h"
#include "ui/lua/LuaHost.h"
#include "ui/lua/LuaModuleRuntime.h"
#include "ui/lua/NativeWidgetRegistry.h"

/**
 * @file GFUiLuaApiTest.cpp
 * @brief What a module's UI script can and cannot do, through the real
 *        registry. No widget is built: native widget factories are never
 *        called here, and every runtime lives on the test's own thread.
 */

namespace GpgFrontend::Test {

namespace {

using UI::CommandProvider;
using UI::CommandRegistry;
using UI::Lua::LuaModuleRuntime;
using UI::Lua::UiContext;
using UI::Lua::UiDocument;

constexpr auto kProvider = "com.example.luat";
constexpr auto kModule = "com.example.luamod";

// --- the commands a script is tested against ------------------------------

struct Echo {
  static constexpr gf::cmd::Meta kMeta{"com.example.luat.echo", "Echo", "", "",
                                       0, 0};
  struct Args {
    gf::cmd::DocumentRef target;
    static constexpr auto Fields() {
      return std::make_tuple(gf::cmd::F("target", &Args::target));
    }
  };
  struct Result {
    qint64 id = 0;
    static constexpr auto Fields() {
      return std::make_tuple(gf::cmd::F("id", &Result::id));
    }
  };
};

std::atomic<int> g_echo_calls{0};
std::atomic<qint64> g_echo_last{0};

auto DoEcho(const gf::cmd::CommandContext&, const Echo::Args& a)
    -> gf::cmd::Outcome<Echo::Result> {
  ++g_echo_calls;
  g_echo_last = a.target.id;
  return gf::cmd::Outcome<Echo::Result>::Success({a.target.id});
}

struct Gpg {
  static constexpr gf::cmd::Meta kMeta{"com.example.luat.gpg", "Gpg", "", "",
                                       GF_HOST_CAP_GPG, 0};
  using Args = gf::cmd::Unit;
  using Result = gf::cmd::Unit;
};

auto DoNothing(const gf::cmd::CommandContext&, const gf::cmd::Unit&)
    -> gf::cmd::Outcome<gf::cmd::Unit> {
  return gf::cmd::Outcome<gf::cmd::Unit>::Success({});
}

struct Secret {
  static constexpr gf::cmd::Meta kMeta{"com.example.luat.secret", "Secret", "",
                                       "", 0, 0};
  using Args = gf::cmd::Unit;
  struct Result {
    gf::cmd::Blob data;
    static constexpr auto Fields() {
      return std::make_tuple(gf::cmd::F("data", &Result::data));
    }
  };
};

auto DoSecret(const gf::cmd::CommandContext&, const gf::cmd::Unit&)
    -> gf::cmd::Outcome<Secret::Result> {
  return gf::cmd::Outcome<Secret::Result>::Success(
      {UI::MakeHostBlob(GFBuffer(QByteArray("s3cret")))});
}

struct Sink {
  static constexpr gf::cmd::Meta kMeta{"com.example.luat.sink", "Sink", "", "",
                                       0, 0};
  struct Args {
    gf::cmd::Blob data;
    static constexpr auto Fields() {
      return std::make_tuple(gf::cmd::F("data", &Args::data));
    }
  };
  using Result = gf::cmd::Unit;
};

std::atomic<int> g_sunk{0};
auto DoSink(const gf::cmd::CommandContext&, const Sink::Args& a)
    -> gf::cmd::Outcome<gf::cmd::Unit> {
  if (QByteArray(a.data.Data(), static_cast<int>(a.data.Size())) == "s3cret") {
    ++g_sunk;
  }
  return gf::cmd::Outcome<gf::cmd::Unit>::Success({});
}

/// Never answers: for continuation limits and teardown.
struct Park {
  static constexpr gf::cmd::Meta kMeta{"com.example.luat.park", "Park", "", "",
                                       0, 0};
  using Args = gf::cmd::Unit;
  using Result = gf::cmd::Unit;
};

std::vector<gf::cmd::Reply<gf::cmd::Unit>> g_parked;
void DoPark(const gf::cmd::CommandContext&, gf::cmd::Unit,
            gf::cmd::Reply<gf::cmd::Unit> reply) {
  g_parked.push_back(reply);
}

/// Answers with a result its schema does not describe.
struct Liar {
  static constexpr gf::cmd::Meta kMeta{"com.example.luat.liar", "Liar", "", "",
                                       0, 0};
  using Args = gf::cmd::Unit;
  using Result = Echo::Result;
};

template <typename C, auto H>
void RegisterTestCommand() {
  const auto b = gf::cmd::Bind<C, H>();
  CommandProvider p;
  p.id = QString::fromUtf8(b.id);
  p.owner = kProvider;
  p.descriptor = b.describe();
  p.required_caps = C::kMeta.required_caps;
  p.flags = C::kMeta.flags;
  p.run = b.run;
  ASSERT_EQ(CommandRegistry::Instance().Register(std::move(p)), GF_CMD_OK);
}

class LuaApiTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    RegisterTestCommand<Echo, &DoEcho>();
    RegisterTestCommand<Gpg, &DoNothing>();
    RegisterTestCommand<Secret, &DoSecret>();
    RegisterTestCommand<Sink, &DoSink>();
    RegisterTestCommand<Park, &DoPark>();

    CommandProvider liar;
    liar.id = "com.example.luat.liar";
    liar.owner = kProvider;
    liar.descriptor = gf::cmd::Describe<Liar>();
    liar.run = [](const gf::cmd::CommandContext&, QCborMap,
                  std::vector<gf::cmd::Blob>, gf::cmd::Completer done) {
      done({GF_CMD_OK, 0, QCborMap{{QStringLiteral("nonsense"), 1}}, {}, {}});
    };
    ASSERT_EQ(CommandRegistry::Instance().Register(std::move(liar)), GF_CMD_OK);

    UI::NativeWidgetEntry dialog;
    dialog.owner = kModule;
    dialog.id = QString(kModule) + ".inspector";
    dialog.kind = UI::NativeWidgetKind::kDIALOG;
    dialog.create = [](quint64, const QCborMap&) -> QWidget* { return nullptr; };
    UI::NativeWidgetRegistry::Instance().Register(dialog);

    UI::NativeWidgetEntry editor = dialog;
    editor.id = QString(kModule) + ".editor";
    editor.kind = UI::NativeWidgetKind::kDOCUMENT;
    editor.multi_instance = true;
    UI::NativeWidgetRegistry::Instance().Register(editor);

    UI::NativeWidgetEntry foreign = dialog;
    foreign.owner = "com.example.other";
    foreign.id = "com.example.other.inspector";
    UI::NativeWidgetRegistry::Instance().Register(foreign);
  }

  static void TearDownTestSuite() {
    CommandRegistry::Instance().RemoveAllFor(kProvider);
    UI::NativeWidgetRegistry::Instance().RemoveAllFor(kModule);
    UI::NativeWidgetRegistry::Instance().RemoveAllFor("com.example.other");
    g_parked.clear();
  }

  static auto Make(uint32_t caps = GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM |
                                   GF_HOST_CAP_STORAGE)
      -> std::unique_ptr<LuaModuleRuntime> {
    return std::make_unique<LuaModuleRuntime>(kModule, caps);
  }

  static auto Load(LuaModuleRuntime& rt, const char* code,
                   QString* error = nullptr) -> bool {
    static int n = 0;
    QString local;
    return rt.Load(QByteArray(code), QString("chunk%1").arg(++n),
                   error != nullptr ? error : &local);
  }

  static auto Doc(qint64 id) -> UiContext {
    UiContext ctx;
    ctx.document = UiDocument{id, "text", false, [] { return true; }};
    return ctx;
  }

  /// Run the test thread's event loop until @p done or a timeout.
  template <typename F>
  static auto Pump(F done) -> bool {
    QElapsedTimer t;
    t.start();
    while (!done() && t.elapsed() < 5000) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    return done();
  }
};

constexpr auto kEchoAction = R"lua(
ui.action {
  id = "echo",
  anchor = ui.anchor("main.menu.help"),
  command = commands.get("com.example.luat.echo"),
  update = function(ctx)
    if not ctx.document then return { visible = false } end
    return { visible = true, enabled = true, args = { target = ctx.document } }
  end
}
)lua";

}  // namespace

// ------------------------------------------------------------ loading

TEST_F(LuaApiTest, AFailedLoadRollsBackEverythingItRegistered) {
  auto rt = Make();
  QString error;
  EXPECT_FALSE(Load(*rt,
                    R"lua(
ui.action { id = "a", anchor = ui.anchor("main.menu.help"),
            command = commands.get("com.example.luat.echo") }
local nope = commands.get("com.example.luat.missing")
)lua",
                    &error));
  EXPECT_TRUE(error.contains("unknown command")) << error.toStdString();
  EXPECT_TRUE(rt->Actions().isEmpty()) << "the action registered before the "
                                          "failure went with it";
  EXPECT_TRUE(Load(*rt, kEchoAction));
  EXPECT_EQ(rt->Actions().size(), 1);
}

TEST_F(LuaApiTest, EveryReferenceIsCheckedAtLoad) {
  auto rt = Make();
  QString error;
  EXPECT_FALSE(Load(*rt, "ui.anchor('main.menu.nowhere')", &error));
  EXPECT_TRUE(error.contains("main.menu.help")) << "lists the valid anchors";
  EXPECT_FALSE(Load(*rt, "ui.anchor('editor')", &error))
      << "a mount anchor is not an action anchor";
  EXPECT_FALSE(Load(*rt, "ui.subscribe{event='MAINWINDOW_MENU_MOUNTED', "
                         "handler=function() end}",
                    &error))
      << "no legacy event reaches Lua";
  EXPECT_FALSE(Load(*rt, "ui.subscribe{event='anything.at.all', "
                         "handler=function() end}"));
  EXPECT_FALSE(Load(*rt, "native.widget('missing')", &error));
  EXPECT_FALSE(Load(*rt, "native.widget('../com.example.other.inspector')"));
  EXPECT_FALSE(Load(*rt, "native.factory('inspector')"))
      << "a single widget is not a factory";
}

TEST_F(LuaApiTest, AnActionDeclaresOnlyWhatItMay) {
  auto rt = Make();
  const auto action = [&](const char* fields) {
    const auto code =
        QString("ui.action{ anchor = ui.anchor('main.menu.help'), command = "
                "commands.get('com.example.luat.echo'), %1 }")
            .arg(fields)
            .toUtf8();
    return Load(*rt, code.constData());
  };
  EXPECT_FALSE(action("id = 'x', title = 'Hello'"))
      << "no text in Lua: titles come from the command";
  EXPECT_FALSE(action("id = 'x', update = 42"));
  EXPECT_FALSE(action("id = 'x', icon = '/etc/passwd'"));
  EXPECT_FALSE(action("id = 'Bad Id'"));
  EXPECT_TRUE(action("id = 'x', order = 3, icon = ':/icons/help.png'"));
  EXPECT_FALSE(action("id = 'x'")) << "an id is registered once";
}

TEST_F(LuaApiTest, CapabilitiesAreTheModulesAndNoMore) {
  auto ui_only = Make(GF_HOST_CAP_UI);
  QString error;
  EXPECT_FALSE(Load(*ui_only, "commands.get('com.example.luat.gpg')", &error));
  EXPECT_TRUE(error.contains("capability")) << error.toStdString();
  EXPECT_TRUE(Load(*ui_only, "assert(native == nil)"))
      << "no ui.custom, no native table";
  EXPECT_FALSE(Load(*ui_only, "ui.mount{ id='d', anchor=ui.anchor.dialog{}, "
                              "widget=nil }",
                    &error));
  EXPECT_TRUE(error.contains("ui.custom")) << error.toStdString();
  EXPECT_FALSE(Load(*ui_only, "state.get('x')")) << "no storage, no state";

  auto custom = Make();
  EXPECT_TRUE(Load(*custom,
                   "ui.mount{ id='d', anchor=ui.anchor.dialog{}, "
                   "widget=native.widget('inspector') }"));
  EXPECT_FALSE(Load(*custom, "ui.mount{ id='e', anchor=ui.anchor.editor{"
                             "document_type='x'}, "
                             "widget=native.widget('inspector') }"))
      << "an editor needs a document widget, and a factory";
  EXPECT_TRUE(Load(*custom, "ui.mount{ id='e', anchor=ui.anchor.editor{"
                            "document_type='x', extensions={'eml'}}, "
                            "widget=native.factory('editor') }"));
  EXPECT_EQ(custom->Mounts().size(), 2);
}

// ------------------------------------------------------------ evaluation

TEST_F(LuaApiTest, UpdateComputesStateAndTypedArguments) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, kEchoAction));
  const auto id = QString(kModule) + ".echo";

  auto st = rt->Evaluate(id, {});
  EXPECT_TRUE(st.error.isEmpty()) << st.error.toStdString();
  EXPECT_FALSE(st.visible);

  st = rt->Evaluate(id, Doc(42));
  ASSERT_TRUE(st.error.isEmpty()) << st.error.toStdString();
  EXPECT_TRUE(st.visible);
  EXPECT_TRUE(st.enabled);
  EXPECT_EQ(st.args.value("target").toMap().value("id").toInteger(), 42);
}

TEST_F(LuaApiTest, UpdateIsPure) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, R"lua(
local echo = commands.get("com.example.luat.echo")
ui.action { id = "a", anchor = ui.anchor("main.menu.help"), command = echo,
  update = function(ctx) commands.invoke(echo, {}) return {} end }
ui.action { id = "b", anchor = ui.anchor("main.menu.help"), command = echo,
  update = function(ctx) state.set("k", 1) return {} end }
ui.action { id = "c", anchor = ui.anchor("main.menu.help"), command = echo,
  update = function(ctx) ui.action{} return {} end }
)lua"));
  for (const char* id : {"a", "b", "c"}) {
    const auto st = rt->Evaluate(QString(kModule) + "." + id, Doc(1));
    EXPECT_TRUE(st.error.contains("not allowed in update")) << id << ": "
                                                           << st.error.toStdString();
    EXPECT_FALSE(st.visible);
  }
}

TEST_F(LuaApiTest, AnUpdateResultIsValidated) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, R"lua(
local echo = commands.get("com.example.luat.echo")
local function add(id, f)
  ui.action { id = id, anchor = ui.anchor("main.menu.help"), command = echo,
              update = f }
end
add("unknown", function() return { shiny = true } end)
add("typed", function() return { visible = "yes" } end)
add("checked", function() return { checked = true } end)
add("args", function(ctx) return { args = { target = 7 } } end)
add("extra", function(ctx)
  return { args = { target = ctx.document, more = 1 } } end)
add("notable", function() return 1 end)
)lua"));
  for (const char* id :
       {"unknown", "typed", "checked", "args", "extra", "notable"}) {
    const auto st = rt->Evaluate(QString(kModule) + "." + id, Doc(1));
    EXPECT_FALSE(st.error.isEmpty()) << id;
    EXPECT_FALSE(st.visible) << id;
  }
}

// A context handle is good for the one call it was passed to. Keeping one
// past that finds nothing; keeping a :ref() is how to hold on.
TEST_F(LuaApiTest, AContextHandleDiesWithItsCall) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, R"lua(
local echo = commands.get("com.example.luat.echo")
ui.action { id = "keep", anchor = ui.anchor("main.menu.help"), command = echo,
  update = function(ctx)
    if kept == nil then kept = ctx.document return { visible = false } end
    return { args = { target = kept } }
  end }
ui.action { id = "ref", anchor = ui.anchor("main.menu.help"), command = echo,
  update = function(ctx)
    if kept_ref == nil then kept_ref = ctx.document:ref() return {} end
    return { args = { target = kept_ref } }
  end }
ui.action { id = "fields", anchor = ui.anchor("main.menu.help"), command = echo,
  update = function(ctx)
    if old == nil then old = ctx return { visible = false } end
    assert(old.document == nil, "a stale context reads as empty")
    return { args = { target = ctx.document } }
  end }
ui.action { id = "wrongkind", anchor = ui.anchor("main.menu.help"),
  command = echo,
  update = function(ctx) return { args = { target = ctx.key } } end }
)lua"));
  const auto key = [] {
    auto c = Doc(5);
    c.key = gf::cmd::KeyRef{0, "ID", "FPR", false};
    return c;
  };

  rt->Evaluate(QString(kModule) + ".keep", Doc(1));
  auto st = rt->Evaluate(QString(kModule) + ".keep", Doc(2));
  EXPECT_TRUE(st.error.contains("outlived")) << st.error.toStdString();

  rt->Evaluate(QString(kModule) + ".ref", Doc(3));
  st = rt->Evaluate(QString(kModule) + ".ref", Doc(4));
  ASSERT_TRUE(st.error.isEmpty()) << st.error.toStdString();
  EXPECT_EQ(st.args.value("target").toMap().value("id").toInteger(), 3);

  rt->Evaluate(QString(kModule) + ".fields", Doc(1));
  st = rt->Evaluate(QString(kModule) + ".fields", Doc(2));
  EXPECT_TRUE(st.error.isEmpty()) << st.error.toStdString();

  st = rt->Evaluate(QString(kModule) + ".wrongkind", key());
  EXPECT_FALSE(st.error.isEmpty()) << "a Key is not a DocumentRef";
}

TEST_F(LuaApiTest, TriggerReevaluatesAgainstTheContextOfNow) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, kEchoAction));
  const auto id = QString(kModule) + ".echo";

  const auto before = g_echo_calls.load();
  EXPECT_EQ(rt->Trigger(id, {}), GF_CMD_E_DISABLED)
      << "no document now, whatever the menu showed";
  EXPECT_EQ(g_echo_calls.load(), before);

  EXPECT_EQ(rt->Trigger(id, Doc(99)), GF_CMD_OK);
  EXPECT_EQ(g_echo_calls.load(), before + 1);
  EXPECT_EQ(g_echo_last.load(), 99);
}

// ------------------------------------------------------------ invoking

TEST_F(LuaApiTest, AContinuationGetsASchemaCheckedResult) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, R"lua(
local echo = commands.get("com.example.luat.echo")
local liar = commands.get("com.example.luat.liar")
ui.subscribe { event = "document.saved", handler = function(ctx, e)
  commands.invoke(echo, { target = e.document }, function(r, err)
    got_id, got_err = r and r.id, err
  end)
  commands.invoke(liar, {}, function(r, err) liar_r, liar_err = r, err end)
end }
ui.action { id = "probe", anchor = ui.anchor("main.menu.help"), command = echo,
  update = function()
    assert(got_id == 12, tostring(got_id))
    assert(liar_r == nil and liar_err == "bad_result", tostring(liar_err))
    return { visible = false }
  end }
)lua"));
  rt->Deliver("document.saved", Doc(12), 12);
  ASSERT_TRUE(Pump([&] { return rt->Snapshot().pending_calls == 0; }));
  const auto st = rt->Evaluate(QString(kModule) + ".probe", Doc(12));
  EXPECT_TRUE(st.error.isEmpty()) << st.error.toStdString();
}

TEST_F(LuaApiTest, SecretsStayOutOfOrdinaryLuaValues) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, R"lua(
local secret = commands.get("com.example.luat.secret")
local sink = commands.get("com.example.luat.sink")
ui.subscribe { event = "app.ui_ready", handler = function()
  local ok, err = pcall(commands.invoke, sink, { data = "s3cret" })
  refused = not ok and tostring(err):find("cannot become one") ~= nil
  commands.invoke(secret, {}, function(r)
    blob_is_userdata = type(r.data) == "userdata"
    blob_is_opaque = r.data.bytes == nil and r.data[1] == nil
    commands.invoke(sink, { data = r.data })       -- forwarded: moves
    reuse_ok = pcall(commands.invoke, sink, { data = r.data })
  end)
end }
ui.action { id = "probe", anchor = ui.anchor("main.menu.help"),
  command = sink, update = function()
    assert(refused, "a string became a Blob")
    assert(blob_is_userdata and blob_is_opaque, "a Blob was readable")
    assert(not reuse_ok, "a Blob was used twice")
    return { visible = false }
  end }
)lua"));
  const auto sunk = g_sunk.load();
  rt->Deliver("app.ui_ready", {});
  ASSERT_TRUE(Pump([&] { return g_sunk.load() == sunk + 1; }));
  const auto st = rt->Evaluate(QString(kModule) + ".probe", {});
  EXPECT_TRUE(st.error.isEmpty() || st.error.startsWith("args"))
      << st.error.toStdString();
  EXPECT_FALSE(st.error.contains("assert")) << st.error.toStdString();
}

TEST_F(LuaApiTest, AtMostThirtyTwoResultsAreOwed) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, R"lua(
local park = commands.get("com.example.luat.park")
ui.subscribe { event = "app.ui_ready", handler = function()
  for i = 1, 32 do assert(commands.invoke(park, {}, function() end)) end
  capped = not pcall(commands.invoke, park, {}, function() end)
end }
)lua"));
  rt->Deliver("app.ui_ready", {});
  EXPECT_EQ(rt->Snapshot().pending_calls, 32);
  EXPECT_TRUE(rt->Snapshot().last_error.isEmpty())
      << rt->Snapshot().last_error.toStdString();
  rt->Teardown();
  g_parked.clear();
}

// Lua has no path of its own: every refusal is the registry's, and matches
// what a C++ caller with the same grant gets.
TEST_F(LuaApiTest, LuaGetsExactlyWhatTheRegistryGivesAnyCaller) {
  const auto via_cpp = [](const QString& id, uint32_t caps) {
    return CommandRegistry::Instance()
        .Invoke(id, {}, {}, UI::CommandCaller{kModule, caps, "test"}, {},
                [](gf::cmd::RawResult) {})
        .status;
  };
  EXPECT_EQ(via_cpp("com.example.luat.gpg", GF_HOST_CAP_UI), GF_CMD_E_DENIED);
  EXPECT_EQ(via_cpp("com.example.luat.missing", GF_HOST_CAP_UI),
            GF_CMD_E_UNKNOWN);

  auto rt = Make(GF_HOST_CAP_UI);
  QString error;
  EXPECT_FALSE(Load(*rt, "commands.get('com.example.luat.gpg')", &error))
      << "refused as the registry would: the grant lacks gpg";
  EXPECT_FALSE(Load(*rt, "commands.get('com.example.luat.missing')"));

  auto granted = Make(GF_HOST_CAP_UI | GF_HOST_CAP_GPG);
  ASSERT_TRUE(Load(*granted, R"lua(
local gpg = commands.get("com.example.luat.gpg")
ui.subscribe { event = "app.ui_ready", handler = function()
  call = commands.invoke(gpg, {})
end }
ui.action { id = "p", anchor = ui.anchor("main.menu.help"), command = gpg,
  update = function() assert(call ~= nil) return { visible = false } end }
)lua"));
  granted->Deliver("app.ui_ready", {});
  const auto st = granted->Evaluate(QString(kModule) + ".p", {});
  EXPECT_TRUE(st.error.isEmpty()) << st.error.toStdString();
}

// ------------------------------------------------------------ state

TEST_F(LuaApiTest, StateIsTheModulesOwn) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, R"lua(
ui.subscribe { event = "app.ui_ready", handler = function()
  state.set("lua_test/colour", "blue")
  state.set("lua_test/list", { 1, 2, 3 })
end }
)lua"));
  rt->Deliver("app.ui_ready", {});
  ASSERT_TRUE(Load(*rt, R"lua(
assert(state.get("lua_test/colour") == "blue")
assert(#state.get("lua_test/list") == 3)
assert(state.get("lua_test/none", 7) == 7)
assert(not pcall(state.get, "../escape"))
assert(not pcall(state.host, "general/anything"))
)lua"));
  ASSERT_TRUE(Load(*rt, R"lua(
ui.subscribe { event = "document.saved", handler = function()
  state.set("lua_test/colour", nil) state.set("lua_test/list", nil) end }
)lua"));
  rt->Deliver("document.saved", {});
}

// ------------------------------------------------------------ teardown

TEST_F(LuaApiTest, TeardownRunsInOrderOnceAndLetsNothingBackIn) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, R"lua(
local park = commands.get("com.example.luat.park")
ui.action { id = "a", anchor = ui.anchor("main.menu.help"), command = park }
ui.subscribe { event = "app.ui_ready", handler = function()
  commands.invoke(park, {}, function() late = true end)
end }
)lua"));
  rt->Deliver("app.ui_ready", {});
  ASSERT_EQ(rt->Snapshot().pending_calls, 1);

  rt->Teardown();
  rt->Teardown();  // idempotent
  EXPECT_EQ(rt->TeardownLog(),
            (QStringList{"callbacks stopped", "calls cancelled",
                         "actions and subscriptions removed", "mounts removed",
                         "handles invalidated", "state closed"}));
  EXPECT_TRUE(rt->Actions().isEmpty());

  // The command it was waiting on finishes now; nothing enters Lua.
  for (auto& r : g_parked) r.Ok({});
  g_parked.clear();
  QCoreApplication::processEvents();
  EXPECT_EQ(rt->Evaluate(QString(kModule) + ".a", {}).visible, false);
  rt->Deliver("app.ui_ready", {});
  QString error;
  EXPECT_FALSE(Load(*rt, "x = 1", &error));
}

TEST_F(LuaApiTest, StoppingIsImmediateFromAnyThread) {
  auto rt = Make();
  ASSERT_TRUE(Load(*rt, kEchoAction));
  rt->StopCallbacks();
  EXPECT_FALSE(rt->Evaluate(QString(kModule) + ".echo", Doc(1)).visible);
}

// ------------------------------------------------------------ reference

TEST_F(LuaApiTest, TheAnchorCatalogIsStable) {
  const auto reference = UI::Lua::AnchorCatalogReference();
  for (const char* id :
       {"main.menu.file.workspace", "main.menu.advanced", "main.menu.help",
        "main.menu.import_key", "editor.context", "key.details.actions",
        "settings", "editor", "dialog"}) {
    EXPECT_NE(UI::Lua::FindAnchor(id), nullptr) << id;
    EXPECT_TRUE(reference.contains(QLatin1String(id))) << id;
  }
  EXPECT_EQ(UI::Lua::FindAnchor("main_menu.help"), nullptr);
  EXPECT_TRUE(UI::Lua::LuaApiReference().contains("ui.action"));
}

}  // namespace GpgFrontend::Test
