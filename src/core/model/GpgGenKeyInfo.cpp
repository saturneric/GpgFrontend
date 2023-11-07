/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "GpgGenKeyInfo.h"

#include <algorithm>
#include <boost/format.hpp>
#include <cassert>

namespace GpgFrontend {

void GenKeyInfo::SetAlgo(const GenKeyInfo::KeyGenAlgo &m_algo) {
  SPDLOG_DEBUG("set algo name: {}", m_algo.first);
  // Check algo if supported
  std::string algo_args = m_algo.second;
  if (standalone_) {
    if (!subkey_) {
      auto support_algo = GetSupportedKeyAlgoStandalone();
      auto algo_it = std::find_if(
          support_algo.begin(), support_algo.end(),
          [=](const KeyGenAlgo &o) { return o.second == algo_args; });
      // Algo Not Supported
      if (algo_it == support_algo.end()) return;
    } else {
      auto support_algo = GetSupportedSubkeyAlgoStandalone();
      auto algo_it = std::find_if(
          support_algo.begin(), support_algo.end(),
          [=](const KeyGenAlgo &o) { return o.second == algo_args; });
      // Algo Not Supported
      if (algo_it == support_algo.end()) return;
    }
  } else {
    if (!subkey_) {
      auto support_algo = GetSupportedKeyAlgo();
      auto algo_it = std::find_if(
          support_algo.begin(), support_algo.end(),
          [=](const KeyGenAlgo &o) { return o.second == algo_args; });
      // Algo Not Supported
      if (algo_it == support_algo.end()) return;
    } else {
      auto support_algo = GetSupportedSubkeyAlgo();
      auto algo_it = std::find_if(
          support_algo.begin(), support_algo.end(),
          [=](const KeyGenAlgo &o) { return o.second == algo_args; });
      // Algo Not Supported
      if (algo_it == support_algo.end()) return;
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
  } else if (algo_args == "cv25519") {
    SetAllowAuthentication(false);
    allow_change_authentication_ = false;

    SetAllowSigning(false);
    allow_change_signing_ = false;

    SetAllowCertification(false);
    allow_change_certification_ = false;

    suggest_min_key_size_ = 1024;
    suggest_max_key_size_ = 4096;
    suggest_size_addition_step_ = 1024;
    SetKeyLength(2048);
  } else if (algo_args == "nistp256" || algo_args == "nistp384" ||
             algo_args == "nistp521") {
    SetAllowAuthentication(false);
    allow_change_authentication_ = false;

    SetAllowSigning(false);
    allow_change_signing_ = false;

    SetAllowCertification(false);
    allow_change_certification_ = false;

    suggest_min_key_size_ = -1;
    suggest_max_key_size_ = -1;
    suggest_size_addition_step_ = -1;
    SetKeyLength(-1);
  } else if (algo_args == "brainpoolp256r1") {
    SetAllowAuthentication(false);
    allow_change_authentication_ = false;

    SetAllowSigning(false);
    allow_change_signing_ = false;

    SetAllowCertification(false);
    allow_change_certification_ = false;

    suggest_min_key_size_ = -1;
    suggest_max_key_size_ = -1;
    suggest_size_addition_step_ = -1;
    SetKeyLength(-1);
  }

  this->algo_ = algo_args;
}

void GenKeyInfo::reset_options() {
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

auto GenKeyInfo::GetKeySizeStr() const -> std::string {
  if (key_size_ > 0) {
    return std::to_string(key_size_);
  }
  return {};
}

void GenKeyInfo::SetKeyLength(int m_key_size) {
  if (m_key_size < suggest_min_key_size_ ||
      m_key_size > suggest_max_key_size_) {
    return;
  }
  GenKeyInfo::key_size_ = m_key_size;
}

void GenKeyInfo::SetExpireTime(const boost::posix_time::ptime &m_expired) {
  if (!IsNonExpired()) {
    GenKeyInfo::expired_ = m_expired;
  }
}

void GenKeyInfo::SetNonExpired(bool m_non_expired) {
  if (!m_non_expired) this->expired_ = boost::posix_time::from_time_t(0);
  GenKeyInfo::non_expired_ = m_non_expired;
}

void GenKeyInfo::SetAllowEncryption(bool m_allow_encryption) {
  if (allow_change_encryption_) {
    GenKeyInfo::allow_encryption_ = m_allow_encryption;
  }
}

void GenKeyInfo::SetAllowCertification(bool m_allow_certification) {
  if (allow_change_certification_) {
    GenKeyInfo::allow_certification_ = m_allow_certification;
  }
}

GenKeyInfo::GenKeyInfo(bool m_is_sub_key, bool m_standalone)
    : standalone_(m_standalone), subkey_(m_is_sub_key) {
  assert(!GetSupportedKeyAlgo().empty());
  SetAlgo(GetSupportedKeyAlgo()[0]);
}

auto GenKeyInfo::GetSupportedKeyAlgo()
    -> const std::vector<GenKeyInfo::KeyGenAlgo> & {
  static const std::vector<GenKeyInfo::KeyGenAlgo> kSupportKeyAlgo = {
      {"RSA", "RSA"},
      {"DSA", "DSA"},
      {"ECDSA", "ED25519"},
  };
  return kSupportKeyAlgo;
}

auto GenKeyInfo::GetSupportedSubkeyAlgo()
    -> const std::vector<GenKeyInfo::KeyGenAlgo> & {
  static const std::vector<GenKeyInfo::KeyGenAlgo> kSupportSubkeyAlgo = {
      {"RSA", "RSA"},
      {"DSA", "DSA"},
      {"ECDSA", "ED25519"},
      {"ECDH NIST P-256", "NISTP256"},
      {"ECDH NIST P-384", "NISTP384"},
      {"ECDH NIST P-521", "NISTP521"},
      // {"ECDH BrainPool P-256", "BRAINPOOlP256R1"}
  };
  return kSupportSubkeyAlgo;
}

auto GenKeyInfo::GetSupportedKeyAlgoStandalone()
    -> const std::vector<GenKeyInfo::KeyGenAlgo> & {
  static const std::vector<GenKeyInfo::KeyGenAlgo>
      kSupportSubkeyAlgoStandalone = {
          {"RSA", "RSA"},
          {"DSA", "DSA"},
      };
  return kSupportSubkeyAlgoStandalone;
}

auto GenKeyInfo::GetSupportedSubkeyAlgoStandalone()
    -> const std::vector<GenKeyInfo::KeyGenAlgo> & {
  static const std::vector<GenKeyInfo::KeyGenAlgo>
      kSupportSubkeyAlgoStandalone = {
          {"RSA", "RSA"},
          {"DSA", "DSA"},
      };
  return kSupportSubkeyAlgoStandalone;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsSubKey() const -> bool { return subkey_; }

/**
 * @brief Set the Is Sub Key object
 *
 * @param m_sub_key
 */
void GenKeyInfo::SetIsSubKey(bool m_sub_key) {
  GenKeyInfo::subkey_ = m_sub_key;
}

/**
 * @brief Get the Userid object
 *
 * @return std::string
 */
[[nodiscard]] auto GenKeyInfo::GetUserid() const -> std::string {
  auto uid_format = boost::format("%1%(%2%)<%3%>") % this->name_ %
                    this->comment_ % this->email_;
  return uid_format.str();
}

/**
 * @brief Set the Name object
 *
 * @param m_name
 */
void GenKeyInfo::SetName(const std::string &m_name) { this->name_ = m_name; }

/**
 * @brief Set the Email object
 *
 * @param m_email
 */
void GenKeyInfo::SetEmail(const std::string &m_email) {
  this->email_ = m_email;
}

/**
 * @brief Set the Comment object
 *
 * @param m_comment
 */
void GenKeyInfo::SetComment(const std::string &m_comment) {
  this->comment_ = m_comment;
}

/**
 * @brief Get the Name object
 *
 * @return std::string
 */
[[nodiscard]] auto GenKeyInfo::GetName() const -> std::string { return name_; }

/**
 * @brief Get the Email object
 *
 * @return std::string
 */
[[nodiscard]] auto GenKeyInfo::GetEmail() const -> std::string {
  return email_;
}

/**
 * @brief Get the Comment object
 *
 * @return std::string
 */
[[nodiscard]] auto GenKeyInfo::GetComment() const -> std::string {
  return comment_;
}

/**
 * @brief Get the Algo object
 *
 * @return const std::string&
 */
[[nodiscard]] auto GenKeyInfo::GetAlgo() const -> const std::string & {
  return algo_;
}

/**
 * @brief Get the Key Size object
 *
 * @return int
 */
[[nodiscard]] auto GenKeyInfo::GetKeyLength() const -> int { return key_size_; }

/**
 * @brief Get the Expired object
 *
 * @return const boost::posix_time::ptime&
 */
[[nodiscard]] auto GenKeyInfo::GetExpireTime() const
    -> const boost::posix_time::ptime & {
  return expired_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsNonExpired() const -> bool {
  return non_expired_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsNoPassPhrase() const -> bool {
  return this->no_passphrase_;
}

/**
 * @brief Set the Non Pass Phrase object
 *
 * @param m_non_pass_phrase
 */
void GenKeyInfo::SetNonPassPhrase(bool m_non_pass_phrase) {
  GenKeyInfo::no_passphrase_ = m_non_pass_phrase;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsAllowSigning() const -> bool {
  return allow_signing_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsAllowNoPassPhrase() const -> bool {
  return allow_no_pass_phrase_;
}

/**
 * @brief Set the Allow Signing object
 *
 * @param m_allow_signing
 */
void GenKeyInfo::SetAllowSigning(bool m_allow_signing) {
  if (allow_change_signing_) GenKeyInfo::allow_signing_ = m_allow_signing;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsAllowEncryption() const -> bool {
  return allow_encryption_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsAllowCertification() const -> bool {
  return allow_certification_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsAllowAuthentication() const -> bool {
  return allow_authentication_;
}

/**
 * @brief Set the Allow Authentication object
 *
 * @param m_allow_authentication
 */
void GenKeyInfo::SetAllowAuthentication(bool m_allow_authentication) {
  if (allow_change_authentication_) {
    GenKeyInfo::allow_authentication_ = m_allow_authentication;
  }
}

/**
 * @brief Get the Pass Phrase object
 *
 * @return const std::string&
 */
[[nodiscard]] auto GenKeyInfo::GetPassPhrase() const -> const std::string & {
  return passphrase_;
}

/**
 * @brief Set the Pass Phrase object
 *
 * @param m_pass_phrase
 */
void GenKeyInfo::SetPassPhrase(const std::string &m_pass_phrase) {
  GenKeyInfo::passphrase_ = m_pass_phrase;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsAllowChangeSigning() const -> bool {
  return allow_change_signing_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsAllowChangeEncryption() const -> bool {
  return allow_change_encryption_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsAllowChangeCertification() const -> bool {
  return allow_change_certification_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto GenKeyInfo::IsAllowChangeAuthentication() const -> bool {
  return allow_change_authentication_;
}

/**
 * @brief Get the Suggest Max Key Size object
 *
 * @return int
 */
[[nodiscard]] auto GenKeyInfo::GetSuggestMaxKeySize() const -> int {
  return suggest_max_key_size_;
}

/**
 * @brief Get the Suggest Min Key Size object
 *
 * @return int
 */
[[nodiscard]] auto GenKeyInfo::GetSuggestMinKeySize() const -> int {
  return suggest_min_key_size_;
}

/**
 * @brief Get the Size Change Step object
 *
 * @return int
 */
[[nodiscard]] auto GenKeyInfo::GetSizeChangeStep() const -> int {
  return suggest_size_addition_step_;
}

}  // namespace GpgFrontend
