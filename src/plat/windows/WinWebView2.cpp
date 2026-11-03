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

#include "GpgFrontend.h"

#if defined(Q_OS_WINDOWS) && defined(GF_HAS_WEBVIEW2_SDK)

#include "plat/windows/WinWebView2.h"

#include <QDir>

namespace GpgFrontend::UI::WebView2 {

namespace {

/// Chromium is told directly: no talking to anyone a page did not ask for,
/// and no DNS warm-up for links. Everything a page asks for is policed per
/// request by the engine that shows it.
constexpr const wchar_t* kBrowserArguments =
    L"--disable-background-networking --dns-prefetch-disable";

// --- what the loader is handed before anything of the engine exists ----------

/// One of the Host's schemes, as the engine must know it before the browser
/// process starts; see CreateEnvironment().
class SchemeRegistration final : public ICoreWebView2CustomSchemeRegistration {
 public:
  explicit SchemeRegistration(std::wstring scheme)
      : scheme_(std::move(scheme)) {}

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
    *value = DupCoString(scheme_.c_str());
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
  std::wstring scheme_;
};

class EnvironmentOptions final : public ICoreWebView2EnvironmentOptions,
                                 public ICoreWebView2EnvironmentOptions4 {
 public:
  explicit EnvironmentOptions(std::vector<std::wstring> schemes)
      : schemes_(std::move(schemes)) {}

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
        CoTaskMemAlloc(sizeof(void*) * schemes_.size()));
    if (list == nullptr) return E_OUTOFMEMORY;
    for (size_t i = 0; i < schemes_.size(); ++i) {
      list[i] = new SchemeRegistration(schemes_[i]);
    }
    *registrations = list;
    *count = static_cast<UINT32>(schemes_.size());
    return S_OK;
  }
  auto STDMETHODCALLTYPE SetCustomSchemeRegistrations(
      UINT32, ICoreWebView2CustomSchemeRegistration**) -> HRESULT override {
    return S_OK;
  }

 private:
  ~EnvironmentOptions() = default;
  ULONG refs_ = 1;
  std::vector<std::wstring> schemes_;
};

// --- the loader ---------------------------------------------------------------

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

}  // namespace

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

auto RuntimeUsable() -> bool {
  static int checked = -1;
  if (checked != -1) return checked == 1;

  auto* loader = LoadWebView2Loader();
  if (loader == nullptr) {
    LOG_I() << "no WebView2Loader.dll";
    checked = 0;
    return false;
  }
  const auto version = AvailableRuntime(loader);
  const auto compare = reinterpret_cast<CompareVersionsFn>(
      GetProcAddress(loader, "CompareBrowserVersions"));
  int newer = -1;
  if (version.isEmpty() || compare == nullptr ||
      FAILED(compare(version.toStdWString().c_str(), kMinRuntime, &newer)) ||
      newer < 0) {
    LOG_I() << "no usable WebView2 runtime (found"
            << (version.isEmpty() ? u"none"_s : version) << ")";
    checked = 0;
    return false;
  }
  LOG_D() << "WebView2 runtime" << version << "is usable";
  checked = 1;
  return true;
}

void CreateEnvironment(const QString& folder,
                       const std::vector<std::wstring>& schemes,
                       EnvironmentDone done) {
  auto* loader = LoadWebView2Loader();
  const auto create =
      loader == nullptr
          ? nullptr
          : reinterpret_cast<CreateEnvironmentFn>(GetProcAddress(
                loader, "CreateCoreWebView2EnvironmentWithOptions"));
  if (create == nullptr) {
    done(E_FAIL, nullptr);
    return;
  }
  const auto path =
      QDir::toNativeSeparators(QDir::temp().filePath(folder)).toStdWString();
  auto* options = new EnvironmentOptions(schemes);
  auto* handler =
      new Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
          [done](HRESULT result, ICoreWebView2Environment* environment) {
            done(result, environment);
            return S_OK;
          });
  const auto hr = create(nullptr, path.c_str(), options, handler);
  options->Release();
  handler->Release();
  if (FAILED(hr)) done(hr, nullptr);
}

void LockDownCommon(ICoreWebView2Settings* s) {
  s->put_AreDevToolsEnabled(FALSE);
  s->put_AreHostObjectsAllowed(FALSE);
  s->put_IsStatusBarEnabled(FALSE);
  s->put_IsBuiltInErrorPageEnabled(FALSE);
  s->put_IsZoomControlEnabled(FALSE);

  Ptr<ICoreWebView2Settings3> s3;
  s->QueryInterface(kIidSettings3, reinterpret_cast<void**>(s3.Put()));
  if (s3) s3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
  Ptr<ICoreWebView2Settings4> s4;
  s->QueryInterface(kIidSettings4, reinterpret_cast<void**>(s4.Put()));
  if (s4) {
    s4->put_IsGeneralAutofillEnabled(FALSE);
    s4->put_IsPasswordAutosaveEnabled(FALSE);
  }
  Ptr<ICoreWebView2Settings5> s5;
  s->QueryInterface(kIidSettings5, reinterpret_cast<void**>(s5.Put()));
  if (s5) s5->put_IsPinchZoomEnabled(FALSE);
  Ptr<ICoreWebView2Settings6> s6;
  s->QueryInterface(kIidSettings6, reinterpret_cast<void**>(s6.Put()));
  if (s6) s6->put_IsSwipeNavigationEnabled(FALSE);
  // SmartScreen looks URLs up over the network.
  Ptr<ICoreWebView2Settings8> s8;
  s->QueryInterface(kIidSettings8, reinterpret_cast<void**>(s8.Put()));
  if (s8) s8->put_IsReputationCheckingRequired(FALSE);
}

}  // namespace GpgFrontend::UI::WebView2

#endif
