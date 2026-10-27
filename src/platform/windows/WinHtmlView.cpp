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
 * mechanisms where that one has WebKit's. The SDK header is plain COM, so
 * the few interfaces this file implements -- the completed and event
 * handlers, the environment options, the scheme registration -- are written
 * out by hand; their IIDs are spelled here because the header carries them
 * in an MSVC attribute MinGW does not read.
 *
 * WebView2Loader.dll is resolved at run time. No loader, no Evergreen
 * runtime, or a runtime too old to register the part scheme: the renderer
 * is simply never registered, and HTML mail stays source.
 */

#include "GpgFrontend.h"

#if defined(Q_OS_WINDOWS) && defined(GF_HAS_WEBVIEW2_SDK)

#include <QDir>
#include <QPointer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWindow>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "platform/common/PlatformHtmlView.h"
#include "ui/html/HtmlViewService.h"

// clang-format off
#include <windows.h>
#include <shlwapi.h>
#include "WebView2.h"
// clang-format on

namespace HtmlView = GpgFrontend::UI::HtmlView;
using GpgFrontend::UI::HtmlViewService;

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

/// The oldest Evergreen runtime with custom scheme registration (110) --
/// which also has InPrivate controller options (108). Older runtimes cannot
/// serve gfpart at all, so there is no renderer on them.
constexpr const wchar_t* kMinRuntime = L"110.0.1587.40";

/// Chromium is told directly: no talking to anyone the document did not ask
/// for, and no DNS warm-up for links. Everything the document asks for is
/// policed per request below.
constexpr const wchar_t* kBrowserArguments =
    L"--disable-background-networking --dns-prefetch-disable";

// --- the IIDs this file answers for or asks about ---------------------------

