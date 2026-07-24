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

// RFC 9580 decryption known-answer tests for the rPGP engine.
//
// Each case feeds a committed ciphertext vector (produced offline by
// scripts/gen_rpgp_test_vectors.sh) into MessageCryptoOperation::DecryptSync
// and asserts the engine recovers the known plaintext (or, for the legacy
// non-integrity-protected container, refuses to). Vectors cover the RFC 9580
// encryption containers: v1 SEIPD/MDC (sec 5.13.1), v2 SEIPD/AEAD (sec 5.13.2),
// password-based SKESK, multi-recipient PKESK, and the deprecated Tag-9 SED.

#include <cstring>

#include "RpgpCoreTestRfc9580.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

// Decrypt a vector and, on success, return the recovered plaintext buffer.
auto DecryptVector(const QString& name, GpgError* out_err) -> GFBuffer {
  auto ct = LoadRfc9580Vector(name);
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  *out_err = err;
  if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return {};
  EXPECT_TRUE((dobj->Check<GpgDecryptResult, GFBuffer>()));
  return ExtractParams<GFBuffer>(dobj, 1);
}

}  // namespace

// --- v1 SEIPD / MDC (encrypted to fixture key1) -----------------------------

TEST_F(RpgpCoreTest, Rfc9580DecryptV1SeipdRecoversPlaintext) {
  GpgError err;
  auto out = DecryptVector("enc_v1seipd_mdc.pgp", &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out, LoadRfc9580Vector("payload.txt"));
}

