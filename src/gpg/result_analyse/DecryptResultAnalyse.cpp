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

#include "gpg/result_analyse/DecryptResultAnalyse.h"

#include "gpg/function/GpgKeyGetter.h"

GpgFrontend::DecryptResultAnalyse::DecryptResultAnalyse(GpgError error,
                                                        GpgDecrResult result)
    : error(error), result(std::move(result)) {}

void GpgFrontend::DecryptResultAnalyse::do_analyse() {
  stream << "[#]" << _("Decrypt Operation");

  if (gpgme_err_code(error) == GPG_ERR_NO_ERROR) {
    stream << "[" << _("Success") << "]" << std::endl;
  } else {
    stream << "[" << _("Failed") << "] " << gpgme_strerror(error) << std::endl;
    setStatus(-1);
    if (result != nullptr && result->unsupported_algorithm != nullptr) {
      stream << "------------>" << std::endl;
      stream << _("Unsupported Algo") << ": " << result->unsupported_algorithm
             << std::endl;
    }
  }

  if (result != nullptr && result->recipients != nullptr) {
    stream << "------------>" << std::endl;
    if (result->file_name != nullptr) {
      stream << _("File Name") << ": " << result->file_name << std::endl;
      stream << std::endl;
    }
    if (result->is_mime) {
      stream << _("MIME") << ": " << _("true") << std::endl;
    }

    auto reci = result->recipients;
    if (reci != nullptr) stream << _("Recipient(s)") << ": " << std::endl;
    while (reci != nullptr) {
      print_reci(stream, reci);
      reci = reci->next;
    }
    stream << "<------------" << std::endl;
  }

  stream << std::endl;
}

bool GpgFrontend::DecryptResultAnalyse::print_reci(std::stringstream &stream,
                                                   gpgme_recipient_t reci) {
  bool keyFound = true;
  stream << "  {>} " << _("Recipient") << ": ";

  auto key = GpgFrontend::GpgKeyGetter::GetInstance().GetKey(reci->keyid);
  if (key.good()) {
    stream << key.name().c_str();
    if (!key.email().empty()) {
      stream << "<" << key.email().c_str() << ">";
    }
  } else {
    stream << "<" << _("Unknown") << ">";
    setStatus(0);
    keyFound = false;
  }

  stream << std::endl;

  stream << "      " << _("Keu ID") << ": " << reci->keyid << std::endl;
  stream << "      " << _("Public Algo") << ": "
         << gpgme_pubkey_algo_name(reci->pubkey_algo) << std::endl;

  return keyFound;
}
