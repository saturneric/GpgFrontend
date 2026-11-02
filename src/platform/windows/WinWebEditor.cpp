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

/**
 * @file WinWebEditor.cpp
 * @brief The Host's own web editor engine on Windows: WebView2.
 *
 * The same shape as the macOS engine (MacWebEditor.mm), with WebView2's
 * mechanisms where that one has WebKit's; the COM plumbing it shares with
 * the HTML view renderer is in WinWebView2.h.
 *
 * It runs a browser process of its own, apart from the HTML view's: the two
 * register different schemes, and an environment's options are fixed per
 * user data folder. Like that one, it starts only when an editor is first
 * created, and is never registered without the loader or a runtime new
 * enough for custom schemes.
 *
 * One thing WebKit refuses that WebView2 offers no hook for: a file picker
 * opened by an <input type="file">. Only the module's own page runs here,
 * and what the user picks reaches nothing but that page.
 */

#include "GpgFrontend.h"

#if defined(Q_OS_WINDOWS) && defined(GF_HAS_WEBVIEW2_SDK)

#include <QBackingStore>
#include <QGuiApplication>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWindow>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "platform/common/PlatformWebEditor.h"
#include "platform/windows/WinWebView2.h"
#include "ui/html/WebEditorDocument.h"
#include "ui/html/WebEditorService.h"

// clang-format off
#include <shlwapi.h>
// clang-format on

namespace WebEditor = GpgFrontend::UI::WebEditor;
using GpgFrontend::UI::WebEditorService;
using namespace GpgFrontend::UI::WebView2;

namespace {

/// The Host's own engine, as the service knows it: the empty owner.
const QString kOwner;

/// `window.gfBridge` ({post(obj), onMessage(fn)}), as on every engine, over
/// WebView2's own channel. Injected by the engine, not served: the page's
/// policy does not apply to it, and the page cannot replace it. Frames are
/// never loaded, but the script would run in one, so it keeps to the top.
constexpr auto kBridgeScript = LR"JS(
(function () {
  "use strict";
  if (window !== window.top) return;
  var webview = window.chrome && window.chrome.webview;
  if (!webview) return;
  var send = webview.postMessage.bind(webview);
  var listeners = [];
  function fixed(name, value) {
    Object.defineProperty(window, name, {
      configurable: false, enumerable: false, writable: false, value: value
    });
  }
  fixed("gfBridge", Object.freeze({
    post: function (obj) { send(JSON.stringify(obj)); },
    onMessage: function (fn) { if (typeof fn === "function") listeners.push(fn); }
  }));
  webview.addEventListener("message", function (event) {
    if (typeof event.data !== "string") return;
    var obj;
    try { obj = JSON.parse(event.data); } catch (e) { return; }
    listeners.forEach(function (fn) { fn(obj); });
  });
})();
)JS";

/// What the Host says when the page's process went away.
constexpr auto kCrashed = R"({"type":"engine.crashed"})";

/// Hands keyboard focus on to the engine's own HWND when Qt gives it to the
/// window the engine hangs off -- tabbing into the editor, say.
class FocusForwarder : public QObject {
 public:
  FocusForwarder(QWindow* window, std::function<void()> forward)
      : QObject(window), forward_(std::move(forward)) {
    window->installEventFilter(this);
  }

 protected:
  auto eventFilter(QObject* watched, QEvent* event) -> bool override {
    if (event->type() == QEvent::FocusIn) forward_();
    return QObject::eventFilter(watched, event);
  }

 private:
  std::function<void()> forward_;
};

/// What an editor shows where its page is not yet: the colour a text field
/// is, never WebView2's white or a window nothing has painted.
auto BackdropColor() -> QColor {
  return QGuiApplication::palette().color(QPalette::Base);
}

/// The window the engine hangs off, painting the backdrop itself until the
/// engine's own window covers it.
class BackdropWindow : public QWindow {
 public:
  BackdropWindow() : store_(this) {}

 protected:
  void exposeEvent(QExposeEvent*) override { Paint(); }

  void resizeEvent(QResizeEvent* event) override {
    store_.resize(event->size());
    Paint();
  }

