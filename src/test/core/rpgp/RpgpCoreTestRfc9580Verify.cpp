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

// RFC 9580 signature-verification known-answer tests for the rPGP engine.
//
// Positive vectors (good SHA-512 / v6 signatures) must verify as Valid; the
// negative vectors exercise the policy gates the audit hardened: a SHA-1
// signature (weak hash, sec 9.5), a signature from an expired key, a signature
// from a revoked key, a byte-mutated signature, and detached verification
// against the wrong data. The two-signer vector checks per-signature issuer
// attribution (finding B-Verify) -- a signature must be attributed to the exact
// key that made it, never to another issuer that merely appears in the message.

#include <algorithm>

#include "RpgpCoreTestRfc9580.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

struct VerifyOutcome {
  GpgError err;
  QContainer<GpgSignature> sigs;
  QString detail;
};

// Run a verification: detached when `sig` is non-empty, inline/cleartext when
// `sig` is an empty buffer.
auto RunVerify(const GFBuffer& data, const GFBuffer& sig) -> VerifyOutcome {
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(data, sig);
  VerifyOutcome out;
  out.err = err;
  if (dobj->Check<GpgVerifyResult, GFBuffer>()) {
    auto r = ExtractParams<GpgVerifyResult>(dobj, 0);
    out.sigs = r.GetSignature();
    out.detail = r.ErrorDetail();
  }
  return out;
}

auto AnyValid(const QContainer<GpgSignature>& sigs) -> bool {
  return std::any_of(sigs.cbegin(), sigs.cend(),
                     [](const auto& s) { return SignatureIsValid(s); });
}

auto CountValid(const QContainer<GpgSignature>& sigs) -> int {
  return static_cast<int>(
      std::count_if(sigs.cbegin(), sigs.cend(),
                    [](const auto& s) { return SignatureIsValid(s); }));
}

}  // namespace

// --- Positive: good signatures verify ---------------------------------------

TEST_F(RpgpCoreTest, Rfc9580VerifyGoodDetachedIsValid) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  ASSERT_EQ(CheckGpgError(out.err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(out.sigs.empty());
  EXPECT_TRUE(SignatureIsValid(out.sigs.front()));
}

TEST_F(RpgpCoreTest, Rfc9580VerifyGoodDetachedAttributesIssuer) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  ASSERT_FALSE(out.sigs.empty());
  // The reported issuer is aux_good's signing subkey.
  EXPECT_EQ(out.sigs.front().GetFingerprint(), QString(kAuxGoodSignFpr));
}

TEST_F(RpgpCoreTest, Rfc9580VerifyGoodDetachedReportsStrongHash) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  ASSERT_FALSE(out.sigs.empty());
  // aux_good signs with SHA-512.
  EXPECT_EQ(out.sigs.front().GetHashAlgo().toUpper(), QString("SHA512"));
}

TEST_F(RpgpCoreTest, Rfc9580VerifyGoodDetachedSingleSignature) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  EXPECT_EQ(out.sigs.size(), 1);
}

TEST_F(RpgpCoreTest, Rfc9580VerifyGoodDetachedHasCreationTime) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  ASSERT_FALSE(out.sigs.empty());
  EXPECT_TRUE(out.sigs.front().GetCreateTime().isValid());
}

