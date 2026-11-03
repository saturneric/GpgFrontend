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
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QTemporaryDir>
#include <QUrl>
#include <QWidget>

#ifdef Q_OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "GpgFrontendTest.h"
#include "host/SdkTestContext.h"
#include "platform/common/PlatformWebEditor.h"
#include "sdk/GFSDKBuffer.h"
#include "sdk/GFSDKUI.h"
#include "sdk/GFSDKWebEditor.h"
#include "ui/html/HtmlViewDocument.h"
#include "ui/html/WebEditorDocument.h"
#include "ui/html/WebEditorService.h"
#include "ui/lua/NativeInstances.h"
#include "ui/lua/NativeWidgetRegistry.h"

/**
 * @file GFUiWebEditorTest.cpp
 * @brief Web editors against a fake engine: who may put an editor where,
 *        what a page may load, what may cross the bridge, and what happens
 *        when the editor or its engine goes away.
 *
 * Through the public C ABI with real minted contexts. Widgets and every SDK
 * call live on the GUI thread, which these tests are not on.
 */

namespace GpgFrontend::Test {

namespace {

namespace HtmlView = UI::HtmlView;
namespace WebEditor = UI::WebEditor;
using UI::WebEditorService;

constexpr auto kConsumer = "com.example.web.consumer";
constexpr auto kOtherConsumer = "com.example.web.other";
constexpr auto kProvider = "com.example.web.provider";

constexpr uint32_t kEditorCaps =
    GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM | GF_HOST_CAP_WEB_EDITOR;
constexpr uint32_t kProviderCaps =
    GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM | GF_HOST_CAP_WEB_EDITOR_PROVIDER;

void OnGui(const std::function<void()>& fn) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), fn,
                            Qt::BlockingQueuedConnection);
}

void Settle() {
  OnGui([] {});
  OnGui([] {});
}

/// WebKit answers on the main run loop, which the offscreen platform the
/// tests run under never spins on its own; the application, on Cocoa,
/// always does.
void SpinRunLoop() {
#ifdef Q_OS_MACOS
  OnGui([] { CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false); });
#endif
}

auto HasHostEngine() -> bool {
  bool has = false;
  OnGui([&] {
    has = WebEditorService::Instance().ProviderOwners().contains(QString());
  });
  return has;
}

/// Where the platform has an engine, wait for it: it registers once its
/// rules have compiled, which may still be under way when the first test
/// starts. Once: after that it is there or it never will be.
auto WaitForHostEngine() -> bool {
#ifdef Q_OS_MACOS
  static bool waited = false;
  for (int i = 0; !waited && i < 200 && !HasHostEngine(); ++i) SpinRunLoop();
  waited = true;
#endif
  return HasHostEngine();
}

auto CreateRoot(void*, uint64_t, GFBufferView) -> void* {
  auto* root = new QWidget();
  auto* slot = new QWidget(root);
  slot->setObjectName("slot");
  return root;
}
void DlgOpened(void*, uint64_t, GFBufferView) {}
const GFDialogWidgetOps kDlg = {sizeof(GFDialogWidgetOps), &DlgOpened};

class Container : public UI::NativeContainer {};

struct EngineLog {
  int created = 0;
  int destroyed = 0;
  QString session;
  QList<QByteArray> posted;
  QList<QPointer<QWidget>> widgets;
};
EngineLog g_engine;

auto FakeCreate(void*, uint64_t, const char* session) -> void* {
  ++g_engine.created;
  g_engine.session = QString::fromUtf8(session);
  auto* w = new QWidget();
  g_engine.widgets.append(w);
  return w;
}
auto FakePost(void*, uint64_t, const char* json, size_t len) -> int {
  g_engine.posted.append(QByteArray(json, static_cast<qsizetype>(len)));
  return 0;
}
void FakeDestroyed(void*, uint64_t) { ++g_engine.destroyed; }

const GFWebEditorProviderOps kOps = {sizeof(GFWebEditorProviderOps),
                                     &FakeCreate, &FakePost, &FakeDestroyed};

struct Heard {
  int event;
  QByteArray text;
};
QList<Heard> g_heard;