 private:
  void Paint() {
    if (!isExposed()) return;
    const QRect rect(QPoint(), size());
    store_.beginPaint(rect);
    QPainter painter(store_.paintDevice());
    painter.fillRect(rect, BackdropColor());
    painter.end();
    store_.endPaint();
    store_.flush(rect);
  }

  QBackingStore store_;
};

}  // namespace

namespace GpgFrontend::UI {

namespace {

/**
 * @brief One editor's engine side: the two schemes, the bridge, and every
 *        question WebView2 asks it.
 *
 * Everything here runs on the GUI thread, where the engine delivers its
 * events. The controller arrives asynchronously; the page is loaded, and
 * whatever the consumer posts is delivered, once it has.
 */
class EditorHost : public std::enable_shared_from_this<EditorHost> {
 public:
  EditorHost(quint64 editor, QString session, QWindow* window)
      : editor_(editor),
        session_(std::move(session)),
        index_(QString("%1://%2/index.html")
                   .arg(QLatin1StringView(WebEditor::kAssetScheme), session_)),
        window_(window) {}

  ~EditorHost() {
    for (auto& json : outbox_) Wipe(json);
  }

  /// The widget side: a QWindow the engine's HWND tree hangs off, inside a
  /// container the service may reparent freely.
  auto MakeWidget() -> QWidget* {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(QWidget::createWindowContainer(window_, widget));
    const auto self = weak_from_this();
    const auto follow = [self] {
      if (const auto host = self.lock()) host->UpdateBounds();
    };
    QObject::connect(window_, &QWindow::widthChanged, window_, follow);
    QObject::connect(window_, &QWindow::heightChanged, window_, follow);
    QObject::connect(window_, &QWindow::screenChanged, window_, follow);
    QObject::connect(window_, &QWindow::visibleChanged, window_, [self] {
      if (const auto host = self.lock()) host->ApplyVisibility();
    });
    new FocusForwarder(window_, [self] {
      const auto host = self.lock();
      if (host && host->controller_) {
        host->controller_->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
      }
    });
    return widget;
  }

  /// Told when the browser process itself is gone, which ends every editor
  /// it had, not only this one.
  void OnBrowserGone(std::function<void()> gone) {
    browser_gone_ = std::move(gone);
  }

  /// The one environment, kept by every host so responses can be minted.
  void KeepEnvironment(ICoreWebView2Environment* environment) {
    environment_.Keep(environment);
  }

  void StartController(ICoreWebView2Environment10* env) {
    Ptr<ICoreWebView2ControllerOptions> options;
    auto hr = env->CreateCoreWebView2ControllerOptions(options.Put());
    // Nothing written to disk, nothing shared with any other profile.
    if (SUCCEEDED(hr)) hr = options->put_IsInPrivateModeEnabled(TRUE);
    const auto hwnd = reinterpret_cast<HWND>(window_->winId());
    const auto self = weak_from_this();
    if (SUCCEEDED(hr)) {
      hr = env->CreateCoreWebView2ControllerWithOptions(
          hwnd, options.Get(),
          new Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
              [self](HRESULT result, ICoreWebView2Controller* controller) {
                if (const auto host = self.lock()) {
                  host->OnController(result, controller);
                }
                return S_OK;
              }));
    }
    if (FAILED(hr)) OnController(hr, nullptr);
  }

  /// (Re)load the editor's page, its one allowed main-frame load.
  void LoadPage() {
    if (detached_) return;
    ready_ = false;
    ApplyVisibility();
    expected_main_ = index_;
    awaiting_load_ = true;
    have_expected_navigation_ = false;
    load_pending_ = true;
    NavigateIfReady();
  }

  /// Deliver one JSON object, or keep it until the page is ready for it.
  void Post(const QByteArray& json) {
    if (detached_) return;
    if (!ready_) {
      outbox_.append(json);
      return;
    }
    // A string the page parses itself: nothing in it is ever run as script.
    const auto wide = QString::fromUtf8(json).toStdWString();
    webview_->PostWebMessageAsString(wide.c_str());
  }

  /// The editor is gone: nothing it still hears reaches the service.
  void Detach() {
    detached_ = true;
    ready_ = false;
    awaiting_load_ = false;
    have_expected_navigation_ = false;
    load_pending_ = false;
    expected_main_.clear();
    for (auto& json : outbox_) Wipe(json);
    outbox_.clear();
    if (webview_) webview_->Stop();
    if (controller_) controller_->Close();
    webview_.Drop();
    controller_.Drop();
  }

