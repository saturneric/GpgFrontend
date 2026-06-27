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

#include "core/model/GFEngineSupportIf.h"
#include "core/model/GpgKey.h"
#include "core/typedef/CoreTypedef.h"

namespace GpgFrontend {

/**
 * @brief Intrinsic, non-operational properties of a key algorithm.
 *
 * Unlike GpgOperation (what a key can *do*), traits describe what an algorithm
 * *is*. Stored as a bitmask so an algorithm can carry several traits.
 */
enum KeyAlgoTrait : uint16_t {
  kALGO_TRAIT_NONE = 0,
  kPQC = 1 << 0,  ///< post-quantum / quantum-resistant
  kECC = 1 << 1,  ///< elliptic-curve family (EdDSA / ECDSA / ECDH)
};

struct KeyAlgoSpec;

class GF_CORE_EXPORT KeyAlgo {
 public:
  KeyAlgo() = default;

  // Implicit on purpose: lets the algorithm tables be written as readable,
  // designated-initialized KeyAlgoSpec literals.
  KeyAlgo(const KeyAlgoSpec &spec);  // NOLINT(google-explicit-constructor)

  KeyAlgo(const KeyAlgo &) = default;

  auto operator=(const KeyAlgo &) -> KeyAlgo & = default;

  auto operator==(const KeyAlgo &o) const -> bool;

  auto operator!=(const KeyAlgo &o) const -> bool;

  [[nodiscard]] auto Id() const -> QString;

  [[nodiscard]] auto Name() const -> QString;

  [[nodiscard]] auto KeyLength() const -> int;

  [[nodiscard]] auto Type() const -> QString;

  [[nodiscard]] auto CanEncrypt() const -> bool;

  [[nodiscard]] auto CanSign() const -> bool;

  [[nodiscard]] auto CanAuth() const -> bool;

  [[nodiscard]] auto CanCert() const -> bool;

  /**
   * @brief Whether this algorithm is post-quantum (quantum-resistant).
   *
   * Covers the NIST PQC suite exposed through the OpenPGP draft: ML-KEM
   * (Kyber hybrid), ML-DSA (hybrid signing) and SLH-DSA.
   *
   * @return true if the algorithm provides post-quantum resistance.
   */
  [[nodiscard]] auto IsPostQuantum() const -> bool;

  /**
   * @brief Whether this algorithm belongs to the elliptic-curve family.
   *
   * Carried as the kECC trait, set centrally for every curve by the ECC family
   * generator. Used to group curves together in the UI.
   *
   * @return true if the algorithm is elliptic-curve based.
   */
  [[nodiscard]] auto IsEcc() const -> bool;

  [[nodiscard]] auto SupportedVersion() const -> EngineSupportList;

  [[nodiscard]] auto SubAlgos(int channel) const -> QContainer<KeyAlgo>;

 private:
  QString id_;
  QString name_;
  QString type_;
  int length_;
  bool encrypt_;
  bool sign_;
  bool auth_;
  bool cert_;
  EngineSupportList support_ifs_;

  // for hybrid algorithms
  QContainer<QPair<KeyAlgo, EngineSupportList>> sub_algos_;

  int traits_ =
      kALGO_TRAIT_NONE;  ///< intrinsic algorithm traits (KeyAlgoTrait)
};

/**
 * @brief Plain, designated-initializable description of a key algorithm.
 *
 * Used to declare the algorithm tables with named fields instead of positional
 * arguments. Implicitly converts to KeyAlgo.
 */
struct KeyAlgoSpec {
  QString id;                 ///< unique algorithm id (e.g. "rsa2048")
  QString name;               ///< display name (e.g. "RSA")
  QString type;               ///< crypto family (e.g. "RSA", "ECDSA")
  int length = 0;             ///< key length in bits
  int opera = 0;              ///< GpgOperation capability bitmask
  EngineSupportList support;  ///< per-engine minimum versions
  QContainer<QPair<KeyAlgo, EngineSupportList>> sub_algos =
      {};                         ///< for hybrids
  int traits = kALGO_TRAIT_NONE;  ///< KeyAlgoTrait bitmask
};

class GF_CORE_EXPORT KeyGenerateInfo : public QObject {
  Q_OBJECT
 public:
  static const KeyAlgo kNoneAlgo;
  static const QContainer<KeyAlgo> kPrimaryKeyAlgos;
  static const QContainer<KeyAlgo> kHybridPrimaryKeyAlgo;
  static const QContainer<KeyAlgo> kSubKeyAlgos;
  static const QContainer<KeyAlgo> kHybridSubKeyAlgos;

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
   * @brief get supported subkey algorithms without considering the primary key
   *
   * @return const QContainer<KeyGenAlgo>&
   */
  static auto GetSupportedSubkeyAlgo(int channel) -> QContainer<KeyAlgo>;

  /**
   * @brief get supported subkey algorithms based on the primary key
   *
   * @param channel
   * @param key
   * @return auto
   */
  static auto GetSupportedSubkeyAlgo(int channel, const GpgKey &key)
      -> QContainer<KeyAlgo>;

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

  /**
   * @brief
   *
   * @return const KeyAlgo&
   */
  [[nodiscard]] auto SubAlgo() const -> const KeyAlgo &;

  /**
   * @brief Set the Sub Algo object
   *
   */
  void SetSubAlgo(const KeyAlgo &);

  /**
   * @brief Get the requested OpenPGP key format version.
   *
   * Only meaningful for engines that can emit more than one key format (e.g.
   * rPGP). 0 means "let the engine decide" (typically v4); 4 and 6 request the
   * v4 / v6 packet format respectively.
   *
   * @return requested key version (0 = engine default).
   */
  [[nodiscard]] auto GetKeyVersion() const -> int;

  /**
   * @brief Set the requested OpenPGP key format version.
   *
   * @param version 0 for engine default, or 4 / 6 for the v4 / v6 packet format.
   */
  void SetKeyVersion(int version);

 private:
  bool subkey_ = false;  ///<
  QString name_;         ///<
  QString email_;        ///<
  QString comment_;      ///<

  KeyAlgo algo_;      ///<
  KeyAlgo sub_algo_;  ///< for hybrid subkey
  int key_version_ = 0;  ///< requested key format version (0 = engine default)
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
