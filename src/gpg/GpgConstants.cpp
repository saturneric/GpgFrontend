/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
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

const char *GpgFrontend::GpgConstants::PGP_CRYPT_BEGIN =
    "-----BEGIN PGP MESSAGE-----";
const char *GpgFrontend::GpgConstants::PGP_CRYPT_END =
    "-----END PGP MESSAGE-----";
const char *GpgFrontend::GpgConstants::PGP_SIGNED_BEGIN =
    "-----BEGIN PGP SIGNED MESSAGE-----";
const char *GpgFrontend::GpgConstants::PGP_SIGNED_END =
    "-----END PGP SIGNATURE-----";
const char *GpgFrontend::GpgConstants::PGP_SIGNATURE_BEGIN =
    "-----BEGIN PGP SIGNATURE-----";
const char *GpgFrontend::GpgConstants::PGP_SIGNATURE_END =
    "-----END PGP SIGNATURE-----";
const char *GpgFrontend::GpgConstants::GPG_FRONTEND_SHORT_CRYPTO_HEAD =
    "GpgF_Scpt://";

gpgme_error_t GpgFrontend::check_gpg_error(gpgme_error_t err) {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    LOG(ERROR) << "[Error " << gpg_err_code(err)
               << "] Source: " << gpgme_strsource(err)
               << " Description: " << gpgme_strerror(err);
  }
  return err;
}

gpg_err_code_t GpgFrontend::check_gpg_error_2_err_code(gpgme_error_t err) {
  auto err_code = gpg_err_code(err);
  if (err_code != GPG_ERR_NO_ERROR) {
    LOG(ERROR) << "[Error " << gpg_err_code(err)
               << "] Source: " << gpgme_strsource(err)
               << " Description: " << gpgme_strerror(err);
  }
  return err_code;
}

// error-handling
gpgme_error_t GpgFrontend::check_gpg_error(gpgme_error_t err,
                                           const std::string &comment) {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    LOG(ERROR) << "[Error " << gpg_err_code(err)
               << "] Source: " << gpgme_strsource(err)
               << " Description: " << gpgme_strerror(err) << " "
               << comment.c_str();
  }
  return err;
}

std::string
GpgFrontend::beautify_fingerprint(GpgFrontend::BypeArrayRef fingerprint) {
  uint len = fingerprint.size();
  if ((len > 0) && (len % 4 == 0))
    for (uint n = 0; 4 * (n + 1) < len; ++n)
      fingerprint.insert(static_cast<int>(5u * n + 4u), " ");
  return fingerprint;
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

// trim from both ends (in place)
static inline std::string trim(std::string &s) {
  ltrim(s);
  rtrim(s);
  return s;
}

/*
 * isSigned returns:
 * - 0, if text isn't signed at all
 * - 1, if text is partially signed
 * - 2, if text is completly signed
 */
int GpgFrontend::text_is_signed(GpgFrontend::BypeArrayRef text) {
  if (trim(text).starts_with(GpgConstants::PGP_SIGNED_BEGIN) &&
      trim(text).ends_with(GpgConstants::PGP_SIGNED_END))
    return 2;
  else if (text.find(GpgConstants::PGP_SIGNED_BEGIN) != std::string::npos &&
           text.find(GpgConstants::PGP_SIGNED_END) != std::string::npos)
    return 1;

  else
    return 0;
}
