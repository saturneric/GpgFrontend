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

#include "GpgKeyGenerateInfo.h"

#include <array>
#include <cassert>

#include "core/function/openpgp/helper/Async.h"
#include "core/function/openpgp/traits/KeyGenerationTraits.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend {

namespace {

auto FindAlgoByIdAndType(const QContainer<KeyAlgo> &algos, const QString &id,
                         const QString &type) -> KeyAlgo {
  const auto it =
      std::find_if(algos.cbegin(), algos.cend(), [&](const KeyAlgo &algo) {
        return algo.Id() == id && algo.Type() == type;
      });

  assert(it != algos.cend());
  return it != algos.cend() ? *it : KeyAlgo{};
}

auto FindSubAlgo(const QString &id, const QString &type) -> KeyAlgo {
  return FindAlgoByIdAndType(KeyGenerateInfo::kSubKeyAlgos, id, type);
}

auto FindPrimaryAlgo(const QString &id, const QString &type) -> KeyAlgo {
  return FindAlgoByIdAndType(KeyGenerateInfo::kPrimaryKeyAlgos, id, type);
}

// --- Family generators -----------------------------------------------------
// Several families differ only by key length (and, between primary and subkey
// use, by allowed operations). Generate them from a single source of truth
// instead of repeating near-identical entries.

auto RsaFamily(int opera) -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos;
  for (int bits : {1024, 2048, 3072, 4096}) {
    algos.append(KeyAlgoSpec{.id = QString("rsa%1").arg(bits),
                             .name = "RSA",
                             .type = "RSA",
                             .length = bits,
                             .opera = opera,
                             .support = {{OpenPGPEngine::kGNUPG, "2.2.0"},
                                         {OpenPGPEngine::kRPGP, "0.1.0"}}});
  }
  return algos;
}

auto DsaFamily(int opera) -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos;
  for (int bits : {1024, 2048, 3072}) {
    algos.append(KeyAlgoSpec{.id = QString("dsa%1").arg(bits),
                             .name = "DSA",
                             .type = "DSA",
                             .length = bits,
                             .opera = opera,
                             .support = {{OpenPGPEngine::kGNUPG, "2.2.0"},
                                         {OpenPGPEngine::kRPGP, "0.1.0"}}});
  }
  return algos;
}

auto ElgamalFamily() -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos;
  for (int bits : {1024, 2048, 3072, 4096}) {
    algos.append(KeyAlgoSpec{.id = QString("elg%1").arg(bits),
                             .name = "ELG-E",
                             .type = "ELG-E",
                             .length = bits,
                             .opera = kENCRYPT,
                             .support = {{OpenPGPEngine::kGNUPG, "2.2.0"}}});
  }
  return algos;
}

// One ECC curve as it should appear for a given role (signing or encryption);
// the operation bitmask is supplied by the caller because it differs between
// primary keys and subkeys.
struct EccCurveSpec {
  QString id;
  QString name;
  QString type;  ///< "EdDSA", "ECDSA" or "ECDH"
  int length;
  EngineSupportList support;
};

auto EccFamily(int opera, const QContainer<EccCurveSpec> &curves)
    -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos;
  algos.reserve(curves.size());
  for (const auto &c : curves) {
    algos.append(KeyAlgoSpec{.id = c.id,
                             .name = c.name,
                             .type = c.type,
                             .length = c.length,
                             .opera = opera,
                             .support = c.support,
                             .traits = kECC});
  }
  return algos;
}

auto SlhDsaFamily(int opera) -> QContainer<KeyAlgo> {
  struct Variant {
    const char *id;
    const char *name;
    int length;
  };
  static constexpr std::array kVariants = {
      Variant{"slhdsashake128s", "SLH-DSA-SHAKE-128S", 128},
      Variant{"slhdsashake128f", "SLH-DSA-SHAKE-128F", 128},
      Variant{"slhdsashake256s", "SLH-DSA-SHAKE-256S", 256},
  };

  QContainer<KeyAlgo> algos;
  for (const auto &v : kVariants) {
    algos.append(KeyAlgoSpec{.id = v.id,
                             .name = v.name,
                             .type = "SLH-DSA",
                             .length = v.length,
                             .opera = opera,
                             .support = {{OpenPGPEngine::kRPGP, "0.1.2"}},
                             .traits = kPQC});
  }
  return algos;
}

