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
    LOG(ERROR) << "[Error " << gpg_err_code(err)
               << "] Source: " << gpgme_strsource(err)
               << " Description: " << gpgme_strerror(err);
  }
  return err;
}

gpg_err_code_t GpgFrontend::check_gpg_error_2_err_code(gpgme_error_t err,
                                                       gpgme_error_t predict) {
  auto err_code = gpg_err_code(err);
  if (err_code != predict) {
    LOG(ERROR) << "[Error " << gpg_err_code(err)
               << "] Source: " << gpgme_strsource(err)
               << " Description: " << gpgme_strerror(err);
  }
  return err_code;
}

// error-handling
gpgme_error_t GpgFrontend::check_gpg_error(gpgme_error_t err,
                                           const std::string& comment) {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    LOG(ERROR) << "[Error " << gpg_err_code(err)
               << "] Source: " << gpgme_strsource(err)
               << " Description: " << gpgme_strerror(err) << " " << comment;
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

std::string GpgFrontend::read_all_data_in_file(const std::string& path) {
  using namespace boost::filesystem;
  class path file_info(path.c_str());

  if (!exists(file_info) || !is_regular_file(path))
    throw std::runtime_error("no permission");

  std::ifstream in_file;
  in_file.open(path, std::ios::in);
  if (!in_file.good()) throw std::runtime_error("cannot open file");
  std::istreambuf_iterator<char> begin(in_file);
  std::istreambuf_iterator<char> end;
  std::string in_buffer(begin, end);
  in_file.close();
  return in_buffer;
}

bool GpgFrontend::write_buffer_to_file(const std::string& path,
                                       const std::string& out_buffer) {
  std::ofstream out_file(boost::filesystem::path(path).string(), std::ios::out);
  if (!out_file.good()) return false;
  out_file.write(out_buffer.c_str(), out_buffer.size());
  out_file.close();
  return true;
}

std::string GpgFrontend::get_file_extension(const std::string& path) {
  // Create a Path object from given string
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
  // Create a Path object from given string
  boost::filesystem::path path_obj(path);
  // Check if file name in the path object has extension
  if (path_obj.has_filename()) {
    // Fetch the extension from path object and return
    return (path_obj.parent_path() / path_obj.stem()).string();
  }
  // In case of no extension return empty string
  throw std::runtime_error("invalid file path");
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
