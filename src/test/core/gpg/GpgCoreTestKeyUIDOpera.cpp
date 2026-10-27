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

#include "GpgCoreTest.h"
#include "core/GFConstants.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/KeyManagementOperation.h"
#include "core/function/openpgp/UserIdOperation.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/GpgUtils.h"

static const char* test_private_key_data = R"(
-----BEGIN PGP PRIVATE KEY BLOCK-----

lHQEZ0N2exMFK4EEAAoCAwTmBGWDMqzfdx1fkGjuWHP/R6F2ZvvlolcVNJAImWKl
ew0ncmSH1/pOQ7vAud9yPnkDZfhJpcKHl1G4q/Bu5YyiAAEA5sIYAJtXCFrI5upG
Lk8XmdqGCn5c8gKsWVUikrqdKkcOLrQbZmZmZmZmKGZmZmZmKTxmZmZmQGZmZi5m
ZmY+iJkEExMIAEEWIQQwn1gjkF+wDtnB6TLy2N+l8QneRwUCZ0N2ewIbIwUJA8Jm
8wULCQgHAgIiAgYVCgkICwIEFgIDAQIeBwIXgAAKCRDy2N+l8QneR2ptAQCRE6T3
HhNp7CHVk1DkxTIj4i7Mw1Em7Cwvctr8usPhBwD/YECZLMowPLNZO4GFhIH+1Etd
GZ2d0Gbb51DUlPZUO2K0HGdnZ2dnZyhnZ2dnZyk8Z2dnZ2dAZ2dnLmdnZz6ImQQT
EwgAQRYhBDCfWCOQX7AO2cHpMvLY36XxCd5HBQJnRjCtAhsjBQkDwmbzBQsJCAcC
AiICBhUKCQgLAgQWAgMBAh4HAheAAAoJEPLY36XxCd5HaD8BAPvBCdzFSeGXyFXh
4Sn4tVpwlnJOwzC0ECGYSieaNjocAQChWIXVinXAQ5U/oslqR2+Gg1s2o8gTKVbv
ZE6T+6Vvc7QgaGhoaGhoKGhoaGhoaGgpPGhoaGhoQGhoaGguaGhoaD6ImQQTEwgA
QRYhBDCfWCOQX7AO2cHpMvLY36XxCd5HBQJnRjC2AhsjBQkDwmbzBQsJCAcCAiIC
BhUKCQgLAgQWAgMBAh4HAheAAAoJEPLY36XxCd5HqE0A/R9wq+ZobC2Iztoudcg/
eKXp1hs1D9zv7R6KkLdbB4zXAP0ch5qZnrb+U/wIuhq+oOwJknMsD/njB263eUgb
AXNh2rQdeXl5eXkoeXl5eXl5KTx5eXl5eUB5eXl5Lnl5eT6ImQQTEwgAQRYhBDCf
WCOQX7AO2cHpMvLY36XxCd5HBQJnRjDBAhsjBQkDwmbzBQsJCAcCAiICBhUKCQgL
AgQWAgMBAh4HAheAAAoJEPLY36XxCd5HcycA/RwytAvY8ryaR4qUPN8g1NSX3Dv8
SHYYu2cZJuck+lVxAQDxcyljEIZKVqOpNfWRZyqcRvE8kr64PymJTAPYOVdBuA==
=4Hms
-----END PGP PRIVATE KEY BLOCK-----
)";

