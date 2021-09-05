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

#include "gpg/result_analyse/EncryptResultAnalyse.h"

GpgFrontend::EncryptResultAnalyse::EncryptResultAnalyse(GpgError error,
                                                        GpgEncrResult result)
    : error(error), result(std::move(result)) {}

void GpgFrontend::EncryptResultAnalyse::do_analyse() {
  LOG(INFO) << "Start Encrypt Result Analyse";

  stream << "[#] Encrypt Operation ";

  if (gpgme_err_code(error) == GPG_ERR_NO_ERROR)
    stream << "[Success]" << std::endl;
  else {
    stream << "[Failed] " << gpgme_strerror(error) << std::endl;
    setStatus(-1);
  }

  if (!~status) {
    stream << "------------>" << std::endl;
    if (result != nullptr) {
      stream << "Invalid Recipients: " << std::endl;
      auto inv_reci = result->invalid_recipients;
      while (inv_reci != nullptr) {
        stream << "Fingerprint: " << inv_reci->fpr << std::endl;
        stream << "Reason: " << gpgme_strerror(inv_reci->reason) << std::endl;
        stream << std::endl;

        inv_reci = inv_reci->next;
      }
    }
    stream << "<------------" << std::endl;
  }

  stream << std::endl;
}