void OnEvent(void*, uint64_t, int event, const char* text, size_t len) {
  g_heard.append({event, text == nullptr
                             ? QByteArray()
                             : QByteArray(text, static_cast<qsizetype>(len))});
}

class WebEditorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    g_engine = {};
    g_heard.clear();
    ASSERT_TRUE(assets_.isValid());
    write("index.html",
          "<!doctype html><html><head><title>t</title></head>"
          "<body><script src=\"editor.js\"></script></body></html>");
    write("editor.js", "window.x = 1;");
    write("secret.txt", "never served");
    // The Host's own engine outranks any module's: detached for the test,
    // and put back after it.
    host_engine_ = WaitForHostEngine();
    OnGui([this] {
      auto& service = WebEditorService::Instance();
      // The application may have a real engine; these tests use the fake.
      for (const auto& owner : service.ProviderOwners()) {
        if (!owner.startsWith(u"com.example."_s)) service.WithdrawModule(owner);
      }
      if (host_engine_) {
        service.UnregisterProvider(
            {}, QString::fromLatin1(UI::kPlatformWebEditorEngineName));
      }
      service.SetAssetRootForTest(kConsumer, assets_.path() + u"/"_s);
      service.SetLinkHandler([this](const QUrl& url) { opened_ << url; });
      GFWebEditorProviderSpec spec{};
      spec.struct_size = sizeof(spec);
      spec.interface_version = GF_WEB_EDITOR_PROVIDER_V1;
      spec.name = "fake";
      spec.ops = &kOps;
      ASSERT_EQ(GFWebEditorProviderRegister(provider_(), &spec),
                GF_HTML_VIEW_OK);
    });
  }

  void TearDown() override {
    OnGui([this] {
      auto& service = WebEditorService::Instance();
      for (const auto* m : {kConsumer, kOtherConsumer, kProvider}) {
        service.WithdrawModule(m);
        UI::NativeInstances::Instance().WithdrawAll(m);
        UI::NativeWidgetRegistry::Instance().RemoveAllFor(m);
      }
      service.SetAssetRootForTest(kConsumer, {});
      service.SetLinkHandler({});
      if (host_engine_) UI::RegisterPlatformWebEditorEngine();
    });
    OnGui([this] {
      for (const auto& root : roots_) delete root.data();
    });
    Settle();
  }

  void write(const QString& name, const QByteArray& body) {
    QFile f(assets_.path() + u"/"_s + name);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write(body);
  }

  auto make_instance(SdkTestContext& ctx, const QString& module, QWidget** slot)
      -> quint64 {
    GFNativeWidgetSpec s{};
    s.struct_size = sizeof(s);
    s.name = "note";
    s.kind = GF_NATIVE_DIALOG;
    s.create = &CreateRoot;
    s.dialog = &kDlg;
    quint64 id = 0;
    OnGui([&] {
      GFNativeWidgetRegister(ctx(), &s);
      const auto instance = UI::NativeInstances::Instance().Create(
          module + ".note", {}, &container_);
      if (!instance.has_value()) return;
      id = instance->id;
      roots_.append(instance->widget);
      *slot = instance->widget->findChild<QWidget*>("slot");
    });
    return id;
  }

  auto make_editor() -> uint64_t {
    QWidget* slot = nullptr;
    const auto instance = make_instance(consumer_, kConsumer, &slot);
    uint64_t editor = 0;
    OnGui([&] {
      EXPECT_EQ(GFWebEditorCreate(consumer_(), instance, slot, &OnEvent,
                                  nullptr, &editor),
                GF_HTML_VIEW_OK);
    });
    return editor;
  }

  auto resolve(uint64_t editor, const QString& url, QByteArray* data = nullptr,
               QString* mime = nullptr) -> int {
    int rc = 0;
    OnGui([&] {
      GFBufferRef d = nullptr;
      GFBufferRef m = nullptr;
      rc = GFWebEditorProviderResolve(provider_(), editor,
                                      url.toUtf8().constData(), &d, &m);
      if (rc == GF_HTML_VIEW_OK) {
        auto* ctx = provider_();
        if (data != nullptr) {
          *data = QByteArray(static_cast<const char*>(GFBufferData(ctx, d)),
                             static_cast<qsizetype>(GFBufferSize(ctx, d)));
        }
        if (mime != nullptr) {
          *mime =
              QString::fromUtf8(static_cast<const char*>(GFBufferData(ctx, m)),
                                static_cast<qsizetype>(GFBufferSize(ctx, m)));
        }
        GFBufferRelease(ctx, d);
        GFBufferRelease(ctx, m);
      }
    });
    return rc;
  }

  auto url(const QString& scheme, const QString& path) const -> QString {
    return u"%1://%2/%3"_s.arg(scheme, g_engine.session, path);
  }

  QTemporaryDir assets_;
  SdkTestContext consumer_{kConsumer, kEditorCaps};
  SdkTestContext other_{kOtherConsumer, kEditorCaps};
  SdkTestContext provider_{kProvider, kProviderCaps};
  Container container_;
  QList<QPointer<QWidget>> roots_;
  QList<QUrl> opened_;
  bool host_engine_ = false;
};

}  // namespace