namespace GpgFrontend::Test {

TEST_F(GpgCoreTest, CoreDeleteUIDTestA) {
  auto info = KeyImportExportOperation::GetInstance().ImportKey(
      GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr("F2D8DFA5F109DE47");
  ASSERT_TRUE(key->IsGood());

  auto uids = key->UIDs();

  ASSERT_EQ(uids.size(), 4);
  ASSERT_EQ(uids[2].GetUID(), "gggggg(ggggg)<ggggg@ggg.ggg>");

  ASSERT_OP_WITHIN(UserIdOperation::GetInstance().DeleteUID(
                       key, "gggggg(ggggg)<ggggg@ggg.ggg>"),
                   3000);

  GpgKeyRepository::GetInstance().FlushKeyCache();
  key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("F2D8DFA5F109DE47");
  ASSERT_TRUE(key != nullptr);

  uids = key->UIDs();

  ASSERT_EQ(uids.size(), 3);
  ASSERT_EQ(uids[2].GetUID(), "hhhhhh(hhhhhhh)<hhhhh@hhhh.hhhh>");

  KeyManagementOperation::GetInstance().DeleteKey(key);
  GpgKeyRepository::GetInstance().FlushKeyCache();
}

TEST_F(GpgCoreTest, CoreRevokeUIDTestA) {
  GpgKeyRepository::GetInstance().FlushKeyCache();
  auto info = KeyImportExportOperation::GetInstance().ImportKey(
      GFBuffer(QString::fromLatin1(test_private_key_data)));

  ASSERT_EQ(info->not_imported, 0);
  ASSERT_EQ(info->imported, 1);

  auto key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                 .GetKeyPtr("F2D8DFA5F109DE47");
  ASSERT_TRUE(key != nullptr);

  auto uids = key->UIDs();

  ASSERT_EQ(uids.size(), 4);
  ASSERT_EQ(uids[2].GetUID(), "gggggg(ggggg)<ggggg@ggg.ggg>");

  ASSERT_OP_WITHIN(
      UserIdOperation::GetInstance().RevokeUID(
          key, "gggggg(ggggg)<ggggg@ggg.ggg>", 4, "H\nEEEEEL\n\n\n\nL   \n0\n"),
      3000);

  GpgKeyRepository::GetInstance().FlushKeyCache();
  key = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
            .GetKeyPtr("F2D8DFA5F109DE47");
  ASSERT_TRUE(key != nullptr);

  uids = key->UIDs();

  ASSERT_EQ(uids.size(), 4);
  ASSERT_EQ(uids[2].GetUID(), "gggggg(ggggg)<ggggg@ggg.ggg>");
  ASSERT_TRUE(uids[2].GetRevoked());

  KeyManagementOperation::GetInstance().DeleteKey(key);
  GpgKeyRepository::GetInstance().FlushKeyCache();
}

namespace {

// Unprotected RSA test key from :/test/gpg_keys (pv3.key), able to certify.
constexpr auto kCertifierKeyId = "8465C55B25C9B7D1";
constexpr auto kTargetKeyId = "F2D8DFA5F109DE47";

auto ImportCertifyTarget() -> GpgKeyPtr {
  GpgKeyRepository::GetInstance().FlushKeyCache();
  KeyImportExportOperation::GetInstance().ImportKey(
      GFBuffer(QString::fromLatin1(test_private_key_data)));
  return GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
      .GetKeyPtr(kTargetKeyId);
}

auto ReloadKey(const QString& key_id) -> GpgKeyPtr {
  GpgKeyRepository::GetInstance().FlushKeyCache();
  return GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
      .GetKeyPtr(key_id);
}

auto CertifierKeys() -> GpgAbstractKeyPtrList {
  auto signer = GpgKeyRepository::GetInstance(kGpgFrontendDefaultChannel)
                    .GetKeyPtr(kCertifierKeyId);
  if (signer == nullptr) return {};
  return {signer};
}

// The certifier's signatures on a user ID, ignoring revocation packets.
auto CertificationsBy(const GpgUID& uid, const QString& signer_id)
    -> QList<GpgKeySignature> {
  QList<GpgKeySignature> found;
  auto signatures = uid.GetSignatures();
  for (const auto& sig : *signatures) {
    if (sig.GetKeyID() == signer_id && !sig.IsRevoked()) found.push_back(sig);
  }
  return found;
}

auto RevocationsBy(const GpgUID& uid, const QString& signer_id) -> int {
  int count = 0;
  auto signatures = uid.GetSignatures();
  for (const auto& sig : *signatures) {
    if (sig.GetKeyID() == signer_id && sig.IsRevoked()) count++;
  }
  return count;
}

void DeleteCertifyTarget() {
  auto key = ReloadKey(kTargetKeyId);
  if (key != nullptr) KeyManagementOperation::GetInstance().DeleteKey(key);
  GpgKeyRepository::GetInstance().FlushKeyCache();
}

}  // namespace

TEST_F(GpgCoreTest, CoreSignKeyUIDWithExpiryTestA) {
  auto key = ImportCertifyTarget();
  ASSERT_TRUE(key != nullptr);
  const auto signers = CertifierKeys();
  ASSERT_EQ(signers.size(), 1);

  const auto target_uid = key->UIDs()[1].GetUID();
  const auto expires = QDateTime::currentDateTime().addYears(1);

  auto [err, _] = KeyManagementOperation::GetInstance().SignKeySync(
      key, signers, target_uid, expires);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  key = ReloadKey(kTargetKeyId);
  ASSERT_TRUE(key != nullptr);
  const auto uids = key->UIDs();
  ASSERT_EQ(uids.size(), 4);

  // Only the chosen user ID is certified, by the chosen signer (not gpg's
  // default key).
  for (int i = 0; i < uids.size(); i++) {
    const auto sigs = CertificationsBy(uids[i], kCertifierKeyId);
    if (i != 1) {
      EXPECT_TRUE(sigs.isEmpty()) << "uid " << i;
      continue;
    }
    ASSERT_EQ(sigs.size(), 1);

    // gpgme takes seconds from now; an absolute timestamp there would land
    // decades later.
    const auto delta = std::llabs(sigs.front().GetExpireTime().secsTo(expires));
    EXPECT_LT(delta, 120)
        << sigs.front().GetExpireTime().toString().toStdString();
  }

  DeleteCertifyTarget();
}

TEST_F(GpgCoreTest, CoreSignKeyUIDNoExpiryTestA) {
  auto key = ImportCertifyTarget();
  ASSERT_TRUE(key != nullptr);

  auto [err, _] = KeyManagementOperation::GetInstance().SignKeySync(
      key, CertifierKeys(), key->UIDs()[2].GetUID(), std::nullopt);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  key = ReloadKey(kTargetKeyId);
  ASSERT_TRUE(key != nullptr);
  const auto sigs = CertificationsBy(key->UIDs()[2], kCertifierKeyId);
  ASSERT_EQ(sigs.size(), 1);
  EXPECT_EQ(sigs.front().GetExpireTime().toSecsSinceEpoch(), 0);

  DeleteCertifyTarget();
}

TEST_F(GpgCoreTest, CoreSignKeyAllUIDsTestA) {
  auto key = ImportCertifyTarget();
  ASSERT_TRUE(key != nullptr);

  // An empty user ID means every user ID, not a search for "".
  auto [err, _] = KeyManagementOperation::GetInstance().SignKeySync(
      key, CertifierKeys(), QString{}, std::nullopt);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  key = ReloadKey(kTargetKeyId);
  ASSERT_TRUE(key != nullptr);
  for (const auto& uid : key->UIDs()) {
    EXPECT_EQ(CertificationsBy(uid, kCertifierKeyId).size(), 1)
        << uid.GetUID().toStdString();
  }

  DeleteCertifyTarget();
}

TEST_F(GpgCoreTest, CoreSignKeyRejectsBadInputTestA) {
  auto key = ImportCertifyTarget();
  ASSERT_TRUE(key != nullptr);
  const auto target_uid = key->UIDs()[1].GetUID();

  // No usable signer must fail, never fall back to gpg's default key.
  auto [no_signer_err, _1] = KeyManagementOperation::GetInstance().SignKeySync(
      key, {}, target_uid, std::nullopt);
  EXPECT_EQ(CheckGpgError(no_signer_err), GPG_ERR_NO_SECKEY);

  auto [past_err, _2] = KeyManagementOperation::GetInstance().SignKeySync(
      key, CertifierKeys(), target_uid,
      QDateTime::currentDateTime().addDays(-1));
  EXPECT_EQ(CheckGpgError(past_err), GPG_ERR_INV_VALUE);

  key = ReloadKey(kTargetKeyId);
  ASSERT_TRUE(key != nullptr);
  for (const auto& uid : key->UIDs()) {
    auto signatures = uid.GetSignatures();
    for (const auto& sig : *signatures) {
      // Only the key's own self-signatures.
      EXPECT_EQ(sig.GetKeyID(), kTargetKeyId) << uid.GetUID().toStdString();
    }
  }

  DeleteCertifyTarget();
}

TEST_F(GpgCoreTest, CoreRevKeySignatureTestA) {
  auto key = ImportCertifyTarget();
  ASSERT_TRUE(key != nullptr);
  const auto target_uid = key->UIDs()[1].GetUID();

  auto [sign_err, _1] = KeyManagementOperation::GetInstance().SignKeySync(
      key, CertifierKeys(), target_uid, std::nullopt);
  ASSERT_EQ(CheckGpgError(sign_err), GPG_ERR_NO_ERROR);

  key = ReloadKey(kTargetKeyId);
  ASSERT_TRUE(key != nullptr);

  // A signer with no key here at all cannot revoke.
  auto [missing_err, _2] =
      KeyManagementOperation::GetInstance().RevKeySignatureSync(
          key, {{"0123456789ABCDEF", target_uid}});
  EXPECT_EQ(CheckGpgError(missing_err), GPG_ERR_NO_SECKEY);

  // The pair names the user ID on the certified key, not the signer's.
  auto [rev_err, _3] =
      KeyManagementOperation::GetInstance().RevKeySignatureSync(
          key, {{kCertifierKeyId, target_uid}});
  ASSERT_EQ(CheckGpgError(rev_err), GPG_ERR_NO_ERROR);

  key = ReloadKey(kTargetKeyId);
  ASSERT_TRUE(key != nullptr);
  EXPECT_EQ(RevocationsBy(key->UIDs()[1], kCertifierKeyId), 1);

  DeleteCertifyTarget();
}

}  // namespace GpgFrontend::Test