// The Kyber hybrid KEMs pair with the same set of classical ECDH curves; only
// the cv25519 and x448 partners differ in engine support between key sizes.
auto KyberHybridCurves(const EngineSupportList &cv25519_support,
                       const EngineSupportList &x448_support)
    -> QContainer<QPair<KeyAlgo, EngineSupportList>> {
  const EngineSupportList gnupg_only{{OpenPGPEngine::kGNUPG, "2.5.0"}};
  return {
      {FindSubAlgo("cv25519", "ECDH"), cv25519_support},
      {FindSubAlgo("nistp256", "ECDH"), gnupg_only},
      {FindSubAlgo("nistp384", "ECDH"), gnupg_only},
      {FindSubAlgo("nistp521", "ECDH"), gnupg_only},
      {FindSubAlgo("brainpoolp256r1", "ECDH"), gnupg_only},
      {FindSubAlgo("brainpoolp384r1", "ECDH"), gnupg_only},
      {FindSubAlgo("brainpoolp512r1", "ECDH"), gnupg_only},
      {FindSubAlgo("x448", "ECDH"), x448_support},
  };
}

}  // namespace

const KeyAlgo KeyGenerateInfo::kNoneAlgo =
    KeyAlgoSpec{.id = "none",
                .name = KeyGenerateInfo::tr("None"),
                .type = "None",
                .length = 0,
                .opera = kNONE,
                .support = {{OpenPGPEngine::kGNUPG, "0.0.0"},
                            {OpenPGPEngine::kRPGP, "0.0.0"}}};

const QContainer<KeyAlgo> KeyGenerateInfo::kPrimaryKeyAlgos =
    []() -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos{kNoneAlgo};

  algos += RsaFamily(kENCRYPT | kSIGN | kAUTH | kCERT);
  algos += DsaFamily(kSIGN | kAUTH | kCERT);

  // ECC: every curve here is a primary (signing/cert) key.
  const EngineSupportList ecc_gnupg_rpgp{{OpenPGPEngine::kGNUPG, "2.2.0"},
                                         {OpenPGPEngine::kRPGP, "0.1.0"}};
  const EngineSupportList ecc_gnupg23{{OpenPGPEngine::kGNUPG, "2.3.0"}};
  const EngineSupportList ecc_gnupg23_rpgp{{OpenPGPEngine::kGNUPG, "2.3.0"},
                                           {OpenPGPEngine::kRPGP, "0.1.0"}};

  algos +=
      EccFamily(kSIGN | kAUTH | kCERT,
                {
                    {"ed25519", "ED25519", "EdDSA", 255, ecc_gnupg_rpgp},
                    {"nistp256", "NIST", "ECDSA", 256, ecc_gnupg_rpgp},
                    {"nistp384", "NIST", "ECDSA", 384, ecc_gnupg_rpgp},
                    {"nistp521", "NIST", "ECDSA", 521, ecc_gnupg_rpgp},
                    {"brainpoolp256r1", "BrainPooL", "ECDSA", 256, ecc_gnupg23},
                    {"brainpoolp384r1", "BrainPooL", "ECDSA", 384, ecc_gnupg23},
                    {"brainpoolp512r1", "BrainPooL", "ECDSA", 512, ecc_gnupg23},
                    {"ed448", "ED448", "EdDSA", 448, ecc_gnupg23_rpgp},
                    {"secp256k1", "SECP256K1", "EdDSA", 256, ecc_gnupg23_rpgp},
                });

  algos += SlhDsaFamily(kSIGN | kAUTH);

  return algos;
}();

const QContainer<KeyAlgo> KeyGenerateInfo::kHybridPrimaryKeyAlgo = {
    KeyAlgoSpec{.id = "mldsa65",
                .name = "ML-DSA",
                .type = "HYBRID-SIGN",
                .length = 65,
                .opera = kSIGN | kAUTH,
                .support = {{OpenPGPEngine::kRPGP, "0.1.2"}},
                .sub_algos = {{FindPrimaryAlgo("ed25519", "EdDSA"),
                               {{OpenPGPEngine::kRPGP, "0.1.2"}}}},
                .traits = kPQC},
    KeyAlgoSpec{.id = "mldsa87",
                .name = "ML-DSA",
                .type = "HYBRID-SIGN",
                .length = 87,
                .opera = kSIGN | kAUTH,
                .support = {{OpenPGPEngine::kRPGP, "0.1.2"}},
                .sub_algos = {{FindPrimaryAlgo("ed448", "EdDSA"),
                               {{OpenPGPEngine::kRPGP, "0.1.2"}}}},
                .traits = kPQC},
};