 private:
  /// The engine is seen only once its page is ready: until then, and while
  /// it reloads, the backdrop shows instead of a page half drawn.
  void ApplyVisibility() {
    if (!controller_) return;
    const bool visible = ready_ && !window_.isNull() && window_->isVisible();
    controller_->put_IsVisible(visible ? TRUE : FALSE);
  }

  /// The page loads once the engine is driven and the bridge is in place;
  /// a load asked for before then waits.
  void NavigateIfReady() {
    if (!load_pending_ || !webview_ || !bridge_ready_) return;
    load_pending_ = false;
    const auto wide = index_.toStdWString();
    webview_->Navigate(wide.c_str());
  }

  void UpdateBounds() {
    if (!controller_ || window_.isNull()) return;
    RECT bounds{};
    GetClientRect(reinterpret_cast<HWND>(window_->winId()), &bounds);
    controller_->put_Bounds(bounds);
  }

  void OnController(HRESULT result, ICoreWebView2Controller* controller) {
    if (detached_) {
      if (controller != nullptr) controller->Close();
      return;
    }
    Ptr<ICoreWebView2> webview;
    if (SUCCEEDED(result) && controller != nullptr) {
      result = controller->get_CoreWebView2(webview.Put());
    }
    if (FAILED(result) || !webview) {
      // An engine that cannot be driven cannot load the page, and the
      // editor never becomes ready. Under gf_test's offscreen platform there
      // is no real HWND, so this is also the normal path there.
      LOG_W() << "web editor" << editor_
              << "got no WebView2 controller:" << QString::number(result, 16);
      if (controller != nullptr) controller->Close();
      awaiting_load_ = false;
      load_pending_ = false;
      return;
    }
    controller_.Keep(controller);
    webview_.Keep(webview.Get());
    // What shows before the page paints its own background is the backdrop,
    // not white.
    Ptr<ICoreWebView2Controller2> controller2;
    controller->QueryInterface(kIidController2,
                               reinterpret_cast<void**>(controller2.Put()));
    if (controller2) {
      const auto color = BackdropColor();
      controller2->put_DefaultBackgroundColor(
          {255, static_cast<BYTE>(color.red()), static_cast<BYTE>(color.green()),
           static_cast<BYTE>(color.blue())});
    }
    LockDown();
    Police();
    UpdateBounds();
    ApplyVisibility();

    // The bridge must be there before the page's first script runs, and
    // adding it completes asynchronously.
    const auto self = weak_from_this();
    const auto hr = webview_->AddScriptToExecuteOnDocumentCreated(
        kBridgeScript,
        new Callback<
            ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
            [self](HRESULT added, LPCWSTR) {
              const auto host = self.lock();
              if (!host || host->detached_) return S_OK;
              if (FAILED(added)) {
                host->BridgeFailed(added);
                return S_OK;
              }
              host->bridge_ready_ = true;
              host->NavigateIfReady();
              return S_OK;
            }));
    if (FAILED(hr)) BridgeFailed(hr);
  }

  void BridgeFailed(HRESULT result) {
    LOG_W() << "web editor" << editor_ << "could not add its bridge:"
            << QString::number(result, 16);
    awaiting_load_ = false;
    load_pending_ = false;
  }

  /// Off in the engine's own settings: everything but the page's own script,
  /// its one channel to the Host, and the editing menu.
  void LockDown() {
    Ptr<ICoreWebView2Settings> s;
    if (FAILED(webview_->get_Settings(s.Put())) || !s) return;
    s->put_IsScriptEnabled(TRUE);
    s->put_IsWebMessageEnabled(TRUE);
    s->put_AreDefaultScriptDialogsEnabled(FALSE);
    // Cut, copy, paste and spelling; with DevTools off, nothing to inspect.
    s->put_AreDefaultContextMenusEnabled(TRUE);
    LockDownCommon(s.Get());
  }