constexpr GUID kIidUnknown = {
    0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
constexpr GUID kIidEnvironmentOptions = {
    0x2fde08a8, 0x1e9a, 0x4766, {0x8c, 0x05, 0x95, 0xa9, 0xce, 0xb9, 0xd1, 0xc5}};
constexpr GUID kIidEnvironmentOptions4 = {
    0xac52d13f, 0x0d38, 0x475a, {0x9d, 0xca, 0x87, 0x65, 0x80, 0xd6, 0x79, 0x3e}};
constexpr GUID kIidCustomSchemeRegistration = {
    0xd60ac92c, 0x37a6, 0x4b26, {0xa3, 0x9e, 0x95, 0xcf, 0xe5, 0x90, 0x47, 0xbb}};
constexpr GUID kIidEnvironment10 = {
    0xee0eb9df, 0x6f12, 0x46ce, {0xb5, 0x3f, 0x3f, 0x47, 0xb9, 0xc9, 0x28, 0xe0}};
constexpr GUID kIidWebView2_4 = {
    0x20d02d59, 0x6df2, 0x42dc, {0xbd, 0x06, 0xf9, 0x8a, 0x69, 0x4b, 0x13, 0x02}};
constexpr GUID kIidWebView2_18 = {
    0x7a626017, 0x28be, 0x49b2, {0xb8, 0x65, 0x3b, 0xa2, 0xb3, 0x52, 0x2d, 0x90}};
constexpr GUID kIidSettings3 = {
    0xfdb5ab74, 0xaf33, 0x4854, {0x84, 0xf0, 0x0a, 0x63, 0x1d, 0xeb, 0x5e, 0xba}};
constexpr GUID kIidSettings4 = {
    0xcb56846c, 0x4168, 0x4d53, {0xb0, 0x4f, 0x03, 0xb6, 0xd6, 0x79, 0x6f, 0xf2}};
constexpr GUID kIidSettings5 = {
    0x183e7052, 0x1d03, 0x43a0, {0xab, 0x99, 0x98, 0xe0, 0x43, 0xb6, 0x6b, 0x39}};
constexpr GUID kIidSettings6 = {
    0x11cb3acd, 0x9bc8, 0x43b8, {0x83, 0xbf, 0xf4, 0x07, 0x53, 0x71, 0x4f, 0x87}};
constexpr GUID kIidSettings8 = {
    0x9e6b0e8f, 0x86ad, 0x4e81, {0x81, 0x47, 0xa9, 0xb5, 0xed, 0xb6, 0x86, 0x50}};

template <typename I>
struct HandlerTraits;
#define GF_WV2_HANDLER(IFACE, SENDER, ARGS, ...)                   \
  template <>                                                      \
  struct HandlerTraits<IFACE> {                                    \
    using Sender = SENDER;                                         \
    using Args = ARGS;                                             \
    static constexpr GUID kIid = __VA_ARGS__;                      \
  };
GF_WV2_HANDLER(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,
               HRESULT, ICoreWebView2Environment*,
               {0x4e8a3389, 0xc9d8, 0x4bd2,
                {0xb6, 0xb5, 0x12, 0x4f, 0xee, 0x6c, 0xc1, 0x4d}})
GF_WV2_HANDLER(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler,
               HRESULT, ICoreWebView2Controller*,
               {0x6c4819f3, 0xc9b7, 0x4260,
                {0x81, 0x27, 0xc9, 0xf5, 0xbd, 0xe7, 0xf6, 0x8c}})
GF_WV2_HANDLER(ICoreWebView2NavigationStartingEventHandler, ICoreWebView2*,
               ICoreWebView2NavigationStartingEventArgs*,
               {0x9adbe429, 0xf36d, 0x432b,
                {0x9d, 0xdc, 0xf8, 0x88, 0x1f, 0xbd, 0x76, 0xe3}})
GF_WV2_HANDLER(ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2*,
               ICoreWebView2NavigationCompletedEventArgs*,
               {0xd33a35bf, 0x1c49, 0x4f98,
                {0x93, 0xab, 0x00, 0x6e, 0x05, 0x33, 0xfe, 0x1c}})
GF_WV2_HANDLER(ICoreWebView2WebResourceRequestedEventHandler, ICoreWebView2*,
               ICoreWebView2WebResourceRequestedEventArgs*,
               {0xab00b74c, 0x15f1, 0x4646,
                {0x80, 0xe8, 0xe7, 0x63, 0x41, 0xd2, 0x5d, 0x71}})
GF_WV2_HANDLER(ICoreWebView2NewWindowRequestedEventHandler, ICoreWebView2*,
               ICoreWebView2NewWindowRequestedEventArgs*,
               {0xd4c185fe, 0xc81c, 0x4989,
                {0x97, 0xaf, 0x2d, 0x3f, 0xa7, 0xab, 0x56, 0x51}})
GF_WV2_HANDLER(ICoreWebView2PermissionRequestedEventHandler, ICoreWebView2*,
               ICoreWebView2PermissionRequestedEventArgs*,
               {0x15e1c6a3, 0xc72a, 0x4df3,
                {0x91, 0xd7, 0xd0, 0x97, 0xfb, 0xec, 0x6b, 0xfd}})
GF_WV2_HANDLER(ICoreWebView2DownloadStartingEventHandler, ICoreWebView2*,
               ICoreWebView2DownloadStartingEventArgs*,
               {0xefedc989, 0xc396, 0x41ca,
                {0x83, 0xf7, 0x07, 0xf8, 0x45, 0xa5, 0x57, 0x24}})
GF_WV2_HANDLER(ICoreWebView2ProcessFailedEventHandler, ICoreWebView2*,
               ICoreWebView2ProcessFailedEventArgs*,
               {0x79e0aea4, 0x990b, 0x42d9,
                {0xaa, 0x1d, 0x0f, 0xcc, 0x2e, 0x5b, 0xc7, 0xf1}})
GF_WV2_HANDLER(ICoreWebView2LaunchingExternalUriSchemeEventHandler,
               ICoreWebView2*,
               ICoreWebView2LaunchingExternalUriSchemeEventArgs*,
               {0x74f712e0, 0x8165, 0x43a9,
                {0xa1, 0x3f, 0x0c, 0xce, 0x59, 0x7e, 0x75, 0xdf}})
#undef GF_WV2_HANDLER

/// One completed or event handler, as a lambda. The engine owns it after
/// add_*/Release, and drops it when the view closes.
template <typename I>
class Callback final : public I {
 public:
  using Fn = std::function<HRESULT(typename HandlerTraits<I>::Sender,
                                   typename HandlerTraits<I>::Args)>;
  explicit Callback(Fn fn) : fn_(std::move(fn)) {}

  auto STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out)
      -> HRESULT override {
    if (out == nullptr) return E_POINTER;
    if (IsEqualIID(riid, HandlerTraits<I>::kIid) ||
        IsEqualIID(riid, kIidUnknown)) {
      *out = static_cast<I*>(this);
      AddRef();
      return S_OK;
    }
    *out = nullptr;
    return E_NOINTERFACE;
  }
  auto STDMETHODCALLTYPE AddRef() -> ULONG override { return ++refs_; }
  auto STDMETHODCALLTYPE Release() -> ULONG override {
    const auto left = --refs_;
    if (left == 0) delete this;
    return left;
  }
  auto STDMETHODCALLTYPE Invoke(typename HandlerTraits<I>::Sender sender,
                                typename HandlerTraits<I>::Args args)
      -> HRESULT override {
    return fn_(sender, args);
  }

 private:
  ~Callback() = default;
  ULONG refs_ = 1;
  Fn fn_;
};

