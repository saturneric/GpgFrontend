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

#include "BinaryValidate.h"

#include <assuan.h>
#include <gpg-error.h>
#include <gpgme.h>

// OpenSSL
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include "test/GpgFrontendTest.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)

#include <dlfcn.h>

namespace {

auto GetLoadedLibraryPath(void *symbol_address) -> QString {
  Dl_info info;

  if (dladdr(symbol_address, &info) == 0 || (info.dli_fname == nullptr)) {
    return {};
  }

  return {info.dli_fname};
}

auto VerifySignatureByOS(const QString &path) -> bool { return true; }

auto FindLoadedLibraries(const QStringList &keywords) -> QStringList {
  return {};
}

}  // namespace

#elif defined(Q_OS_WINDOWS)

#include <psapi.h>
#include <softpub.h>
#include <windows.h>
#include <wintrust.h>

namespace {

auto GetLoadedLibraryPath(void *symbol_address) -> QString {
  HMODULE h_module = nullptr;
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCSTR>(symbol_address),
                          &h_module)) {
    return {};
  }

  wchar_t wpath_buffer[MAX_PATH] = {0};
  DWORD len = GetModuleFileNameW(h_module, wpath_buffer, MAX_PATH);
  if (len == 0 || len == MAX_PATH) {
    return {};
  }

  return QString::fromWCharArray(wpath_buffer, static_cast<int>(len));
}

auto VerifySignatureByOS(const QString &path) -> bool {
  LONG l_status;
  GUID wvt_policy_guid = WINTRUST_ACTION_GENERIC_VERIFY_V2;

  WINTRUST_FILE_INFO file_data = {0};
  file_data.cbStruct = sizeof(WINTRUST_FILE_INFO);
  file_data.pcwszFilePath = (LPCWSTR)path.utf16();
  file_data.hFile = NULL;
  file_data.pgKnownSubject = NULL;

  WINTRUST_DATA win_trust_data = {0};
  win_trust_data.cbStruct = sizeof(WINTRUST_DATA);
  win_trust_data.dwUIChoice = WTD_UI_NONE;
  win_trust_data.fdwRevocationChecks = WTD_REVOKE_NONE;
  win_trust_data.dwUnionChoice = WTD_CHOICE_FILE;
  win_trust_data.pFile = &file_data;
  win_trust_data.dwStateAction = 0;
  win_trust_data.hWVTStateData = NULL;
  win_trust_data.pwszURLReference = NULL;
  win_trust_data.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;

  l_status = WinVerifyTrust(NULL, &wvt_policy_guid, &win_trust_data);

  return (l_status == ERROR_SUCCESS);
}

auto FindLoadedLibraries(const QStringList &keywords) -> QStringList {
  QStringList result;
  HMODULE h_mods[1024];
  HANDLE h_process = GetCurrentProcess();
  DWORD cb_needed;

  if (EnumProcessModules(h_process, h_mods, sizeof(h_mods), &cb_needed)) {
    for (unsigned int i = 0; i < (cb_needed / sizeof(HMODULE)); ++i) {
      TCHAR sz_mod_name[MAX_PATH];
      if (GetModuleFileName(h_mods[i], sz_mod_name,
                            sizeof(sz_mod_name) / sizeof(TCHAR))) {
        auto mod = QString::fromWCharArray(sz_mod_name);
        for (const auto &kw : keywords) {
          if (mod.contains(kw, Qt::CaseInsensitive)) {
            result << mod;
            break;
          }
        }
      }
    }
  }
  return result;
}

}  // namespace

#else

auto GetLoadedLibraryPath(void *) -> QString { return {}; }

#endif

namespace {

// checking the trust of base libraries
const auto kBaseLibs = QStringList{
    // OpenSSL
    "libssl",
    "libcrypto",
    // Qt5
    "Qt5Core",
    "Qt5Gui",
    "Qt5Widgets",
    "Qt5Network",
    "Qt5Svg",
    "Qt5Xml",
    // Qt6
    "Qt6Core",
    "Qt6Gui",
    "Qt6Widgets",
    "Qt6Network",
    "Qt6Svg",
    "Qt6Xml",
};

auto ValidateLibrary(const QString &lib_path) -> std::tuple<QString, bool> {
  // check if library exists
  if (!QFileInfo(lib_path).exists()) return {lib_path, false};
  return {lib_path, VerifySignatureByOS(lib_path)};
}

auto ValidateLibrary(void *symbol_address) -> std::tuple<QString, bool> {
  return ValidateLibrary(GetLoadedLibraryPath(symbol_address));
}

}  // namespace

auto ValidateLibraries() -> bool {
  auto libs = FindLoadedLibraries(kBaseLibs);
  qInfo() << "Loaded Base Libraries:" << libs;

  for (const auto &path : libs) {
    auto [lib, succ_lib] = ValidateLibrary(path);
    if (!succ_lib) {
      qCritical() << "a dynamic link library failed verification and may be at "
                     "risk of being tampered with: "
                  << lib;
      return false;
    }
  }

  auto [core, succ_core] =
      ValidateLibrary(reinterpret_cast<void *>(GFCoreValidateSymbol));

  if (!succ_core) {
    qCritical() << "a dynamic link library failed verification and may be at "
                   "risk of being tampered with: "
                << core;
  }

  auto [ui, succ_ui] =
      ValidateLibrary(reinterpret_cast<void *>(GFUIValidateSymbol));

  if (!succ_ui) {
    qCritical() << "a dynamic link library failed verification and may be at "
                   "risk of being tampered with: "
                << ui;
  }

  auto [test, succ_test] =
      ValidateLibrary(reinterpret_cast<void *>(GFTestValidateSymbol));

  if (!succ_test) {
    qCritical() << "a dynamic link library failed verification and may be at "
                   "risk of being tampered with: "
                << test;
  }

  return succ_core && succ_ui && succ_test;
}
