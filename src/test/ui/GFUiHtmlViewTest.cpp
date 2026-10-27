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
#include <QPointer>
#include <QUrl>
#include <QWidget>

#ifdef Q_OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "GpgFrontendTest.h"
#include "core/SdkTestContext.h"
#include "core/module/ModuleLibraryPin.h"
#include "platform/common/PlatformHtmlView.h"
#include "sdk/GFSDKBuffer.h"
#include "sdk/GFSDKHtmlView.h"
#include "sdk/GFSDKUI.h"
#include "ui/html/HtmlViewDocument.h"
#include "ui/html/HtmlViewService.h"
#include "ui/lua/NativeInstances.h"
#include "ui/lua/NativeWidgetRegistry.h"

/**
 * @file GFUiHtmlViewTest.cpp
 * @brief HTML views against a fake renderer: who may put a view where, what
 *        a renderer may ask for, and what happens when it goes away.
 *
 * Everything goes through the public C ABI with real minted contexts, so a
 * refusal here is the refusal a module would get. The renderer is a fake
 * that records every call; the engines are tested against real mail
 * elsewhere. Widgets and every SDK call live on the GUI thread, which these
 * tests are not on.
 */

namespace GpgFrontend::Test {

namespace {

namespace HtmlView = UI::HtmlView;
using UI::HtmlViewService;

constexpr auto kConsumer = "com.example.html.consumer";
constexpr auto kOtherConsumer = "com.example.html.other";
constexpr auto kProvider = "com.example.html.provider";
constexpr auto kOtherProvider = "com.example.html.provider2";

constexpr uint32_t kViewCaps =
    GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM | GF_HOST_CAP_HTML_VIEW;
constexpr uint32_t kProviderCaps =
    GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM | GF_HOST_CAP_HTML_PROVIDER;
constexpr uint32_t kEverything =
    GF_HTML_POLICY_NO_NETWORK | GF_HTML_POLICY_PARTS_ONLY |
    GF_HTML_POLICY_NO_SCRIPT | GF_HTML_POLICY_NO_NAVIGATION |
    GF_HTML_POLICY_NO_POPUPS | GF_HTML_POLICY_NO_DOWNLOADS |
    GF_HTML_POLICY_NO_PERSISTENCE | GF_HTML_POLICY_NO_PERMISSIONS |
    GF_HTML_POLICY_NO_DNS_PREFETCH | GF_HTML_POLICY_NO_PLUGINS;

void OnGui(const std::function<void()>& fn) {
  QMetaObject::invokeMethod(QCoreApplication::instance(), fn,
                            Qt::BlockingQueuedConnection);
}

/// Let whatever is already queued on the GUI thread run: part answers are
/// posted, never delivered inside the request.
void Settle() {
  OnGui([] {});
  OnGui([] {});
}

/// Whether the Host's own renderer -- the empty owner -- is registered.
auto HasHostRenderer() -> bool {
  bool has = false;
  OnGui([&] {
    has = HtmlViewService::Instance().ProviderOwners().contains(QString());
  });
  return has;
}

/// Where the platform has an engine, wait for its renderer: it registers
/// once the engine's rules have compiled, which may still be under way when
/// the first test starts. Once: after that it is there or it never will be.
auto WaitForHostRenderer() -> bool {
#ifdef Q_OS_MACOS
  static bool waited = false;
  for (int i = 0; !waited && i < 200 && !HasHostRenderer(); ++i) {
    // WebKit answers on the main run loop, which the offscreen platform the
    // tests run under never spins on its own; the application, on Cocoa,
    // always does.
    OnGui([] { CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false); });
  }
  waited = true;
#endif
  return HasHostRenderer();
}

// --- the consumer's native widget: a root with an empty slot inside

auto CreateRoot(void*, uint64_t, GFBufferView) -> void* {
  auto* root = new QWidget();
  auto* slot = new QWidget(root);
  slot->setObjectName("slot");
  return root;
}
void DlgOpened(void*, uint64_t, GFBufferView) {}
const GFDialogWidgetOps kDlg = {sizeof(GFDialogWidgetOps), &DlgOpened};

class Container : public UI::NativeContainer {};

// --- the fake renderer

GFSDKContext* g_part_ctx = nullptr;  ///< the renderer's context

struct RendererLog {
  int created = 0;
  int loaded = 0;
  int cleared = 0;
  int destroyed = 0;
  bool refuse_create = false;
  QString last_session;
  QByteArray last_document;
  QList<QPointer<QWidget>> widgets;
};
RendererLog g_renderer;

auto FakeCreate(void*, uint64_t, const GFHtmlViewPolicy* policy) -> void* {
  if (g_renderer.refuse_create || policy == nullptr) return nullptr;
  ++g_renderer.created;
  auto* w = new QWidget();
  g_renderer.widgets.append(w);
  return w;
}
auto FakeLoad(void*, uint64_t, const char* session, GFBufferView document)
    -> int {
  ++g_renderer.loaded;
  g_renderer.last_session = QString::fromUtf8(session);
  // Lent for the call, to the renderer: read through the renderer's context.
  const auto* data =
      static_cast<const char*>(GFBufferData(g_part_ctx, document));
  g_renderer.last_document =
      data == nullptr ? QByteArray()
                      : QByteArray(data, static_cast<qsizetype>(GFBufferSize(
                                             g_part_ctx, document)));
  return 0;
}
void FakeClear(void*, uint64_t) { ++g_renderer.cleared; }
void FakeDestroyed(void*, uint64_t) { ++g_renderer.destroyed; }

const GFHtmlViewProviderOps kOps = {sizeof(GFHtmlViewProviderOps), &FakeCreate,
                                    &FakeLoad, &FakeClear, &FakeDestroyed};

auto Spec(uint32_t enforceable, uint32_t features = 0)
    -> GFHtmlViewProviderSpec {
  GFHtmlViewProviderSpec s{};
  s.struct_size = sizeof(s);
  s.interface_version = GF_HTML_VIEW_PROVIDER_V1;
  s.name = "fake";
  s.enforceable = enforceable;
  s.features = features;
  s.ops = &kOps;
  return s;
}

// --- what the consumer heard

struct EventLog {
  int loaded = 0;
  int lost = 0;
  QStringList links;
};
EventLog g_events;

void OnEvent(void*, uint64_t, int event, int, const char* text) {
  switch (event) {
    case GF_HTML_VIEW_EVENT_LOADED:
      ++g_events.loaded;
      break;
    case GF_HTML_VIEW_EVENT_LINK:
      g_events.links << QString::fromUtf8(text);
      break;
    case GF_HTML_VIEW_EVENT_LOST:
      ++g_events.lost;
      break;
    default:
      break;
  }
}

// --- part answers

struct PartAnswer {
  uint64_t request = 0;
  int status = -1;
  QString mime;
  bool had_data = false;
};
QList<PartAnswer> g_parts;

void OnPart(void*, uint64_t request, int status, const char* mime,
            GFBufferRef data) {
  g_parts.append(
      PartAnswer{request, status, QString::fromUtf8(mime), data != nullptr});
  if (data != nullptr) GFBufferRelease(g_part_ctx, data);
}

// --- the fixture

class HtmlViewTest : public ::testing::Test {
 protected:
  void SetUp() override {
    g_renderer = {};
    g_events = {};
    g_parts.clear();
    g_part_ctx = provider_();
    // The Host's own renderer outranks any module's: detached for the test,
    // and put back after it.
    host_renderer_ = WaitForHostRenderer();
    OnGui([this] {
      // The process may have a real renderer: gf_test starts the
      // application, modules and all, and a tree built with the WebEngine
      // module activates it. These tests are about the fake alone.
      auto& service = HtmlViewService::Instance();
      for (const auto& owner : service.ProviderOwners()) {
        if (!owner.startsWith(u"com.example."_s)) service.WithdrawModule(owner);
      }
      if (host_renderer_) {
        service.UnregisterProvider(
            {}, QString::fromLatin1(UI::kPlatformHtmlViewRendererName));
      }
      HtmlViewService::Instance().SetLinkHandler(
          [this](const QUrl& url) { opened_ << url.toString(); });
    });
  }

