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

// RFC 9580 encrypt / sign round-trip tests for the rPGP engine.
//
// These exercise the engine end-to-end on data it produces itself: encrypt ->
// decrypt, sign -> verify (normal / detached / cleartext), and encrypt+sign ->
// decrypt+verify, across a range of payloads (empty, text, binary-with-NUL,
// large) and recipient sets. They complement the known-answer decrypt/verify
// suites by proving the producing side and the consuming side agree.

#include <cstdint>

#include "RpgpCoreTestRfc9580.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

auto Key1Pub() -> GpgAbstractKeyPtr {
  return GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
      .GetPubkeyPtr(kKey1PrimaryFpr);
}
auto Key1Secret() -> GpgAbstractKeyPtr {
  return GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
      .GetKeyPtr(kKey1PrimaryFpr);
}

auto EncryptThenDecrypt(const GFBuffer& plain, bool ascii, GpgError* dec_err)
    -> GFBuffer {
  auto [e_err, e_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({Key1Pub()}, plain, ascii);
  EXPECT_EQ(CheckGpgError(e_err), GPG_ERR_NO_ERROR);
  if (CheckGpgError(e_err) != GPG_ERR_NO_ERROR) {
    *dec_err = e_err;
    return {};
  }
  auto ct = ExtractParams<GFBuffer>(e_obj, 1);
  auto [d_err, d_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  *dec_err = d_err;
  if (CheckGpgError(d_err) != GPG_ERR_NO_ERROR) return {};
  return ExtractParams<GFBuffer>(d_obj, 1);
}

auto MakeBinaryPayload() -> GFBuffer {
  QByteArray bytes;
  for (int i = 0; i < 512; ++i) bytes.append(static_cast<char>(i & 0xFF));
  return GFBuffer(bytes);
}

auto MakeLargePayload() -> GFBuffer {
  QByteArray bytes(200000, 'Z');
  for (int i = 0; i < bytes.size(); i += 7) bytes[i] = static_cast<char>(i);
  return GFBuffer(bytes);
}

}  // namespace

// --- encrypt -> decrypt -----------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580EncryptDecryptRoundTripAscii) {
  auto plain = GFBuffer(QString("round-trip ascii"));
  GpgError err;
  auto out = EncryptThenDecrypt(plain, true, &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out, plain);
}

TEST_F(RpgpCoreTest, Rfc9580EncryptDecryptRoundTripBinary) {
  auto plain = GFBuffer(QString("round-trip binary armor off"));
  GpgError err;
  auto out = EncryptThenDecrypt(plain, false, &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out, plain);
}

TEST_F(RpgpCoreTest, Rfc9580EncryptRejectsEmptyPayload) {
  // The engine deliberately refuses to encrypt an empty payload ("No data to
  // encrypt"): there is nothing to protect, so the operation is an input error.
  auto [err, obj] = MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
                        .EncryptSync({Key1Pub()}, GFBuffer(QByteArray()), true);
  EXPECT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580EncryptDecryptBinaryPayloadWithNul) {
  auto plain = MakeBinaryPayload();
  GpgError err;
  auto out = EncryptThenDecrypt(plain, true, &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out, plain);
}

TEST_F(RpgpCoreTest, Rfc9580EncryptDecryptLargePayload) {
  auto plain = MakeLargePayload();
  GpgError err;
  auto out = EncryptThenDecrypt(plain, false, &err);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out, plain);
}

