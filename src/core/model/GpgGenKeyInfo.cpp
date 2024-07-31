/**
 * Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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

#include <cassert>

#include "module/ModuleManager.h"
#include "utils/CommonUtils.h"

namespace GpgFrontend {

void GenKeyInfo::SetAlgo(const QString &t_algo_args) {
  auto algo_args = t_algo_args.toLower();

  // reset all options
  reset_options();

  if (!this->subkey_) {
    this->SetAllowCertification(true);

  } else {
    this->SetAllowCertification(false);
  }

  this->allow_change_certification_ = false;

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
  } else if (algo_args == "elg") {
    /**
     * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths
     * ranging from 1024 to 4096 bits.
     */

    SetAllowEncryption(true);

    SetAllowAuthentication(false);
    allow_change_authentication_ = false;

    SetAllowSigning(false);
    allow_change_signing_ = false;

    suggest_min_key_size_ = 1024;
    suggest_max_key_size_ = 4096;
    suggest_size_addition_step_ = 1024;
    SetKeyLength(3072);

  } else if (algo_args == "ed25519") {
    SetAllowEncryption(false);
    allow_change_encryption_ = false;

    suggest_min_key_size_ = -1;
    suggest_max_key_size_ = -1;
    suggest_size_addition_step_ = -1;
    SetKeyLength(-1);
  } else if (algo_args == "cv25519" || algo_args == "x448") {
    SetAllowAuthentication(false);
    allow_change_authentication_ = false;

    SetAllowSigning(false);
    allow_change_signing_ = false;

    suggest_min_key_size_ = -1;
    suggest_max_key_size_ = -1;
    suggest_size_addition_step_ = -1;
    SetKeyLength(-1);
  } else if (algo_args == "ed448") {
    SetAllowEncryption(false);
    allow_change_encryption_ = false;

    // why not support signing? test later...
    SetAllowSigning(false);
    allow_change_signing_ = false;

    suggest_min_key_size_ = -1;
    suggest_max_key_size_ = -1;
    suggest_size_addition_step_ = -1;
    SetKeyLength(-1);
  } else if (algo_args == "nistp256" || algo_args == "nistp384" ||
             algo_args == "nistp521" || algo_args == "brainpoolp256r1" ||
             algo_args == "brainpoolp384r1" || algo_args == "brainpoolp512r1") {
    if (!subkey_) {  // for primary key is always ecdsa

      SetAllowEncryption(false);
      allow_change_encryption_ = false;

    } else {  // for subkey key is always ecdh

      SetAllowAuthentication(false);
      allow_change_authentication_ = false;

      SetAllowSigning(false);
      allow_change_signing_ = false;
    }

    suggest_min_key_size_ = -1;
    suggest_max_key_size_ = -1;
    suggest_size_addition_step_ = -1;
    SetKeyLength(-1);
  } else {
    LOG_W() << "unsupported genkey algo arguments: " << algo_args;
    return;
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

auto GenKeyInfo::GetKeySizeStr() const -> QString {
  if (key_size_ > 0) {
    return QString::number(key_size_);
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

void GenKeyInfo::SetExpireTime(const QDateTime &m_expired) {
  if (!IsNonExpired()) {
    GenKeyInfo::expired_ = m_expired;
  }
}

void GenKeyInfo::SetNonExpired(bool m_non_expired) {
  if (!m_non_expired) this->expired_ = QDateTime::fromSecsSinceEpoch(0);
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

GenKeyInfo::GenKeyInfo(bool m_is_sub_key) : subkey_(m_is_sub_key) {
  assert(!GetSupportedKeyAlgo().empty());
  SetAlgo(std::get<0>(GetSupportedKeyAlgo()[0]));
}

auto GenKeyInfo::GetSupportedKeyAlgo()
    -> const std::vector<GenKeyInfo::KeyGenAlgo> & {
  static std::vector<GenKeyInfo::KeyGenAlgo> k_support_key_algo = {
      {"RSA", "RSA", ""},
      {"DSA", "DSA", ""},
  };
  static bool initialized = false;

  if (!initialized) {
    const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.gnupg_version", QString{"2.0.0"});
    if (GFCompareSoftwareVersion(gnupg_version, "2.0.0") < 0) {
      // do nothing
    } else if (GFCompareSoftwareVersion(gnupg_version, "2.3.0") < 0) {
      k_support_key_algo = {
          {"RSA", "RSA", ""},
          {"DSA", "DSA", ""},
          {"ECDSA (ED25519)", "ED25519", ""},
          {"ECDSA (NIST P-256)", "NISTP256", ""},
          {"ECDSA (NIST P-384)", "NISTP384", ""},
          {"ECDSA (NIST P-521)", "NISTP521", ""},
      };
    } else {
      k_support_key_algo = {
          {"RSA", "RSA", ""},
          {"DSA", "DSA", ""},
          {"ECDSA (ED25519)", "ED25519", ""},
          {"ECDSA (NIST P-256)", "NISTP256", ""},
          {"ECDSA (NIST P-384)", "NISTP384", ""},
          {"ECDSA (NIST P-521)", "NISTP521", ""},
          {"ECDSA (BrainPooL P-256)", "BRAINPOOLP256R1", ""},
          {"ECDSA (BrainPooL P-384)", "BRAINPOOLP384R1", ""},
          {"ECDSA (BrainPooL P-512)", "BRAINPOOLP512R1", ""},
      };
    }

    initialized = true;
  }

  return k_support_key_algo;
}

auto GenKeyInfo::GetSupportedSubkeyAlgo()
    -> const std::vector<GenKeyInfo::KeyGenAlgo> & {
  static std::vector<GenKeyInfo::KeyGenAlgo> k_support_subkey_algo = {
      {"RSA", "", "RSA"},
      {"DSA", "", "DSA"},
      {"ELG-E", "", "ELG"},
  };
  static bool initialized = false;

  if (!initialized) {
    const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
        "core", "gpgme.ctx.gnupg_version", QString{"2.0.0"});
    if (GFCompareSoftwareVersion(gnupg_version, "2.0.0") < 0) {
      // do nothing
    } else if (GFCompareSoftwareVersion(gnupg_version, "2.3.0") < 0) {
      k_support_subkey_algo = {
          {"RSA", "", "RSA"},
          {"DSA", "", "DSA"},
          {"ELG-E", "", "ELG"},
          {"ECDSA (ED25519)", "", "ED25519"},
          {"ECDH (CV25519)", "", "CV25519"},
          {"ECDH (NIST P-256)", "", "NISTP256"},
          {"ECDH (NIST P-384)", "", "NISTP384"},
          {"ECDH (NIST P-521)", "", "NISTP521"},
      };
    } else {
      k_support_subkey_algo = {
          {"RSA", "", "RSA"},
          {"DSA", "", "DSA"},
          {"ELG-E", "", "ELG"},
          {"ECDSA (ED25519)", "", "ED25519"},
          {"ECDSA (ED448)", "", "ED448"},
          {"ECDH (CV25519)", "", "CV25519"},
          {"ECDH (X448)", "", "X448"},
          {"ECDH (NIST P-256)", "", "NISTP256"},
          {"ECDH (NIST P-384)", "", "NISTP384"},
          {"ECDH (NIST P-521)", "", "NISTP521"},
          {"ECDH (BrainPooL P-256)", "", "BRAINPOOLP256R1"},
          {"ECDH (BrainPooL P-384)", "", "BRAINPOOLP384R1"},
          {"ECDH (BrainPooL P-512)", "", "BRAINPOOLP512R1"},
      };
    }

    initialized = true;
  }

  return k_support_subkey_algo;
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
 * @return QString
 */
[[nodiscard]] auto GenKeyInfo::GetUserid() const -> QString {
  return QString("%1(%2)<%3>").arg(name_).arg(comment_).arg(email_);
}

/**
 * @brief Set the Name object
 *
 * @param m_name
 */
void GenKeyInfo::SetName(const QString &m_name) { this->name_ = m_name; }

/**
 * @brief Set the Email object
 *
 * @param m_email
 */
void GenKeyInfo::SetEmail(const QString &m_email) { this->email_ = m_email; }

/**
 * @brief Set the Comment object
 *
 * @param m_comment
 */
void GenKeyInfo::SetComment(const QString &m_comment) {
  this->comment_ = m_comment;
}

/**
 * @brief Get the Name object
 *
 * @return QString
 */
[[nodiscard]] auto GenKeyInfo::GetName() const -> QString { return name_; }

/**
 * @brief Get the Email object
 *
 * @return QString
 */
[[nodiscard]] auto GenKeyInfo::GetEmail() const -> QString { return email_; }

/**
 * @brief Get the Comment object
 *
 * @return QString
 */
[[nodiscard]] auto GenKeyInfo::GetComment() const -> QString {
  return comment_;
}

/**
 * @brief Get the Algo object
 *
 * @return const QString&
 */
[[nodiscard]] auto GenKeyInfo::GetAlgo() const -> const QString & {
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
 * @return const QDateTime&
 */
[[nodiscard]] auto GenKeyInfo::GetExpireTime() const -> const QDateTime & {
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
 * @return const QString&
 */
[[nodiscard]] auto GenKeyInfo::GetPassPhrase() const -> const QString & {
  return passphrase_;
}

/**
 * @brief Set the Pass Phrase object
 *
 * @param m_pass_phrase
 */
void GenKeyInfo::SetPassPhrase(const QString &m_pass_phrase) {
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