  void TearDown() override {
    OnGui([this] {
      auto& service = HtmlViewService::Instance();
      for (const auto* m :
           {kConsumer, kOtherConsumer, kProvider, kOtherProvider}) {
        service.WithdrawModule(m);
        UI::NativeInstances::Instance().WithdrawAll(m);
        UI::NativeWidgetRegistry::Instance().RemoveAllFor(m);
      }
      service.SetLinkHandler({});
      if (host_renderer_) UI::RegisterPlatformHtmlViewRenderer();
    });
    OnGui([this] {
      for (const auto& root : roots_) delete root.data();
    });
    Settle();
    g_part_ctx = nullptr;
  }

  /// A live instance of @p ctx's widget, and the empty slot inside it.
  auto make_instance(SdkTestContext& ctx, const QString& module, QWidget** slot)
      -> quint64 {
    GFNativeWidgetSpec s{};
    s.struct_size = sizeof(s);
    s.name = "mail";
    s.kind = GF_NATIVE_DIALOG;
    s.create = &CreateRoot;
    s.dialog = &kDlg;
    quint64 id = 0;
    OnGui([&] {
      GFNativeWidgetRegister(ctx(), &s);  // once per module; later: refused
      const auto instance = UI::NativeInstances::Instance().Create(
          module + ".mail", {}, &container_);
      if (!instance.has_value()) return;
      id = instance->id;
      roots_.append(instance->widget);
      *slot = instance->widget->findChild<QWidget*>("slot");
    });
    return id;
  }