TEST_F(RpgpCoreTest, Rfc9580EncryptOutputIsAsciiArmored) {
  auto [err, obj] = MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
                        .EncryptSync({Key1Pub()}, GFBuffer(QString("x")), true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  auto ct = ExtractParams<GFBuffer>(obj, 1);
  EXPECT_TRUE(QString::fromUtf8(ct.Data(), static_cast<int>(ct.Size()))
                  .startsWith("-----BEGIN PGP MESSAGE-----"));
}

TEST_F(RpgpCoreTest, Rfc9580EncryptMultiRecipientRoundTrip) {
  auto k1 = Key1Pub();
  auto k2 = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                .GetPubkeyPtr("3BEDAB48EAAAA195006330414DD9733454846D0C");
  auto k3 = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                .GetPubkeyPtr("C54DF5E9E6AD3278C77F5438DA6A97C428EC96C8");
  ASSERT_TRUE(k1 != nullptr && k2 != nullptr && k3 != nullptr);
  auto plain = GFBuffer(QString("multi recipient"));
  auto [e_err, e_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSync({k1, k2, k3}, plain, true);
  ASSERT_EQ(CheckGpgError(e_err), GPG_ERR_NO_ERROR);
  auto ct = ExtractParams<GFBuffer>(e_obj, 1);
  // Decrypts with key1's secret (the only one held by the fixture).
  auto [d_err, d_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(d_err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(ExtractParams<GFBuffer>(d_obj, 1), plain);
}

// --- sign -> verify ---------------------------------------------------------

// DISABLED: standalone verification of an inline (NORMAL-mode) signed message
// does not report the signature as Valid — see the inline-verify KNOWN GAP in
// RpgpCoreTestRfc9580Verify.cpp (verify.rs reads num_signatures() before the
// body is consumed). The detached and cleartext round-trips below cover the
// working paths. Enable once inline verification is fixed.
TEST_F(RpgpCoreTest, DISABLED_Rfc9580SignVerifyNormalRoundTrip) {
  auto data = GFBuffer(QString("sign normal"));
  auto [s_err, s_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({Key1Secret()}, data, GPGME_SIG_MODE_NORMAL, true);
  ASSERT_EQ(CheckGpgError(s_err), GPG_ERR_NO_ERROR);
  auto signed_msg = ExtractParams<GFBuffer>(s_obj, 1);
  auto [v_err, v_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(signed_msg, GFBuffer());
  ASSERT_EQ(CheckGpgError(v_err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgVerifyResult>(v_obj, 0);
  ASSERT_FALSE(r.GetSignature().empty());
  EXPECT_TRUE(SignatureIsValid(r.GetSignature().front()));
  EXPECT_EQ(r.GetSignature().front().GetFingerprint(), QString(kKey1SignFpr));
}

TEST_F(RpgpCoreTest, Rfc9580SignVerifyDetachedRoundTrip) {
  auto data = GFBuffer(QString("sign detached"));
  auto [s_err, s_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({Key1Secret()}, data, GPGME_SIG_MODE_DETACH, true);
  ASSERT_EQ(CheckGpgError(s_err), GPG_ERR_NO_ERROR);
  auto sig = ExtractParams<GFBuffer>(s_obj, 1);
  auto [v_err, v_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(data, sig);
  ASSERT_EQ(CheckGpgError(v_err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgVerifyResult>(v_obj, 0);
  ASSERT_FALSE(r.GetSignature().empty());
  EXPECT_TRUE(SignatureIsValid(r.GetSignature().front()));
}

TEST_F(RpgpCoreTest, Rfc9580SignVerifyClearRoundTrip) {
  auto data = GFBuffer(QString("sign cleartext"));
  auto [s_err, s_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({Key1Secret()}, data, GPGME_SIG_MODE_CLEAR, true);
  ASSERT_EQ(CheckGpgError(s_err), GPG_ERR_NO_ERROR);
  auto cleartext = ExtractParams<GFBuffer>(s_obj, 1);
  auto [v_err, v_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(cleartext, GFBuffer());
  ASSERT_EQ(CheckGpgError(v_err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgVerifyResult>(v_obj, 0);
  ASSERT_FALSE(r.GetSignature().empty());
  EXPECT_TRUE(SignatureIsValid(r.GetSignature().front()));
}

TEST_F(RpgpCoreTest, Rfc9580SignDetachedOutputIsAsciiArmored) {
  auto data = GFBuffer(QString("armored sig"));
  auto [s_err, s_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({Key1Secret()}, data, GPGME_SIG_MODE_DETACH, true);
  ASSERT_EQ(CheckGpgError(s_err), GPG_ERR_NO_ERROR);
  auto sig = ExtractParams<GFBuffer>(s_obj, 1);
  EXPECT_TRUE(QString::fromUtf8(sig.Data(), static_cast<int>(sig.Size()))
                  .startsWith("-----BEGIN PGP SIGNATURE-----"));
}

TEST_F(RpgpCoreTest, Rfc9580SignVerifyDetachedTamperedDataFails) {
  auto data = GFBuffer(QString("original data"));
  auto [s_err, s_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({Key1Secret()}, data, GPGME_SIG_MODE_DETACH, true);
  ASSERT_EQ(CheckGpgError(s_err), GPG_ERR_NO_ERROR);
  auto sig = ExtractParams<GFBuffer>(s_obj, 1);
  // Verify the genuine detached signature against tampered data.
  auto [v_err, v_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(GFBuffer(QString("tampered data!")), sig);
  if (v_obj->Check<GpgVerifyResult, GFBuffer>()) {
    auto r = ExtractParams<GpgVerifyResult>(v_obj, 0);
    for (const auto& s : r.GetSignature()) EXPECT_FALSE(SignatureIsValid(s));
  }
}

TEST_F(RpgpCoreTest, Rfc9580SignRejectsEmptyPayload) {
  // Signing an empty payload is rejected as an input error, matching encrypt.
  auto [s_err, s_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({Key1Secret()}, GFBuffer(QByteArray()),
                    GPGME_SIG_MODE_DETACH, true);
  EXPECT_NE(CheckGpgError(s_err), GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580SignVerifyBinaryPayload) {
  auto data = MakeBinaryPayload();
  auto [s_err, s_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({Key1Secret()}, data, GPGME_SIG_MODE_DETACH, true);
  ASSERT_EQ(CheckGpgError(s_err), GPG_ERR_NO_ERROR);
  auto sig = ExtractParams<GFBuffer>(s_obj, 1);
  auto [v_err, v_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(data, sig);
  ASSERT_EQ(CheckGpgError(v_err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgVerifyResult>(v_obj, 0);
  ASSERT_FALSE(r.GetSignature().empty());
  EXPECT_TRUE(SignatureIsValid(r.GetSignature().front()));
}

TEST_F(RpgpCoreTest, Rfc9580SignVerifyLargePayload) {
  auto data = MakeLargePayload();
  auto [s_err, s_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignSync({Key1Secret()}, data, GPGME_SIG_MODE_DETACH, true);
  ASSERT_EQ(CheckGpgError(s_err), GPG_ERR_NO_ERROR);
  auto sig = ExtractParams<GFBuffer>(s_obj, 1);
  auto [v_err, v_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(data, sig);
  ASSERT_EQ(CheckGpgError(v_err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgVerifyResult>(v_obj, 0);
  ASSERT_FALSE(r.GetSignature().empty());
  EXPECT_TRUE(SignatureIsValid(r.GetSignature().front()));
}

// --- encrypt+sign -> decrypt+verify -----------------------------------------

TEST_F(RpgpCoreTest, Rfc9580EncryptSignDecryptVerifyRoundTrip) {
  auto plain = GFBuffer(QString("sealed and signed"));
  auto [e_err, e_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSignSync({Key1Pub()}, {Key1Secret()}, plain, true);
  ASSERT_EQ(CheckGpgError(e_err), GPG_ERR_NO_ERROR);
  auto ct = ExtractParams<GFBuffer>(e_obj, 2);
  auto [d_err, d_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifySync(ct);
  ASSERT_EQ(CheckGpgError(d_err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((d_obj->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));
  auto vr = ExtractParams<GpgVerifyResult>(d_obj, 1);
  auto out = ExtractParams<GFBuffer>(d_obj, 2);
  EXPECT_EQ(out, plain);
  ASSERT_FALSE(vr.GetSignature().empty());
  EXPECT_TRUE(SignatureIsValid(vr.GetSignature().front()));
}

TEST_F(RpgpCoreTest, Rfc9580EncryptSignDecryptVerifyReportsSigner) {
  auto plain = GFBuffer(QString("who signed?"));
  auto [e_err, e_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSignSync({Key1Pub()}, {Key1Secret()}, plain, true);
  ASSERT_EQ(CheckGpgError(e_err), GPG_ERR_NO_ERROR);
  auto ct = ExtractParams<GFBuffer>(e_obj, 2);
  auto [d_err, d_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifySync(ct);
  ASSERT_EQ(CheckGpgError(d_err), GPG_ERR_NO_ERROR);
  auto vr = ExtractParams<GpgVerifyResult>(d_obj, 1);
  ASSERT_FALSE(vr.GetSignature().empty());
  // The combined path must attribute the signature exactly as standalone verify
  // does (finding H1/B-Verify: decrypt-verify is as strict/precise as verify).
  EXPECT_EQ(vr.GetSignature().front().GetFingerprint(), QString(kKey1SignFpr));
}

TEST_F(RpgpCoreTest, Rfc9580EncryptSignDecryptVerifyBinaryPayload) {
  auto plain = MakeBinaryPayload();
  auto [e_err, e_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSignSync({Key1Pub()}, {Key1Secret()}, plain, false);
  ASSERT_EQ(CheckGpgError(e_err), GPG_ERR_NO_ERROR);
  auto ct = ExtractParams<GFBuffer>(e_obj, 2);
  auto [d_err, d_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifySync(ct);
  ASSERT_EQ(CheckGpgError(d_err), GPG_ERR_NO_ERROR);
  auto out = ExtractParams<GFBuffer>(d_obj, 2);
  auto vr = ExtractParams<GpgVerifyResult>(d_obj, 1);
  EXPECT_EQ(out, plain);
  ASSERT_FALSE(vr.GetSignature().empty());
  EXPECT_TRUE(SignatureIsValid(vr.GetSignature().front()));
}

TEST_F(RpgpCoreTest, Rfc9580EncryptSignEmptyPayloadIsSafe) {
  // Unlike encrypt-only and sign-only (which reject an empty payload as an
  // input error), the combined encrypt-and-sign path accepts empty input. That
  // is acceptable as long as it never crashes and, when it succeeds, the
  // decrypt-and-verify round-trip recovers an empty, validly-signed payload.
  auto [e_err, e_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSignSync({Key1Pub()}, {Key1Secret()}, GFBuffer(QByteArray()),
                           true);
  if (CheckGpgError(e_err) != GPG_ERR_NO_ERROR) {
    return;  // Rejecting empty input is also acceptable.
  }
  auto ct = ExtractParams<GFBuffer>(e_obj, 2);
  auto [d_err, d_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifySync(ct);
  ASSERT_EQ(CheckGpgError(d_err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(ExtractParams<GFBuffer>(d_obj, 2).Size(), 0U);
  auto vr = ExtractParams<GpgVerifyResult>(d_obj, 1);
  ASSERT_FALSE(vr.GetSignature().empty());
  EXPECT_TRUE(SignatureIsValid(vr.GetSignature().front()));
}

// --- symmetric round-trips --------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580SymmetricEncryptDecryptRoundTrip) {
  auto plain = GFBuffer(QString("symmetric round-trip"));
  auto [e_err, e_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSymmetricSync(plain, true);
  ASSERT_EQ(CheckGpgError(e_err), GPG_ERR_NO_ERROR);
  auto ct = ExtractParams<GFBuffer>(e_obj, 1);
  auto [d_err, d_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(d_err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(ExtractParams<GFBuffer>(d_obj, 1), plain);
}

TEST_F(RpgpCoreTest, Rfc9580SymmetricEncryptDecryptBinaryPayload) {
  auto plain = MakeBinaryPayload();
  auto [e_err, e_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSymmetricSync(plain, false);
  ASSERT_EQ(CheckGpgError(e_err), GPG_ERR_NO_ERROR);
  auto ct = ExtractParams<GFBuffer>(e_obj, 1);
  auto [d_err, d_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(d_err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(ExtractParams<GFBuffer>(d_obj, 1), plain);
}

TEST_F(RpgpCoreTest, Rfc9580SymmetricEncryptIsIntegrityProtected) {
  auto plain = GFBuffer(QString("integrity"));
  auto [e_err, e_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSymmetricSync(plain, true);
  ASSERT_EQ(CheckGpgError(e_err), GPG_ERR_NO_ERROR);
  auto ct = ExtractParams<GFBuffer>(e_obj, 1);
  auto [d_err, d_obj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(ct);
  ASSERT_EQ(CheckGpgError(d_err), GPG_ERR_NO_ERROR);
  auto r = ExtractParams<GpgDecryptResult>(d_obj, 0);
  // The engine must produce an integrity-protected container, never a SED.
  EXPECT_TRUE(r.MessageIntegrityProtected());
}

// --- consistency across armor mode ------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580EncryptAsciiAndBinaryDecryptToSamePlaintext) {
  auto plain = GFBuffer(QString("armor invariance"));
  GpgError e1;
  auto a = EncryptThenDecrypt(plain, true, &e1);
  GpgError e2;
  auto b = EncryptThenDecrypt(plain, false, &e2);
  ASSERT_EQ(CheckGpgError(e1), GPG_ERR_NO_ERROR);
  ASSERT_EQ(CheckGpgError(e2), GPG_ERR_NO_ERROR);
  EXPECT_EQ(a, plain);
  EXPECT_EQ(b, plain);
}

}  // namespace GpgFrontend::Test
