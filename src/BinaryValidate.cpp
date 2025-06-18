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

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)

#include <dlfcn.h>

namespace {

auto GetLoadedLibraryPath(void *symbol_address) -> QString {
  Dl_info info;

  if (dladdr(symbol_address, &info) == 0 || (info.dli_fname == nullptr)) {
    return {};
  }

  return {info.dli_fname};
}

}  // namespace

#elif defined(Q_OS_WINDOWS)

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

auto VerifySignatureByWinVerifyTrust(const QString &path) -> bool {
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

auto FindLoadedOpenSSL() -> QStringList {
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
        if (mod.contains("libssl", Qt::CaseInsensitive) ||
            mod.contains("libcrypto", Qt::CaseInsensitive)) {
          result << mod;
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

auto LoadEmbeddedPublicKey() -> EVP_PKEY * {
  QFile key_file(":/keys/public.pem");
  if (!key_file.open(QIODevice::ReadOnly)) {
    qWarning()
        << "unable to read public key from resource file: /keys/public.pem";
    return nullptr;
  }

  auto pem_data = key_file.readAll();
  key_file.close();

  auto *bio =
      BIO_new_mem_buf(pem_data.constData(), static_cast<int>(pem_data.size()));
  if (bio == nullptr) {
    qWarning() << "BIO_new_mem_buf error";
    return nullptr;
  }

  EVP_PKEY *pub_key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
  BIO_free(bio);

  if (pub_key == nullptr) {
    qWarning()
        << "PEM_read_bio_PUBKEY parsing failed, is it a valid PEM format?";
    ERR_print_errors_fp(stderr);
  }

  return pub_key;
}

auto FileToByteArray(const QString &path, QByteArray &out) -> bool {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    qWarning() << "cannot open file:" << path;
    return false;
  }
  out = f.readAll();
  f.close();
  return true;
}

auto VerifySignature(const QByteArray &lib_data, const QByteArray &sig_data,
                     EVP_PKEY *pub_key) -> bool {
  if (pub_key == nullptr) return false;

  EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
  if (md_ctx == nullptr) {
    qWarning() << "EVP_MD_CTX_new error";
    return false;
  }

  if (EVP_DigestVerifyInit(md_ctx, nullptr, EVP_sha256(), nullptr, pub_key) <=
      0) {
    qWarning() << "EVP_DigestVerifyInit error";
    ERR_print_errors_fp(stderr);
    EVP_MD_CTX_free(md_ctx);
    return false;
  }

  if (EVP_DigestVerifyUpdate(
          md_ctx, reinterpret_cast<const unsigned char *>(lib_data.constData()),
          static_cast<size_t>(lib_data.size())) <= 0) {
    qWarning() << "EVP_DigestVerifyUpdate error";
    ERR_print_errors_fp(stderr);
    EVP_MD_CTX_free(md_ctx);
    return false;
  }

  int ret = EVP_DigestVerifyFinal(
      md_ctx, reinterpret_cast<const unsigned char *>(sig_data.constData()),
      static_cast<size_t>(sig_data.size()));

  EVP_MD_CTX_free(md_ctx);

  if (ret == 1) return true;

  if (ret == 0) {
    qWarning() << "signature does not match, the library file "
                  "may have been tampered with!";
    return false;
  }

  qWarning() << "EVP_DigestVerifyFinal error";
  ERR_print_errors_fp(stderr);
  return false;
}

auto ValidateLibrary(const QString &lib_path, EVP_PKEY *key)
    -> std::tuple<QString, bool> {
  // check if library exists
  if (!QFileInfo(lib_path).exists()) return {lib_path, false};

#ifdef Q_OS_WINDOWS
  // use win verify trust api at first try
  if (VerifySignatureByWinVerifyTrust(lib_path)) return {lib_path, true};
#endif

  // we must now ensure that we have the public key
  if (key == nullptr) return {lib_path, true};

  QByteArray lib_data;
  auto succ = FileToByteArray(lib_path, lib_data);
  if (!succ) return {lib_path, false};

  auto sig_path = lib_path + ".sig";

  QByteArray sig_data;
  succ = FileToByteArray(sig_path, sig_data);
  if (!succ) return {lib_path, false};

  if (key == nullptr) return {lib_path, false};

  return {lib_path, VerifySignature(lib_data, sig_data, key)};
}

auto ValidateLibrary(void *symbol_address, EVP_PKEY *key)
    -> std::tuple<QString, bool> {
  return ValidateLibrary(GetLoadedLibraryPath(symbol_address), key);
}

}  // namespace

auto ValidateLibraries() -> bool {
  auto *pub_key = LoadEmbeddedPublicKey();
  if (pub_key == nullptr) {
    qWarning() << "unable to obtain public key of binary signing";
  }

#ifdef Q_OS_WINDOWS

  // we need to check openssl at first
  auto libs = FindLoadedOpenSSL();
  qInfo() << "Loaded OpenSSL DLL:" << libs;

  for (const auto &path : libs) {
    auto [ssl, succ_ssl] = ValidateLibrary(path, pub_key);
    if (!succ_ssl) {
      qCritical()
          << "the dynamic link library failed verification and may be at "
             "risk of being tampered with: "
          << ssl;
      return false;
    }
  }

#endif

  auto [core, succ_core] =
      ValidateLibrary(reinterpret_cast<void *>(GFCoreValidateSymbol), pub_key);

  if (!succ_core) {
    qCritical() << "the dynamic link library failed verification and may be at "
                   "risk of being tampered with: "
                << core;
  }

  auto [ui, succ_ui] =
      ValidateLibrary(reinterpret_cast<void *>(GFUIValidateSymbol), pub_key);

  if (!succ_ui) {
    qCritical() << "the dynamic link library failed verification and may be at "
                   "risk of being tampered with: "
                << ui;
  }

  auto [test, succ_test] =
      ValidateLibrary(reinterpret_cast<void *>(GFTestValidateSymbol), pub_key);

  if (!succ_test) {
    qCritical() << "the dynamic link library failed verification and may be at "
                   "risk of being tampered with: "
                << test;
  }

  return succ_core && succ_ui && succ_test;
}
