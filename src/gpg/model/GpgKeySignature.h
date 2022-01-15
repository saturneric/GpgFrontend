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

#ifndef GPGFRONTEND_GPGKEYSIGNATURE_H
#define GPGFRONTEND_GPGKEYSIGNATURE_H

#include <boost/date_time.hpp>
#include <string>

#include "gpg/GpgConstants.h"

/**
 * @brief
 *
 */
namespace GpgFrontend {

/**
 * @brief
 *
 */
class GpgKeySignature {
 public:
  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsRevoked() const { return signature_ref_->revoked; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsExpired() const { return signature_ref_->expired; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsInvalid() const { return signature_ref_->invalid; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsExportable() const { return signature_ref_->exportable; }

  /**
   * @brief
   *
   * @return gpgme_error_t
   */
  [[nodiscard]] gpgme_error_t GetStatus() const {
    return signature_ref_->status;
  }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetKeyID() const { return signature_ref_->keyid; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetPubkeyAlgo() const {
    return gpgme_pubkey_algo_name(signature_ref_->pubkey_algo);
  }

  /**
   * @brief Create a time object
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetCreateTime() const {
    return boost::posix_time::from_time_t(signature_ref_->timestamp);
  }

  /**
   * @brief
   *
   * @return boost::posix_time::ptime
   */
  [[nodiscard]] boost::posix_time::ptime GetExpireTime() const {
    return boost::posix_time::from_time_t(signature_ref_->expires);
  }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetUID() const { return signature_ref_->uid; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetName() const { return signature_ref_->name; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetEmail() const { return signature_ref_->email; }

  /**
   * @brief
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetComment() const {
    return signature_ref_->comment;
  }

  /**
   * @brief Construct a new Gpg Key Signature object
   *
   */
  GpgKeySignature() = default;

  /**
   * @brief Destroy the Gpg Key Signature object
   *
   */
  ~GpgKeySignature() = default;

  /**
   * @brief Construct a new Gpg Key Signature object
   *
   * @param sig
   */
  explicit GpgKeySignature(gpgme_key_sig_t sig);

  /**
   * @brief Construct a new Gpg Key Signature object
   *
   */
  GpgKeySignature(GpgKeySignature &&) noexcept = default;

  /**
   * @brief Construct a new Gpg Key Signature object
   *
   */
  GpgKeySignature(const GpgKeySignature &) = delete;

  /**
   * @brief
   *
   * @return GpgKeySignature&
   */
  GpgKeySignature &operator=(GpgKeySignature &&) noexcept = default;

  /**
   * @brief
   *
   * @return GpgKeySignature&
   */
  GpgKeySignature &operator=(const GpgKeySignature &) = delete;

 private:
  using KeySignatrueRefHandler =
      std::unique_ptr<struct _gpgme_key_sig,
                      std::function<void(gpgme_key_sig_t)>>;  ///<

  KeySignatrueRefHandler signature_ref_ = nullptr;  ///<
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGKEYSIGNATURE_H
