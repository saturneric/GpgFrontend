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

#pragma once

/**
 * @file WinWebView2.h
 * @brief What the Host's two WebView2 engines share: the HTML view renderer
 *        (WinHtmlView.cpp) and the web editor engine (WinWebEditor.cpp).
 *
 * The SDK header is plain COM, so the few interfaces implemented here -- the
 * completed and event handlers, the environment options, the scheme
 * registration -- are written out by hand; their IIDs are spelled here
 * because the header carries them in an MSVC attribute MinGW does not read.
 *
 * WebView2Loader.dll is resolved at run time. Only ever included by sources
 * built with GF_HAS_WEBVIEW2_SDK on Windows.
 */

#include <functional>
#include <string>
#include <vector>

// clang-format off
#include <windows.h>
#include "WebView2.h"
// clang-format on

namespace GpgFrontend::UI::WebView2 {

/// The oldest Evergreen runtime with custom scheme registration (110) --
/// which also has InPrivate controller options (108). Older runtimes cannot
/// serve the Host's schemes at all, so there is no engine on them.
inline constexpr const wchar_t* kMinRuntime = L"110.0.1587.40";

// --- the IIDs answered for or asked about ------------------------------------

inline constexpr GUID kIidUnknown = {
    0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
inline constexpr GUID kIidEnvironmentOptions = {
    0x2fde08a8, 0x1e9a, 0x4766, {0x8c, 0x05, 0x95, 0xa9, 0xce, 0xb9, 0xd1, 0xc5}};
inline constexpr GUID kIidEnvironmentOptions4 = {
    0xac52d13f, 0x0d38, 0x475a, {0x9d, 0xca, 0x87, 0x65, 0x80, 0xd6, 0x79, 0x3e}};
inline constexpr GUID kIidCustomSchemeRegistration = {
    0xd60ac92c, 0x37a6, 0x4b26, {0xa3, 0x9e, 0x95, 0xcf, 0xe5, 0x90, 0x47, 0xbb}};
inline constexpr GUID kIidController2 = {
    0xc979903e, 0xd4ca, 0x4228, {0x92, 0xeb, 0x47, 0xee, 0x3f, 0xa9, 0x6e, 0xab}};
inline constexpr GUID kIidEnvironment10 = {
    0xee0eb9df, 0x6f12, 0x46ce, {0xb5, 0x3f, 0x3f, 0x47, 0xb9, 0xc9, 0x28, 0xe0}};
inline constexpr GUID kIidWebView2_4 = {
    0x20d02d59, 0x6df2, 0x42dc, {0xbd, 0x06, 0xf9, 0x8a, 0x69, 0x4b, 0x13, 0x02}};
inline constexpr GUID kIidWebView2_18 = {
    0x7a626017, 0x28be, 0x49b2, {0xb8, 0x65, 0x3b, 0xa2, 0xb3, 0x52, 0x2d, 0x90}};
inline constexpr GUID kIidSettings3 = {
    0xfdb5ab74, 0xaf33, 0x4854, {0x84, 0xf0, 0x0a, 0x63, 0x1d, 0xeb, 0x5e, 0xba}};
inline constexpr GUID kIidSettings4 = {
    0xcb56846c, 0x4168, 0x4d53, {0xb0, 0x4f, 0x03, 0xb6, 0xd6, 0x79, 0x6f, 0xf2}};
inline constexpr GUID kIidSettings5 = {
    0x183e7052, 0x1d03, 0x43a0, {0xab, 0x99, 0x98, 0xe0, 0x43, 0xb6, 0x6b, 0x39}};
inline constexpr GUID kIidSettings6 = {
    0x11cb3acd, 0x9bc8, 0x43b8, {0x83, 0xbf, 0xf4, 0x07, 0x53, 0x71, 0x4f, 0x87}};
inline constexpr GUID kIidSettings8 = {
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
GF_WV2_HANDLER(ICoreWebView2WebMessageReceivedEventHandler, ICoreWebView2*,
               ICoreWebView2WebMessageReceivedEventArgs*,
               {0x57213f19, 0x00e6, 0x49fa,
                {0x8e, 0x07, 0x89, 0x8e, 0xa0, 0x1e, 0xcb, 0xd2}})
GF_WV2_HANDLER(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler,
               HRESULT, LPCWSTR,
               {0xb99369f3, 0x9b11, 0x47b5,
                {0xbc, 0x6f, 0x8e, 0x78, 0x95, 0xfc, 0xea, 0x17}})
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

auto DupCoString(const wchar_t* text) -> LPWSTR;

auto TakeCoString(LPWSTR text) -> QString;

inline void Wipe(QByteArray& bytes) {
  if (!bytes.isEmpty()) bytes.fill('\0');
  bytes.clear();
}

/// WebView2Loader.dll, beside the binaries first, then on the search path;
/// null without one.
auto LoadWebView2Loader() -> HMODULE;

/**
 * @brief Whether this machine can run either engine: the loader is there,
 *        and the Evergreen runtime is at least kMinRuntime. Nothing of the
 *        engine starts for this; the answer is found once and kept.
 */
auto RuntimeUsable() -> bool;

using EnvironmentDone =
    std::function<void(HRESULT result, ICoreWebView2Environment* environment)>;

/**
 * @brief Start a browser process for @p folder (under the temporary
 *        directory), with @p schemes registered: each with an authority (the
 *        session), treated as secure, and open to requests from any document
 *        origin, because every request still lands in the caller's handler,
 *        which knows the sessions it will serve.
 *
 * @p done is called exactly once, on the GUI thread -- synchronously when
 * the loader cannot even be asked.
 *
 * Every environment on one folder must be created with the same options, so
 * each engine has a folder of its own.
 */
void CreateEnvironment(const QString& folder,
                       const std::vector<std::wstring>& schemes,
                       EnvironmentDone done);

/**
 * @brief Off, in the engine's own settings: whatever no page the Host shows
 *        has any use for -- DevTools, host objects, the status bar, the error
 *        page, zoom, the browser's own accelerator keys, autofill, password
 *        saving, pinch and swipe, and SmartScreen.
 *
 * Script, script dialogs, web messages and context menus are each engine's
 * own call.
 */
void LockDownCommon(ICoreWebView2Settings* settings);

}  // namespace GpgFrontend::UI::WebView2
