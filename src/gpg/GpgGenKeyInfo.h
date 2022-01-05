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

#ifndef GPGFRONTEND_GPGGENKEYINFO_H
#define GPGFRONTEND_GPGGENKEYINFO_H

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/greg_duration_types.hpp>
#include <boost/format.hpp>
#include <string>
#include <vector>

namespace GpgFrontend {

class GenKeyInfo {
  bool standalone_ = false;
  bool subkey_ = false;
  std::string name_;
  std::string email_;
  std::string comment_;

  std::string algo_;
  int key_size_ = 2048;
  boost::posix_time::ptime expired_ =
      boost::posix_time::second_clock::local_time() +
      boost::gregorian::years(2);
  bool non_expired_ = false;

  bool no_passphrase_ = false;
  bool allow_no_pass_phrase_ = true;

  int suggest_max_key_size_ = 4096;
  int suggest_size_addition_step_ = 1024;
  int suggest_min_key_size_ = 1024;

  std::string passphrase_;

 public:
  static const std::vector<std::string> &getSupportedKeyAlgo();

  static const std::vector<std::string> &getSupportedSubkeyAlgo();

  static const std::vector<std::string> &getSupportedKeyAlgoStandalone();

  static const std::vector<std::string> &getSupportedSubkeyAlgoStandalone();

  [[nodiscard]] bool isSubKey() const { return subkey_; }

  void setIsSubKey(bool m_sub_key) { GenKeyInfo::subkey_ = m_sub_key; }

  [[nodiscard]] std::string getUserid() const {
    auto uid_format = boost::format("%1%(%2%)<%3%>") % this->name_ %
                      this->comment_ % this->email_;
    return uid_format.str();
  }

  void setName(const std::string &m_name) { this->name_ = m_name; }

  void setEmail(const std::string &m_email) { this->email_ = m_email; }

  void setComment(const std::string &m_comment) { this->comment_ = m_comment; }

  [[nodiscard]] std::string getName() const { return name_; }

  [[nodiscard]] std::string getEmail() const { return email_; }

  [[nodiscard]] std::string getComment() const { return comment_; }

  [[nodiscard]] const std::string &getAlgo() const { return algo_; }

  void setAlgo(const std::string &m_algo);

  [[nodiscard]] std::string getKeySizeStr() const;

  [[nodiscard]] int getKeySize() const { return key_size_; }

  void setKeySize(int m_key_size);

  [[nodiscard]] const boost::posix_time::ptime &getExpired() const {
    return expired_;
  }

  void setExpired(const boost::posix_time::ptime &m_expired);

  [[nodiscard]] bool isNonExpired() const { return non_expired_; }

  void setNonExpired(bool m_non_expired);

  [[nodiscard]] bool isNoPassPhrase() const { return this->no_passphrase_; }

  void setNonPassPhrase(bool m_non_pass_phrase) {
    GenKeyInfo::no_passphrase_ = m_non_pass_phrase;
  }

  [[nodiscard]] bool isAllowSigning() const { return allowSigning; }

  [[nodiscard]] bool isAllowNoPassPhrase() const {
    return allow_no_pass_phrase_;
  }

  void setAllowSigning(bool m_allow_signing) {
    if (allowChangeSigning) GenKeyInfo::allowSigning = m_allow_signing;
  }

  [[nodiscard]] bool isAllowEncryption() const { return allowEncryption; }

  void setAllowEncryption(bool m_allow_encryption);

  [[nodiscard]] bool isAllowCertification() const { return allowCertification; }

  void setAllowCertification(bool m_allow_certification);

  [[nodiscard]] bool isAllowAuthentication() const {
    return allowAuthentication;
  }

  void setAllowAuthentication(bool m_allow_authentication) {
    if (allowChangeAuthentication)
      GenKeyInfo::allowAuthentication = m_allow_authentication;
  }

  [[nodiscard]] const std::string &getPassPhrase() const { return passphrase_; }

  void setPassPhrase(const std::string &m_pass_phrase) {
    GenKeyInfo::passphrase_ = m_pass_phrase;
  }

  [[nodiscard]] bool isAllowChangeSigning() const { return allowChangeSigning; }
  [[nodiscard]] bool isAllowChangeEncryption() const {
    return allowChangeEncryption;
  }

  [[nodiscard]] bool isAllowChangeCertification() const {
    return allowChangeCertification;
  }

  [[nodiscard]] bool isAllowChangeAuthentication() const {
    return allowChangeAuthentication;
  }

  [[nodiscard]] int getSuggestMaxKeySize() const {
    return suggest_max_key_size_;
  }

  [[nodiscard]] int getSuggestMinKeySize() const {
    return suggest_min_key_size_;
  }

  [[nodiscard]] int getSizeChangeStep() const {
    return suggest_size_addition_step_;
  }

 private:
  bool allowEncryption = true;
  bool allowChangeEncryption = true;

  bool allowCertification = true;
  bool allowChangeCertification = true;

  bool allowAuthentication = true;
  bool allowChangeAuthentication = true;

  bool allowSigning = true;
  bool allowChangeSigning = true;

  void reset_options();

 public:
  explicit GenKeyInfo(bool m_is_sub_key = false, bool m_standalone = false);
};

}  // namespace GpgFrontend

#endif  // GPGFRONTEND_GPGGENKEYINFO_H
