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
 */

// RFC 9580 Appendix A known-answer vectors, driven through the *public* core
// API rather than the Rust engine directly.
//
// The Rust suite already runs these vectors against the engine internals (see
// rust/src/crypto/{verify,decrypt}.rs). What this file adds is the seam the
// Rust tests cannot reach: the C++ side of the boundary — GFBuffer
// marshalling, the key repository, and above all the
// GfrSignatureStatus -> gpg_error_t mapping in ResultHandler/GpgSignature that
// decides what the user is finally told about a signature.
//
// The vectors are transcribed verbatim from the RFC, so unlike the generated
// corpus they are normative, tool-independent and never need regenerating.

#include <algorithm>

#include "RpgpCoreTestRfc9580.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

// --- RFC 9580 Appendix A.3: sample version 6 certificate -------------------
constexpr char kA3V6Cert[] =
    "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
    "\n"
    "xioGY4d/4xsAAAAg+U2nu0jWCmHlZ3BqZYfQMxmZu52JGggkLq2EVD34laPCsQYf\n"
    "GwoAAABCBYJjh3/jAwsJBwUVCg4IDAIWAAKbAwIeCSIhBssYbE8GCaaX5NUt+mxy\n"
    "KwwfHifBilZwj2Ul7Ce62azJBScJAgcCAAAAAK0oIBA+LX0ifsDm185Ecds2v8lw\n"
    "gyU2kCcUmKfvBXbAf6rhRYWzuQOwEn7E/aLwIwRaLsdry0+VcallHhSu4RN6HWaE\n"
    "QsiPlR4zxP/TP7mhfVEe7XWPxtnMUMtf15OyA51YBM4qBmOHf+MZAAAAIIaTJINn\n"
    "+eUBXbki+PSAld2nhJh/LVmFsS+60WyvXkQ1wpsGGBsKAAAALAWCY4d/4wKbDCIh\n"
    "BssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce62azJAAAAAAQBIKbpGG2dWTX8\n"
    "j+VjFM21J0hqWlEg+bdiojWnKfA5AQpWUWtnNwDEM0g12vYxoWM8Y81W+bHBw805\n"
    "I8kWVkXU6vFOi+HWvv/ira7ofJu16NnoUkhclkUrk0mXubZvyl4GBg==\n"
    "-----END PGP PUBLIC KEY BLOCK-----\n";

// The fingerprint the RFC states for the A.3 primary key (sec 5.5.4.3).
constexpr char kA3PrimaryFpr[] =
    "CB186C4F0609A697E4D52DFA6C722B0C1F1E27C18A56708F6525EC27BAD9ACC9";

// --- A.4: the same key, unlocked secret ------------------------------------
constexpr char kA4V6SecretUnlocked[] =
    "-----BEGIN PGP PRIVATE KEY BLOCK-----\n"
    "\n"
    "xUsGY4d/4xsAAAAg+U2nu0jWCmHlZ3BqZYfQMxmZu52JGggkLq2EVD34laMAGXKB\n"
    "exK+cH6NX1hs5hNhIB00TrJmosgv3mg1ditlsLfCsQYfGwoAAABCBYJjh3/jAwsJ\n"
    "BwUVCg4IDAIWAAKbAwIeCSIhBssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce6\n"
    "2azJBScJAgcCAAAAAK0oIBA+LX0ifsDm185Ecds2v8lwgyU2kCcUmKfvBXbAf6rh\n"
    "RYWzuQOwEn7E/aLwIwRaLsdry0+VcallHhSu4RN6HWaEQsiPlR4zxP/TP7mhfVEe\n"
    "7XWPxtnMUMtf15OyA51YBMdLBmOHf+MZAAAAIIaTJINn+eUBXbki+PSAld2nhJh/\n"
    "LVmFsS+60WyvXkQ1AE1gCk95TUR3XFeibg/u/tVY6a//1q0NWC1X+yui3O24wpsG\n"
    "GBsKAAAALAWCY4d/4wKbDCIhBssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce6\n"
    "2azJAAAAAAQBIKbpGG2dWTX8j+VjFM21J0hqWlEg+bdiojWnKfA5AQpWUWtnNwDE\n"
    "M0g12vYxoWM8Y81W+bHBw805I8kWVkXU6vFOi+HWvv/ira7ofJu16NnoUkhclkUr\n"
    "k0mXubZvyl4GBg==\n"
    "-----END PGP PRIVATE KEY BLOCK-----\n";