// --- the pure half --------------------------------------------------------

TEST(WebEditorDocumentTest, ThePolicyComesFirstInTheHead) {
  const auto out = WebEditor::InjectPolicy(
      "<html><head><meta http-equiv=\"Content-Security-Policy\" "
      "content=\"script-src *\"></head></html>");
  const auto ours = out.indexOf("default-src 'none'");
  const auto theirs = out.indexOf("script-src *");
  ASSERT_GE(ours, 0);
  EXPECT_LT(ours, theirs) << "the Host's policy precedes the page's";
  EXPECT_TRUE(
      WebEditor::InjectPolicy("<p>no head</p>").contains("default-src"));
  const auto csp = WebEditor::ContentSecurityPolicy();
  EXPECT_FALSE(csp.contains(u"unsafe-eval"_s));
  EXPECT_TRUE(csp.contains(u"script-src gfasset:;"_s))
      << "script only from the bundle, never inline";
  EXPECT_TRUE(csp.contains(u"connect-src 'none'"_s));
}

TEST(WebEditorDocumentTest, RequestsAreTakenApartStrictly) {
  const auto session = HtmlView::NewSession();
  const auto at = [&](const QString& scheme, const QString& rest) {
    return u"%1://%2/%3"_s.arg(scheme, session, rest);
  };
  const auto ok = WebEditor::ParseRequest(at(u"gfasset"_s, u"js/editor.js"_s));
  ASSERT_TRUE(ok.has_value());
  EXPECT_TRUE(ok->asset);
  EXPECT_EQ(ok->session, session);
  EXPECT_EQ(ok->path, u"js/editor.js"_s);
  EXPECT_FALSE(WebEditor::ParseRequest(at(u"https"_s, u"x.js"_s)).has_value());
  EXPECT_FALSE(
      WebEditor::ParseRequest(at(u"gfasset"_s, u"../x"_s)).has_value());
  EXPECT_FALSE(
      WebEditor::ParseRequest(at(u"gfres"_s, u"%2e%2e/secret"_s)).has_value());
  EXPECT_FALSE(WebEditor::ParseRequest(at(u"gfres"_s, {})).has_value());
  EXPECT_FALSE(
      WebEditor::ParseRequest(u"gfasset://abc/js/editor.js"_s).has_value())
      << "a session is only ever one the Host made";
  EXPECT_FALSE(WebEditor::IsCleanPath(u"a\\b"_s));
  EXPECT_TRUE(WebEditor::IsCleanPath(u"assets/1.png"_s));
}

TEST(WebEditorDocumentTest, OnlyWellFormedMessagesCross) {
  EXPECT_TRUE(WebEditor::IsValidMessage(R"({"type":"dirty","gen":3})"));
  EXPECT_FALSE(WebEditor::IsValidMessage("[1,2]"));
  EXPECT_FALSE(WebEditor::IsValidMessage(R"({"gen":3})"));
  EXPECT_FALSE(WebEditor::IsValidMessage(R"({"type":""})"));
  EXPECT_FALSE(WebEditor::IsValidMessage("{not json"));
  EXPECT_FALSE(
      WebEditor::IsValidMessage("{\"type\":\"" + QByteArray(65, 'x') + "\"}"));
  EXPECT_FALSE(WebEditor::IsAllowedResourceMime(u"image/svg+xml"_s));
  EXPECT_TRUE(WebEditor::IsAllowedResourceMime(u"image/png"_s));
  EXPECT_EQ(WebEditor::AssetMime(u"x.txt"_s), QString());
}