  /// A view in a fresh instance of the consumer's widget, on the fake.
  auto make_view() -> uint64_t {
    QWidget* slot = nullptr;
    const auto instance = make_instance(consumer_, kConsumer, &slot);
    uint64_t view = 0;
    OnGui([&] {
      EXPECT_EQ(
          GFHtmlViewCreate(consumer_(), instance, slot,
                           GF_HTML_VIEW_PRESET_MAIL, &OnEvent, nullptr, &view),
          GF_HTML_VIEW_OK);
    });
    return view;
  }

  void register_fake(uint32_t enforceable = kEverything,
                     uint32_t features = 0) {
    const auto spec = Spec(enforceable, features);
    OnGui([&] {
      ASSERT_EQ(GFHtmlProviderRegister(provider_(), &spec), GF_HTML_VIEW_OK);
    });
  }

  using Parts = QList<std::tuple<const char*, const char*, QByteArray>>;

  /// Already on the GUI thread. The GUI thread keeps running between two
  /// blocking calls from here, so a step that must follow another with no
  /// queued answer in between is made inside one OnGui() with these.
  auto load_now(uint64_t view, const QByteArray& body, const Parts& parts,
                size_t* dropped = nullptr) -> int {
    auto* body_ref = GFBufferNewFromBytes(consumer_(), body.constData(),
                                          static_cast<size_t>(body.size()));
    QList<GFHtmlPart> c;
    for (const auto& [cid, mime, bytes] : parts) {
      GFHtmlPart p{};
      p.struct_size = sizeof(p);
      p.cid = cid;
      p.mime = mime;
      p.data = GFBufferNewFromBytes(consumer_(), bytes.constData(),
                                    static_cast<size_t>(bytes.size()));
      c.append(p);
    }
    return GFHtmlViewLoad(consumer_(), view, body_ref, c.constData(),
                          static_cast<size_t>(c.size()), dropped);
  }

  auto ask(uint64_t view, const QString& session, const char* cid,
           SdkTestContext* who = nullptr) -> std::pair<int, uint64_t> {
    auto& ctx = who != nullptr ? *who : provider_;
    std::pair<int, uint64_t> out{};
    out.first =
        GFHtmlProviderRequestPart(ctx(), view, session.toUtf8().constData(),
                                  cid, &OnPart, nullptr, &out.second);
    return out;
  }

  auto load_body(uint64_t view, const QByteArray& body, const Parts& parts,
                 size_t* dropped = nullptr) -> int {
    int rc = 0;
    OnGui([&] { rc = load_now(view, body, parts, dropped); });
    return rc;
  }

  auto request_part(uint64_t view, const QString& session, const char* cid,
                    SdkTestContext* who = nullptr) -> std::pair<int, uint64_t> {
    std::pair<int, uint64_t> out{};
    OnGui([&] { out = ask(view, session, cid, who); });
    return out;
  }

  SdkTestContext consumer_{kConsumer, kViewCaps};
  SdkTestContext other_consumer_{kOtherConsumer, kViewCaps};
  SdkTestContext provider_{kProvider, kProviderCaps};
  SdkTestContext other_provider_{kOtherProvider, kProviderCaps};
  Container container_;
  bool host_renderer_ = false;
  QStringList opened_;
  QList<QPointer<QWidget>> roots_;
};

}  // namespace

// --- the document

TEST(HtmlViewDocumentTest, ThePolicyComesFirstAndTheBodyStaysAnAttribute) {
  const auto session = HtmlView::NewSession();
  ASSERT_TRUE(HtmlView::IsSessionName(session));

  const QString hostile =
      "\"></iframe><script>alert(1)</script><meta http-equiv=\"Content-"
      "Security-Policy\" content=\"default-src *\"><iframe srcdoc=\"";
  const auto doc = QString::fromUtf8(HtmlView::BuildDocument(hostile, session));

  const auto csp = doc.indexOf("Content-Security-Policy");
  const auto frame = doc.indexOf("<iframe");
  ASSERT_GE(csp, 0);
  EXPECT_LT(csp, doc.indexOf("</head>")) << "the policy is in <head>";
  EXPECT_LT(csp, frame) << "and before anything of the sender's";
  EXPECT_TRUE(doc.contains(
      QString("<iframe sandbox=\"%1\" ").arg(HtmlView::SandboxTokens())));
  for (const auto* token :
       {"allow-scripts", "allow-forms", "allow-same-origin", "allow-downloads",
        "allow-popups-to-escape-sandbox", "allow-top-navigation "}) {
    EXPECT_FALSE(HtmlView::SandboxTokens().contains(token)) << token;
  }
  EXPECT_TRUE(doc.contains("srcdoc=\"&lt;base target=&quot;_top&quot;&gt;"))
      << "links go to the top document, where the renderer sees them";
  EXPECT_EQ(doc.count("<iframe"), 1)
      << "the body opened no element: " << doc.toStdString();
  EXPECT_EQ(doc.count("<script"), 0);
  EXPECT_EQ(doc.count("</iframe>"), 1);
  EXPECT_TRUE(doc.contains("default-src 'none'"));
  EXPECT_FALSE(doc.contains("navigate-to")) << "not a directive any engine has";
}