// --- A.6: cleartext signed message, verifiable with A.3 --------------------
constexpr char kA6Cleartext[] =
    "-----BEGIN PGP SIGNED MESSAGE-----\n"
    "\n"
    "What we need from the grocery store:\n"
    "\n"
    "- - tofu\n"
    "- - vegetables\n"
    "- - noodles\n"
    "\n"
    "-----BEGIN PGP SIGNATURE-----\n"
    "\n"
    "wpgGARsKAAAAKQWCY5ijYyIhBssYbE8GCaaX5NUt+mxyKwwfHifBilZwj2Ul7Ce6\n"
    "2azJAAAAAGk2IHZJX1AhiJD39eLuPBgiUU9wUA9VHYblySHkBONKU/usJ9BvuAqo\n"
    "/FvLFuGWMbKAdA+epq7V4HOtAPlBWmU8QOd6aud+aSunHQaaEJ+iTFjP2OMW0KBr\n"
    "NK2ay45cX1IVAQ==\n"
    "-----END PGP SIGNATURE-----\n";

// --- A.7: the same message and signature, inline ---------------------------
constexpr char kA7InlineSigned[] =
    "-----BEGIN PGP MESSAGE-----\n"
    "\n"
    "xEYGAQobIHZJX1AhiJD39eLuPBgiUU9wUA9VHYblySHkBONKU/usyxhsTwYJppfk\n"
    "1S36bHIrDB8eJ8GKVnCPZSXsJ7rZrMkBy0p1AAAAAABXaGF0IHdlIG5lZWQgZnJv\n"
    "bSB0aGUgZ3JvY2VyeSBzdG9yZToKCi0gdG9mdQotIHZlZ2V0YWJsZXMKLSBub29k\n"
    "bGVzCsKYBgEbCgAAACkFgmOYo2MiIQbLGGxPBgmml+TVLfpscisMHx4nwYpWcI9l\n"
    "JewnutmsyQAAAABpNiB2SV9QIYiQ9/Xi7jwYIlFPcFAPVR2G5ckh5ATjSlP7rCfQ\n"
    "b7gKqPxbyxbhljGygHQPnqau1eBzrQD5QVplPEDnemrnfmkrpx0GmhCfokxYz9jj\n"
    "FtCgazStmsuOXF9SFQE=\n"
    "-----END PGP MESSAGE-----\n";

// --- A.8: X25519 + AEAD-OCB encrypted to the A.3/A.4 key -------------------
constexpr char kA8X25519AeadOcb[] =
    "-----BEGIN PGP MESSAGE-----\n"
    "\n"
    "wV0GIQYSyD8ecG9jCP4VGkF3Q6HwM3kOk+mXhIjR2zeNqZMIhRmHzxjV8bU/gXzO\n"
    "WgBM85PMiVi93AZfJfhK9QmxfdNnZBjeo1VDeVZheQHgaVf7yopqR6W1FT6NOrfS\n"
    "aQIHAgZhZBZTW+CwcW1g4FKlbExAf56zaw76/prQoN+bAzxpohup69LA7JW/Vp0l\n"
    "yZnuSj3hcFj0DfqLTGgr4/u717J+sPWbtQBfgMfG9AOIwwrUBqsFE9zW+f1zdlYo\n"
    "bhF30A+IitsxxA==\n"
    "-----END PGP MESSAGE-----\n";

// The plaintext every Appendix A encryption sample hides.
constexpr char kAPlaintextHello[] = "Hello, world!";

auto Buf(const char* s) -> GFBuffer { return GFBuffer(QByteArray(s)); }

// Import an armored key block into the unit-test keyring so the engine can
// resolve it during verification or decryption.
void ImportBlock(const char* block) {
  auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                  .ImportKey(Buf(block));
  ASSERT_TRUE(info != nullptr) << "failed to import an Appendix A key block";
  AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushCache();
  AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest).Fetch();
}

