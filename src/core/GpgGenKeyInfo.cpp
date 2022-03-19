/**
 * Copyright (C) 2021 Saturneric
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "core/GpgGenKeyInfo.h"

#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/gregorian/greg_duration.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <string>
#include <vector>

void GpgFrontend::GenKeyInfo::SetAlgo(const std::string &m_algo) {
  LOG(INFO) << "set algo" << m_algo;
  // Check algo if supported
  std::string algo_args = std::string(m_algo);
  boost::algorithm::to_upper(algo_args);
  if (standalone_) {
    if (!subkey_) {
      auto support_algo = GetSupportedKeyAlgoStandalone();
      auto it = std::find(support_algo.begin(), support_algo.end(), algo_args);
      // Algo Not Supported
      if (it == support_algo.end()) return;
    } else {
      auto support_algo = GetSupportedSubkeyAlgoStandalone();
      auto it = std::find(support_algo.begin(), support_algo.end(), algo_args);
      // Algo Not Supported
      if (it == support_algo.end()) return;
    }
  } else {
    if (!subkey_) {
      auto support_algo = GetSupportedKeyAlgo();
      auto it = std::find(support_algo.begin(), support_algo.end(), algo_args);
      // Algo Not Supported
      if (it == support_algo.end()) return;
    } else {
      auto support_algo = GetSupportedSubkeyAlgo();
      auto it = std::find(support_algo.begin(), support_algo.end(), algo_args);
      // Algo Not Supported
      if (it == support_algo.end()) return;
    }
  }

  // reset all options
  reset_options();

  if (!this->subkey_) {
    this->SetAllowCertification(true);
  } else {
    this->SetAllowCertification(false);
  }

  this->allow_change_certification_ = false;

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
    SetKeyLength(2048);

  } else if (algo_args == "dsa") {
    /**
     * Algorithm (DSA) as a government standard for digital signatures.
     * Originally, it supported key lengths between 512 and 1024 bits.
     * Recently, NIST has declared 512-bit keys obsolete:
     * now, DSA is available in 1024, 2048 and 3072-bit lengths.
     */
    SetAllowEncryption(false);
    allow_change_encryption_ = false;

    suggest_min_key_size_ = 1024;
    suggest_max_key_size_ = 3072;
    suggest_size_addition_step_ = 1024;
    SetKeyLength(2048);

  } else if (algo_args == "ed25519") {
    /**
     * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths
     * ranging from 1024 to 4096 bits.
     */
    SetAllowEncryption(false);
    allow_change_encryption_ = false;

    suggest_min_key_size_ = -1;
    suggest_max_key_size_ = -1;
    suggest_size_addition_step_ = -1;
    SetKeyLength(-1);
  } else if (algo_args == "elg") {
    /**
     * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths
     * ranging from 1024 to 4096 bits.
     */
    SetAllowAuthentication(false);
    allow_change_authentication_ = false;

    SetAllowSigning(false);
    allow_change_signing_ = false;

    suggest_min_key_size_ = 1024;
    suggest_max_key_size_ = 4096;
    suggest_size_addition_step_ = 1024;
    SetKeyLength(2048);
  }
  this->algo_ = algo_args;
}

void GpgFrontend::GenKeyInfo::reset_options() {
  allow_change_encryption_ = true;
  SetAllowEncryption(true);

  allow_change_certification_ = true;
  SetAllowCertification(true);

  allow_change_signing_ = true;
  SetAllowSigning(true);

  allow_change_authentication_ = true;
  SetAllowAuthentication(true);

  passphrase_.clear();
}

std::string GpgFrontend::GenKeyInfo::GetKeySizeStr() const {
  if (key_size_ > 0) {
    return std::to_string(key_size_);
  } else {
    return {};
  }
}

void GpgFrontend::GenKeyInfo::SetKeyLength(int m_key_size) {
  if (m_key_size < suggest_min_key_size_ ||
      m_key_size > suggest_max_key_size_) {
    return;
  }
  GenKeyInfo::key_size_ = m_key_size;
}

void GpgFrontend::GenKeyInfo::SetExpireTime(
    const boost::posix_time::ptime &m_expired) {
  using namespace boost::gregorian;
  if (!IsNonExpired()) {
    GenKeyInfo::expired_ = m_expired;
  }
}

void GpgFrontend::GenKeyInfo::SetNonExpired(bool m_non_expired) {
  using namespace boost::posix_time;
  if (!m_non_expired) this->expired_ = from_time_t(0);
  GenKeyInfo::non_expired_ = m_non_expired;
}

void GpgFrontend::GenKeyInfo::SetAllowEncryption(bool m_allow_encryption) {
  if (allow_change_encryption_)
    GenKeyInfo::allow_encryption_ = m_allow_encryption;
}

void GpgFrontend::GenKeyInfo::SetAllowCertification(
    bool m_allow_certification) {
  if (allow_change_certification_)
    GenKeyInfo::allow_certification_ = m_allow_certification;
}

GpgFrontend::GenKeyInfo::GenKeyInfo(bool m_is_sub_key, bool m_standalone)
    : standalone_(m_standalone), subkey_(m_is_sub_key) {
  SetAlgo("rsa");
}

const std::vector<std::string> &GpgFrontend::GenKeyInfo::GetSupportedKeyAlgo() {
  static const std::vector<std::string> support_key_algo = {"RSA", "DSA",
                                                            "ED25519"};
  return support_key_algo;
}

const std::vector<std::string>
    &GpgFrontend::GenKeyInfo::GetSupportedSubkeyAlgo() {
  static const std::vector<std::string> support_subkey_algo = {"RSA", "DSA",
                                                               "ED25519"};
  return support_subkey_algo;
}

const std::vector<std::string>
    &GpgFrontend::GenKeyInfo::GetSupportedKeyAlgoStandalone() {
  static const std::vector<std::string> support_subkey_algo_standalone = {
      "RSA", "DSA"};
  return support_subkey_algo_standalone;
}

const std::vector<std::string>
    &GpgFrontend::GenKeyInfo::GetSupportedSubkeyAlgoStandalone() {
  static const std::vector<std::string> support_subkey_algo_standalone = {
      "RSA", "DSA", "ELG-E"};
  return support_subkey_algo_standalone;
}