  /// The event hooks: every request answered here, every navigation judged,
  /// every window, download and permission refused, and the bridge heard.
  void Police() {
    const auto self = weak_from_this();
    EventRegistrationToken token{};

    // Every URL, every context: no request bypasses OnResource, so nothing
    // the Host does not serve ever leaves the machine.
    webview_->AddWebResourceRequestedFilter(
        L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
    webview_->add_WebResourceRequested(
        new Callback<ICoreWebView2WebResourceRequestedEventHandler>(
            [self](ICoreWebView2*,
                   ICoreWebView2WebResourceRequestedEventArgs* args) {
              if (const auto host = self.lock()) host->OnResource(args);
              return S_OK;
            }),
        &token);

    webview_->add_WebMessageReceived(
        new Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [self](ICoreWebView2*,
                   ICoreWebView2WebMessageReceivedEventArgs* args) {
              if (const auto host = self.lock()) host->OnMessage(args);
              return S_OK;
            }),
        &token);

    webview_->add_NavigationStarting(
        new Callback<ICoreWebView2NavigationStartingEventHandler>(
            [self](ICoreWebView2*,
                   ICoreWebView2NavigationStartingEventArgs* args) {
              if (const auto host = self.lock()) {
                host->OnNavigation(args, true);
              }
              return S_OK;
            }),
        &token);
    webview_->add_FrameNavigationStarting(
        new Callback<ICoreWebView2NavigationStartingEventHandler>(
            [self](ICoreWebView2*,
                   ICoreWebView2NavigationStartingEventArgs* args) {
              if (const auto host = self.lock()) {
                host->OnNavigation(args, false);
              }
              return S_OK;
            }),
        &token);
    webview_->add_NavigationCompleted(
        new Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [self](ICoreWebView2*,
                   ICoreWebView2NavigationCompletedEventArgs* args) {
              if (const auto host = self.lock()) host->OnCompleted(args);
              return S_OK;
            }),
        &token);
    // A popup is only ever a way for a click to reach the Host; no window
    // is made.
    webview_->add_NewWindowRequested(
        new Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [self](ICoreWebView2*,
                   ICoreWebView2NewWindowRequestedEventArgs* args) {
              args->put_Handled(TRUE);
              BOOL user = FALSE;
              args->get_IsUserInitiated(&user);
              LPWSTR uri = nullptr;
              args->get_Uri(&uri);
              const auto url = TakeCoString(uri);
              const auto host = self.lock();
              if (host && user == TRUE) host->ReportLink(url);
              return S_OK;
            }),
        &token);
    webview_->add_PermissionRequested(
        new Callback<ICoreWebView2PermissionRequestedEventHandler>(
            [](ICoreWebView2*, ICoreWebView2PermissionRequestedEventArgs* args) {
              return args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
            }),
        &token);
    webview_->add_ProcessFailed(
        new Callback<ICoreWebView2ProcessFailedEventHandler>(
            [self](ICoreWebView2*, ICoreWebView2ProcessFailedEventArgs* args) {
              if (const auto host = self.lock()) host->OnProcessFailed(args);
              return S_OK;
            }),
        &token);

    Ptr<ICoreWebView2_4> webview4;
    webview_.Get()->QueryInterface(kIidWebView2_4,
                                   reinterpret_cast<void**>(webview4.Put()));
    if (webview4) {
      webview4->add_DownloadStarting(
          new Callback<ICoreWebView2DownloadStartingEventHandler>(
              [](ICoreWebView2*, ICoreWebView2DownloadStartingEventArgs* args) {
                args->put_Cancel(TRUE);
                return args->put_Handled(TRUE);
              }),
          &token);
    }
    // A mailto: or other external protocol is a navigation the engine would
    // otherwise hand to the OS behind its own dialog. It is a link like any
    // other: the Host's to confirm and open.
    Ptr<ICoreWebView2_18> webview18;
    webview_.Get()->QueryInterface(kIidWebView2_18,
                                   reinterpret_cast<void**>(webview18.Put()));
    if (webview18) {
      webview18->add_LaunchingExternalUriScheme(
          new Callback<ICoreWebView2LaunchingExternalUriSchemeEventHandler>(
              [self](ICoreWebView2*,
                     ICoreWebView2LaunchingExternalUriSchemeEventArgs* args) {
                args->put_Cancel(TRUE);
                LPWSTR uri = nullptr;
                args->get_Uri(&uri);
                const auto url = TakeCoString(uri);
                BOOL user = FALSE;
                args->get_IsUserInitiated(&user);
                const auto host = self.lock();
                if (host && user == TRUE) host->ReportLink(url);
                return S_OK;
              }),
          &token);
    }
  }

  // --- the bridge: one string from the page's own document ------------------

  void OnMessage(ICoreWebView2WebMessageReceivedEventArgs* args) {
    // Raised for the top-level document only; frames have their own event,
    // which nothing here listens to.
    if (detached_) return;
    LPWSTR text = nullptr;
    args->get_Source(&text);
    const QUrl source(TakeCoString(text));
    if (source.scheme() != QLatin1StringView(WebEditor::kAssetScheme) ||
        source.host() != session_) {
      return;
    }
    text = nullptr;
    if (FAILED(args->TryGetWebMessageAsString(&text)) || text == nullptr) {
      return;
    }
    // Checked by the service; its content is never logged.
    WebEditorService::Instance().Message(kOwner, editor_,
                                         TakeCoString(text).toUtf8());
  }

  // --- navigation: the page itself, once; nothing else ----------------------

  auto TakeExpectedMainLoad(const QString& url) -> bool {
    if (expected_main_.isEmpty() || url != expected_main_) return false;
    expected_main_.clear();
    return true;
  }

  void OnNavigation(ICoreWebView2NavigationStartingEventArgs* args,
                    bool main_frame) {
    LPWSTR uri = nullptr;
    args->get_Uri(&uri);
    const auto url = TakeCoString(uri);

    if (detached_) {
      args->put_Cancel(TRUE);
      return;
    }
    // Script runs in the module's own page, under the Host's policy, and in
    // nothing else: every other document is refused before it loads.
    if (main_frame && TakeExpectedMainLoad(url)) {
      args->get_NavigationId(&expected_navigation_);
      have_expected_navigation_ = true;
      return;
    }
    // A link the user activated is the Host's to open, outside the editor.
    // Everything else is simply refused.
    BOOL user = FALSE;
    args->get_IsUserInitiated(&user);
    if (user == TRUE) {
      ReportLink(url);
    } else {
      LOG_D() << "web editor" << editor_ << "refused navigation,"
              << (main_frame ? "main" : "sub") << "frame";
    }
    args->put_Cancel(TRUE);
  }

  // Every refused navigation ends in a completion too; only the page's own
  // is the editor loading.
  void OnCompleted(ICoreWebView2NavigationCompletedEventArgs* args) {
    if (detached_ || !awaiting_load_ || !have_expected_navigation_) return;
    UINT64 navigation = 0;
    args->get_NavigationId(&navigation);
    if (navigation != expected_navigation_) return;
    have_expected_navigation_ = false;
    awaiting_load_ = false;
    BOOL ok = FALSE;
    args->get_IsSuccess(&ok);
    if (ok != TRUE) {
      LOG_W() << "web editor" << editor_ << "could not load its page";
      return;
    }
    ready_ = true;
    // What was posted early, in order, before anything the consumer posts on
    // hearing that the page is ready.
    auto early = std::exchange(outbox_, {});
    for (auto& json : early) {
      Post(json);
      Wipe(json);
    }
    WebEditorService::Instance().Ready(kOwner, editor_);
    // Shown only now: what the consumer posts on hearing it -- its theme,
    // say -- is already on its way to the page.
    ApplyVisibility();
  }

  // A crashed page process took the document with it: say so, then start
  // again. A lost browser process took every editor with it.
  void OnProcessFailed(ICoreWebView2ProcessFailedEventArgs* args) {
    COREWEBVIEW2_PROCESS_FAILED_KIND kind{};
    args->get_ProcessFailedKind(&kind);
    if (detached_) return;
    if (kind == COREWEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED) {
      LOG_W() << "web editor" << editor_ << "lost the WebView2 browser process";
      if (browser_gone_) browser_gone_();
      return;
    }
    if (kind != COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_EXITED) return;
    LOG_W() << "web editor" << editor_ << "lost its web content process";
    ready_ = false;
    ApplyVisibility();
    WebEditorService::Instance().Message(kOwner, editor_, kCrashed);
    // Hearing it, the consumer may have destroyed the editor.
    LoadPage();
  }

  void ReportLink(const QString& url) {
    if (detached_ || url.isEmpty()) return;
    WebEditorService::Instance().LinkActivated(kOwner, editor_, url);
  }

  // --- gfasset:// and gfres:// -- whatever the Host serves; every other
  // request is answered with a refusal, so nothing reaches a network --------

  void Answer(ICoreWebView2WebResourceRequestedEventArgs* args,
              IStream* stream, int status, const wchar_t* reason,
              const QString& headers) {
    Ptr<ICoreWebView2WebResourceResponse> response;
    const auto wide = headers.toStdWString();
    if (!environment_ ||
        FAILED(environment_->CreateWebResourceResponse(
            stream, status, reason, wide.c_str(), response.Put()))) {
      return;
    }
    args->put_Response(response.Get());
  }

  void Refuse(ICoreWebView2WebResourceRequestedEventArgs* args) {
    Answer(args, nullptr, 403, L"Blocked", {});
  }

  void OnResource(ICoreWebView2WebResourceRequestedEventArgs* args) {
    Ptr<ICoreWebView2WebResourceRequest> request;
    if (FAILED(args->get_Request(request.Put())) || !request) return;
    LPWSTR text = nullptr;
    request->get_Uri(&text);
    const auto url = TakeCoString(text);
    request->get_Method(&text);
    const auto method = TakeCoString(text);

    if (detached_ || method.compare(u"GET"_s, Qt::CaseInsensitive) != 0) {
      Refuse(args);
      return;
    }
    // Anything but the two schemes, or another session's, is not found.
    GpgFrontend::GFBuffer data;
    QString mime;
    const auto status = WebEditorService::Instance().Resolve(
        kOwner, editor_, url, &data, &mime);
    if (status != WebEditorService::kOK) {
      Refuse(args);
      return;
    }
    IStream* stream =
        SHCreateMemStream(reinterpret_cast<const BYTE*>(data.Data()),
                          static_cast<UINT>(data.Size()));
    if (stream == nullptr) {
      Refuse(args);
      return;
    }
    auto headers =
        u"Content-Type: "_s +
        (mime.startsWith(u"text/"_s) ? mime + u"; charset=utf-8"_s : mime) +
        u"\nCache-Control: no-store"_s;
    // The page carries the policy already; a header says it again, where the
    // page has no say at all.
    if (mime == u"text/html"_s) {
      headers += u"\nContent-Security-Policy: "_s +
                 WebEditor::ContentSecurityPolicy();
    }
    Answer(args, stream, 200, L"OK", headers);
    stream->Release();
  }

  quint64 editor_;
  QString session_;
  QString index_;
  QPointer<QWindow> window_;
  std::function<void()> browser_gone_;
  Ptr<ICoreWebView2Environment> environment_;
  Ptr<ICoreWebView2Controller> controller_;
  Ptr<ICoreWebView2> webview_;
  /// The one main-frame load allowed next, taken when it is asked about.
  QString expected_main_;
  /// The page's own navigation, the only one Ready() is about.
  UINT64 expected_navigation_ = 0;
  bool have_expected_navigation_ = false;
  bool awaiting_load_ = false;
  /// A load asked for before the engine could start it.
  bool load_pending_ = false;
  bool bridge_ready_ = false;
  bool ready_ = false;
  bool detached_ = false;
  /// What the consumer posted before the page could hear it.
  QList<QByteArray> outbox_;
};