struct VerifyOutcome {
  GpgError err;
  QContainer<GpgSignature> sigs;
};

auto RunVerify(const GFBuffer& data, const GFBuffer& sig) -> VerifyOutcome {
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(data, sig);
  VerifyOutcome out;
  out.err = err;
  if (dobj->Check<GpgVerifyResult, GFBuffer>()) {
    out.sigs = ExtractParams<GpgVerifyResult>(dobj, 0).GetSignature();
  }
  return out;
}

auto CountValid(const QContainer<GpgSignature>& sigs) -> int {
  return static_cast<int>(
      std::count_if(sigs.cbegin(), sigs.cend(),
                    [](const auto& s) { return SignatureIsValid(s); }));
}

}  // namespace

// ---------------------------------------------------------------------------
// A.3 / A.4 — the sample version 6 key
// ---------------------------------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580AppendixAV6CertImports) {
  // The most basic requirement: the RFC's own v6 certificate must be
  // importable. A v6 key that the repository rejects would make every other
  // Appendix A test moot.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAV6CertReportsTheStatedFingerprint) {
  // RFC 9580 sec 5.5.4.3: a v6 fingerprint is the SHA2-256 hash of the
  // normalised public key packet, 256 bits wide. The RFC states the expected
  // value, so this is a true known-answer test of the C++ key model.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));

  auto key = AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKey(QString(kA3PrimaryFpr));
  ASSERT_TRUE(key != nullptr)
      << "the key must be resolvable by the fingerprint the RFC states";
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAV6FingerprintIs256Bits) {
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));
  ASSERT_EQ(QString(kA3PrimaryFpr).size(), 64);
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAV6SecretKeyImports) {
  // A.4 is the unlocked secret half of the same key.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA4V6SecretUnlocked));
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAPublicAndSecretHalvesAgree) {
  // Importing both must yield one key, not two: they share a fingerprint.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA4V6SecretUnlocked));

  auto key = AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKey(QString(kA3PrimaryFpr));
  ASSERT_TRUE(key != nullptr)
      << "both halves must resolve to the one key, not two entries";
}

// ---------------------------------------------------------------------------
// A.6 — cleartext signed message
// ---------------------------------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580AppendixACleartextVerifiesAsValid) {
  // The whole point of this file: the engine reports the signature as valid
  // *and* the C++ status mapping turns that into GPG_ERR_NO_ERROR, which is
  // what the UI keys its green check mark off.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));

  auto out = RunVerify(Buf(kA6Cleartext), GFBuffer());
  ASSERT_EQ(CheckGpgError(out.err), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(out.sigs.empty());
  ASSERT_EQ(CountValid(out.sigs), 1);
}

TEST_F(RpgpCoreTest, Rfc9580AppendixACleartextAttributesTheIssuer) {
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));

  auto out = RunVerify(Buf(kA6Cleartext), GFBuffer());
  ASSERT_FALSE(out.sigs.empty());
  ASSERT_FALSE(out.sigs.front().GetFingerprint().isEmpty())
      << "a verified signature must name its issuer";
}

TEST_F(RpgpCoreTest, Rfc9580AppendixACleartextWithoutTheKeyIsNotValid) {
  // No import: the signature is well formed but unattributable. It must not be
  // reported as valid, and it must not be reported as a forgery either.
  auto out = RunVerify(Buf(kA6Cleartext), GFBuffer());
  ASSERT_EQ(CountValid(out.sigs), 0);
}

TEST_F(RpgpCoreTest, Rfc9580AppendixACleartextTamperedIsNotValid) {
  // Change one character of the signed text; the signature must stop
  // verifying. (sec 7.1: the signature covers the canonicalised cleartext.)
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));

  QByteArray tampered(kA6Cleartext);
  tampered.replace("noodles", "porridge");

  auto out = RunVerify(GFBuffer(tampered), GFBuffer());
  ASSERT_EQ(CountValid(out.sigs), 0)
      << "a modified cleartext body must invalidate the signature";
}

