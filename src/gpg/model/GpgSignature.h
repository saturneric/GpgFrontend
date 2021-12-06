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

#ifndef GPGFRONTEND_GPGSIGNATURE_H
#define GPGFRONTEND_GPGSIGNATURE_H

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

#include "gpg/GpgConstants.h"

namespace GpgFrontend {
class GpgSignature {
 public:
  [[nodiscard]] gpgme_validity_t validity() const {
    return _signature_ref->validity;
  }

  [[nodiscard]] gpgme_error_t status() const { return _signature_ref->status; }

  [[nodiscard]] gpgme_error_t summary() const {
    return _signature_ref->summary;
  }

  [[nodiscard]] std::string pubkey_algo() const {
    return gpgme_pubkey_algo_name(_signature_ref->pubkey_algo);
  }

  [[nodiscard]] std::string hash_algo() const {
    return gpgme_hash_algo_name(_signature_ref->hash_algo);
  }

  [[nodiscard]] boost::gregorian::date create_time() const {
    return boost::posix_time::from_time_t(_signature_ref->timestamp).date();
  }
  [[nodiscard]] boost::gregorian::date expire_time() const {
    return boost::posix_time::from_time_t(_signature_ref->exp_timestamp).date();
  }

  [[nodiscard]] std::string fpr() const { return _signature_ref->fpr; }

  GpgSignature() = default;

  ~GpgSignature() = default;

  explicit GpgSignature(gpgme_signature_t sig);

  GpgSignature(GpgSignature &&) noexcept = default;

  GpgSignature(const GpgSignature &) = delete;

  GpgSignature &operator=(GpgSignature &&) noexcept = default;

  GpgSignature &operator=(const GpgSignature &) = delete;

 private:
  using KeySignatrueRefHandler =
      std::unique_ptr<struct _gpgme_signature,
                      std::function<void(gpgme_signature_t)>>;

  KeySignatrueRefHandler _signature_ref = nullptr;
};
}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGSIGNATURE_H