const QContainer<KeyAlgo> KeyGenerateInfo::kSubKeyAlgos =
    []() -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos{kNoneAlgo};

  algos += RsaFamily(kENCRYPT | kSIGN | kAUTH);
  algos += DsaFamily(kSIGN | kAUTH);

  // ECC signing/auth curves.
  const EngineSupportList ecc_gnupg_rpgp{{OpenPGPEngine::kGNUPG, "2.2.0"},
                                         {OpenPGPEngine::kRPGP, "0.1.0"}};
  const EngineSupportList ecc_gnupg23{{OpenPGPEngine::kGNUPG, "2.3.0"}};
  const EngineSupportList ecc_gnupg23_rpgp{{OpenPGPEngine::kGNUPG, "2.3.0"},
                                           {OpenPGPEngine::kRPGP, "0.1.0"}};

  algos += EccFamily(
      kSIGN | kAUTH,
      {
          {"ed25519", "ED25519", "EdDSA", 255, ecc_gnupg_rpgp},
          {"nistp256", "NIST", "ECDSA", 256, ecc_gnupg_rpgp},
          {"nistp384", "NIST", "ECDSA", 384, ecc_gnupg_rpgp},
          {"nistp521", "NIST", "ECDSA", 521, ecc_gnupg_rpgp},
          {"brainpoolp256r1", "BrainPooL", "ECDSA", 256, ecc_gnupg23},
          {"brainpoolp384r1", "BrainPooL", "ECDSA", 384, ecc_gnupg23},
          {"brainpoolp512r1", "BrainPooL", "ECDSA", 512, ecc_gnupg23},
          {"ed448", "ED448", "EdDSA", 448, {{OpenPGPEngine::kRPGP, "0.1.2"}}},
      });

  // ECC encryption curves (ECDH).
  algos += EccFamily(
      kENCRYPT, {
                    {"cv25519", "CV25519", "ECDH", 255, ecc_gnupg_rpgp},
                    {"nistp256", "NIST", "ECDH", 256, ecc_gnupg_rpgp},
                    {"nistp384", "NIST", "ECDH", 384, ecc_gnupg_rpgp},
                    {"nistp521", "NIST", "ECDH", 521, ecc_gnupg_rpgp},
                    {"brainpoolp256r1", "BrainPooL", "ECDH", 256, ecc_gnupg23},
                    {"brainpoolp384r1", "BrainPooL", "ECDH", 384, ecc_gnupg23},
                    {"brainpoolp512r1", "BrainPooL", "ECDH", 512, ecc_gnupg23},
                    {"x448", "X448", "ECDH", 448, ecc_gnupg23_rpgp},
                    {"secp256k1", "SECP256K1", "ECDH", 256, ecc_gnupg23},
                });

  algos += ElgamalFamily();
  algos += SlhDsaFamily(kSIGN | kAUTH);

  return algos;
}();