TEST_F(RpgpCoreTest, Rfc9580VerifyGoodCleartextIsValid) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("sig_good_cleartext.asc"), GFBuffer());
  ASSERT_EQ(CheckGpgError(out.err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(out.sigs.empty());
  EXPECT_TRUE(SignatureIsValid(out.sigs.front()));
}

// Standalone verification of an *inline* (one-pass) signed message. One-pass
// signatures live in trailing packets, so verify.rs (Inline branch) now drains
// the message body before reading reader.num_signatures() and verifying each
// index; a genuine one-pass signature is reported Valid, matching the detached
// and decrypt-then-verify paths.
TEST_F(RpgpCoreTest, Rfc9580VerifyGoodInlineV6IsValid) {
  ImportAuxKeys({"aux_v6.asc"});
  auto out = RunVerify(LoadRfc9580Vector("sig_good_inline_v6.pgp"), GFBuffer());
  ASSERT_EQ(CheckGpgError(out.err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(out.sigs.empty());
  EXPECT_TRUE(SignatureIsValid(out.sigs.front()));
}

TEST_F(RpgpCoreTest, Rfc9580VerifyV6DetachedIsValid) {
  ImportAuxKeys({"aux_v6.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_v6_detached.sig"));
  ASSERT_EQ(CheckGpgError(out.err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(out.sigs.empty());
  EXPECT_TRUE(SignatureIsValid(out.sigs.front()));
}

TEST_F(RpgpCoreTest, Rfc9580VerifyV6DetachedReportsIssuer) {
  ImportAuxKeys({"aux_v6.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_v6_detached.sig"));
  ASSERT_FALSE(out.sigs.empty());
  // A v6 issuer fingerprint is 32 bytes (64 hex chars).
  EXPECT_GE(out.sigs.front().GetFingerprint().size(), 40);
}

// --- Negative: weak hash (SHA-1, RFC 9580 sec 9.5) --------------------------

TEST_F(RpgpCoreTest, Rfc9580VerifySha1DetachedIsNotValid) {
  ImportAuxKeys({"aux_sha1.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_sha1_detached.sig"));
  // The signature is cryptographically sound but uses a weak digest; the
  // weak-hash gate must prevent it from being reported as Valid.
  EXPECT_FALSE(AnyValid(out.sigs));
}

// --- Negative: expired key --------------------------------------------------
//
// A detached signature made by an *expired* signing key must not be reported
// Valid. cert_primary_usable() (mod.rs) now gates every verification path on
// primary-key expiration (§5.2.3.13), and subkey_usable_for_verify() rejects an
// expired signing subkey (or any subkey of an expired primary) as well, on top
// of the signature's own Signature Expiration Time (§5.2.3.18).

TEST_F(RpgpCoreTest, Rfc9580VerifyExpiredKeyIsNotValid) {
  ImportAuxKeys({"aux_expired.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_expired.sig"));
  EXPECT_FALSE(AnyValid(out.sigs));
}

// --- Negative: revoked key --------------------------------------------------
//
// A detached signature made by a signing *subkey* whose *primary* key is
// revoked must not be reported Valid. subkey_usable_for_verify() now folds in
// cert_primary_usable(), so a subkey of a revoked primary is unusable — revoking
// the primary invalidates the whole certificate (RFC 9580 §5.2.1.11).

TEST_F(RpgpCoreTest, Rfc9580VerifyRevokedKeyIsNotValid) {
  ImportAuxKeys({"aux_revoked.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_revokedkey.sig"));
  EXPECT_FALSE(AnyValid(out.sigs));
}

// --- Negative: byte-mutated (broken) signature ------------------------------

TEST_F(RpgpCoreTest, Rfc9580VerifyMutatedSignatureIsNotValid) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_bad_mutated.sig"));
  EXPECT_FALSE(AnyValid(out.sigs));
}

// --- Negative: detached signature over the wrong data -----------------------

TEST_F(RpgpCoreTest, Rfc9580VerifyDetachedWrongDataIsNotValid) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(GFBuffer(QString("a completely different payload")),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  EXPECT_FALSE(AnyValid(out.sigs));
}

// --- Negative: signer key not in the keyring --------------------------------

TEST_F(RpgpCoreTest, Rfc9580VerifyUnknownSignerIsNotValid) {
  // Do not import aux_good: the signer's public key cannot be resolved.
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  EXPECT_FALSE(AnyValid(out.sigs));
}

TEST_F(RpgpCoreTest, Rfc9580VerifyUnknownSignerReportsNoKeyStatus) {
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  // The signature is still parsed and attributed; only its key is missing.
  if (!out.sigs.empty()) {
    EXPECT_EQ(CheckGpgError(out.sigs.front().GetStatus()), GPG_ERR_NO_PUBKEY);
  }
}

// --- Two-signer attribution (finding B-Verify) ------------------------------

// Both signatures are genuine; with inline verification fixed, both are Valid.
TEST_F(RpgpCoreTest, Rfc9580VerifyTwoSignerBothKnownBothValid) {
  ImportAuxKeys({"aux_good.asc", "aux_v6.asc"});
  auto out = RunVerify(LoadRfc9580Vector("two_signer.pgp"), GFBuffer());
  ASSERT_EQ(CheckGpgError(out.err), GPG_ERR_NO_ERROR);
  EXPECT_EQ(out.sigs.size(), 2);
  EXPECT_EQ(CountValid(out.sigs), 2);
}

TEST_F(RpgpCoreTest, Rfc9580VerifyTwoSignerOneKnownOnlyOneValid) {
  // Import only aux_good: exactly its signature may be Valid, never the other.
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("two_signer.pgp"), GFBuffer());
  EXPECT_EQ(out.sigs.size(), 2);
  EXPECT_EQ(CountValid(out.sigs), 1);
}

TEST_F(RpgpCoreTest, Rfc9580VerifyTwoSignerAttributionIsPerIssuer) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("two_signer.pgp"), GFBuffer());
  // The single Valid signature must be the one issued by aux_good, proving the
  // status is attributed per issuer rather than smeared across the message.
  for (const auto& s : out.sigs) {
    if (SignatureIsValid(s)) {
      EXPECT_EQ(s.GetFingerprint(), QString(kAuxGoodSignFpr));
    }
  }
}

TEST_F(RpgpCoreTest, Rfc9580VerifyTwoSignerNoneKnownNoneValid) {
  // Neither signer imported -> no signature may be Valid.
  auto out = RunVerify(LoadRfc9580Vector("two_signer.pgp"), GFBuffer());
  EXPECT_FALSE(AnyValid(out.sigs));
}

// --- Summary bits -----------------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580VerifyGoodSignatureSummaryIsGreen) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  ASSERT_FALSE(out.sigs.empty());
  EXPECT_TRUE((out.sigs.front().GetSummary() & GPGME_SIGSUM_VALID) != 0U);
}

TEST_F(RpgpCoreTest, Rfc9580VerifyWeakSignatureSummaryIsRed) {
  ImportAuxKeys({"aux_sha1.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_sha1_detached.sig"));
  for (const auto& s : out.sigs) {
    EXPECT_TRUE((s.GetSummary() & GPGME_SIGSUM_RED) != 0U);
  }
}

// --- Good detached verify is repeatable -------------------------------------

TEST_F(RpgpCoreTest, Rfc9580VerifyGoodDetachedIsRepeatable) {
  ImportAuxKeys({"aux_good.asc"});
  auto a = RunVerify(LoadRfc9580Vector("payload.txt"),
                     LoadRfc9580Vector("sig_good_detached.sig"));
  auto b = RunVerify(LoadRfc9580Vector("payload.txt"),
                     LoadRfc9580Vector("sig_good_detached.sig"));
  EXPECT_EQ(AnyValid(a.sigs), AnyValid(b.sigs));
  EXPECT_TRUE(AnyValid(a.sigs));
}

}  // namespace GpgFrontend::Test
