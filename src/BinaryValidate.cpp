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

#if (defined(__APPLE__) && defined(__MACH__)) || defined(__linux__)

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

#elif defined(_WIN32) || defined(WIN32)

#include <windows.h>

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

auto ValidateLibrary(void *symbol_address, EVP_PKEY *key)
    -> std::tuple<QString, bool> {
  auto lib_path = GetLoadedLibraryPath(symbol_address);

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

}  // namespace

auto ValidateLibraries() -> bool {
  auto *pub_key = LoadEmbeddedPublicKey();
  if (pub_key == nullptr) {
    qCritical() << "unable to obtain public key for binary signing";
    return false;
  }

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

auto EnforceBinaryValidation() -> bool {
  return QString::fromUtf8(ENFORCE_BINARY_VALIDATION).toInt() == 1;
}