// Refer: https://lists.gnupg.org/pipermail/gnupg-devel/2024-May/035537.html
const QContainer<KeyAlgo> KeyGenerateInfo::kHybridSubKeyAlgos = {
    KeyAlgoSpec{
        .id = "ky768",
        .name = "Kyber",
        .type = "HYBRID-KEM",
        .length = 768,
        .opera = kENCRYPT,
        .support = {{OpenPGPEngine::kGNUPG, "2.5.0"},
                    {OpenPGPEngine::kRPGP, "0.1.2"}},
        .sub_algos = KyberHybridCurves(
            {{OpenPGPEngine::kGNUPG, "2.5.0"}, {OpenPGPEngine::kRPGP, "0.1.2"}},
            {{OpenPGPEngine::kGNUPG, "2.5.0"}}),
        .traits = kPQC},
    KeyAlgoSpec{
        .id = "kyber1024",
        .name = "Kyber",
        .type = "HYBRID-KEM",
        .length = 1024,
        .opera = kENCRYPT,
        .support = {{OpenPGPEngine::kGNUPG, "2.5.0"},
                    {OpenPGPEngine::kRPGP, "0.1.2"}},
        .sub_algos = KyberHybridCurves({{OpenPGPEngine::kGNUPG, "2.5.0"}},
                                       {{OpenPGPEngine::kGNUPG, "2.5.0"},
                                        {OpenPGPEngine::kRPGP, "0.1.2"}}),
        .traits = kPQC},
    KeyAlgoSpec{.id = "mldsa65",
                .name = "ML-DSA",
                .type = "HYBRID-SIGN",
                .length = 65,
                .opera = kSIGN | kAUTH,
                .support = {{OpenPGPEngine::kRPGP, "0.1.2"}},
                .sub_algos = {{FindSubAlgo("ed25519", "EdDSA"),
                               {{OpenPGPEngine::kRPGP, "0.1.2"}}}},
                .traits = kPQC},
    KeyAlgoSpec{.id = "mldsa87",
                .name = "ML-DSA",
                .type = "HYBRID-SIGN",
                .length = 87,
                .opera = kSIGN | kAUTH,
                .support = {{OpenPGPEngine::kRPGP, "0.1.2"}},
                .sub_algos = {{FindSubAlgo("ed448", "EdDSA"),
                               {{OpenPGPEngine::kRPGP, "0.1.2"}}}},
                .traits = kPQC},
};

auto KeyGenerateInfo::GetSupportedKeyAlgo(int channel) -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos;

  for (const auto &algo : kPrimaryKeyAlgos) {
    if (!GpgContextSupportIf(channel, algo.SupportedVersion())) continue;

    algos.append(algo);
  }

  // Add hybrid primary key algos if supported since we only have few hybrid
  // primary key algos, we can directly add them as a normal algo temperately.
  // Add hybrid subkey algos if supported
  for (const auto &algo : kHybridPrimaryKeyAlgo) {
    if (!GpgContextSupportIf(channel, algo.SupportedVersion())) continue;

    algos.append(algo);
  }

  std::sort(algos.begin(), algos.end(), [](const KeyAlgo &a, const KeyAlgo &b) {
    if (a.Name() != b.Name()) return a.Name() < b.Name();
    if (a.KeyLength() != b.KeyLength()) return a.KeyLength() < b.KeyLength();
    if (a.Type() != b.Type()) return a.Type() < b.Type();
    return a.Id() < b.Id();
  });

  return algos;
}

auto KeyGenerateInfo::GetSupportedSubkeyAlgo(int channel)
    -> QContainer<KeyAlgo> {
  QContainer<KeyAlgo> algos;

  for (const auto &algo : kSubKeyAlgos) {
    if (!GpgContextSupportIf(channel, algo.SupportedVersion())) continue;

    algos.append(algo);
  }

  // Add hybrid subkey algos if supported
  for (const auto &algo : kHybridSubKeyAlgos) {
    if (!GpgContextSupportIf(channel, algo.SupportedVersion())) continue;

    algos.append(algo);
  }

  std::sort(algos.begin(), algos.end(), [](const KeyAlgo &a, const KeyAlgo &b) {
    if (a.Name() != b.Name()) return a.Name() < b.Name();
    if (a.KeyLength() != b.KeyLength()) return a.KeyLength() < b.KeyLength();
    if (a.Type() != b.Type()) return a.Type() < b.Type();
    return a.Id() < b.Id();
  });

  return algos;
}

auto KeyGenerateInfo::GetSupportedSubkeyAlgo(int channel, const GpgKey &key)
    -> QContainer<KeyAlgo> {
  auto algos = GetSupportedSubkeyAlgo(channel);
  auto &ctx = OpenPGPContext::GetInstance(channel);
  return RunRegisteredForward<FilterKeyAlgoByKeyTag>(ctx, key, algos);
}

KeyGenerateInfo::KeyGenerateInfo(bool is_subkey)
    : subkey_(is_subkey),
      algo_(kNoneAlgo),
      expired_(QDateTime::currentDateTime().toLocalTime().addYears(2)) {}

auto KeyGenerateInfo::SearchPrimaryKeyAlgo(const QString &algo_id)
    -> std::tuple<bool, KeyAlgo> {
  auto it =
      std::find_if(kPrimaryKeyAlgos.cbegin(), kPrimaryKeyAlgos.cend(),
                   [=](const KeyAlgo &algo) { return algo.Id() == algo_id; });

  if (it != kPrimaryKeyAlgos.cend()) {
    return {true, *it};
  }

  return {false, KeyAlgo{}};
}