TEST(HtmlViewDocumentTest, EscapingCoversEveryCharacterThatEndsAnAttribute) {
  EXPECT_EQ(HtmlView::EscapeAttribute("a&b\"c<d>e"),
            "a&amp;b&quot;c&lt;d&gt;e");
  EXPECT_EQ(HtmlView::EscapeAttribute(QString(QChar(0))),
            QString(QChar(QChar::ReplacementCharacter)));
  // Invalid UTF-8 is replaced on the way into a QString, never passed on.
  const auto doc = HtmlView::BuildDocument(
      QString::fromUtf8(QByteArray("ok\xff\xfe", 4)), HtmlView::NewSession());
  EXPECT_TRUE(QString::fromUtf8(doc).contains(QChar::ReplacementCharacter));
}

TEST(HtmlViewDocumentTest, CidReferencesAreRewrittenOnlyWhereImagesAre) {
  const QString s(32, u'a');
  const auto out = HtmlView::RewriteCidReferences(
      "<img src=\"cid:one@x\"><img src=cid:two@x>"
      "<td background='cid:three@x'>"
      "<img srcset=\"cid:a@x 1x, cid:b@x 2x\">"
      "<div style=\"background:url(cid:four@x)\">"
      "<a href=\"cid:link@x\">cid:text@x</a>",
      s);
  for (const auto* cid :
       {"one@x", "two@x", "three@x", "a@x", "b@x", "four@x"}) {
    EXPECT_TRUE(out.contains(HtmlView::PartUrl(s, cid)))
        << cid << " " << out.toStdString();
  }
  EXPECT_TRUE(out.contains("href=\"cid:link@x\"")) << "a link is not an image";
  EXPECT_TRUE(out.contains(">cid:text@x<")) << "and text is text";
}

TEST(HtmlViewDocumentTest, ContentIdsCompareTheWayRfc2392Says) {
  EXPECT_EQ(HtmlView::NormalizeCid("<part1@example.com>"), "part1@example.com");
  EXPECT_EQ(HtmlView::NormalizeCid("cid:part1%40example.com"),
            "part1@example.com");
  EXPECT_EQ(HtmlView::NormalizeCid("  "), "");
}

TEST(HtmlViewDocumentTest, OnlyPlainImagesArePartsAtAll) {
  EXPECT_TRUE(HtmlView::IsAllowedPartMime("image/png"));
  EXPECT_TRUE(HtmlView::IsAllowedPartMime("IMAGE/JPEG; name=x.jpg"));
  EXPECT_FALSE(HtmlView::IsAllowedPartMime("image/svg+xml"))
      << "SVG is a document";
  EXPECT_FALSE(HtmlView::IsAllowedPartMime("text/html"));
  EXPECT_FALSE(HtmlView::IsAllowedPartMime("font/woff2"));
  EXPECT_FALSE(HtmlView::IsAllowedPartMime(""));
}

// --- availability and default denial

TEST_F(HtmlViewTest, WithoutARendererEverythingAnswersUnavailable) {
  QWidget* slot = nullptr;
  const auto instance = make_instance(consumer_, kConsumer, &slot);
  ASSERT_NE(instance, 0U);
  OnGui([&] {
    EXPECT_EQ(GFHtmlViewAvailable(consumer_(), GF_HTML_VIEW_PRESET_MAIL),
              GF_HTML_VIEW_E_UNAVAILABLE);
    uint64_t view = 7;
    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), instance, slot, GF_HTML_VIEW_PRESET_MAIL,
                         &OnEvent, nullptr, &view),
        GF_HTML_VIEW_E_UNAVAILABLE);
    EXPECT_EQ(view, 0U);
  });
}

TEST_F(HtmlViewTest, TheTablesNeedTheirCapabilities) {
  SdkTestContext plain("com.example.html.plain",
                       GF_HOST_CAP_UI | GF_HOST_CAP_UI_CUSTOM);
  EXPECT_EQ(plain()->host->html_view, nullptr);
  EXPECT_EQ(plain()->host->html_provider, nullptr);
  EXPECT_NE(consumer_()->host->html_view, nullptr);
  EXPECT_EQ(consumer_()->host->html_provider, nullptr)
      << "embedding a view does not make a module a renderer";

  const auto spec = Spec(kEverything);
  OnGui([&] {
    EXPECT_EQ(GFHtmlViewAvailable(plain(), GF_HTML_VIEW_PRESET_MAIL),
              GF_HTML_VIEW_E_UNAVAILABLE);
    EXPECT_EQ(GFHtmlProviderRegister(consumer_(), &spec),
              GF_HTML_VIEW_E_DENIED);
  });
}

