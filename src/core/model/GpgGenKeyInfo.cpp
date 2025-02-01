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

const QContainer<KeyAlgo> GenKeyInfo::kPrimaryKeyAlgos = {
    /**
     * Algorithm (DSA) as a government standard for digital signatures.
     * Originally, it supported key lengths between 512 and 1024 bits.
     * Recently, NIST has declared 512-bit keys obsolete:
     * now, DSA is available in 1024, 2048 and 3072-bit lengths.
     */
    {"rsa1024", "RSA", "RSA", 1024, kENCRYPT | kSIGN | kAUTH | kCERT, "2.2.0"},
    {"rsa2048", "RSA", "RSA", 2048, kENCRYPT | kSIGN | kAUTH | kCERT, "2.2.0"},
    {"rsa3072", "RSA", "RSA", 3072, kENCRYPT | kSIGN | kAUTH | kCERT, "2.2.0"},
    {"rsa4096", "RSA", "RSA", 4096, kENCRYPT | kSIGN | kAUTH | kCERT, "2.2.0"},
    {"dsa1024", "DSA", "DSA", 1024, kSIGN | kAUTH | kCERT, "2.2.0"},
    {"dsa2048", "DSA", "DSA", 2048, kSIGN | kAUTH | kCERT, "2.2.0"},
    {"dsa3072", "DSA", "DSA", 3072, kSIGN | kAUTH | kCERT, "2.2.0"},
    {"ed25519", "ED25519", "EdDSA", 256, kSIGN | kAUTH | kCERT, "2.2.0"},
    {"nistp256", "NIST", "ECDSA", 256, kSIGN | kAUTH | kCERT, "2.2.0"},
    {"nistp384", "NIST", "ECDSA", 384, kSIGN | kAUTH | kCERT, "2.2.0"},
    {"nistp521", "NIST", "ECDSA", 521, kSIGN | kAUTH | kCERT, "2.2.0"},
    {"brainpoolp256r1", "BrainPooL", "ECDSA", 256, kSIGN | kAUTH | kCERT,
     "2.3.0"},
    {"brainpoolp384r1", "BrainPooL", "ECDSA", 384, kSIGN | kAUTH | kCERT,
     "2.3.0"},
    {"brainpoolp512r1", "BrainPooL", "ECDSA", 512, kSIGN | kAUTH | kCERT,
     "2.3.0"},
    {"ed448", "ED448", "EdDSA", 448, kSIGN | kAUTH | kCERT, "2.3.0"},
    {"secp256k1", "ED448", "EdDSA", 256, kSIGN | kAUTH | kCERT, "2.3.0"},
};

const QContainer<KeyAlgo> GenKeyInfo::kSubKeyAlgos = {
    {"rsa1024", "RSA", "RSA", 1024, kENCRYPT | kSIGN | kAUTH, "2.2.0"},
    {"rsa2048", "RSA", "RSA", 2048, kENCRYPT | kSIGN | kAUTH, "2.2.0"},
    {"rsa3072", "RSA", "RSA", 3072, kENCRYPT | kSIGN | kAUTH, "2.2.0"},
    {"rsa4096", "RSA", "RSA", 4096, kENCRYPT | kSIGN | kAUTH, "2.2.0"},
    {"dsa1024", "DSA", "DSA", 1024, kSIGN | kAUTH, "2.2.0"},
    {"dsa2048", "DSA", "DSA", 2048, kSIGN | kAUTH, "2.2.0"},
    {"dsa3072", "DSA", "DSA", 3072, kSIGN | kAUTH, "2.2.0"},
    {"ed25519", "ED25519", "EdDSA", 256, kSIGN | kAUTH, "2.2.0"},
    {"cv25519", "CV25519", "ECDH", 256, kENCRYPT, "2.2.0"},
    {"nistp256", "NIST", "ECDH", 256, kENCRYPT, "2.2.0"},
    {"nistp384", "NIST", "ECDH", 384, kENCRYPT, "2.2.0"},
    {"nistp521", "NIST", "ECDH", 521, kENCRYPT, "2.2.0"},
    {"brainpoolp256r1", "BrainPooL", "ECDH", 256, kENCRYPT, "2.3.0"},
    {"brainpoolp384r1", "BrainPooL", "ECDH", 384, kENCRYPT, "2.3.0"},
    {"brainpoolp512r1", "BrainPooL", "ECDH", 512, kENCRYPT, "2.3.0"},
    {"x448", "X448", "ECDH", 448, kENCRYPT, "2.3.0"},
    {"secp256k1", "SECP256K1", "ECDH", 256, kENCRYPT, "2.3.0"},
    /**
     * GnuPG supports the Elgamal asymmetric encryption algorithm in key lengths
     * ranging from 1024 to 4096 bits.
     */
    {"elg1024", "ELG-E", "ELG-E", 1024, kENCRYPT, "2.2.0"},
    {"elg2048", "ELG-E", "ELG-E", 2048, kENCRYPT, "2.2.0"},
    {"elg3072", "ELG-E", "ELG-E", 3072, kENCRYPT, "2.2.0"},
    {"elg4096", "ELG-E", "ELG-E", 4096, kENCRYPT, "2.2.0"},
};