class WinWebEditorEngine : public WebEditorEngine {
 public:
  auto Create(quint64 editor, const QString& session) -> QWidget* override {
    if (environment_failed_) return nullptr;
    auto* window = new BackdropWindow();
    const auto host = std::make_shared<EditorHost>(editor, session, window);
    auto* widget = host->MakeWidget();
    host->OnBrowserGone([this] { BrowserGone(); });
    editors_[editor] = host;
    host->LoadPage();
    if (environment10_) {
      host->KeepEnvironment(environment_.Get());
      host->StartController(environment10_.Get());
    } else {
      waiting_.push_back(host);
      EnsureEnvironment();
    }
    return widget;
  }

  auto Post(quint64 editor, const QByteArray& json) -> bool override {
    const auto it = editors_.find(editor);
    if (it == editors_.end()) return false;
    it->second->Post(json);
    return true;
  }

  void Destroyed(quint64 editor) override {
    const auto it = editors_.find(editor);
    if (it == editors_.end()) return;
    it->second->Detach();
    editors_.erase(it);
  }

 private:
  /// The engine's browser process is not started for an app that never
  /// opens an editor: the first editor asks for the environment,
  /// asynchronously, and every editor created in the meantime waits with it.
  void EnsureEnvironment() {
    if (creating_) return;
    creating_ = true;
    CreateEnvironment(
        u"GpgFrontend.WebEditor"_s,
        {QString(QLatin1StringView(WebEditor::kAssetScheme)).toStdWString(),
         QString(QLatin1StringView(WebEditor::kResourceScheme)).toStdWString()},
        [this](HRESULT result, ICoreWebView2Environment* environment) {
          OnEnvironment(result, environment);
        });
  }

