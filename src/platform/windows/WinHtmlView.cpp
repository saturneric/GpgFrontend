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
 * @file WinHtmlView.cpp
 * @brief The Host's own HTML view renderer on Windows: WebView2.
 *
 * The same shape as the macOS renderer (MacHtmlView.mm), with WebView2's
 * mechanisms where that one has WebKit's. The COM plumbing it shares with
 * the web editor engine is in WinWebView2.h.
 *
 * WebView2Loader.dll is resolved at run time. No loader, no Evergreen
 * runtime, or a runtime too old to register the part scheme: the renderer
 * is simply never registered, and HTML mail stays source.
 */

#include "GpgFrontend.h"

#if defined(Q_OS_WINDOWS) && defined(GF_HAS_WEBVIEW2_SDK)

#include <QPointer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWindow>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "platform/common/PlatformHtmlView.h"
#include "platform/windows/WinWebView2.h"
#include "ui/html/HtmlViewService.h"

// clang-format off
#include <shlwapi.h>
// clang-format on

namespace HtmlView = GpgFrontend::UI::HtmlView;
using GpgFrontend::UI::HtmlViewService;
using namespace GpgFrontend::UI::WebView2;

namespace {

/// The Host's own renderer, as the service knows it: the empty owner.
const QString kOwner;

/// Every bit, each enforced by WebView2 itself; how, is next to where.
/// NO_PLUGINS rests on two engine facts: the sandboxed srcdoc frame may not
/// instantiate plugin content at all, and plugin-shaped parts never get
/// bytes, because every request is answered by this file and parts are
/// images only.
constexpr uint32_t kEnforceable =
    HtmlView::kNO_NETWORK | HtmlView::kPARTS_ONLY | HtmlView::kNO_SCRIPT |
    HtmlView::kNO_NAVIGATION | HtmlView::kNO_POPUPS | HtmlView::kNO_DOWNLOADS |
    HtmlView::kNO_PERSISTENCE | HtmlView::kNO_PERMISSIONS |
    HtmlView::kNO_DNS_PREFETCH | HtmlView::kNO_PLUGINS;

}  // namespace

namespace GpgFrontend::UI {

namespace {

/**
 * @brief One view's engine side: the part scheme, and every question
 *        WebView2 asks about what the document may do.
 *
 * Everything here runs on the GUI thread, where the engine delivers its
 * events. The controller arrives asynchronously; a Load that came first
 * waits for it.
 */
class ViewHost : public std::enable_shared_from_this<ViewHost> {
 public:
  ViewHost(quint64 view, QWindow* window) : view_(view), window_(window) {}

  ~ViewHost() { Wipe(document_); }

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
    QObject::connect(window_, &QWindow::visibleChanged, window_,
                     [self](bool visible) {
                       const auto host = self.lock();
                       if (host && host->controller_) {
                         host->controller_->put_IsVisible(visible ? TRUE
                                                                  : FALSE);
                       }
                     });
    return widget;
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

  void Load(const QString& session, const QByteArray& document) {
    Wipe(document_);
    session_ = session;
    document_ = document;
    expected_main_ = QString("%1://%2/").arg(
        QLatin1StringView(HtmlView::kPartScheme), session);
    awaiting_load_ = true;
    have_expected_navigation_ = false;
    Navigate(expected_main_);
  }

  void Clear() {
    session_.clear();
    Wipe(document_);
    awaiting_load_ = false;
    have_expected_navigation_ = false;
    expected_main_ = u"about:blank"_s;
    Navigate(expected_main_);
  }

  /// The view is gone: whatever it asked for is cancelled, every request the
  /// engine still holds open is refused, and nothing it still hears reaches
  /// the service.
  void Detach() {
    detached_ = true;
    for (auto& [request, pending] : parts_) {
      HtmlViewService::Instance().CancelPart(kOwner, request);
      Refuse(pending.args.Get());
      pending.deferral->Complete();
    }
    parts_.clear();
    session_.clear();
    Wipe(document_);
    awaiting_load_ = false;
    pending_url_.clear();
    if (webview_) webview_->Stop();
    if (controller_) controller_->Close();
    webview4_.Drop();
    webview_.Drop();
    controller_.Drop();
  }