auto KeyGenerateInfo::SearchSubKeyAlgo(const QString &algo_id)
    -> std::tuple<bool, KeyAlgo> {
  const auto all_sub_algos = GetSupportedSubkeyAlgo(0);

  auto it = std::find_if(
      all_sub_algos.cbegin(), all_sub_algos.cend(),
      [=](const KeyAlgo &algo) -> bool { return algo.Id() == algo_id; });

  if (it != all_sub_algos.cend()) {
    return {true, *it};
  }

  return {false, KeyAlgo{}};
}

void KeyGenerateInfo::SetAlgo(const KeyAlgo &algo) {
  // reset all options
  reset_options();

  this->SetAllowCert(algo.CanCert());
  this->allow_change_certification_ = false;

  SetAllowEncr(algo.CanEncrypt());
  allow_change_encryption_ = algo.CanEncrypt();

  SetAllowSign(algo.CanSign());
  allow_change_signing_ = algo.CanSign();

  SetAllowAuth(algo.CanAuth());
  allow_change_authentication_ = algo.CanAuth();

  this->algo_ = algo;
}

void KeyGenerateInfo::reset_options() {
  allow_change_encryption_ = true;
  SetAllowEncr(true);

  allow_change_certification_ = true;
  SetAllowCert(true);

  allow_change_signing_ = true;
  SetAllowSign(true);

  allow_change_authentication_ = true;
  SetAllowAuth(true);
}

void KeyGenerateInfo::SetExpireTime(const QDateTime &m_expired) {
  KeyGenerateInfo::expired_ = m_expired;
}

void KeyGenerateInfo::SetNonExpired(bool m_non_expired) {
  if (!m_non_expired) this->expired_ = QDateTime::fromSecsSinceEpoch(0);
  KeyGenerateInfo::non_expired_ = m_non_expired;
}

void KeyGenerateInfo::SetAllowEncr(bool m_allow_encryption) {
  if (allow_change_encryption_) {
    KeyGenerateInfo::allow_encryption_ = m_allow_encryption;
  }
}

