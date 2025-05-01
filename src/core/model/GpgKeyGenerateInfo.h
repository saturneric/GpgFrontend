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

#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

class GF_CORE_EXPORT KeyAlgo {
 public:
  KeyAlgo() = default;

  KeyAlgo(QString id, QString name, QString type, int length, int opera,
          QString supported_version);

  KeyAlgo(const KeyAlgo &) = default;

  auto operator=(const KeyAlgo &) -> KeyAlgo & = default;

  auto operator==(const KeyAlgo &o) const -> bool;

  [[nodiscard]] auto Id() const -> QString;

  [[nodiscard]] auto Name() const -> QString;

  [[nodiscard]] auto KeyLength() const -> int;

  [[nodiscard]] auto Type() const -> QString;

  [[nodiscard]] auto CanEncrypt() const -> bool;

  [[nodiscard]] auto CanSign() const -> bool;

  [[nodiscard]] auto CanAuth() const -> bool;

  [[nodiscard]] auto CanCert() const -> bool;

  [[nodiscard]] auto SupportedVersion() const -> QString;

 private:
  QString id_;
  QString name_;
  QString type_;
  int length_;
  bool encrypt_;
  bool sign_;
  bool auth_;
  bool cert_;
  QString supported_version_;
};

class GF_CORE_EXPORT KeyGenerateInfo : public QObject {
  Q_OBJECT
 public:
  static const KeyAlgo kNoneAlgo;
  static const QContainer<KeyAlgo> kPrimaryKeyAlgos;
  static const QContainer<KeyAlgo> kSubKeyAlgos;

  /**
   * @brief Construct a new Gen Key Info object
   *
   * @param m_is_sub_key
   * @param m_standalone
   */
  explicit KeyGenerateInfo(bool is_subkey = false);

  /**
   * @brief Get the Supported Key Algo object
   *
   * @return const QContainer<KeyGenAlgo>&
   */
  static auto GetSupportedKeyAlgo(int channel) -> QContainer<KeyAlgo>;

  /**
   * @brief Get the Supported Subkey Algo object
   *
   * @return const QContainer<KeyGenAlgo>&
   */
  static auto GetSupportedSubkeyAlgo(int channel) -> QContainer<KeyAlgo>;

  /**
   * @brief
   *
   * @param algo_id
   * @return std::tuple<bool, KeyAlgo>
   */
  static auto SearchPrimaryKeyAlgo(const QString &algo_id)
      -> std::tuple<bool, KeyAlgo>;

  /**
   * @brief
   *
   * @param algo_id
   * @return std::tuple<bool, KeyAlgo>
   */
  static auto SearchSubKeyAlgo(const QString &algo_id)
      -> std::tuple<bool, KeyAlgo>;

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
  void SetIsSubKey(bool);

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
  void SetName(const QString &name);

  /**
   * @brief Set the Email object
   *
   * @param m_email
   */
  void SetEmail(const QString &email);

  /**
   * @brief Set the Comment object
   *
   * @param m_comment
   */
  void SetComment(const QString &comment);

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
  [[nodiscard]] auto GetAlgo() const -> const KeyAlgo &;

  /**
   * @brief Set the Algo object
   *
   * @param m_algo
   */
  void SetAlgo(const KeyAlgo &);

  /**
   * @brief Get the Key Size object
   *
   * @return int
   */
  [[nodiscard]] auto GetKeyLength() const -> int;

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
  [[nodiscard]] auto IsAllowSign() const -> bool;

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
  void SetAllowSign(bool m_allow_signing);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowEncr() const -> bool;

  /**
   * @brief Set the Allow Encryption object
   *
   * @param m_allow_encryption
   */
  void SetAllowEncr(bool m_allow_encryption);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowCert() const -> bool;

  /**
   * @brief Set the Allow Certification object
   *
   * @param m_allow_certification
   */
  void SetAllowCert(bool m_allow_certification);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowAuth() const -> bool;

  /**
   * @brief Set the Allow Authentication object
   *
   * @param m_allow_authentication
   */
  void SetAllowAuth(bool m_allow_authentication);

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowModifySign() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowModifyEncr() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowModifyCert() const -> bool;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  [[nodiscard]] auto IsAllowModifyAuth() const -> bool;

 private:
  bool subkey_ = false;  ///<
  QString name_;         ///<
  QString email_;        ///<
  QString comment_;      ///<

  KeyAlgo algo_;  ///<
  QDateTime expired_;
  bool non_expired_ = false;  ///<

  bool no_passphrase_ = false;        ///<
  bool allow_no_pass_phrase_ = true;  ///<

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