// --- the service, through the ABI -------------------------------------------

TEST_F(WebEditorTest, AnEditorServesItsOwnPageUnderThePolicy) {
  const auto editor = make_editor();
  ASSERT_NE(editor, 0U);
  EXPECT_EQ(g_engine.created, 1);
  ASSERT_FALSE(g_engine.session.isEmpty());

  QByteArray body;
  QString mime;
  ASSERT_EQ(resolve(editor, url("gfasset", "index.html"), &body, &mime),
            GF_HTML_VIEW_OK);
  EXPECT_EQ(mime, u"text/html"_s);
  EXPECT_TRUE(body.contains("default-src 'none'"));
  ASSERT_EQ(resolve(editor, url("gfasset", "editor.js"), &body, &mime),
            GF_HTML_VIEW_OK);
  EXPECT_EQ(body, "window.x = 1;");

  // Not a type the page may load, not the session, not this editor's.
  EXPECT_NE(resolve(editor, url("gfasset", "secret.txt")), GF_HTML_VIEW_OK);
  EXPECT_NE(resolve(editor, u"gfasset://other/index.html"_s), GF_HTML_VIEW_OK);
  EXPECT_NE(resolve(editor, u"https://example.org/x.js"_s), GF_HTML_VIEW_OK);
}

TEST_F(WebEditorTest, ResourcesAreTheConsumersAndOnlyImages) {
  const auto editor = make_editor();
  ASSERT_NE(editor, 0U);
  OnGui([&] {
    auto* ctx = consumer_();
    EXPECT_EQ(GFWebEditorPutResource(ctx, editor, "assets/a.png", "image/png",
                                     GFBufferNewFromBytes(ctx, "PNG", 3)),
              GF_HTML_VIEW_OK);
    EXPECT_NE(
        GFWebEditorPutResource(ctx, editor, "assets/b.svg", "image/svg+xml",
                               GFBufferNewFromBytes(ctx, "<svg/>", 6)),
        GF_HTML_VIEW_OK);
    EXPECT_NE(GFWebEditorPutResource(ctx, editor, "../escape.png", "image/png",
                                     GFBufferNewFromBytes(ctx, "x", 1)),
              GF_HTML_VIEW_OK);
    // Another module cannot reach into this editor at all.
    EXPECT_EQ(
        GFWebEditorPutResource(other_(), editor, "assets/c.png", "image/png",
                               GFBufferNewFromBytes(other_(), "x", 1)),
        GF_HTML_VIEW_E_DENIED);
  });
  QByteArray body;
  ASSERT_EQ(resolve(editor, url("gfres", "assets/a.png"), &body),
            GF_HTML_VIEW_OK);
  EXPECT_EQ(body, "PNG");
  OnGui([&] {
    EXPECT_EQ(GFWebEditorRemoveResource(consumer_(), editor, "assets/a.png"),
              GF_HTML_VIEW_OK);
  });
  EXPECT_NE(resolve(editor, url("gfres", "assets/a.png")), GF_HTML_VIEW_OK);
}

TEST_F(WebEditorTest, MessagesAreCheckedBothWays) {
  const auto editor = make_editor();
  ASSERT_NE(editor, 0U);
  OnGui([&] {
    EXPECT_EQ(GFWebEditorPost(consumer_(), editor, R"({"type":"load"})", 15),
              GF_HTML_VIEW_OK);
    EXPECT_NE(GFWebEditorPost(consumer_(), editor, "[]", 2), GF_HTML_VIEW_OK);
    const QByteArray from_page(R"({"type":"dirty","gen":1})");
    EXPECT_EQ(GFWebEditorProviderMessage(
                  provider_(), editor, from_page.constData(), from_page.size()),
              GF_HTML_VIEW_OK);
    EXPECT_NE(GFWebEditorProviderMessage(provider_(), editor, "nope", 4),
              GF_HTML_VIEW_OK);
  });
  ASSERT_EQ(g_engine.posted.size(), 1);
  EXPECT_EQ(g_engine.posted[0], R"({"type":"load"})");
  ASSERT_EQ(g_heard.size(), 1);
  EXPECT_EQ(g_heard[0].event, GF_WEB_EDITOR_EVENT_MESSAGE);
  EXPECT_EQ(g_heard[0].text, R"({"type":"dirty","gen":1})");
}