void KeyGenerateInfo::SetAllowCert(bool m_allow_certification) {
  if (allow_change_certification_) {
    KeyGenerateInfo::allow_certification_ = m_allow_certification;
  }
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsSubKey() const -> bool { return subkey_; }

/**
 * @brief Set the Is Sub Key object
 *
 * @param m_sub_key
 */
void KeyGenerateInfo::SetIsSubKey(bool m_sub_key) {
  KeyGenerateInfo::subkey_ = m_sub_key;
}

/**
 * @brief Get the Userid object
 *
 * @return QString
 */
[[nodiscard]] auto KeyGenerateInfo::GetUserid() const -> QString {
  return AssembleUserId(name_, comment_, email_);
}

/**
 * @brief Set the Name object
 *
 * @param m_name
 */
void KeyGenerateInfo::SetName(const QString &m_name) { this->name_ = m_name; }

/**
 * @brief Set the Email object
 *
 * @param m_email
 */
void KeyGenerateInfo::SetEmail(const QString &m_email) {
  this->email_ = m_email;
}

/**
 * @brief Set the Comment object
 *
 * @param m_comment
 */
void KeyGenerateInfo::SetComment(const QString &m_comment) {
  this->comment_ = m_comment;
}

/**
 * @brief Get the Name object
 *
 * @return QString
 */
[[nodiscard]] auto KeyGenerateInfo::GetName() const -> QString { return name_; }

/**
 * @brief Get the Email object
 *
 * @return QString
 */
[[nodiscard]] auto KeyGenerateInfo::GetEmail() const -> QString {
  return email_;
}

/**
 * @brief Get the Comment object
 *
 * @return QString
 */
[[nodiscard]] auto KeyGenerateInfo::GetComment() const -> QString {
  return comment_;
}

/**
 * @brief Get the Algo object
 *
 * @return const QString&
 */
[[nodiscard]] auto KeyGenerateInfo::GetAlgo() const -> const KeyAlgo & {
  return algo_;
}

/**
 * @brief Get the Key Size object
 *
 * @return int
 */
[[nodiscard]] auto KeyGenerateInfo::GetKeyLength() const -> int {
  return algo_.KeyLength();
}

/**
 * @brief Get the Expired object
 *
 * @return const QDateTime&
 */
[[nodiscard]] auto KeyGenerateInfo::GetExpireTime() const -> const QDateTime & {
  return expired_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsNonExpired() const -> bool {
  return non_expired_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsNoPassPhrase() const -> bool {
  return this->no_passphrase_;
}

/**
 * @brief Set the Non Pass Phrase object
 *
 * @param m_non_pass_phrase
 */
void KeyGenerateInfo::SetNonPassPhrase(bool m_non_pass_phrase) {
  KeyGenerateInfo::no_passphrase_ = m_non_pass_phrase;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowSign() const -> bool {
  return allow_signing_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowNoPassPhrase() const -> bool {
  return allow_no_pass_phrase_;
}

/**
 * @brief Set the Allow Signing object
 *
 * @param m_allow_signing
 */
void KeyGenerateInfo::SetAllowSign(bool m_allow_signing) {
  if (allow_change_signing_) KeyGenerateInfo::allow_signing_ = m_allow_signing;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowEncr() const -> bool {
  return allow_encryption_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowCert() const -> bool {
  return allow_certification_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowAuth() const -> bool {
  return allow_authentication_;
}

/**
 * @brief Set the Allow Authentication object
 *
 * @param m_allow_authentication
 */
void KeyGenerateInfo::SetAllowAuth(bool m_allow_authentication) {
  if (allow_change_authentication_) {
    KeyGenerateInfo::allow_authentication_ = m_allow_authentication;
  }
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowModifySign() const -> bool {
  return allow_change_signing_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowModifyEncr() const -> bool {
  return allow_change_encryption_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowModifyCert() const -> bool {
  return allow_change_certification_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
[[nodiscard]] auto KeyGenerateInfo::IsAllowModifyAuth() const -> bool {
  return allow_change_authentication_;
}

KeyAlgo::KeyAlgo(const KeyAlgoSpec &spec)
    : id_(spec.id),
      name_(spec.name),
      type_(spec.type),
      length_(spec.length),
      support_ifs_(spec.support),
      sub_algos_(spec.sub_algos),
      traits_(spec.traits) {
  encrypt_ = ((spec.opera & kENCRYPT) != 0);
  sign_ = ((spec.opera & kSIGN) != 0);
  auth_ = ((spec.opera & kAUTH) != 0);
  cert_ = ((spec.opera & kCERT) != 0);
}

auto KeyAlgo::Id() const -> QString { return id_; }

auto KeyAlgo::Name() const -> QString { return name_; }

auto KeyAlgo::KeyLength() const -> int { return length_; }

auto KeyAlgo::Type() const -> QString { return type_; }

auto KeyAlgo::CanEncrypt() const -> bool { return encrypt_; }

auto KeyAlgo::CanSign() const -> bool { return sign_; }

auto KeyAlgo::CanAuth() const -> bool { return auth_; }

auto KeyAlgo::CanCert() const -> bool { return cert_; }

auto KeyAlgo::IsPostQuantum() const -> bool { return (traits_ & kPQC) != 0; }

auto KeyAlgo::IsEcc() const -> bool { return (traits_ & kECC) != 0; }

auto KeyAlgo::SupportedVersion() const -> QContainer<EngineSupportIf> {
  return support_ifs_;
}

auto KeyAlgo::operator==(const KeyAlgo &o) const -> bool {
  return id_ == o.id_ && type_ == o.type_ && length_ == o.length_;
}

auto KeyAlgo::operator!=(const KeyAlgo &o) const -> bool {
  return !(*this == o);
}

[[nodiscard]] auto KeyAlgo::SubAlgos(int channel) const -> QContainer<KeyAlgo> {
  auto result = QContainer<KeyAlgo>();
  for (const auto &sub_algo : sub_algos_) {
    if (GpgContextSupportIf(channel, sub_algo.second)) {
      result.append(sub_algo.first);
    }
  }
  return result;
}

[[nodiscard]] auto KeyGenerateInfo::SubAlgo() const -> const KeyAlgo & {
  return sub_algo_;
}

void KeyGenerateInfo::SetSubAlgo(const KeyAlgo &sub_algo) {
  sub_algo_ = sub_algo;
}
}  // namespace GpgFrontend