TEST_F(HtmlViewTest, ARendererThatCannotEnforceEveryBitIsNeverUsed) {
  register_fake(kEverything & ~GF_HTML_POLICY_NO_DNS_PREFETCH);
  QWidget* slot = nullptr;
  const auto instance = make_instance(consumer_, kConsumer, &slot);
  OnGui([&] {
    EXPECT_EQ(GFHtmlViewAvailable(consumer_(), GF_HTML_VIEW_PRESET_MAIL),
              GF_HTML_VIEW_E_POLICY);
    uint64_t view = 0;
    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), instance, slot, GF_HTML_VIEW_PRESET_MAIL,
                         &OnEvent, nullptr, &view),
        GF_HTML_VIEW_E_POLICY);
  });
  EXPECT_EQ(g_renderer.created, 0) << "not even asked";
}

TEST_F(HtmlViewTest, ARendererThatRefusesAtRunTimeCreatesNothing) {
  register_fake();
  g_renderer.refuse_create = true;
  QWidget* slot = nullptr;
  const auto instance = make_instance(consumer_, kConsumer, &slot);
  OnGui([&] {
    uint64_t view = 0;
    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), instance, slot, GF_HTML_VIEW_PRESET_MAIL,
                         &OnEvent, nullptr, &view),
        GF_HTML_VIEW_E_FAILED);
    EXPECT_EQ(HtmlViewService::Instance().ViewCount(), 0);
  });
}

TEST_F(HtmlViewTest, ARendererMayRegisterFromItsActivationThread) {
  // Modules activate on their runner, not the GUI thread; this test thread
  // stands in for it. The registration is accepted here and lands there.
  const auto spec = Spec(kEverything);
  ASSERT_EQ(GFHtmlProviderRegister(provider_(), &spec), GF_HTML_VIEW_OK);
  EXPECT_TRUE(Module::IsModuleLibraryPinned(kProvider))
      << "pinned at once, before any deactivation could race it";
  Settle();
  OnGui([&] {
    EXPECT_EQ(GFHtmlViewAvailable(consumer_(), GF_HTML_VIEW_PRESET_MAIL),
              GF_HTML_VIEW_OK);
  });
}

TEST_F(HtmlViewTest, AnUnknownInterfaceGenerationIsRefused) {
  auto spec = Spec(kEverything);
  spec.interface_version = GF_HTML_VIEW_PROVIDER_V1 + 1;
  OnGui([&] {
    EXPECT_EQ(GFHtmlProviderRegister(provider_(), &spec),
              GF_HTML_VIEW_E_INVALID);
  });
}

TEST_F(HtmlViewTest, ARendererThatNeedsARestartIsListedButNeverUsed) {
  register_fake(kEverything, GF_HTML_PROVIDER_FEATURE_RESTART_REQUIRED);
  QWidget* slot = nullptr;
  const auto instance = make_instance(consumer_, kConsumer, &slot);
  OnGui([&] {
    EXPECT_EQ(GFHtmlViewAvailable(consumer_(), GF_HTML_VIEW_PRESET_MAIL),
              GF_HTML_VIEW_E_RESTART_REQUIRED)
        << "never served on a scheme its engine does not know";
    uint64_t view = 0;
    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), instance, slot, GF_HTML_VIEW_PRESET_MAIL,
                         &OnEvent, nullptr, &view),
        GF_HTML_VIEW_E_RESTART_REQUIRED);
    EXPECT_EQ(GFHtmlProviderUnregister(provider_(), "fake"), GF_HTML_VIEW_OK);
  });
  EXPECT_EQ(g_renderer.created, 0);
  register_fake();
  OnGui([&] {
    EXPECT_EQ(GFHtmlViewAvailable(consumer_(), GF_HTML_VIEW_PRESET_MAIL),
              GF_HTML_VIEW_OK);
  });
}

// --- ownership

