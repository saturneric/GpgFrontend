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

#ifndef GPGFRONTEND_GPGGENKEYINFO_H
#define GPGFRONTEND_GPGGENKEYINFO_H

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/greg_duration_types.hpp>
#include <boost/format.hpp>
#include <string>
#include <vector>

#include "GpgFrontend.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT GenKeyInfo {
  bool standalone_ = false;  ///<
  bool subkey_ = false;      ///<
  std::string name_;         ///<
  std::string email_;        ///<
  std::string comment_;      ///<

  std::string algo_;  ///<
  int key_size_ = 2048;
  boost::posix_time::ptime expired_ =
      boost::posix_time::second_clock::local_time() +
      boost::gregorian::years(2);  ///<
  bool non_expired_ = false;       ///<

  bool no_passphrase_ = false;        ///<
  bool allow_no_pass_phrase_ = true;  ///<

  int suggest_max_key_size_ = 4096;        ///<
  int suggest_size_addition_step_ = 1024;  ///<
  int suggest_min_key_size_ = 1024;        ///<

  std::string passphrase_;  ///<

 public:
  /**
   * @brief Get the Supported Key Algo object
   *
   * @return const std::vector<std::string>&
   */
  static const std::vector<std::string> &GetSupportedKeyAlgo();

  /**
   * @brief Get the Supported Subkey Algo object
   *
   * @return const std::vector<std::string>&
   */
  static const std::vector<std::string> &GetSupportedSubkeyAlgo();

  /**
   * @brief Get the Supported Key Algo Standalone object
   *
   * @return const std::vector<std::string>&
   */
  static const std::vector<std::string> &GetSupportedKeyAlgoStandalone();

  /**
   * @brief Get the Supported Subkey Algo Standalone object
   *
   * @return const std::vector<std::string>&
   */
  static const std::vector<std::string> &GetSupportedSubkeyAlgoStandalone();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsSubKey() const { return subkey_; }

  /**
   * @brief Set the Is Sub Key object
   *
   * @param m_sub_key
   */
  void SetIsSubKey(bool m_sub_key) { GenKeyInfo::subkey_ = m_sub_key; }

  /**
   * @brief Get the Userid object
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetUserid() const {
    auto uid_format = boost::format("%1%(%2%)<%3%>") % this->name_ %
                      this->comment_ % this->email_;
    return uid_format.str();
  }

  /**
   * @brief Set the Name object
   *
   * @param m_name
   */
  void SetName(const std::string &m_name) { this->name_ = m_name; }

  /**
   * @brief Set the Email object
   *
   * @param m_email
   */
  void SetEmail(const std::string &m_email) { this->email_ = m_email; }

  /**
   * @brief Set the Comment object
   *
   * @param m_comment
   */
  void SetComment(const std::string &m_comment) { this->comment_ = m_comment; }

  /**
   * @brief Get the Name object
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetName() const { return name_; }

  /**
   * @brief Get the Email object
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetEmail() const { return email_; }

  /**
   * @brief Get the Comment object
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetComment() const { return comment_; }

  /**
   * @brief Get the Algo object
   *
   * @return const std::string&
   */
  [[nodiscard]] const std::string &GetAlgo() const { return algo_; }

  /**
   * @brief Set the Algo object
   *
   * @param m_algo
   */
  void SetAlgo(const std::string &m_algo);

  /**
   * @brief Get the Key Size Str object
   *
   * @return std::string
   */
  [[nodiscard]] std::string GetKeySizeStr() const;

  /**
   * @brief Get the Key Size object
   *
   * @return int
   */
  [[nodiscard]] int GetKeyLength() const { return key_size_; }

  /**
   * @brief Set the Key Size object
   *
   * @param m_key_size
   */
  void SetKeyLength(int m_key_size);

  /**
   * @brief Get the Expired object
   *
   * @return const boost::posix_time::ptime&
   */
  [[nodiscard]] const boost::posix_time::ptime &GetExpireTime() const {
    return expired_;
  }

  /**
   * @brief Set the Expired object
   *
   * @param m_expired
   */
  void SetExpireTime(const boost::posix_time::ptime &m_expired);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsNonExpired() const { return non_expired_; }

  /**
   * @brief Set the Non Expired object
   *
   * @param m_non_expired
   */
  void SetNonExpired(bool m_non_expired);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsNoPassPhrase() const { return this->no_passphrase_; }

  /**
   * @brief Set the Non Pass Phrase object
   *
   * @param m_non_pass_phrase
   */
  void SetNonPassPhrase(bool m_non_pass_phrase) {
    GenKeyInfo::no_passphrase_ = m_non_pass_phrase;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsAllowSigning() const { return allow_signing_; }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsAllowNoPassPhrase() const {
    return allow_no_pass_phrase_;
  }

  /**
   * @brief Set the Allow Signing object
   *
   * @param m_allow_signing
   */
  void SetAllowSigning(bool m_allow_signing) {
    if (allow_change_signing_) GenKeyInfo::allow_signing_ = m_allow_signing;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsAllowEncryption() const { return allow_encryption_; }

  /**
   * @brief Set the Allow Encryption object
   *
   * @param m_allow_encryption
   */
  void SetAllowEncryption(bool m_allow_encryption);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsAllowCertification() const {
    return allow_certification_;
  }

  /**
   * @brief Set the Allow Certification object
   *
   * @param m_allow_certification
   */
  void SetAllowCertification(bool m_allow_certification);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsAllowAuthentication() const {
    return allow_authentication_;
  }

  /**
   * @brief Set the Allow Authentication object
   *
   * @param m_allow_authentication
   */
  void SetAllowAuthentication(bool m_allow_authentication) {
    if (allow_change_authentication_)
      GenKeyInfo::allow_authentication_ = m_allow_authentication;
  }

  /**
   * @brief Get the Pass Phrase object
   *
   * @return const std::string&
   */
  [[nodiscard]] const std::string &GetPassPhrase() const { return passphrase_; }

  /**
   * @brief Set the Pass Phrase object
   *
   * @param m_pass_phrase
   */
  void SetPassPhrase(const std::string &m_pass_phrase) {
    GenKeyInfo::passphrase_ = m_pass_phrase;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsAllowChangeSigning() const {
    return allow_change_signing_;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsAllowChangeEncryption() const {
    return allow_change_encryption_;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsAllowChangeCertification() const {
    return allow_change_certification_;
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] bool IsAllowChangeAuthentication() const {
    return allow_change_authentication_;
  }

  /**
   * @brief Get the Suggest Max Key Size object
   *
   * @return int
   */
  [[nodiscard]] int GetSuggestMaxKeySize() const {
    return suggest_max_key_size_;
  }

  /**
   * @brief Get the Suggest Min Key Size object
   *
   * @return int
   */
  [[nodiscard]] int GetSuggestMinKeySize() const {
    return suggest_min_key_size_;
  }

  /**
   * @brief Get the Size Change Step object
   *
   * @return int
   */
  [[nodiscard]] int GetSizeChangeStep() const {
    return suggest_size_addition_step_;
  }

 private:
  bool allow_encryption_ = true;             ///<
  bool allow_change_encryption_ = true;      ///<
  bool allow_certification_ = true;          ///<
  bool allow_change_certification_ = true;   ///<
  bool allow_authentication_ = true;         ///<
  bool allow_change_authentication_ = true;  ///<
  bool allow_signing_ = true;                ///<
  bool allow_change_signing_ = true;         ///<

  /**
   * @brief
   *
   */
  void reset_options();

 public:
  /**
   * @brief Construct a new Gen Key Info object
   *
   * @param m_is_sub_key
   * @param m_standalone
   */
  explicit GenKeyInfo(bool m_is_sub_key = false, bool m_standalone = false);
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGGENKEYINFO_H