TEST_F(RpgpCoreTest, Rfc9580DecryptV1SeipdReportsIntegrityProtected) {
  auto ct = LoadRfc9580Vector("enc_v1seipd_mdc.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgDecryptResult>(dobj, 0);
  // RFC 9580 sec 5.13: a SEIPD packet is integrity protected.
  EXPECT_TRUE(r.MessageIntegrityProtected());
}

TEST_F(RpgpCoreTest, Rfc9580DecryptV1SeipdReportsRecipient) {
  auto ct = LoadRfc9580Vector("enc_v1seipd_mdc.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgDecryptResult>(dobj, 0);
  ASSERT_FALSE(r.Recipients().empty());
  EXPECT_FALSE(r.Recipients().front().keyid.isEmpty());
}

// --- v2 SEIPD / AEAD-OCB (encrypted to the aux v6 key) ----------------------
//
// Decrypting a message addressed to a v6 recipient. analyze_encrypted_envelope()
// now extracts the recipient of a v6 PKESK via pkesk.fingerprint() (pkesk.id()
// errors for v6), and the guard gates on has_pkesk rather than a non-empty
// recipient list. The 64-hex fingerprint resolves the secret key on the C++
// side (which looks up by key-id OR fingerprint), and the unlock-matching logic
// compares the recipient id against both the key ID and the fingerprint.
// RFC 9580 sec 5.13.2.

TEST_F(RpgpCoreTest, Rfc9580DecryptV2SeipdAeadRecoversPlaintext) {
  // The v2/AEAD vector is encrypted to the aux v6 key, whose secret is not in
  // the default fixture keyring; import it first.
  ImportAuxKeys({"aux_v6.asc"});
  GpgError err;
  auto out = DecryptVector("enc_v2seipd_ocb.pgp", &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out, LoadRfc9580Vector("payload.txt"));
}

TEST_F(RpgpCoreTest, Rfc9580DecryptV2SeipdAeadReportsIntegrityProtected) {
  ImportAuxKeys({"aux_v6.asc"});
  auto ct = LoadRfc9580Vector("enc_v2seipd_ocb.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgDecryptResult>(dobj, 0);
  // An AEAD container is inherently integrity protected (sec 5.13.2).
  EXPECT_TRUE(r.MessageIntegrityProtected());
}

TEST_F(RpgpCoreTest, Rfc9580DecryptV2SeipdAeadWithoutKeyFails) {
  // Without importing the aux v6 secret key, the AEAD vector must not decrypt.
  auto ct = LoadRfc9580Vector("enc_v2seipd_ocb.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  EXPECT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
}

// --- Multi-recipient PKESK --------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580DecryptMultiRecipientRecoversPlaintext) {
  // Encrypted to key1+key2+key3; the fixture holds key1's secret.
  GpgError err;
  auto out = DecryptVector("enc_multi_recipient.pgp", &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out, LoadRfc9580Vector("payload.txt"));
}

TEST_F(RpgpCoreTest, Rfc9580DecryptMultiRecipientReportsRecipients) {
  auto ct = LoadRfc9580Vector("enc_multi_recipient.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgDecryptResult>(dobj, 0);
  // The message advertises three PKESK packets.
  EXPECT_GE(r.Recipients().size(), 1);
}

// --- Password-based symmetric (SKESK) ---------------------------------------

TEST_F(RpgpCoreTest, Rfc9580DecryptSymmetricV1RecoversPlaintext) {
  // The fixture passphrase fetcher supplies "123456".
  GpgError err;
  auto out = DecryptVector("enc_symmetric_v1.pgp", &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out, LoadRfc9580Vector("payload.txt"));
}

TEST_F(RpgpCoreTest, Rfc9580DecryptSymmetricV2RecoversPlaintext) {
  GpgError err;
  auto out = DecryptVector("enc_symmetric_v2.pgp", &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out, LoadRfc9580Vector("payload.txt"));
}

TEST_F(RpgpCoreTest, Rfc9580DecryptSymmetricV1ReportsNoRecipient) {
  auto ct = LoadRfc9580Vector("enc_symmetric_v1.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgDecryptResult>(dobj, 0);
  // Password-based encryption carries no public-key recipients.
  EXPECT_TRUE(r.Recipients().empty());
}

TEST_F(RpgpCoreTest, Rfc9580DecryptSymmetricV2ReportsIntegrityProtected) {
  auto ct = LoadRfc9580Vector("enc_symmetric_v2.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgDecryptResult>(dobj, 0);
  EXPECT_TRUE(r.MessageIntegrityProtected());
}

// --- Legacy SED (Tag 9) MUST be rejected (finding H2, RFC 9580 sec 13.7) ----

TEST_F(RpgpCoreTest, Rfc9580DecryptLegacySedIsRejected) {
  auto ct = LoadRfc9580Vector("enc_sed_tag9.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  // The engine must refuse a non-integrity-protected packet outright.
  EXPECT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptLegacySedReleasesNoPlaintext) {
  auto ct = LoadRfc9580Vector("enc_sed_tag9.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
  // Even though the passphrase is known, no plaintext may be surfaced.
  if (dobj->Check<GpgDecryptResult, GFBuffer>()) {
    auto out = ExtractParams<GFBuffer>(dobj, 1);
    EXPECT_TRUE(out.Empty());
  }
}

TEST_F(RpgpCoreTest, Rfc9580DecryptLegacySedReportsErrorDetail) {
  auto ct = LoadRfc9580Vector("enc_sed_tag9.pgp");
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((dobj->Check<GpgDecryptResult, GFBuffer>()));
  auto r = ExtractParams<GpgDecryptResult>(dobj, 0);
  // The user must learn why the message was refused.
  EXPECT_FALSE(r.ErrorDetail().isEmpty());
}

// --- Determinism / stability ------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580DecryptV1SeipdIsRepeatable) {
  GpgError e1;
  auto a = DecryptVector("enc_v1seipd_mdc.pgp", &e1);
  GpgError e2;
  auto b = DecryptVector("enc_v1seipd_mdc.pgp", &e2);
  ASSERT_EQ(CheckGpgError(e1), GPG_ERR_NO_ERROR);
  ASSERT_EQ(CheckGpgError(e2), GPG_ERR_NO_ERROR);
  EXPECT_EQ(a, b);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptExactByteLength) {
  GpgError err;
  auto out = DecryptVector("enc_v1seipd_mdc.pgp", &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  // The known plaintext is a fixed 58-byte string.
  EXPECT_EQ(out.Size(), std::strlen(kRfc9580Payload));
}

TEST_F(RpgpCoreTest, Rfc9580DecryptSymmetricV1MatchesV2Plaintext) {
  GpgError e1;
  auto a = DecryptVector("enc_symmetric_v1.pgp", &e1);
  GpgError e2;
  auto b = DecryptVector("enc_symmetric_v2.pgp", &e2);
  ASSERT_EQ(CheckGpgError(e1), GPG_ERR_NO_ERROR);
  ASSERT_EQ(CheckGpgError(e2), GPG_ERR_NO_ERROR);
  EXPECT_EQ(a, b);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptMultiRecipientMatchesSingle) {
  GpgError e1;
  auto a = DecryptVector("enc_v1seipd_mdc.pgp", &e1);
  GpgError e2;
  auto b = DecryptVector("enc_multi_recipient.pgp", &e2);
  ASSERT_EQ(CheckGpgError(e1), GPG_ERR_NO_ERROR);
  ASSERT_EQ(CheckGpgError(e2), GPG_ERR_NO_ERROR);
  EXPECT_EQ(a, b);
}

}  // namespace GpgFrontend::Test