TEST_F(WebEditorTest, LinksGoToTheHostNotThePage) {
  const auto editor = make_editor();
  OnGui([&] {
    EXPECT_EQ(GFWebEditorProviderLinkActivated(provider_(), editor,
                                               "https://example.org/a"),
              GF_HTML_VIEW_OK);
  });
  Settle();
  ASSERT_EQ(opened_.size(), 1);
  EXPECT_EQ(opened_[0], QUrl(u"https://example.org/a"_s));
}

TEST_F(WebEditorTest, AnotherModulesInstanceIsRefused) {
  QWidget* slot = nullptr;
  const auto instance = make_instance(consumer_, kConsumer, &slot);
  OnGui([&] {
    uint64_t editor = 0;
    EXPECT_EQ(
        GFWebEditorCreate(other_(), instance, slot, &OnEvent, nullptr, &editor),
        GF_HTML_VIEW_E_DENIED);
    EXPECT_EQ(editor, 0U);
  });
  EXPECT_EQ(g_engine.created, 0);
}

TEST_F(WebEditorTest, DestroyingForgetsTheSessionAndItsResources) {
  const auto editor = make_editor();
  const auto session_url = url("gfasset", "index.html");
  OnGui([&] {
    auto* ctx = consumer_();
    GFWebEditorPutResource(ctx, editor, "assets/a.png", "image/png",
                           GFBufferNewFromBytes(ctx, "PNG", 3));
    EXPECT_EQ(GFWebEditorDestroy(consumer_(), editor), GF_HTML_VIEW_OK);
  });
  EXPECT_EQ(g_engine.destroyed, 1);
  EXPECT_NE(resolve(editor, session_url), GF_HTML_VIEW_OK);
  OnGui([&] {
    EXPECT_NE(GFWebEditorPost(consumer_(), editor, R"({"type":"x"})", 12),
              GF_HTML_VIEW_OK);
  });
}

TEST_F(WebEditorTest, AWithdrawnEngineTellsItsConsumers) {
  const auto editor = make_editor();
  ASSERT_NE(editor, 0U);
  OnGui([] { WebEditorService::Instance().WithdrawModule(kProvider); });
  ASSERT_FALSE(g_heard.isEmpty());
  EXPECT_EQ(g_heard.last().event, GF_WEB_EDITOR_EVENT_LOST);
  OnGui([&] { EXPECT_NE(GFWebEditorAvailable(consumer_()), GF_HTML_VIEW_OK); });
}

// --- the Host's own engine, for real --------------------------------------

#if defined(Q_OS_MACOS) || defined(Q_OS_WINDOWS)

namespace {

/// The fixture, with the fake withdrawn and the Host's engine back.
class WebEditorPlatformTest : public WebEditorTest {
 protected:
  void SetUp() override {
    WebEditorTest::SetUp();
    ASSERT_TRUE(host_engine_)
        << "the platform's engine never registered itself";
    OnGui([] {
      WebEditorService::Instance().WithdrawModule(kProvider);
      UI::RegisterPlatformWebEditorEngine();
    });
  }

  /// What the page said, by its "type"; and whether it said it at all.
  [[nodiscard]] static auto said(const QString& type) -> QJsonObject {
    for (const auto& h : std::as_const(g_heard)) {
      if (h.event != GF_WEB_EDITOR_EVENT_MESSAGE) continue;
      const auto obj = QJsonDocument::fromJson(h.text).object();
      if (obj.value(u"type"_s).toString() == type) return obj;
    }
    return {};
  }

  /// Spin until @p done, or about ten seconds.
  static auto until(const std::function<bool()>& done) -> bool {
    for (int i = 0; i < 200; ++i) {
      if (done()) return true;
      SpinRunLoop();
    }
    return done();
  }
};

}  // namespace