  void OnEnvironment(HRESULT result, ICoreWebView2Environment* environment) {
    Ptr<ICoreWebView2Environment10> env10;
    if (SUCCEEDED(result) && environment != nullptr) {
      environment->QueryInterface(kIidEnvironment10,
                                  reinterpret_cast<void**>(env10.Put()));
    }
    creating_ = false;
    if (!env10) {
      // No engine after all: the provider withdraws itself, and every editor
      // it was asked for is destroyed with LOST. Queued: this may be inside
      // Create, whose editor the service has yet to mount.
      LOG_W() << "the WebView2 environment could not be created:"
              << QString::number(result, 16);
      environment_failed_ = true;
      waiting_.clear();
      QTimer::singleShot(0, &WebEditorService::Instance(), [] {
        WebEditorService::Instance().UnregisterProvider(
            kOwner, QString::fromLatin1(kPlatformWebEditorEngineName));
      });
      return;
    }
    environment_.Keep(environment);
    environment10_.Keep(env10.Get());
    const auto waiting = std::move(waiting_);
    waiting_.clear();
    for (const auto& weak : waiting) {
      if (const auto host = weak.lock()) {
        host->KeepEnvironment(environment_.Get());
        host->StartController(environment10_.Get());
      }
    }
  }

  /// Every editor heard it; the first is enough. Their editors are destroyed
  /// with LOST, the dead environment is let go, and the engine is back at
  /// once for the next editor, which starts a fresh one.
  void BrowserGone() {
    if (browser_gone_) return;
    browser_gone_ = true;
    QTimer::singleShot(0, &WebEditorService::Instance(), [this] {
      const auto name = QString::fromLatin1(kPlatformWebEditorEngineName);
      WebEditorService::Instance().UnregisterProvider(kOwner, name);
      environment10_.Drop();
      environment_.Drop();
      browser_gone_ = false;
      RegisterPlatformWebEditorEngine();
    });
  }