TEST_F(HtmlViewTest, AViewGoesOnlyIntoTheCallersOwnInstanceAndSlot) {
  register_fake();
  QWidget* slot = nullptr;
  const auto mine = make_instance(consumer_, kConsumer, &slot);
  QWidget* other_slot = nullptr;
  const auto theirs =
      make_instance(other_consumer_, kOtherConsumer, &other_slot);
  ASSERT_NE(mine, 0U);
  ASSERT_NE(theirs, 0U);

  OnGui([&] {
    uint64_t view = 0;
    // Someone else's instance, even with its real slot.
    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), theirs, other_slot,
                         GF_HTML_VIEW_PRESET_MAIL, &OnEvent, nullptr, &view),
        GF_HTML_VIEW_E_DENIED);
    // My instance, someone else's slot.
    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), mine, other_slot,
                         GF_HTML_VIEW_PRESET_MAIL, &OnEvent, nullptr, &view),
        GF_HTML_VIEW_E_DENIED);
    // A widget that is in nobody's instance.
    QWidget loose;
    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), mine, &loose, GF_HTML_VIEW_PRESET_MAIL,
                         &OnEvent, nullptr, &view),
        GF_HTML_VIEW_E_DENIED);
    // An address that is no widget at all: refused without being followed.
    int not_a_widget = 0;
    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), mine, &not_a_widget,
                         GF_HTML_VIEW_PRESET_MAIL, &OnEvent, nullptr, &view),
        GF_HTML_VIEW_E_DENIED);
    // An instance number nobody was given.
    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), mine + 1000, slot,
                         GF_HTML_VIEW_PRESET_MAIL, &OnEvent, nullptr, &view),
        GF_HTML_VIEW_E_DENIED);
    EXPECT_EQ(view, 0U);

    EXPECT_EQ(
        GFHtmlViewCreate(consumer_(), mine, slot, GF_HTML_VIEW_PRESET_MAIL,
                         &OnEvent, nullptr, &view),
        GF_HTML_VIEW_OK);
    EXPECT_NE(view, 0U);
  });
  EXPECT_EQ(g_renderer.created, 1);
}

TEST_F(HtmlViewTest, AViewAnswersOnlyToItsConsumerAndItsRenderer) {
  register_fake();
  const auto view = make_view();
  ASSERT_NE(view, 0U);
  ASSERT_EQ(load_body(view, "<p>hi</p>", {{"<p@x>", "image/png", "PNG"}}),
            GF_HTML_VIEW_OK);
  const auto session = HtmlViewService::Instance().CurrentSession(view);

  OnGui([&] {
    auto* body = GFBufferNewFromBytes(other_consumer_(), "x", 1);
    EXPECT_EQ(
        GFHtmlViewLoad(other_consumer_(), view, body, nullptr, 0, nullptr),
        GF_HTML_VIEW_E_DENIED);
    EXPECT_EQ(GFHtmlViewClear(other_consumer_(), view), GF_HTML_VIEW_E_DENIED);
    EXPECT_EQ(GFHtmlViewDestroy(other_consumer_(), view),
              GF_HTML_VIEW_E_DENIED);
    EXPECT_EQ(GFHtmlProviderLoaded(other_provider_(), view, 1),
              GF_HTML_VIEW_E_DENIED);
    EXPECT_EQ(GFHtmlProviderLinkActivated(other_provider_(), view,
                                          "https://example.com/"),
              GF_HTML_VIEW_E_DENIED);
  });
  EXPECT_EQ(request_part(view, session, "p@x", &other_provider_).first,
            GF_HTML_VIEW_E_DENIED)
      << "a renderer that does not serve the view gets none of its parts";
  EXPECT_EQ(HtmlViewService::Instance().ViewCount(), 1);
}

// --- the document and its session

TEST_F(HtmlViewTest, TheRendererGetsTheHostsDocumentAndASessionName) {
  register_fake();
  const auto view = make_view();
  size_t dropped = 99;
  ASSERT_EQ(load_body(view, "<img src=\"cid:p@x\">",
                      {{"<p@x>", "image/png", "PNG"},
                       {"<p@x>", "image/png", "again"},       // duplicate
                       {"<s@x>", "image/svg+xml", "<svg/>"},  // not an image
                       {"", "image/png", "nameless"}},
                      &dropped),
            GF_HTML_VIEW_OK);
  EXPECT_EQ(dropped, 3U);
  EXPECT_EQ(g_renderer.loaded, 1);
  EXPECT_TRUE(HtmlView::IsSessionName(g_renderer.last_session));
  EXPECT_EQ(g_renderer.last_session,
            HtmlViewService::Instance().CurrentSession(view));

  // What the renderer was handed is the Host's document, not the body.
  const auto doc = QString::fromUtf8(g_renderer.last_document);
  EXPECT_TRUE(doc.startsWith("<!DOCTYPE html>")) << doc.toStdString();
  EXPECT_TRUE(doc.contains(HtmlView::ContentSecurityPolicy()));
  EXPECT_TRUE(doc.contains(HtmlView::EscapeAttribute(
      HtmlView::PartUrl(g_renderer.last_session, "p@x"))))
      << "the reference points at this session: " << doc.toStdString();
}

TEST_F(HtmlViewTest, ABodyOverTheLimitIsRefused) {
  register_fake();
  const auto view = make_view();
  const auto limit =
      HtmlView::PolicyFor(GF_HTML_VIEW_PRESET_MAIL)->max_document_bytes;
  EXPECT_EQ(
      load_body(view, QByteArray(static_cast<qsizetype>(limit) + 1, 'a'), {}),
      GF_HTML_VIEW_E_TOO_LARGE);
  EXPECT_EQ(g_renderer.loaded, 0);
}