// On Windows that takes the WebView2 SDK at build time and the Evergreen
// runtime here.
TEST_F(WebEditorPlatformTest, TheHostsOwnEngineIsTheOneUsed) {
  OnGui([&] {
    EXPECT_EQ(GFWebEditorAvailable(consumer_()), GF_HTML_VIEW_OK);
    EXPECT_TRUE(WebEditorService::Instance().ProviderOwners().contains(
        QString()));
  });
}

#endif

// WebView2 needs a real HWND, which the offscreen platform the tests run
// under has none of; on Windows the page is only seen in the application.
#ifdef Q_OS_MACOS

TEST_F(WebEditorPlatformTest, ARealPageLoadsTalksAndStaysInsideThePolicy) {
  // An inline script the policy must stop, and the module's own file that
  // it must let run.
  write("index.html",
        "<!doctype html><html><head><title>t</title></head><body>"
        "<script>gfBridge.post({type: \"inline\"});</script>"
        "<script src=\"editor.js\"></script></body></html>");
  write("editor.js", R"JS(
gfBridge.onMessage(function (m) { gfBridge.post({type: "echo", got: m.type}); });
function probe(type, src, width) {
  var img = new Image();
  img.onload = function () {
    gfBridge.post({type: type, ok: img.naturalWidth === width});
  };
  img.onerror = function () { gfBridge.post({type: type, ok: false}); };
  img.src = src;
}
probe("res", "gfres://" + location.host + "/dot.png", 1);
probe("net", "https://example.invalid/x.png", 1);
gfBridge.post({type: "hello"});
)JS");
  // A 1x1 PNG.
  const auto png = QByteArray::fromBase64(
      "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA"
      "60e6kgAAAABJRU5ErkJggg==");

  QWidget* slot = nullptr;
  const auto instance = make_instance(consumer_, kConsumer, &slot);
  uint64_t editor = 0;
  OnGui([&] {
    auto* ctx = consumer_();
    ASSERT_EQ(
        GFWebEditorCreate(ctx, instance, slot, &OnEvent, nullptr, &editor),
        GF_HTML_VIEW_OK);
    // Before the page has asked for anything: it is answered on the run
    // loop, which has not turned yet.
    ASSERT_EQ(GFWebEditorPutResource(
                  ctx, editor, "dot.png", "image/png",
                  GFBufferNewFromBytes(ctx, png.constData(), png.size())),
              GF_HTML_VIEW_OK);
    // Kept until the page is ready, then delivered.
    ASSERT_EQ(GFWebEditorPost(ctx, editor, R"({"type":"early"})", 16),
              GF_HTML_VIEW_OK);
  });
  ASSERT_NE(editor, 0U);
  EXPECT_EQ(g_engine.created, 0) << "the fake engine was used";

  const auto ready = [] {
    for (const auto& h : std::as_const(g_heard)) {
      if (h.event == GF_WEB_EDITOR_EVENT_READY) return true;
    }
    return false;
  };
  ASSERT_TRUE(until([&] {
    return ready() && !said(u"hello"_s).isEmpty() &&
           !said(u"res"_s).isEmpty() && !said(u"net"_s).isEmpty() &&
           !said(u"echo"_s).isEmpty();
  })) << "heard " << g_heard.size() << " events";

  EXPECT_EQ(said(u"echo"_s).value(u"got"_s).toString(), u"early"_s);
  EXPECT_TRUE(said(u"res"_s).value(u"ok"_s).toBool())
      << "a resource the consumer put was not served";
  EXPECT_FALSE(said(u"net"_s).value(u"ok"_s).toBool())
      << "the page reached the network";
  EXPECT_TRUE(said(u"inline"_s).isEmpty()) << "an inline script ran";

  // Both ways once ready, too.
  g_heard.clear();
  OnGui([&] {
    EXPECT_EQ(GFWebEditorPost(consumer_(), editor, R"({"type":"ping"})", 15),
              GF_HTML_VIEW_OK);
  });
  ASSERT_TRUE(until([&] { return !said(u"echo"_s).isEmpty(); }));
  EXPECT_EQ(said(u"echo"_s).value(u"got"_s).toString(), u"ping"_s);

  OnGui([&] {
    EXPECT_EQ(GFWebEditorDestroy(consumer_(), editor), GF_HTML_VIEW_OK);
  });
}

#endif

}  // namespace GpgFrontend::Test