auto GenKeyInfo::GetSupportedKeyAlgo() -> QContainer<KeyAlgo> {
  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{"2.0.0"});

  QContainer<KeyAlgo> algos;

  for (const auto &algo : kPrimaryKeyAlgos) {
    if (!algo.IsSupported(gnupg_version)) continue;
    algos.append(algo);
  }

  std::sort(algos.begin(), algos.end(), [](const KeyAlgo &a, const KeyAlgo &b) {
    return a.Name() < b.Name() && a.KeyLength() < b.KeyLength();
  });

  return algos;
}

auto GenKeyInfo::GetSupportedSubkeyAlgo() -> QContainer<KeyAlgo> {
  const auto gnupg_version = Module::RetrieveRTValueTypedOrDefault<>(
      "core", "gpgme.ctx.gnupg_version", QString{"2.0.0"});

  QContainer<KeyAlgo> algos;

  for (const auto &algo : kSubKeyAlgos) {
    if (!algo.IsSupported(gnupg_version)) continue;
    algos.append(algo);
  }

  std::sort(algos.begin(), algos.end(), [](const KeyAlgo &a, const KeyAlgo &b) {
    return a.Name() < b.Name() && a.KeyLength() < b.KeyLength();
  });

  return algos;
}

auto GenKeyInfo::SearchPrimaryKeyAlgo(const QString &algo_id)
    -> std::tuple<bool, KeyAlgo> {
  auto it =
      std::find_if(kPrimaryKeyAlgos.cbegin(), kPrimaryKeyAlgos.cend(),
                   [=](const KeyAlgo &algo) { return algo.Id() == algo_id; });

  if (it != kPrimaryKeyAlgos.cend()) {
    return {true, *it};
  }

  return {false, KeyAlgo{}};
}

auto GenKeyInfo::SearchSubKeyAlgo(const QString &algo_id)
    -> std::tuple<bool, KeyAlgo> {
  auto it =
      std::find_if(kSubKeyAlgos.cbegin(), kSubKeyAlgos.cend(),
                   [=](const KeyAlgo &algo) { return algo.Id() == algo_id; });

  if (it != kSubKeyAlgos.cend()) {
    return {true, *it};
  }

  return {false, KeyAlgo{}};
}

void GenKeyInfo::SetAlgo(const KeyAlgo &algo) {
  // reset all options
  reset_options();

  this->SetAllowCertification(algo.CanCert());
  this->allow_change_certification_ = false;

  SetAllowEncryption(algo.CanEncrypt());
  allow_change_encryption_ = algo.CanEncrypt();

  SetAllowSigning(algo.CanSign());
  allow_change_signing_ = algo.CanSign();

  SetAllowAuthentication(algo.CanAuth());
  allow_change_authentication_ = algo.CanAuth();

  this->algo_ = algo;
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
}

void GenKeyInfo::SetExpireTime(const QDateTime &m_expired) {
  GenKeyInfo::expired_ = m_expired;
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

GenKeyInfo::GenKeyInfo(bool is_subkey) : subkey_(is_subkey) {
  assert(!GetSupportedKeyAlgo().empty());
  SetAlgo(GetSupportedKeyAlgo().front());
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
[[nodiscard]] auto GenKeyInfo::GetAlgo() const -> const KeyAlgo & {
  return algo_;
}

/**
 * @brief Get the Key Size object
 *
 * @return int
 */
[[nodiscard]] auto GenKeyInfo::GetKeyLength() const -> int {
  return algo_.KeyLength();
}

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

KeyAlgo::KeyAlgo(QString id, QString name, QString type, int length, int opera,
                 QString supported_version)
    : id_(std::move(id)),
      name_(std::move(name)),
      type_(std::move(type)),
      length_(length),
      supported_version_(std::move(supported_version)) {
  encrypt_ = ((opera & kENCRYPT) != 0);
  sign_ = ((opera & kSIGN) != 0);
  auth_ = ((opera & kAUTH) != 0);
  cert_ = ((opera & kCERT) != 0);
};

auto KeyAlgo::Id() const -> QString { return id_; }

auto KeyAlgo::Name() const -> QString { return name_; }

auto KeyAlgo::KeyLength() const -> int { return length_; }

auto KeyAlgo::Type() const -> QString { return type_; }

auto KeyAlgo::CanEncrypt() const -> bool { return encrypt_; }

auto KeyAlgo::CanSign() const -> bool { return sign_; }

auto KeyAlgo::CanAuth() const -> bool { return auth_; }

auto KeyAlgo::CanCert() const -> bool { return cert_; }

auto KeyAlgo::IsSupported(const QString &version) const -> bool {
  return GFCompareSoftwareVersion(version, supported_version_) >= 0;
}

}  // namespace GpgFrontend
