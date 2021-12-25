/**
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#include "gpg/GpgConstants.h"

#include <gpg-error.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <string>

const char* GpgFrontend::GpgConstants::PGP_CRYPT_BEGIN =
    "-----BEGIN PGP MESSAGE-----";
const char* GpgFrontend::GpgConstants::PGP_CRYPT_END =
    "-----END PGP MESSAGE-----";
const char* GpgFrontend::GpgConstants::PGP_SIGNED_BEGIN =
    "-----BEGIN PGP SIGNED MESSAGE-----";
const char* GpgFrontend::GpgConstants::PGP_SIGNED_END =
    "-----END PGP SIGNATURE-----";
const char* GpgFrontend::GpgConstants::PGP_SIGNATURE_BEGIN =
    "-----BEGIN PGP SIGNATURE-----";
const char* GpgFrontend::GpgConstants::PGP_SIGNATURE_END =
    "-----END PGP SIGNATURE-----";
const char* GpgFrontend::GpgConstants::GPG_FRONTEND_SHORT_CRYPTO_HEAD =
    "GpgF_Scpt://";

gpgme_error_t GpgFrontend::check_gpg_error(gpgme_error_t err) {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    LOG(ERROR) << "[" << _("Error") << " " << gpg_err_code(err) << "] "
               << _("Source: ") << gpgme_strsource(err) << " "
               << _("Description: ") << gpgme_strerror(err);
  }
  return err;
}

gpg_err_code_t GpgFrontend::check_gpg_error_2_err_code(gpgme_error_t err,
                                                       gpgme_error_t predict) {
  auto err_code = gpg_err_code(err);
  if (err_code != predict) {
    LOG(ERROR) << "[" << _("Error") << " " << gpg_err_code(err) << "] "
               << _("Source: ") << gpgme_strsource(err) << " "
               << _("Description: ") << gpgme_strerror(err);
  }
  return err_code;
}

// error-handling
gpgme_error_t GpgFrontend::check_gpg_error(gpgme_error_t err,
                                           const std::string& comment) {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    LOG(ERROR) << "[" << _("Error") << " " << gpg_err_code(err) << "] "
               << _("Source: ") << gpgme_strsource(err) << " "
               << _("Description: ") << gpgme_strerror(err);
  }
  return err;
}

std::string GpgFrontend::beautify_fingerprint(
    GpgFrontend::BypeArrayConstRef fingerprint) {
  auto len = fingerprint.size();
  std::stringstream out;
  decltype(len) count = 0;
  while (count < len) {
    if (count && !(count % 5)) out << " ";
    out << fingerprint[count];
    count++;
  }
  return out.str();
}

// trim from start (in place)
static inline void ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

// trim from both ends (in place)
static inline std::string trim(std::string& s) {
  ltrim(s);
  rtrim(s);
  return s;
}

std::string GpgFrontend::read_all_data_in_file(const std::string& utf8_path) {
  using namespace boost::filesystem;
  class path file_info(utf8_path.c_str());
  if (!exists(file_info) || !is_regular_file(file_info)) return {};
  std::ifstream in_file;
#ifndef WINDOWS
  in_file.open(file_info.string(), std::ios::in);
#else
  in_file.open(file_info.wstring().c_str(), std::ios::in);
#endif
  if (!in_file.good()) return {};
  std::istreambuf_iterator<char> begin(in_file);
  std::istreambuf_iterator<char> end;
  std::string in_buffer(begin, end);
  in_file.close();
  return in_buffer;
}

bool GpgFrontend::write_buffer_to_file(const std::string& utf8_path,
                                       const std::string& out_buffer) {
  using namespace boost::filesystem;
  class path file_info(utf8_path.c_str());
#ifndef WINDOWS
  std::ofstream out_file(file_info.string(), std::ios::out | std::ios::trunc);
#else
  std::ofstream out_file(file_info.wstring().c_str(),
                         std::ios::out | std::ios::trunc);
#endif
  if (!out_file.good()) return false;
  out_file.write(out_buffer.c_str(), out_buffer.size());
  out_file.close();
  return true;
}

std::string GpgFrontend::get_file_extension(const std::string& path) {
  // Create a path object from given string
  boost::filesystem::path path_obj(path);

  // Check if file name in the path object has extension
  if (path_obj.has_extension()) {
    // Fetch the extension from path object and return
    return path_obj.extension().string();
  }
  // In case of no extension return empty string
  return {};
}

std::string GpgFrontend::get_only_file_name_with_path(const std::string& path) {
  // Create a path object from given string
  boost::filesystem::path path_obj(path);
  // Check if file name in the path object has extension
  if (path_obj.has_filename()) {
    // Fetch the extension from path object and return
    return (path_obj.parent_path() / path_obj.stem()).string();
  }
  // In case of no extension return empty string
  return {};
}

/*
 * isSigned returns:
 * - 0, if text isn't signed at all
 * - 1, if text is partially signed
 * - 2, if text is completly signed
 */
int GpgFrontend::text_is_signed(GpgFrontend::BypeArrayRef text) {
  using boost::algorithm::ends_with;
  using boost::algorithm::starts_with;

  auto trim_text = trim(text);
  if (starts_with(trim_text, GpgConstants::PGP_SIGNED_BEGIN) &&
      ends_with(trim_text, GpgConstants::PGP_SIGNED_END))
    return 2;
  else if (text.find(GpgConstants::PGP_SIGNED_BEGIN) != std::string::npos &&
           text.find(GpgConstants::PGP_SIGNED_END) != std::string::npos)
    return 1;
  else
    return 0;
}

GpgFrontend::GpgEncrResult GpgFrontend::_new_result(
    gpgme_encrypt_result_t&& result) {
  gpgme_result_ref(result);
  return {result, _result_ref_deletor()};
}

GpgFrontend::GpgDecrResult GpgFrontend::_new_result(
    gpgme_decrypt_result_t&& result) {
  gpgme_result_ref(result);
  return {result, _result_ref_deletor()};
}

GpgFrontend::GpgSignResult GpgFrontend::_new_result(
    gpgme_sign_result_t&& result) {
  gpgme_result_ref(result);
  return {result, _result_ref_deletor()};
}

GpgFrontend::GpgVerifyResult GpgFrontend::_new_result(
    gpgme_verify_result_t&& result) {
  gpgme_result_ref(result);
  return {result, _result_ref_deletor()};
}

GpgFrontend::GpgGenKeyResult GpgFrontend::_new_result(
    gpgme_genkey_result_t&& result) {
  gpgme_result_ref(result);
  return {result, _result_ref_deletor()};
}

void GpgFrontend::_result_ref_deletor::operator()(void* _result) {
  DLOG(INFO) << _("Called") << _result;
  if (_result != nullptr) gpgme_result_unref(_result);
}
