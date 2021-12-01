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

const std::vector<std::string> GpgFrontend::GenKeyInfo::SupportedKeyAlgo = {
    "RSA", "DSA", "ED25519"};

const std::vector<std::string> GpgFrontend::GenKeyInfo::SupportedSubkeyAlgo = {
    "RSA", "DSA", "ED25519", "ELG"};

void GpgFrontend::GenKeyInfo::setAlgo(const std::string &m_algo) {
  LOG(INFO) << "GpgFrontend::GenKeyInfo::setAlgo m_algo" << m_algo;

  reset_options();

  if (!this->subKey) {
    this->setAllowCertification(true);
  } else {
    this->setAllowCertification(false);
  }

  this->allowChangeCertification = false;

  std::string lower_algo = std::string(m_algo);
  boost::algorithm::to_lower(lower_algo);

  LOG(INFO) << "GpgFrontend::GenKeyInfo::setAlgo lower_algo" << lower_algo;

  if (lower_algo == "rsa") {
    /**
     * RSA is the world’s premier asymmetric cryptographic algorithm,
     * and is built on the difficulty of factoring extremely large composites.
     * GnuPG supports RSA with key sizes of between 1024 and 4096 bits.
     */
    suggestMinKeySize = 1024;
    suggestMaxKeySize = 4096;
    suggestSizeAdditionStep = 1024;
    setKeySize(2048);

  } else if (lower_algo == "dsa") {
    /**
     * Algorithm (DSA) as a government standard for digital signatures.
     * Originally, it supported key lengths between 512 and 1024 bits.
     * Recently, NIST has declared 512-bit keys obsolete:
     * now, DSA is available in 1024, 2048 and 3072-bit lengths.
     */
    setAllowEncryption(false);
    allowChangeEncryption = false;

    suggestMinKeySize = 1024;
    suggestMaxKeySize = 3072;
    suggestSizeAdditionStep = 1024;
    setKeySize(2048);

  } else if (lower_algo == "ed25519") {
    /**
     * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths
     * ranging from 1024 to 4096 bits.
     */

    setAllowEncryption(false);
    allowChangeEncryption = false;

    suggestMinKeySize = -1;
    suggestMaxKeySize = -1;
    suggestSizeAdditionStep = -1;
    setKeySize(-1);
  } else if (lower_algo == "elg") {
    /**
     * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths
     * ranging from 1024 to 4096 bits.
     */

    setAllowAuthentication(false);
    allowChangeAuthentication = false;

    setAllowSigning(false);
    allowChangeSigning = false;

    suggestMinKeySize = 1024;
    suggestMaxKeySize = 4096;
    suggestSizeAdditionStep = 1024;
    setKeySize(2048);
  }
  this->algo = lower_algo;
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

  passPhrase.clear();
}

std::string GpgFrontend::GenKeyInfo::getKeySizeStr() const {
  if (keySize > 0) {
    return std::to_string(keySize);
  } else {
    return {};
  }
}

void GpgFrontend::GenKeyInfo::setKeySize(int m_key_size) {
  if (m_key_size < suggestMinKeySize || m_key_size > suggestMaxKeySize) {
    return;
  }
  GenKeyInfo::keySize = m_key_size;
}

void GpgFrontend::GenKeyInfo::setExpired(
    const boost::gregorian::date &m_expired) {
  using namespace boost::gregorian;
  auto current = day_clock::local_day();
  if (isNonExpired() && m_expired < current + years(2)) {
    GenKeyInfo::expired = m_expired;
  }
}

void GpgFrontend::GenKeyInfo::setNonExpired(bool m_non_expired) {
  using namespace boost::posix_time;
  if (!m_non_expired) this->expired = from_time_t(0).date();
  GenKeyInfo::nonExpired = m_non_expired;
}

void GpgFrontend::GenKeyInfo::setAllowEncryption(bool m_allow_encryption) {
  if (allowChangeEncryption) GenKeyInfo::allowEncryption = m_allow_encryption;
}

void GpgFrontend::GenKeyInfo::setAllowCertification(
    bool m_allow_certification) {
  if (allowChangeCertification)
    GenKeyInfo::allowCertification = m_allow_certification;
}