/// A reference the scopes here cannot leak.
template <typename T>
class Ptr {
 public:
  Ptr() = default;
  Ptr(const Ptr&) = delete;
  auto operator=(const Ptr&) -> Ptr& = delete;
  Ptr(Ptr&& other) noexcept : p_(other.p_) { other.p_ = nullptr; }
  auto operator=(Ptr&& other) noexcept -> Ptr& {
    if (this != &other) {
      Drop();
      p_ = other.p_;
      other.p_ = nullptr;
    }
    return *this;
  }
  ~Ptr() { Drop(); }

  /// For the T** out-parameter idiom; whatever was held is dropped first.
  auto Put() -> T** {
    Drop();
    return &p_;
  }
  void Keep(T* p) {
    if (p != nullptr) p->AddRef();
    Drop();
    p_ = p;
  }
  void Drop() {
    if (p_ != nullptr) p_->Release();
    p_ = nullptr;
  }
  auto Get() const -> T* { return p_; }
  auto operator->() const -> T* { return p_; }
  explicit operator bool() const { return p_ != nullptr; }

 private:
  T* p_ = nullptr;
};

auto DupCoString(const wchar_t* text) -> LPWSTR {
  const auto bytes = (wcslen(text) + 1) * sizeof(wchar_t);
  auto* copy = static_cast<LPWSTR>(CoTaskMemAlloc(bytes));
  if (copy != nullptr) memcpy(copy, text, bytes);
  return copy;
}

auto TakeCoString(LPWSTR text) -> QString {
  if (text == nullptr) return {};
  auto result = QString::fromWCharArray(text);
  CoTaskMemFree(text);
  return result;
}

void Wipe(QByteArray& bytes) {
  if (!bytes.isEmpty()) bytes.fill('\0');
  bytes.clear();
}

auto PartSchemeW() -> const std::wstring& {
  static const std::wstring scheme =
      QString(QLatin1StringView(HtmlView::kPartScheme)).toStdWString();
  return scheme;
}

// --- what the loader is handed before anything of the engine exists ----------

/**
 * @brief The part scheme, as the engine must know it before the browser
 *        process starts: an authority (the session), treated as secure, and
 *        open to requests from any document origin -- the mail's frame has
 *        an opaque one -- because every request still lands in this file's
 *        handler, which knows the one session it will serve.
 */
class SchemeRegistration final : public ICoreWebView2CustomSchemeRegistration {
 public:
  auto STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out)
      -> HRESULT override {
    if (out == nullptr) return E_POINTER;
    if (IsEqualIID(riid, kIidCustomSchemeRegistration) ||
        IsEqualIID(riid, kIidUnknown)) {
      *out = static_cast<ICoreWebView2CustomSchemeRegistration*>(this);
      AddRef();
      return S_OK;
    }
    *out = nullptr;
    return E_NOINTERFACE;
  }
  auto STDMETHODCALLTYPE AddRef() -> ULONG override { return ++refs_; }
  auto STDMETHODCALLTYPE Release() -> ULONG override {
    const auto left = --refs_;
    if (left == 0) delete this;
    return left;
  }

  auto STDMETHODCALLTYPE get_SchemeName(LPWSTR* value) -> HRESULT override {
    *value = DupCoString(PartSchemeW().c_str());
    return S_OK;
  }
  auto STDMETHODCALLTYPE get_TreatAsSecure(BOOL* value) -> HRESULT override {
    *value = TRUE;
    return S_OK;
  }
  auto STDMETHODCALLTYPE put_TreatAsSecure(BOOL) -> HRESULT override {
    return S_OK;
  }
  auto STDMETHODCALLTYPE GetAllowedOrigins(UINT32* count, LPWSTR** origins)
      -> HRESULT override {
    auto** list = static_cast<LPWSTR*>(CoTaskMemAlloc(sizeof(LPWSTR)));
    if (list == nullptr) return E_OUTOFMEMORY;
    list[0] = DupCoString(L"*");
    *origins = list;
    *count = 1;
    return S_OK;
  }
  auto STDMETHODCALLTYPE SetAllowedOrigins(UINT32, LPCWSTR*)
      -> HRESULT override {
    return S_OK;
  }
  auto STDMETHODCALLTYPE get_HasAuthorityComponent(BOOL* value)
      -> HRESULT override {
    *value = TRUE;
    return S_OK;
  }
  auto STDMETHODCALLTYPE put_HasAuthorityComponent(BOOL) -> HRESULT override {
    return S_OK;
  }

 private:
  ~SchemeRegistration() = default;
  ULONG refs_ = 1;
};