 private:
  struct PendingPart {
    Ptr<ICoreWebView2WebResourceRequestedEventArgs> args;
    Ptr<ICoreWebView2Deferral> deferral;
  };

  void Navigate(const QString& url) {
    if (!webview_) {
      pending_url_ = url;
      return;
    }
    const auto wide = url.toStdWString();
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
      // An engine that cannot be driven cannot render. Under gf_test's
      // offscreen platform there is no real HWND, so this is also the
      // normal path there.
      LOG_W() << "HTML view" << view_
              << "got no WebView2 controller:" << QString::number(result, 16);
      if (controller != nullptr) controller->Close();
      ReportLoadedIfAwaited(false);
      return;
    }
    controller_.Keep(controller);
    webview_.Keep(webview.Get());
    webview.Get()->QueryInterface(kIidWebView2_4,
                                  reinterpret_cast<void**>(webview4_.Put()));
    LockDown();
    Police();
    UpdateBounds();
    controller_->put_IsVisible(window_.isNull() || window_->isVisible() ? TRUE
                                                                        : FALSE);
    if (!pending_url_.isEmpty()) {
      const auto url = pending_url_;
      pending_url_.clear();
      Navigate(url);
    }
  }

  /// Everything a document could do besides sit there, off in the engine's
  /// own settings.
  void LockDown() {
    Ptr<ICoreWebView2Settings> s;
    if (FAILED(webview_->get_Settings(s.Put())) || !s) return;
    s->put_IsScriptEnabled(FALSE);
    s->put_AreDefaultScriptDialogsEnabled(FALSE);
    s->put_IsWebMessageEnabled(FALSE);
    s->put_AreDefaultContextMenusEnabled(FALSE);
    LockDownCommon(s.Get());
  }

