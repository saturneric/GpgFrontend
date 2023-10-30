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

#include "core/GpgConstants.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>

#include "core/function/FileOperator.h"

const char* GpgFrontend::GpgConstants::PGP_CRYPT_BEGIN =
    "-----BEGIN PGP MESSAGE-----";  ///<
const char* GpgFrontend::GpgConstants::PGP_CRYPT_END =
    "-----END PGP MESSAGE-----";  ///<
const char* GpgFrontend::GpgConstants::PGP_SIGNED_BEGIN =
    "-----BEGIN PGP SIGNED MESSAGE-----";  ///<
const char* GpgFrontend::GpgConstants::PGP_SIGNED_END =
    "-----END PGP SIGNATURE-----";  ///<
const char* GpgFrontend::GpgConstants::PGP_SIGNATURE_BEGIN =
    "-----BEGIN PGP SIGNATURE-----";  ///<
const char* GpgFrontend::GpgConstants::PGP_SIGNATURE_END =
    "-----END PGP SIGNATURE-----";  ///<
const char* GpgFrontend::GpgConstants::PGP_PUBLIC_KEY_BEGIN =
    "-----BEGIN PGP PUBLIC KEY BLOCK-----";  ///<
const char* GpgFrontend::GpgConstants::PGP_PRIVATE_KEY_BEGIN =
    "-----BEGIN PGP PRIVATE KEY BLOCK-----";  ///<
const char* GpgFrontend::GpgConstants::GPG_FRONTEND_SHORT_CRYPTO_HEAD =
    "GpgF_Scpt://";  ///<

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

auto GpgFrontend::CheckGpgError(gpgme_error_t err) -> gpgme_error_t {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    SPDLOG_ERROR("[error: {}] source: {} description: {}", gpg_err_code(err),
                 gpgme_strsource(err), gpgme_strerror(err));
  }
  return err;
}

auto GpgFrontend::CheckGpgError2ErrCode(gpgme_error_t err,
                                        gpgme_error_t predict)
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

auto GpgFrontend::CheckGpgError(gpgme_error_t err, const std::string& comment)
    -> gpgme_error_t {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    SPDLOG_WARN("[Error {}] Source: {} description: {} predict: {}",
                gpg_err_code(err), gpgme_strsource(err), gpgme_strerror(err),
                gpgme_strerror(err));
  }
  return err;
}

auto GpgFrontend::BeautifyFingerprint(
    GpgFrontend::BypeArrayConstRef fingerprint) -> std::string {
  auto len = fingerprint.size();
  std::stringstream out;
  decltype(len) count = 0;
  while (count < len) {
    if ((count != 0U) && !(count % 5)) out << " ";
    out << fingerprint[count];
    count++;
  }
  return out.str();
}

auto GpgFrontend::ReadAllDataInFile(const std::string& utf8_path)
    -> std::string {
  std::string data;
  FileOperator::ReadFileStd(utf8_path, data);
  return data;
}

auto GpgFrontend::WriteBufferToFile(const std::string& utf8_path,
                                    const std::string& out_buffer) -> bool {
  return FileOperator::WriteFileStd(utf8_path, out_buffer);
}

auto GpgFrontend::GetFileExtension(const std::string& path) -> std::string {
  // Create a path object from given string
  std::filesystem::path path_obj(path);

  // Check if file name in the path object has extension
  if (path_obj.has_extension()) {
    // Fetch the extension from path object and return
    return path_obj.extension().u8string();
  }
  // In case of no extension return empty string
  return {};
}

auto GpgFrontend::GetOnlyFileNameWithPath(const std::string& path)
    -> std::string {
  // Create a path object from given string
  std::filesystem::path path_obj(path);
  // Check if file name in the path object has extension
  if (path_obj.has_filename()) {
    // Fetch the extension from path object and return
    return (path_obj.parent_path() / path_obj.stem()).u8string();
  }
  // In case of no extension return empty string
  return {};
}

auto GpgFrontend::TextIsSigned(GpgFrontend::BypeArrayRef text) -> int {
  using boost::algorithm::ends_with;
  using boost::algorithm::starts_with;

  auto trim_text = Trim(text);
  if (starts_with(trim_text, GpgConstants::PGP_SIGNED_BEGIN) &&
      ends_with(trim_text, GpgConstants::PGP_SIGNED_END)) {
    return 2;
  }
  if (text.find(GpgConstants::PGP_SIGNED_BEGIN) != std::string::npos &&
      text.find(GpgConstants::PGP_SIGNED_END) != std::string::npos) {
    return 1;
  }
  return 0;
}

auto GpgFrontend::NewResult(gpgme_encrypt_result_t&& result)
    -> GpgFrontend::GpgEncrResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

auto GpgFrontend::NewResult(gpgme_decrypt_result_t&& result)
    -> GpgFrontend::GpgDecrResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

auto GpgFrontend::NewResult(gpgme_sign_result_t&& result)
    -> GpgFrontend::GpgSignResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

auto GpgFrontend::NewResult(gpgme_verify_result_t&& result)
    -> GpgFrontend::GpgVerifyResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

auto GpgFrontend::NewResult(gpgme_genkey_result_t&& result)
    -> GpgFrontend::GpgGenKeyResult {
  gpgme_result_ref(result);
  return {result, ResultRefDeletor()};
}

void GpgFrontend::ResultRefDeletor::operator()(void* _result) {
  SPDLOG_TRACE("gpgme unref {}", _result);
  if (_result != nullptr) gpgme_result_unref(_result);
}

auto GpgFrontend::CompareSoftwareVersion(const std::string& a,
                                         const std::string& b) -> int {
  auto remove_prefix = [](const std::string& version) {
    return version.front() == 'v' ? version.substr(1) : version;
  };

  std::string real_version_a = remove_prefix(a);
  std::string real_version_b = remove_prefix(b);

  std::vector<std::string> split_a;
  std::vector<std::string> split_b;
  boost::split(split_a, real_version_a, boost::is_any_of("."));
  boost::split(split_b, real_version_b, boost::is_any_of("."));

  const auto min_depth = std::min(split_a.size(), split_b.size());

  for (auto i = 0U; i < min_depth; ++i) {
    int num_a = 0;
    int num_b = 0;

    try {
      num_a = boost::lexical_cast<int>(split_a[i]);
      num_b = boost::lexical_cast<int>(split_b[i]);
    } catch (boost::bad_lexical_cast&) {
      // Handle exception if needed
      return 0;
    }

    if (num_a != num_b) {
      return (num_a > num_b) ? 1 : -1;
    }
  }

  if (split_a.size() != split_b.size()) {
    return (split_a.size() > split_b.size()) ? 1 : -1;
  }

  return 0;
}