class EnvironmentOptions final : public ICoreWebView2EnvironmentOptions,
                                 public ICoreWebView2EnvironmentOptions4 {
 public:
  auto STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out)
      -> HRESULT override {
    if (out == nullptr) return E_POINTER;
    if (IsEqualIID(riid, kIidEnvironmentOptions) ||
        IsEqualIID(riid, kIidUnknown)) {
      *out = static_cast<ICoreWebView2EnvironmentOptions*>(this);
      AddRef();
      return S_OK;
    }
    if (IsEqualIID(riid, kIidEnvironmentOptions4)) {
      *out = static_cast<ICoreWebView2EnvironmentOptions4*>(this);
      AddRef();
      return S_OK;
    }
    *out = nullptr;
    return E_NOINTERFACE;
  }
  auto STDMETHODCALLTYPE AddRef() -> ULONG override { return ++refs_; }
  auto STDMETHODCALLTYPE Release() -> ULONG override {
    const auto left = --refs_;
    if (left == 0) delete this;
    return left;
  }

  auto STDMETHODCALLTYPE get_AdditionalBrowserArguments(LPWSTR* value)
      -> HRESULT override {
    *value = DupCoString(kBrowserArguments);
    return S_OK;
  }
  auto STDMETHODCALLTYPE put_AdditionalBrowserArguments(LPCWSTR)
      -> HRESULT override {
    return S_OK;
  }
  auto STDMETHODCALLTYPE get_Language(LPWSTR* value) -> HRESULT override {
    *value = DupCoString(L"");
    return S_OK;
  }
  auto STDMETHODCALLTYPE put_Language(LPCWSTR) -> HRESULT override {
    return S_OK;
  }
  auto STDMETHODCALLTYPE get_TargetCompatibleBrowserVersion(LPWSTR* value)
      -> HRESULT override {
    *value = DupCoString(kMinRuntime);
    return S_OK;
  }
  auto STDMETHODCALLTYPE put_TargetCompatibleBrowserVersion(LPCWSTR)
      -> HRESULT override {
    return S_OK;
  }
  auto STDMETHODCALLTYPE get_AllowSingleSignOnUsingOSPrimaryAccount(BOOL* value)
      -> HRESULT override {
    *value = FALSE;
    return S_OK;
  }
  auto STDMETHODCALLTYPE put_AllowSingleSignOnUsingOSPrimaryAccount(BOOL)
      -> HRESULT override {
    return S_OK;
  }

  auto STDMETHODCALLTYPE GetCustomSchemeRegistrations(
      UINT32* count, ICoreWebView2CustomSchemeRegistration*** registrations)
      -> HRESULT override {
    auto** list = static_cast<ICoreWebView2CustomSchemeRegistration**>(
        CoTaskMemAlloc(sizeof(void*)));
    if (list == nullptr) return E_OUTOFMEMORY;
    list[0] = new SchemeRegistration();
    *registrations = list;
    *count = 1;
    return S_OK;
  }
  auto STDMETHODCALLTYPE SetCustomSchemeRegistrations(
      UINT32, ICoreWebView2CustomSchemeRegistration**) -> HRESULT override {
    return S_OK;
  }

 private:
  ~EnvironmentOptions() = default;
  ULONG refs_ = 1;
};

// --- the loader and the one environment --------------------------------------

using CreateEnvironmentFn = HRESULT(STDAPICALLTYPE*)(
    PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);
using AvailableVersionFn = HRESULT(STDAPICALLTYPE*)(PCWSTR, LPWSTR*);
using CompareVersionsFn = HRESULT(STDAPICALLTYPE*)(PCWSTR, PCWSTR, int*);