  /// The event hooks: every request answered here, every navigation judged,
  /// every window, download and permission refused.
  void Police() {
    const auto self = weak_from_this();
    EventRegistrationToken token{};

    // Every URL, every context: no request bypasses OnResource, so nothing
    // the handler does not serve ever leaves the machine.
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
            [self](ICoreWebView2*, ICoreWebView2ProcessFailedEventArgs*) {
              if (const auto host = self.lock()) {
                LOG_W() << "HTML view" << host->view_
                        << "lost a WebView2 process";
                host->ReportLoadedIfAwaited(false);
              }
              return S_OK;
            }),
        &token);
    if (webview4_) {
      webview4_->add_DownloadStarting(
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
    if (main_frame && TakeExpectedMainLoad(url)) {
      args->get_NavigationId(&expected_navigation_);
      have_expected_navigation_ = true;
      return;
    }
    // The sandboxed frame the body lives in: about:srcdoc, never a request.
    if (!main_frame && (url == u"about:srcdoc"_s || url == u"about:blank"_s)) {
      return;
    }
    // A link the user activated is the Host's to open, outside this view.
    // Everything else is simply refused.
    BOOL user = FALSE;
    args->get_IsUserInitiated(&user);
    if (user == TRUE) {
      ReportLink(url);
    } else {
      LOG_D() << "HTML view" << view_ << "refused navigation,"
              << (main_frame ? "main" : "sub") << "frame:" << url;
    }
    args->put_Cancel(TRUE);
  }

  // Every refused navigation ends in a completion too; only the document's
  // own is the document finishing.
  void OnCompleted(ICoreWebView2NavigationCompletedEventArgs* args) {
    if (!have_expected_navigation_) return;
    UINT64 navigation = 0;
    args->get_NavigationId(&navigation);
    if (navigation != expected_navigation_) return;
    have_expected_navigation_ = false;
    BOOL ok = FALSE;
    args->get_IsSuccess(&ok);
    ReportLoadedIfAwaited(ok == TRUE);
  }

  void ReportLoadedIfAwaited(bool ok) {
    if (detached_ || !awaiting_load_) return;
    awaiting_load_ = false;
    if (!session_.isEmpty()) {
      HtmlViewService::Instance().Loaded(kOwner, view_, ok);
    }
  }

  void ReportLink(const QString& url) {
    if (detached_ || url.isEmpty()) return;
    HtmlViewService::Instance().LinkActivated(kOwner, view_, url);
  }

  // --- gfpart:// -- the document at /, a part anywhere else; every other
  // request is answered with a refusal, so nothing reaches a network --------

  void Answer(ICoreWebView2WebResourceRequestedEventArgs* args,
              IStream* stream, int status, const wchar_t* reason,
              const QString& headers) {
    Ptr<ICoreWebView2WebResourceResponse> response;
    const auto wide = headers.toStdWString();
    if (FAILED(environment_->CreateWebResourceResponse(
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
    const auto url = QUrl(TakeCoString(text));
    request->get_Method(&text);
    const auto method = TakeCoString(text);

    if (detached_ || method.compare(u"GET"_s, Qt::CaseInsensitive) != 0 ||
        url.scheme() != QLatin1StringView(HtmlView::kPartScheme)) {
      Refuse(args);
      return;
    }
    const auto session = url.host();
    const auto path = url.path(QUrl::FullyEncoded);

    if (path.isEmpty() || path == u"/"_s) {
      if (session_.isEmpty() || session != session_) {
        Refuse(args);
        return;
      }
      IStream* stream = SHCreateMemStream(
          reinterpret_cast<const BYTE*>(document_.constData()),
          static_cast<UINT>(document_.size()));
      if (stream == nullptr) {
        Refuse(args);
        return;
      }
      Answer(args, stream, 200, L"OK",
             u"Content-Type: text/html; charset=utf-8"_s);
      stream->Release();
      return;
    }

    const auto cid = QUrl::fromPercentEncoding(path.mid(1).toUtf8());
    PendingPart pending;
    pending.args.Keep(args);
    if (FAILED(args->GetDeferral(pending.deferral.Put())) ||
        !pending.deferral) {
      Refuse(args);
      return;
    }
    const auto self = weak_from_this();
    quint64 request_id = 0;
    const auto status = HtmlViewService::Instance().RequestPart(
        kOwner, view_, session, cid,
        [self](quint64 request, int status, const QString& mime,
               const GpgFrontend::GFBuffer& data) {
          if (const auto host = self.lock()) {
            host->FinishPart(request, status, mime, data);
          }
        },
        &request_id);
    if (status != HtmlViewService::kOK) {
      Refuse(args);
      pending.deferral->Complete();
      return;
    }
    parts_.emplace(request_id, std::move(pending));
  }

  void FinishPart(quint64 request, int status, const QString& mime,
                  const GpgFrontend::GFBuffer& data) {
    const auto it = parts_.find(request);
    if (it == parts_.end()) return;  // the view is gone
    auto pending = std::move(it->second);
    parts_.erase(it);
    if (status != HtmlViewService::kPART_OK) {
      Refuse(pending.args.Get());
      pending.deferral->Complete();
      return;
    }
    IStream* stream =
        SHCreateMemStream(reinterpret_cast<const BYTE*>(data.Data()),
                          static_cast<UINT>(data.Size()));
    if (stream == nullptr) {
      Refuse(pending.args.Get());
      pending.deferral->Complete();
      return;
    }
    Answer(pending.args.Get(), stream, 200, L"OK", u"Content-Type: "_s + mime);
    stream->Release();
    pending.deferral->Complete();
  }

 public:
  /// The one environment, kept by every host so responses can be minted.
  void KeepEnvironment(ICoreWebView2Environment* environment) {
    environment_.Keep(environment);
  }

 private:
  quint64 view_;
  QPointer<QWindow> window_;
  Ptr<ICoreWebView2Environment> environment_;
  Ptr<ICoreWebView2Controller> controller_;
  Ptr<ICoreWebView2> webview_;
  Ptr<ICoreWebView2_4> webview4_;
  QString session_;
  QByteArray document_;
  /// The one main-frame load allowed next, taken when it is asked about.
  QString expected_main_;
  /// The URL to show once the controller exists.
  QString pending_url_;
  /// The document's own navigation, the only one Loaded() is about.
  UINT64 expected_navigation_ = 0;
  bool have_expected_navigation_ = false;
  bool awaiting_load_ = false;
  bool detached_ = false;
  /// Part requests in flight, by the service's request id.
  std::map<quint64, PendingPart> parts_;
};

class WinHtmlViewRenderer : public HtmlViewRenderer {
 public:
  auto Create(quint64 view, const HtmlView::Policy& policy)
      -> QWidget* override {
    if ((policy.required & ~kEnforceable) != 0 || environment_failed_) {
      return nullptr;
    }
    auto* window = new QWindow();
    const auto host = std::make_shared<ViewHost>(view, window);
    auto* widget = host->MakeWidget();
    views_[view] = host;
    if (environment10_) {
      host->KeepEnvironment(environment_.Get());
      host->StartController(environment10_.Get());
    } else {
      waiting_.push_back(host);
      EnsureEnvironment();
    }
    return widget;
  }

  auto Load(quint64 view, const QString& session, const QByteArray& document)
      -> bool override {
    const auto it = views_.find(view);
    if (it == views_.end()) return false;
    it->second->Load(session, document);
    return true;
  }

  void Clear(quint64 view) override {
    const auto it = views_.find(view);
    if (it != views_.end()) it->second->Clear();
  }

  void Destroyed(quint64 view) override {
    const auto it = views_.find(view);
    if (it == views_.end()) return;
    it->second->Detach();
    views_.erase(it);
  }

 private:
  /// The engine's browser process is not started for an app that never
  /// renders mail: the first view asks for the environment, asynchronously,
  /// and every view created in the meantime waits with it.
  void EnsureEnvironment() {
    if (creating_) return;
    creating_ = true;
    CreateEnvironment(
        u"GpgFrontend.HtmlView"_s,
        {QString(QLatin1StringView(HtmlView::kPartScheme)).toStdWString()},
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
    if (!env10) {
      // No engine after all: the provider withdraws itself, every view it
      // was asked for is destroyed with LOST, and mail falls back to source.
      LOG_W() << "the WebView2 environment could not be created:"
              << QString::number(result, 16);
      environment_failed_ = true;
      creating_ = false;
      waiting_.clear();
      HtmlViewService::Instance().UnregisterProvider(
          kOwner, QString::fromLatin1(kPlatformHtmlViewRendererName));
      return;
    }
    environment_.Keep(environment);
    environment10_.Keep(env10.Get());
    creating_ = false;
    const auto waiting = std::move(waiting_);
    waiting_.clear();
    for (const auto& weak : waiting) {
      if (const auto host = weak.lock()) {
        host->KeepEnvironment(environment_.Get());
        host->StartController(environment10_.Get());
      }
    }
  }

  std::map<quint64, std::shared_ptr<ViewHost>> views_;
  std::vector<std::weak_ptr<ViewHost>> waiting_;
  Ptr<ICoreWebView2Environment> environment_;
  Ptr<ICoreWebView2Environment10> environment10_;
  bool creating_ = false;
  bool environment_failed_ = false;
};

auto RendererPtr() -> std::shared_ptr<WinHtmlViewRenderer> {
  // One renderer for the process: re-registering after a test detached it
  // gives back the same one, whose views the drain has already destroyed.
  static const auto renderer = std::make_shared<WinHtmlViewRenderer>();
  return renderer;
}

}  // namespace

void RegisterPlatformHtmlViewRenderer() {
  // Whether the machine can enforce the policy is known without starting
  // anything: the loader must be there, and the Evergreen runtime new
  // enough to register the part scheme. The browser process itself starts
  // only when a view is first created.
  if (!RuntimeUsable()) {
    LOG_I() << "no built-in HTML view renderer";
    return;
  }

  HtmlViewProviderInfo info;
  info.owner = kOwner;
  info.name = QString::fromLatin1(kPlatformHtmlViewRendererName);
  info.priority = HtmlViewService::kBuiltInPriority;
  info.enforceable = kEnforceable;
  info.restart_required = false;
  // WebView2 will not load the part scheme into an opaque-origin frame, so the
  // mail frame takes the document's own origin. No script ever runs, so this
  // grants nothing but the view's own session-gated parts.
  info.frame_same_origin = true;
  info.renderer = RendererPtr();
  HtmlViewService::Instance().RegisterProvider(std::move(info));
}

}  // namespace GpgFrontend::UI

#endif
