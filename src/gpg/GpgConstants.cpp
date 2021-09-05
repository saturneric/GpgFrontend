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
  // if (gpgmeError != GPG_ERR_NO_ERROR && gpgmeError != GPG_ERR_CANCELED) {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    qDebug() << "[Error " << gpg_err_code(err)
             << "] Source: " << gpgme_strsource(err)
             << " Description: " << gpgme_strerror(err);
  }
  return err;
}

// error-handling
gpgme_error_t GpgFrontend::check_gpg_error(gpgme_error_t err,
                                           const std::string &comment) {
  // if (gpgmeError != GPG_ERR_NO_ERROR && gpgmeError != GPG_ERR_CANCELED) {
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR) {
    qDebug() << "[Error " << gpg_err_code(err)
             << "] Source: " << gpgme_strsource(err)
             << " Description: " << gpgme_strerror(err) << " "
             << comment.c_str();
  }
  return err;
}

std::string GpgFrontend::beautify_fingerprint(std::string fingerprint) {
  uint len = fingerprint.size();
  if ((len > 0) && (len % 4 == 0))
    for (uint n = 0; 4 * (n + 1) < len; ++n)
      fingerprint.insert(static_cast<int>(5u * n + 4u), " ");
  return fingerprint;
}

/*
 * isSigned returns:
 * - 0, if text isn't signed at all
 * - 1, if text is partially signed
 * - 2, if text is completly signed
 */
int GpgFrontend::text_is_signed(const QByteArray &text) {
  if (text.trimmed().startsWith(GpgConstants::PGP_SIGNED_BEGIN) &&
      text.trimmed().endsWith(GpgConstants::PGP_SIGNED_END))
    return 2;
  else if (text.contains(GpgConstants::PGP_SIGNED_BEGIN) &&
           text.contains(GpgConstants::PGP_SIGNED_END))
    return 1;

  else
    return 0;
}