  std::map<quint64, std::shared_ptr<EditorHost>> editors_;
  std::vector<std::weak_ptr<EditorHost>> waiting_;
  Ptr<ICoreWebView2Environment> environment_;
  Ptr<ICoreWebView2Environment10> environment10_;
  bool creating_ = false;
  bool environment_failed_ = false;
  bool browser_gone_ = false;
};

auto EnginePtr() -> std::shared_ptr<WinWebEditorEngine> {
  // One engine for the process: re-registering after a test detached it
  // gives back the same one, whose editors the drain has already destroyed.
  static const auto engine = std::make_shared<WinWebEditorEngine>();
  return engine;
}

}  // namespace

void RegisterPlatformWebEditorEngine() {
  // Whether the machine can run the engine is known without starting
  // anything; the browser process itself starts only when an editor is
  // first created.
  if (!RuntimeUsable()) {
    LOG_I() << "no built-in web editor engine";
    return;
  }

  WebEditorProviderInfo info;
  info.owner = kOwner;
  info.name = QString::fromLatin1(kPlatformWebEditorEngineName);
  info.priority = WebEditorService::kBuiltInPriority;
  info.restart_required = false;
  info.engine = EnginePtr();
  WebEditorService::Instance().RegisterProvider(std::move(info));
}

}  // namespace GpgFrontend::UI

#endif