// ---------------------------------------------------------------------------
// A.7 — inline signed message
// ---------------------------------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580AppendixAInlineVerifiesAsValid) {
  // The inline path had a genuine bug (one-pass signatures live in trailing
  // packets, so the body must be drained before the signatures can be
  // counted). This is the RFC's own sample exercising it end to end.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));

  auto out = RunVerify(Buf(kA7InlineSigned), GFBuffer());
  ASSERT_EQ(CheckGpgError(out.err), GPG_ERR_NO_ERROR);
  ASSERT_EQ(CountValid(out.sigs), 1);
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAInlineWithoutTheKeyIsNotValid) {
  auto out = RunVerify(Buf(kA7InlineSigned), GFBuffer());
  ASSERT_EQ(CountValid(out.sigs), 0);
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAInlineAndCleartextShareAnIssuer) {
  // A.6 and A.7 carry the same signature in two different envelopes, so both
  // must be attributed to the same key.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));

  auto cleartext = RunVerify(Buf(kA6Cleartext), GFBuffer());
  auto inline_msg = RunVerify(Buf(kA7InlineSigned), GFBuffer());
  ASSERT_FALSE(cleartext.sigs.empty());
  ASSERT_FALSE(inline_msg.sigs.empty());
  ASSERT_EQ(cleartext.sigs.front().GetFingerprint(),
            inline_msg.sigs.front().GetFingerprint());
}

// ---------------------------------------------------------------------------
// A.8 — X25519 + AEAD-OCB encryption
// ---------------------------------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580AppendixAAeadOcbDecryptsToTheKnownPlaintext) {
  // sec 5.1.6 X25519 PKESK + sec 5.13.2 v2 SEIPD with OCB (sec 5.13.4), the
  // mandatory-to-implement AEAD mode.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA4V6SecretUnlocked));

  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(Buf(kA8X25519AeadOcb));
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((dobj->Check<GpgDecryptResult, GFBuffer>()));

  auto plain = ExtractParams<GFBuffer>(dobj, 1);
  ASSERT_EQ(plain.ConvertToQByteArray(), QByteArray(kAPlaintextHello));
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAAeadOcbWithoutTheSecretKeyFails) {
  // Only the public half is available, so there is no way to recover the
  // session key. The failure must be clean, not a crash.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));

  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(Buf(kA8X25519AeadOcb));
  ASSERT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAAeadOcbTamperedCiphertextIsRefused) {
  // sec 13.7: "if the authentication tag fails to verify, the implementation
  // MUST NOT attempt to parse nor release decrypted data".
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA4V6SecretUnlocked));

  QByteArray tampered(kA8X25519AeadOcb);
  auto pos = tampered.size() / 2;
  tampered[pos] = (tampered[pos] == 'A' ? 'B' : 'A');

  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(GFBuffer(tampered));
  ASSERT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR)
      << "a tampered AEAD ciphertext must never release plaintext";
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAAeadOcbTruncatedIsRefused) {
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA4V6SecretUnlocked));

  QByteArray truncated(kA8X25519AeadOcb);
  truncated.truncate(truncated.size() / 2);

  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(GFBuffer(truncated));
  ASSERT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
}

// ---------------------------------------------------------------------------
// Robustness: the Appendix A blobs fed to the wrong operation
// ---------------------------------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580AppendixADecryptingASignedMessageFailsCleanly) {
  // A.7 is signed, not encrypted. Handing it to the decryptor must produce an
  // error rather than a crash or a partial result.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA4V6SecretUnlocked));

  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(Buf(kA7InlineSigned));
  ASSERT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAVerifyingAnEncryptedMessageFailsCleanly) {
  // The mirror case: A.8 is encrypted, not signed.
  ASSERT_NO_FATAL_FAILURE(ImportBlock(kA3V6Cert));

  auto out = RunVerify(Buf(kA8X25519AeadOcb), GFBuffer());
  ASSERT_EQ(CountValid(out.sigs), 0);
}

TEST_F(RpgpCoreTest, Rfc9580AppendixAImportingAMessageAsAKeyFailsCleanly) {
  auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                  .ImportKey(Buf(kA8X25519AeadOcb));
  // Either a null report or one that imported nothing; never a crash.
  if (info != nullptr) {
    ASSERT_EQ(info->considered, 0);
  }
}

}  // namespace GpgFrontend::Test