TEST_F(HtmlViewTest, APartIsServedOnlyForTheCurrentSessionOfItsOwnView) {
  register_fake();
  const auto view = make_view();
  const auto other = make_view();
  ASSERT_EQ(load_body(view, "a", {{"<p@x>", "image/png", "PNG"}}),
            GF_HTML_VIEW_OK);
  ASSERT_EQ(load_body(other, "b", {{"<q@x>", "image/png", "PNG"}}),
            GF_HTML_VIEW_OK);
  const auto session = HtmlViewService::Instance().CurrentSession(view);
  const auto other_session = HtmlViewService::Instance().CurrentSession(other);

  EXPECT_EQ(request_part(view, other_session, "q@x").first,
            GF_HTML_VIEW_E_DENIED)
      << "a session is never resolvable from another view";
  EXPECT_EQ(request_part(view, "", "p@x").first, GF_HTML_VIEW_E_DENIED);
  EXPECT_EQ(request_part(view, QString(32, u'0'), "p@x").first,
            GF_HTML_VIEW_E_DENIED);

  std::pair<int, uint64_t> ok;
  std::pair<int, uint64_t> missing;
  OnGui([&] {
    ok = ask(view, session, "cid:p%40x");
    missing = ask(view, session, "nope@x");
    EXPECT_TRUE(g_parts.isEmpty()) << "never answered inside the request";
  });
  ASSERT_EQ(ok.first, GF_HTML_VIEW_OK);
  ASSERT_EQ(missing.first, GF_HTML_VIEW_OK);
  Settle();
  ASSERT_EQ(g_parts.size(), 2);
  EXPECT_EQ(g_parts[0].request, ok.second);
  EXPECT_EQ(g_parts[0].status, GF_HTML_PART_OK);
  EXPECT_EQ(g_parts[0].mime, "image/png");
  EXPECT_TRUE(g_parts[0].had_data);
  EXPECT_EQ(g_parts[1].status, GF_HTML_PART_NOT_FOUND);
  EXPECT_FALSE(g_parts[1].had_data);
}

TEST_F(HtmlViewTest, ANewDocumentMakesEveryRequestOnTheOldOneStale) {
  register_fake();
  const auto view = make_view();
  ASSERT_EQ(load_body(view, "a", {{"<p@x>", "image/png", "PNG"}}),
            GF_HTML_VIEW_OK);
  const auto old_session = HtmlViewService::Instance().CurrentSession(view);
  // Replaced before the answer arrives.
  OnGui([&] {
    ASSERT_EQ(ask(view, old_session, "p@x").first, GF_HTML_VIEW_OK);
    ASSERT_EQ(load_now(view, "b", {{"<p@x>", "image/png", "PNG"}}),
              GF_HTML_VIEW_OK);
  });
  EXPECT_NE(HtmlViewService::Instance().CurrentSession(view), old_session);
  Settle();
  ASSERT_EQ(g_parts.size(), 1);
  EXPECT_EQ(g_parts[0].status, GF_HTML_PART_STALE);
  EXPECT_EQ(request_part(view, old_session, "p@x").first, GF_HTML_VIEW_E_DENIED)
      << "and the old name is refused from then on";

  // Clearing retires the session just the same.
  const auto current = HtmlViewService::Instance().CurrentSession(view);
  OnGui([&] {
    ASSERT_EQ(ask(view, current, "p@x").first, GF_HTML_VIEW_OK);
    EXPECT_EQ(GFHtmlViewClear(consumer_(), view), GF_HTML_VIEW_OK);
  });
  Settle();
  ASSERT_EQ(g_parts.size(), 2);
  EXPECT_EQ(g_parts[1].status, GF_HTML_PART_STALE);
  EXPECT_EQ(g_renderer.cleared, 1);
}

// --- cancellation

TEST_F(HtmlViewTest, ACancelledRequestIsNeverAnsweredAndCancelsOnce) {
  register_fake();
  const auto view = make_view();
  ASSERT_EQ(load_body(view, "a", {{"<p@x>", "image/png", "PNG"}}),
            GF_HTML_VIEW_OK);
  const auto session = HtmlViewService::Instance().CurrentSession(view);
  OnGui([&] {
    const auto request = ask(view, session, "p@x");
    ASSERT_EQ(request.first, GF_HTML_VIEW_OK);
    EXPECT_EQ(GFHtmlProviderCancelPart(other_provider_(), request.second),
              GF_HTML_VIEW_E_DENIED)
        << "only the renderer that asked may cancel";
    EXPECT_EQ(GFHtmlProviderCancelPart(provider_(), request.second),
              GF_HTML_VIEW_OK);
    EXPECT_EQ(GFHtmlProviderCancelPart(provider_(), request.second),
              GF_HTML_VIEW_E_INVALID);
  });
  Settle();
  EXPECT_TRUE(g_parts.isEmpty());
}

