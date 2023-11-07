/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "GpgUtils.h"

#include <boost/algorithm/string.hpp>

#include "core/utils/IOUtils.h"

namespace GpgFrontend {

static inline void Ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

static inline void Rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

static inline auto Trim(std::string& s) -> std::string {
  Ltrim(s);
  Rtrim(s);
  return s;
}

auto CheckGpgError(gpgme_error_t err) -> gpgme_error_t {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    SPDLOG_ERROR("[error: {}] source: {} description: {}", gpg_err_code(err),
                 gpgme_strsource(err), gpgme_strerror(err));
  }
  return err;
}

auto CheckGpgError2ErrCode(gpgme_error_t err, gpgme_error_t predict)
    -> gpg_err_code_t {
  auto err_code = gpg_err_code(err);
  if (err_code != gpg_err_code(predict)) {
    if (err_code == GPG_ERR_NO_ERROR)
      SPDLOG_WARN("[Warning {}] Source: {} description: {} predict: {}",
                  gpg_err_code(err), gpgme_strsource(err), gpgme_strerror(err),
                  gpgme_strerror(err));
    else
      SPDLOG_ERROR("[Error {}] Source: {} description: {} predict: {}",
                   gpg_err_code(err), gpgme_strsource(err), gpgme_strerror(err),
                   gpgme_strerror(err));
  }
  return err_code;
}

auto CheckGpgError(gpgme_error_t err, const std::string& comment)
    -> gpgme_error_t {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    SPDLOG_WARN("[Error {}] Source: {} description: {} predict: {}",
                gpg_err_code(err), gpgme_strsource(err), gpgme_strerror(err),
                gpgme_strerror(err));
  }
  return err;
}

auto TextIsSigned(BypeArrayRef text) -> int {
  using boost::algorithm::ends_with;
  using boost::algorithm::starts_with;

  auto trim_text = Trim(text);
  if (starts_with(trim_text, PGP_SIGNED_BEGIN) &&
      ends_with(trim_text, PGP_SIGNED_END)) {
    return 2;
  }
  if (text.find(PGP_SIGNED_BEGIN) != std::string::npos &&
      text.find(PGP_SIGNED_END) != std::string::npos) {
    return 1;
  }
  return 0;
}

auto NewResult(gpgme_encrypt_result_t&& result) -> GpgEncrResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

auto NewResult(gpgme_decrypt_result_t&& result) -> GpgDecrResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

auto NewResult(gpgme_sign_result_t&& result) -> GpgSignResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

auto NewResult(gpgme_verify_result_t&& result) -> GpgVerifyResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

auto NewResult(gpgme_genkey_result_t&& result) -> GpgGenKeyResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

void ResultRefDeletor::operator()(void* _result) {
  SPDLOG_TRACE("gpgme unref {}", _result);
  if (_result != nullptr) gpgme_result_unref(_result);
}
}  // namespace GpgFrontend
