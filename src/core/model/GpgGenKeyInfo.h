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

#pragma once

#include "core/GpgFrontendCoreExport.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

class GPGFRONTEND_CORE_EXPORT GenKeyInfo {
 public:
  using KeyGenAlgo = std::tuple<QString, QString, QString>;

  /**
   * @brief Construct a new Gen Key Info object
   *
   * @param m_is_sub_key
   * @param m_standalone
   */
  explicit GenKeyInfo(bool m_is_sub_key = false);

  /**
   * @brief Get the Supported Key Algo object
   *
   * @return const QContainer<KeyGenAlgo>&
   */
  static auto GetSupportedKeyAlgo() -> const QContainer<KeyGenAlgo> &;

  /**
   * @brief Get the Supported Subkey Algo object
   *
   * @return const QContainer<KeyGenAlgo>&
   */
  static auto GetSupportedSubkeyAlgo() -> const QContainer<KeyGenAlgo> &;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsSubKey() const -> bool;

  /**
   * @brief Set the Is Sub Key object
   *
   * @param m_sub_key
   */
  void SetIsSubKey(bool m_sub_key);

  /**
   * @brief Get the Userid object
   *
   * @return QString
   */
  [[nodiscard]] auto GetUserid() const -> QString;

  /**
   * @brief Set the Name object
   *
   * @param m_name
   */
  void SetName(const QString &m_name);

  /**
   * @brief Set the Email object
   *
   * @param m_email
   */
  void SetEmail(const QString &m_email);

  /**
   * @brief Set the Comment object
   *
   * @param m_comment
   */
  void SetComment(const QString &m_comment);

  /**
   * @brief Get the Name object
   *
   * @return QString
   */
  [[nodiscard]] auto GetName() const -> QString;

  /**
   * @brief Get the Email object
   *
   * @return QString
   */
  [[nodiscard]] auto GetEmail() const -> QString;

  /**
   * @brief Get the Comment object
   *
   * @return QString
   */
  [[nodiscard]] auto GetComment() const -> QString;

  /**
   * @brief Get the Algo object
   *
   * @return const QString&
   */
  [[nodiscard]] auto GetAlgo() const -> const QString &;

  /**
   * @brief Set the Algo object
   *
   * @param m_algo
   */
  void SetAlgo(const QString &);

  /**
   * @brief Get the Key Size Str object
   *
   * @return QString
   */
  [[nodiscard]] auto GetKeySizeStr() const -> QString;

  /**
   * @brief Get the Key Size object
   *
   * @return int
   */
  [[nodiscard]] auto GetKeyLength() const -> int;

  /**
   * @brief Set the Key Size object
   *
   * @param m_key_size
   */
  void SetKeyLength(int m_key_size);

  /**
   * @brief Get the Expired object
   *
   * @return const QDateTime&
   */
  [[nodiscard]] auto GetExpireTime() const -> const QDateTime &;

  /**
   * @brief Set the Expired object
   *
   * @param m_expired
   */
  void SetExpireTime(const QDateTime &m_expired);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsNonExpired() const -> bool;

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
  [[nodiscard]] auto IsNoPassPhrase() const -> bool;

  /**
   * @brief Set the Non Pass Phrase object
   *
   * @param m_non_pass_phrase
   */
  void SetNonPassPhrase(bool m_non_pass_phrase);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowSigning() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowNoPassPhrase() const -> bool;

  /**
   * @brief Set the Allow Signing object
   *
   * @param m_allow_signing
   */
  void SetAllowSigning(bool m_allow_signing);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowEncryption() const -> bool;

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
  [[nodiscard]] auto IsAllowCertification() const -> bool;

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
  [[nodiscard]] auto IsAllowAuthentication() const -> bool;

  /**
   * @brief Set the Allow Authentication object
   *
   * @param m_allow_authentication
   */
  void SetAllowAuthentication(bool m_allow_authentication);

  /**
   * @brief Get the Pass Phrase object
   *
   * @return const QString&
   */
  [[nodiscard]] auto GetPassPhrase() const -> const QString &;

  /**
   * @brief Set the Pass Phrase object
   *
   * @param m_pass_phrase
   */
  void SetPassPhrase(const QString &m_pass_phrase);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowChangeSigning() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowChangeEncryption() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowChangeCertification() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowChangeAuthentication() const -> bool;

  /**
   * @brief Get the Suggest Max Key Size object
   *
   * @return int
   */
  [[nodiscard]] auto GetSuggestMaxKeySize() const -> int;

  /**
   * @brief Get the Suggest Min Key Size object
   *
   * @return int
   */
  [[nodiscard]] auto GetSuggestMinKeySize() const -> int;

  /**
   * @brief Get the Size Change Step object
   *
   * @return int
   */
  [[nodiscard]] auto GetSizeChangeStep() const -> int;

 private:
  bool subkey_ = false;  ///<
  QString name_;         ///<
  QString email_;        ///<
  QString comment_;      ///<

  QString algo_;  ///<
  int key_size_ = 2048;
  QDateTime expired_ = QDateTime::currentDateTime().addYears(2);
  bool non_expired_ = false;  ///<

  bool no_passphrase_ = false;        ///<
  bool allow_no_pass_phrase_ = true;  ///<

  int suggest_max_key_size_ = 4096;        ///<
  int suggest_size_addition_step_ = 1024;  ///<
  int suggest_min_key_size_ = 1024;        ///<

  QString passphrase_;  ///<

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
};

}  // namespace GpgFrontend
