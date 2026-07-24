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

// RFC 9580 rPGP interoperability regression tests.
//
// These drive the engine end-to-end against sq/gpg-produced known-answer
// vectors that reproduce the consume-side interop bugs found auditing rPGP
// against Sequoia and GnuPG (see project_rpgp_rfc9580_verify_gaps, findings
// B2/B4/B5/B7). Each test names the bug it guards; if the corresponding fix
// regresses, the vector fails here rather than silently mis-verifying.
//
// B1 (issuer-Key-ID-only fallback) has no sq/gpg vector -- both tools always put
// the Issuer Fingerprint subpacket in the hashed area of a v4/v6 signature -- so
// it is covered by Rust unit tests on the sniff/issuer-matching helpers instead.

#include <algorithm>

#include "RpgpCoreTestRfc9580.h"
#include "core/function/openpgp/FileCryptoOperation.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

namespace {

struct VerifyOutcome {
  GpgError err;
  QContainer<GpgSignature> sigs;
  QString detail;
};

// Run a verification: detached when `sig` is non-empty, inline/cleartext when
// `sig` is an empty buffer (`data` then holds the signed message).
auto RunVerify(const GFBuffer& data, const GFBuffer& sig) -> VerifyOutcome {
  auto [err, dobj] = MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
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

auto CountValid(const QContainer<GpgSignature>& sigs) -> int {
  return static_cast<int>(
      std::count_if(sigs.cbegin(), sigs.cend(),
                    [](const auto& s) { return SignatureIsValid(s); }));
}

}  // namespace

// --- B2: distinct signatures from one issuer must not be de-duplicated --------

// The blob carries a SHA-256 and a SHA-1 detached signature from the SAME key.
// Keying de-duplication on the issuer alone (the old behaviour) dropped the
// second signature, hiding the weak hash. Both must be surfaced.
TEST_F(RpgpCoreTest, Rfc9580InteropStrongWeakSameKeyBothSurfaced) {
  ImportAuxKeys({"aux_sha1.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_strong_weak_same_key.sig"));
  EXPECT_EQ(out.sigs.size(), 2);
}

// Exactly the SHA-256 signature is Valid; the SHA-1 one is gated out (sec 9.5),
// never dropped, so its weakness is visible rather than silently discarded.
TEST_F(RpgpCoreTest, Rfc9580InteropStrongWeakSameKeyOnlyStrongValid) {
  ImportAuxKeys({"aux_sha1.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_strong_weak_same_key.sig"));
  EXPECT_EQ(CountValid(out.sigs), 1);
}

// --- B4: per-index attribution on the cleartext path -------------------------

// Cleartext message signed by aux_good and aux_v6. With only aux_good known,
// exactly its signature is Valid; the other must never be smeared Valid.
TEST_F(RpgpCoreTest, Rfc9580InteropTwoSignerCleartextPerIssuer) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("sig_two_signer_cleartext.asc"),
                       GFBuffer());
  EXPECT_EQ(out.sigs.size(), 2);
  EXPECT_EQ(CountValid(out.sigs), 1);
  for (const auto& s : out.sigs) {
    if (SignatureIsValid(s)) {
      EXPECT_EQ(s.GetFingerprint(), QString(kAuxGoodSignFpr));
    }
  }
}

// With both signers known, both cleartext signatures verify.
TEST_F(RpgpCoreTest, Rfc9580InteropTwoSignerCleartextBothValid) {
  ImportAuxKeys({"aux_good.asc", "aux_v6.asc"});
  auto out = RunVerify(LoadRfc9580Vector("sig_two_signer_cleartext.asc"),
                       GFBuffer());
  EXPECT_EQ(out.sigs.size(), 2);
  EXPECT_EQ(CountValid(out.sigs), 2);
}

// --- B4: per-index attribution on the detached path --------------------------

// Two detached signature packets that BOTH name aux_good, but only the first
// verifies (the second has a flipped body byte). The per-cert form would stamp
// both Valid once the genuine one verified; per-index marks only the real one.
TEST_F(RpgpCoreTest, Rfc9580InteropSameIssuerDetachedOnlyGenuineValid) {
  ImportAuxKeys({"aux_good.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_two_same_issuer_one_valid.sig"));
  EXPECT_EQ(out.sigs.size(), 2);
  EXPECT_EQ(CountValid(out.sigs), 1);
}

// The detached-STREAM verifier (large-file path, `verify_detached_stream_internal`)
// formerly used per-cert attribution: once the genuine signature verified it
// stamped EVERY issuer-matching entry Valid. Consolidated onto the same shared
// per-index driver as the in-memory path, only the packet that actually verifies
// is Valid. Same vector as the case above, driven through the FILE API so the
// seekable-stream substrate (not the in-memory buffer) is exercised.
TEST_F(RpgpCoreTest, Rfc9580InteropSameIssuerDetachedStreamOnlyGenuineValid) {
  ImportAuxKeys({"aux_good.asc"});
  auto data_file = CreateTempFileAndWriteData(LoadRfc9580Vector("payload.txt"));
  auto sig_file = CreateTempFileAndWriteData(
      LoadRfc9580Vector("sig_two_same_issuer_one_valid.sig"));

  auto [err, dobj] = FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
                         .VerifyFileSync(data_file, sig_file);
  ASSERT_TRUE((dobj->Check<GpgVerifyResult>()));
  auto r = ExtractParams<GpgVerifyResult>(dobj, 0);
  EXPECT_EQ(r.GetSignature().size(), 2);
  EXPECT_EQ(CountValid(r.GetSignature()), 1);
}

// --- B5: compressed inline signature must decompress before counting ---------

// A ZLIB-compressed one-pass signed message. The inline path now drains the
// (size-bounded) decompressed body before reading the trailing signature, so a
// genuine signature verifies rather than being downgraded.
TEST_F(RpgpCoreTest, Rfc9580InteropCompressedInlineVerifies) {
  ImportAuxKeys({"aux_sha1.asc"});
  auto out = RunVerify(LoadRfc9580Vector("sig_inline_compressed.pgp"),
                       GFBuffer());
  ASSERT_EQ(CheckGpgError(out.err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(out.sigs.empty());
  EXPECT_TRUE(SignatureIsValid(out.sigs.front()));
}

// --- B7: fabricated self-signatures must not be trusted on import ------------

// aux_forged_revocation.asc is aux_good's certificate carrying a FABRICATED
// primary-key revocation (its signature does not verify). An engine that trusts
// unverified self-signatures would treat aux_good as revoked and reject its
// genuine signature. With self-signature validation on import, the bogus
// revocation is ignored and aux_good's real signature is still Valid.
TEST_F(RpgpCoreTest, Rfc9580InteropForgedRevocationIsIgnored) {
  ImportAuxKeys({"aux_forged_revocation.asc"});
  auto out = RunVerify(LoadRfc9580Vector("payload.txt"),
                       LoadRfc9580Vector("sig_good_detached.sig"));
  ASSERT_FALSE(out.sigs.empty());
  EXPECT_TRUE(SignatureIsValid(out.sigs.front()));
}

}  // namespace GpgFrontend::Test
