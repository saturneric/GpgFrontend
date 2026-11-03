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

#include "core/function/gpg/GpgModelConvert.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignature.h"
#include "core/model/GpgVerifyResult.h"

namespace GpgFrontend::Test {

// A default-constructed GpgVerifyResult / GpgSignature is what every *failed*
// verify operation produces: neither the gpgme handle nor the rPGP payload is
// set. These paths need no engine, which is precisely why they are reachable
// when the engine is unavailable or the operation errored out.

TEST(SignatureModelTest, DefaultVerifyResultIsNotGood) {
  GpgVerifyResult result;

  EXPECT_FALSE(result.IsGood());
  EXPECT_TRUE(result.GetSignature().isEmpty());
}

// An rPGP result carries no gpgme handle; it is still a result, as it is for
// the sign, decrypt and verify models.
TEST(SignatureModelTest, EngineNeutralEncryptResultIsGood) {
  EXPECT_FALSE(GpgEncryptResult().IsGood());
  EXPECT_TRUE(GpgEncryptResult(GFEncryptResult{}).IsGood());
}

// No gpgme result means the operation produced none: never a good result.
TEST(SignatureModelTest, MissingGpgmeResultsAreNotGood) {
  EXPECT_FALSE(EncryptResultOf(nullptr).IsGood());
  EXPECT_FALSE(DecryptResultOf(nullptr).IsGood());
}

// The engine-neutral defaults are what rPGP always means.
TEST(SignatureModelTest, EngineNeutralDecryptResultDefaults) {
  GpgDecryptResult result{GFDecryptResult{}};

  EXPECT_TRUE(result.IsGood());
  EXPECT_FALSE(result.MIME());
  EXPECT_TRUE(result.MessageIntegrityProtected());
  EXPECT_TRUE(result.UnsupportedAlgorithm().isEmpty());
  EXPECT_FALSE(GpgDecryptResult().MessageIntegrityProtected());
}

TEST(SignatureModelTest, GpgmeDecryptResultIsCarriedOver) {
  _gpgme_recipient recipient{};
  std::array<char, 17> keyid{"0123456789ABCDEF"};
  recipient.keyid = keyid.data();
  recipient.pubkey_algo = GPGME_PK_RSA;
  recipient.status = GPG_ERR_NO_SECKEY;

  std::array<char, 9> file_name{"note.txt"};
  _gpgme_op_decrypt_result raw{};
  raw.file_name = file_name.data();
  raw.is_mime = 1;
  raw.legacy_cipher_nomdc = 1;
  raw.recipients = &recipient;

  auto result = DecryptResultOf(&raw);
  ASSERT_TRUE(result.IsGood());
  EXPECT_EQ(result.Filename(), "note.txt");
  EXPECT_TRUE(result.MIME());
  EXPECT_FALSE(result.MessageIntegrityProtected());

  auto recipients = result.Recipients();
  ASSERT_EQ(recipients.size(), 1);
  EXPECT_EQ(recipients.front().keyid, "0123456789ABCDEF");
  EXPECT_EQ(recipients.front().status.code, GFErrorCode::kNO_SECRET_KEY);
}

TEST(SignatureModelTest, GpgmeInvalidRecipientsAreCarriedOver) {
  std::array<char, 5> fpr{"ABCD"};
  _gpgme_invalid_key invalid{};
  invalid.fpr = fpr.data();
  invalid.reason = GPG_ERR_UNUSABLE_PUBKEY;

  _gpgme_op_encrypt_result raw{};
  raw.invalid_recipients = &invalid;

  auto result = EncryptResultOf(&raw);
  ASSERT_TRUE(result.IsGood());
  auto invalids = result.InvalidRecipients();
  ASSERT_EQ(invalids.size(), 1);
  EXPECT_EQ(std::get<0>(invalids.front()), "ABCD");
  EXPECT_EQ(std::get<1>(invalids.front()).code,
            GFErrorCode::kUNUSABLE_PUBLIC_KEY);
}

TEST(SignatureModelTest, DefaultVerifyResultReturnsNoSignatures) {
  GpgVerifyResult result;

  auto signatures = result.GetSignature();
  EXPECT_TRUE(signatures.isEmpty());
}

TEST(SignatureModelTest, DefaultVerifyResultErrorDetailIsEmpty) {
  GpgVerifyResult result;

  EXPECT_TRUE(result.ErrorDetail().isEmpty());
}

TEST(SignatureModelTest, DefaultSignatureAccessorsDoNotCrash) {
  GpgSignature sig;

  // every accessor must tolerate the unset state rather than dereferencing a
  // null signature_ref_.
  EXPECT_NO_FATAL_FAILURE({
    (void)sig.GetValidity();
    (void)sig.GetStatus();
    (void)sig.GetSummary();
    (void)sig.GetPubkeyAlgo();
    (void)sig.GetHashAlgo();
    (void)sig.GetCreateTime();
    (void)sig.GetExpireTime();
    (void)sig.GetFingerprint();
    (void)sig.GetSigType();
  });
}

TEST(SignatureModelTest, DefaultSignatureReportsUnknownValidity) {
  GpgSignature sig;

  EXPECT_EQ(sig.GetValidity(), GpgValidity::kUNKNOWN);
}

TEST(SignatureModelTest, DefaultSignatureReportsFailureStatus) {
  GpgSignature sig;

  // an unset signature must never read as a good signature.
  EXPECT_FALSE(sig.GetStatus().Ok());
  EXPECT_EQ((sig.GetSummary() & kSIGSUM_VALID), 0U);
  EXPECT_EQ((sig.GetSummary() & kSIGSUM_GREEN), 0U);
}

TEST(SignatureModelTest, DefaultSignatureReturnsEmptyStrings) {
  GpgSignature sig;

  EXPECT_TRUE(sig.GetPubkeyAlgo().isEmpty());
  EXPECT_TRUE(sig.GetHashAlgo().isEmpty());
  EXPECT_TRUE(sig.GetFingerprint().isEmpty());
  EXPECT_TRUE(sig.GetSigType().isEmpty());
}

TEST(SignatureModelTest, CopiedDefaultSignatureStaysSafe) {
  GpgSignature sig;
  GpgSignature copy(
      sig);  // NOLINT(performance-unnecessary-copy-initialization)

  EXPECT_EQ(copy.GetValidity(), GpgValidity::kUNKNOWN);
  EXPECT_TRUE(copy.GetFingerprint().isEmpty());
}

}  // namespace GpgFrontend::Test