TEST_F(HtmlViewTest, DestroyingAViewDropsWhatItsRendererAskedFor) {
  register_fake();
  const auto view = make_view();
  ASSERT_EQ(load_body(view, "a", {{"<p@x>", "image/png", "PNG"}}),
            GF_HTML_VIEW_OK);
  const auto session = HtmlViewService::Instance().CurrentSession(view);
  OnGui([&] {
    ASSERT_EQ(ask(view, session, "p@x").first, GF_HTML_VIEW_OK);
    EXPECT_EQ(GFHtmlViewDestroy(consumer_(), view), GF_HTML_VIEW_OK);
  });
  Settle();
  EXPECT_TRUE(g_parts.isEmpty()) << "done never runs once the view is gone";
  EXPECT_EQ(g_renderer.destroyed, 1);
  EXPECT_EQ(HtmlViewService::Instance().PendingPartCount(), 0);
  EXPECT_EQ(g_events.lost, 0) << "the consumer asked; it is not told";
}

// --- links

TEST_F(HtmlViewTest, ALinkIsTheHostsToOpenAndTheConsumersToHearAbout) {
  register_fake();
  const auto view = make_view();
  OnGui([&] {
    EXPECT_EQ(GFHtmlProviderLinkActivated(provider_(), view,
                                          "https://example.com/a?b=c"),
              GF_HTML_VIEW_OK);
    EXPECT_EQ(GFHtmlProviderLinkActivated(provider_(), view, "not a url"),
              GF_HTML_VIEW_E_INVALID);
    EXPECT_EQ(GFHtmlProviderLoaded(provider_(), view, 1), GF_HTML_VIEW_OK);
  });
  Settle();
  EXPECT_EQ(g_events.links, QStringList{"https://example.com/a?b=c"});
  EXPECT_EQ(opened_, QStringList{"https://example.com/a?b=c"})
      << "the Host's handler decided, after the renderer returned";
  EXPECT_EQ(g_events.loaded, 1);
}

// --- shutdown

TEST_F(HtmlViewTest, AWithdrawnRendererIsDrainedInOrderAndStaysMapped) {
  register_fake();
  const auto view = make_view();
  ASSERT_EQ(load_body(view, "a", {{"<p@x>", "image/png", "PNG"}}),
            GF_HTML_VIEW_OK);
  const auto session = HtmlViewService::Instance().CurrentSession(view);
  OnGui([&] {
    ASSERT_EQ(ask(view, session, "p@x").first, GF_HTML_VIEW_OK);
    HtmlViewService::Instance().WithdrawModule(kProvider);
  });
  Settle();

  EXPECT_TRUE(g_parts.isEmpty()) << "pending work is dropped, not answered";
  EXPECT_EQ(g_events.lost, 1) << "the consumer falls back";
  EXPECT_EQ(g_renderer.destroyed, 1);
  EXPECT_EQ(HtmlViewService::Instance().ViewCount(), 0);
  EXPECT_EQ(HtmlViewService::Instance().PendingPartCount(), 0);
  EXPECT_TRUE(Module::IsModuleLibraryPinned(kProvider))
      << "an engine is never unmapped from a live process";
  OnGui([&] {
    EXPECT_EQ(GFHtmlViewAvailable(consumer_(), GF_HTML_VIEW_PRESET_MAIL),
              GF_HTML_VIEW_E_UNAVAILABLE);
  });
}

TEST_F(HtmlViewTest, AnInstanceThatGoesTakesItsViewsWithIt) {
  register_fake();
  QWidget* slot = nullptr;
  const auto instance = make_instance(consumer_, kConsumer, &slot);
  OnGui([&] {
    uint64_t view = 0;
    ASSERT_EQ(
        GFHtmlViewCreate(consumer_(), instance, slot, GF_HTML_VIEW_PRESET_MAIL,
                         &OnEvent, nullptr, &view),
        GF_HTML_VIEW_OK);
    UI::NativeInstances::Instance().Destroy(instance);
    EXPECT_EQ(HtmlViewService::Instance().ViewCount(), 0);
  });
  EXPECT_EQ(g_renderer.destroyed, 1);
}

#ifdef Q_OS_MACOS
TEST(HtmlViewPlatformTest, TheHostsOwnWebKitRendererServesTheMailPreset) {
  // Built in, with nothing to install: once the UI is up, mail has a
  // renderer that enforces every bit the preset asks for.
  ASSERT_TRUE(WaitForHostRenderer()) << "WebKit's content rules never compiled";
  int available = HtmlViewService::kFAILED;
  OnGui([&] {
    available = HtmlViewService::Instance().Available(GF_HTML_VIEW_PRESET_MAIL);
  });
  EXPECT_EQ(available, HtmlViewService::kOK);
}
#endif

}  // namespace GpgFrontend::Test