/// The Evergreen runtime's version, through the loader, or empty with no
/// loader or no runtime. Nothing of the engine starts for this.
auto AvailableRuntime(HMODULE loader) -> QString {
  const auto available = reinterpret_cast<AvailableVersionFn>(
      GetProcAddress(loader, "GetAvailableCoreWebView2BrowserVersionString"));
  if (available == nullptr) return {};
  LPWSTR version = nullptr;
  if (FAILED(available(nullptr, &version))) return {};
  return TakeCoString(version);
}

auto LoadWebView2Loader() -> HMODULE {
  // Beside the binaries first -- that is where the build and the package put
  // it -- then the normal search path.
  const auto beside =
      QDir(QCoreApplication::applicationDirPath()).filePath(u"WebView2Loader.dll"_s);
  auto* loader =
      LoadLibraryW(reinterpret_cast<const wchar_t*>(beside.utf16()));
  if (loader == nullptr) loader = LoadLibraryW(L"WebView2Loader.dll");
  return loader;
}

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
    s->put_AreDevToolsEnabled(FALSE);
    s->put_AreDefaultContextMenusEnabled(FALSE);
    s->put_AreHostObjectsAllowed(FALSE);
    s->put_IsStatusBarEnabled(FALSE);
    s->put_IsBuiltInErrorPageEnabled(FALSE);
    s->put_IsZoomControlEnabled(FALSE);

    Ptr<ICoreWebView2Settings3> s3;
    s.Get()->QueryInterface(kIidSettings3,
                            reinterpret_cast<void**>(s3.Put()));
    if (s3) s3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
    Ptr<ICoreWebView2Settings4> s4;
    s.Get()->QueryInterface(kIidSettings4,
                            reinterpret_cast<void**>(s4.Put()));
    if (s4) {
      s4->put_IsGeneralAutofillEnabled(FALSE);
      s4->put_IsPasswordAutosaveEnabled(FALSE);
    }
    Ptr<ICoreWebView2Settings5> s5;
    s.Get()->QueryInterface(kIidSettings5,
                            reinterpret_cast<void**>(s5.Put()));
    if (s5) s5->put_IsPinchZoomEnabled(FALSE);
    Ptr<ICoreWebView2Settings6> s6;
    s.Get()->QueryInterface(kIidSettings6,
                            reinterpret_cast<void**>(s6.Put()));
    if (s6) s6->put_IsSwipeNavigationEnabled(FALSE);
    // SmartScreen looks URLs up over the network.
    Ptr<ICoreWebView2Settings8> s8;
    s.Get()->QueryInterface(kIidSettings8,
                            reinterpret_cast<void**>(s8.Put()));
    if (s8) s8->put_IsReputationCheckingRequired(FALSE);
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

    auto* loader = LoadWebView2Loader();
    const auto create =
        loader == nullptr
            ? nullptr
            : reinterpret_cast<CreateEnvironmentFn>(GetProcAddress(
                  loader, "CreateCoreWebView2EnvironmentWithOptions"));
    if (create == nullptr) {
      OnEnvironment(E_FAIL, nullptr);
      return;
    }
    const auto folder =
        QDir::toNativeSeparators(QDir::temp().filePath(u"GpgFrontend.HtmlView"_s))
            .toStdWString();
    auto* options = new EnvironmentOptions();
    const auto hr = create(
        nullptr, folder.c_str(), options,
        new Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* environment) {
              OnEnvironment(result, environment);
              return S_OK;
            }));
    options->Release();
    if (FAILED(hr)) OnEnvironment(hr, nullptr);
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
  static int checked = -1;
  if (checked == -1) {
    auto* loader = LoadWebView2Loader();
    if (loader == nullptr) {
      LOG_I() << "no WebView2Loader.dll; no built-in HTML view renderer";
      checked = 0;
    } else {
      const auto version = AvailableRuntime(loader);
      const auto compare = reinterpret_cast<CompareVersionsFn>(
          GetProcAddress(loader, "CompareBrowserVersions"));
      int newer = -1;
      if (version.isEmpty() || compare == nullptr ||
          FAILED(compare(version.toStdWString().c_str(), kMinRuntime,
                         &newer)) ||
          newer < 0) {
        LOG_I() << "no usable WebView2 runtime (found"
                << (version.isEmpty() ? u"none"_s : version)
                << "); no built-in HTML view renderer";
        checked = 0;
      } else {
        LOG_D() << "WebView2 runtime" << version << "serves HTML views";
        checked = 1;
      }
    }
  }
  if (checked != 1) return;

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
