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

#include "gpg/GpgGenKeyInfo.h"

#include <easyloggingpp/easylogging++.h>

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/gregorian/greg_duration.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <string>
#include <vector>

void GpgFrontend::GenKeyInfo::setAlgo(const std::string &m_algo) {
  LOG(INFO) << "set algo" << m_algo;
  // Check algo if supported
  std::string algo_args = std::string(m_algo);
  boost::algorithm::to_upper(algo_args);
  if (standalone_) {
    if (!subkey_) {
      auto support_algo = getSupportedKeyAlgoStandalone();
      auto it = std::find(support_algo.begin(), support_algo.end(), algo_args);
      // Algo Not Supported
      if (it == support_algo.end()) return;
    } else {
      auto support_algo = getSupportedSubkeyAlgoStandalone();
      auto it = std::find(support_algo.begin(), support_algo.end(), algo_args);
      // Algo Not Supported
      if (it == support_algo.end()) return;
    }
  } else {
    if (!subkey_) {
      auto support_algo = getSupportedKeyAlgo();
      auto it = std::find(support_algo.begin(), support_algo.end(), algo_args);
      // Algo Not Supported
      if (it == support_algo.end()) return;
    } else {
      auto support_algo = getSupportedSubkeyAlgo();
      auto it = std::find(support_algo.begin(), support_algo.end(), algo_args);
      // Algo Not Supported
      if (it == support_algo.end()) return;
    }
  }

  // reset all options
  reset_options();

  if (!this->subkey_) {
    this->setAllowCertification(true);
  } else {
    this->setAllowCertification(false);
  }

  this->allowChangeCertification = false;

  if (!standalone_) boost::algorithm::to_lower(algo_args);

  if (algo_args == "rsa") {
    /**
     * RSA is the world’s premier asymmetric cryptographic algorithm,
     * and is built on the difficulty of factoring extremely large composites.
     * GnuPG supports RSA with key sizes of between 1024 and 4096 bits.
     */
    suggest_min_key_size_ = 1024;
    suggest_max_key_size_ = 4096;
    suggest_size_addition_step_ = 1024;
    setKeySize(2048);

  } else if (algo_args == "dsa") {
    /**
     * Algorithm (DSA) as a government standard for digital signatures.
     * Originally, it supported key lengths between 512 and 1024 bits.
     * Recently, NIST has declared 512-bit keys obsolete:
     * now, DSA is available in 1024, 2048 and 3072-bit lengths.
     */
    setAllowEncryption(false);
    allowChangeEncryption = false;

    suggest_min_key_size_ = 1024;
    suggest_max_key_size_ = 3072;
    suggest_size_addition_step_ = 1024;
    setKeySize(2048);

  } else if (algo_args == "ed25519") {
    /**
     * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths
     * ranging from 1024 to 4096 bits.
     */
    setAllowEncryption(false);
    allowChangeEncryption = false;

    suggest_min_key_size_ = -1;
    suggest_max_key_size_ = -1;
    suggest_size_addition_step_ = -1;
    setKeySize(-1);
  } else if (algo_args == "elg") {
    /**
     * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths
     * ranging from 1024 to 4096 bits.
     */
    setAllowAuthentication(false);
    allowChangeAuthentication = false;

    setAllowSigning(false);
    allowChangeSigning = false;

    suggest_min_key_size_ = 1024;
    suggest_max_key_size_ = 4096;
    suggest_size_addition_step_ = 1024;
    setKeySize(2048);
  }
  this->algo_ = algo_args;
}

void GpgFrontend::GenKeyInfo::reset_options() {
  allowChangeEncryption = true;
  setAllowEncryption(true);

  allowChangeCertification = true;
  setAllowCertification(true);

  allowChangeSigning = true;
  setAllowSigning(true);

  allowChangeAuthentication = true;
  setAllowAuthentication(true);

  passphrase_.clear();
}

std::string GpgFrontend::GenKeyInfo::getKeySizeStr() const {
  if (key_size_ > 0) {
    return std::to_string(key_size_);
  } else {
    return {};
  }
}

void GpgFrontend::GenKeyInfo::setKeySize(int m_key_size) {
  if (m_key_size < suggest_min_key_size_ ||
      m_key_size > suggest_max_key_size_) {
    return;
  }
  GenKeyInfo::key_size_ = m_key_size;
}

void GpgFrontend::GenKeyInfo::setExpired(
    const boost::posix_time::ptime &m_expired) {
  using namespace boost::gregorian;
  if (!isNonExpired()) {
    GenKeyInfo::expired_ = m_expired;
  }
}

void GpgFrontend::GenKeyInfo::setNonExpired(bool m_non_expired) {
  using namespace boost::posix_time;
  if (!m_non_expired) this->expired_ = from_time_t(0);
  GenKeyInfo::non_expired_ = m_non_expired;
}

void GpgFrontend::GenKeyInfo::setAllowEncryption(bool m_allow_encryption) {
  if (allowChangeEncryption) GenKeyInfo::allowEncryption = m_allow_encryption;
}

void GpgFrontend::GenKeyInfo::setAllowCertification(
    bool m_allow_certification) {
  if (allowChangeCertification)
    GenKeyInfo::allowCertification = m_allow_certification;
}

GpgFrontend::GenKeyInfo::GenKeyInfo(bool m_is_sub_key, bool m_standalone)
    : standalone_(m_standalone), subkey_(m_is_sub_key) {
  setAlgo("rsa");
}

const std::vector<std::string> &GpgFrontend::GenKeyInfo::getSupportedKeyAlgo() {
  static const std::vector<std::string> support_key_algo = {"RSA", "DSA",
                                                            "ED25519"};
  return support_key_algo;
}

const std::vector<std::string>
    &GpgFrontend::GenKeyInfo::getSupportedSubkeyAlgo() {
  static const std::vector<std::string> support_subkey_algo = {"RSA", "DSA",
                                                               "ED25519"};
  return support_subkey_algo;
}

const std::vector<std::string>
    &GpgFrontend::GenKeyInfo::getSupportedKeyAlgoStandalone() {
  static const std::vector<std::string> support_subkey_algo_standalone = {
      "RSA", "DSA"};
  return support_subkey_algo_standalone;
}

const std::vector<std::string>
    &GpgFrontend::GenKeyInfo::getSupportedSubkeyAlgoStandalone() {
  static const std::vector<std::string> support_subkey_algo_standalone = {
      "RSA", "DSA", "ELG-E"};
  return support_subkey_algo_standalone;
}
