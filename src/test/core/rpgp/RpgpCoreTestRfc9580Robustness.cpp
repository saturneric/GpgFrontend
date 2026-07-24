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

// RFC 9580 robustness tests for the rPGP engine.
//
// Attacker-supplied and malformed inputs are fed to every message entrypoint
// (decrypt, verify, decrypt-verify, import). The engine must fail cleanly with
// a typed error -- never crash, never release a wrong plaintext, never report a
// forged signature as Valid. These are also the primary target set for the
// AddressSanitizer harness (scripts/run_tests_asan.sh).

#include "RpgpCoreTestRfc9580.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

namespace {

auto Decrypt(const GFBuffer& in) -> GpgError {
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(in);
  return err;
}

}  // namespace

// --- malformed decrypt inputs -----------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580DecryptGarbageFailsCleanly) {
  EXPECT_NE(CheckGpgError(Decrypt(LoadRfc9580Vector("garbage.bin"))),
            GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptGarbageReportsDetail) {
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(LoadRfc9580Vector("garbage.bin"));
  ASSERT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((dobj->Check<GpgDecryptResult, GFBuffer>()));
  EXPECT_FALSE(
      ExtractParams<GpgDecryptResult>(dobj, 0).ErrorDetail().isEmpty());
}

TEST_F(RpgpCoreTest, Rfc9580DecryptEmptyFailsCleanly) {
  // An empty resource cannot be read back as a buffer, so exercise the empty
  // case with a directly-constructed empty buffer.
  EXPECT_NE(CheckGpgError(Decrypt(GFBuffer())), GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptTruncatedArmorFailsCleanly) {
  EXPECT_NE(CheckGpgError(Decrypt(LoadRfc9580Vector("truncated_armor.asc"))),
            GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptPkeskWithoutSeipdFailsCleanly) {
  EXPECT_NE(CheckGpgError(Decrypt(LoadRfc9580Vector("pkesk_no_seipd.pgp"))),
            GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptCorruptCrcNeverYieldsWrongPlaintext) {
  // The armor CRC-24 is advisory; the engine may ignore it. Whatever it does,
  // it must not crash and must not hand back an incorrect plaintext.
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptSync(LoadRfc9580Vector("corrupt_crc.asc"));
  if (CheckGpgError(err) == GPG_ERR_NO_ERROR) {
    ASSERT_TRUE((dobj->Check<GpgDecryptResult, GFBuffer>()));
    EXPECT_EQ(ExtractParams<GFBuffer>(dobj, 1),
              LoadRfc9580Vector("payload.txt"));
  }
}

TEST_F(RpgpCoreTest, Rfc9580DecryptDetachedSignatureAsMessageFails) {
  // A bare detached signature is not a decryptable message.
  EXPECT_NE(CheckGpgError(Decrypt(LoadRfc9580Vector("sig_good_detached.sig"))),
            GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptRandomLargeGarbageFailsCleanly) {
  QByteArray junk(50000, '\0');
  for (int i = 0; i < junk.size(); ++i)
    junk[i] = static_cast<char>((i * 31) & 0xFF);
  EXPECT_NE(CheckGpgError(Decrypt(GFBuffer(junk))), GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptCiphertextWithoutSecretKeyFails) {
  // The v2/AEAD vector is encrypted to aux_v6, whose secret is not imported.
  EXPECT_NE(CheckGpgError(Decrypt(LoadRfc9580Vector("enc_v2seipd_ocb.pgp"))),
            GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptTruncatedAeadFailsCleanly) {
  ImportAuxKeys({"aux_v6.asc"});
  auto full = LoadRfc9580Vector("enc_v2seipd_ocb.pgp");
  QByteArray cut(full.Data(), static_cast<int>(full.Size() / 2));
  EXPECT_NE(CheckGpgError(Decrypt(GFBuffer(cut))), GPG_ERR_NO_ERROR);
}

// --- malformed verify inputs ------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580VerifyGarbageInlineFailsCleanly) {
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(LoadRfc9580Vector("garbage.bin"), GFBuffer());
  // Either a hard error or a result with no valid signature -- never a crash.
  if (dobj->Check<GpgVerifyResult, GFBuffer>()) {
    auto r = ExtractParams<GpgVerifyResult>(dobj, 0);
    for (const auto& s : r.GetSignature()) EXPECT_FALSE(SignatureIsValid(s));
  }
}

TEST_F(RpgpCoreTest, Rfc9580VerifyGarbageDetachedSigFailsCleanly) {
  ImportAuxKeys({"aux_good.asc"});
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(LoadRfc9580Vector("payload.txt"),
                      LoadRfc9580Vector("garbage.bin"));
  if (dobj->Check<GpgVerifyResult, GFBuffer>()) {
    auto r = ExtractParams<GpgVerifyResult>(dobj, 0);
    for (const auto& s : r.GetSignature()) EXPECT_FALSE(SignatureIsValid(s));
  }
}

TEST_F(RpgpCoreTest, Rfc9580VerifyEmptyDataAndSigIsHandled) {
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(GFBuffer(), GFBuffer());
  // No signature can be valid over empty/empty; must not crash.
  if (dobj->Check<GpgVerifyResult, GFBuffer>()) {
    auto r = ExtractParams<GpgVerifyResult>(dobj, 0);
    for (const auto& s : r.GetSignature()) EXPECT_FALSE(SignatureIsValid(s));
  }
}

TEST_F(RpgpCoreTest, Rfc9580VerifyCiphertextAsSignedMessageIsNotValid) {
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(LoadRfc9580Vector("enc_v1seipd_mdc.pgp"), GFBuffer());
  if (dobj->Check<GpgVerifyResult, GFBuffer>()) {
    auto r = ExtractParams<GpgVerifyResult>(dobj, 0);
    for (const auto& s : r.GetSignature()) EXPECT_FALSE(SignatureIsValid(s));
  }
}

TEST_F(RpgpCoreTest, Rfc9580VerifyTruncatedDetachedSigFailsCleanly) {
  ImportAuxKeys({"aux_good.asc"});
  auto full = LoadRfc9580Vector("sig_good_detached.sig");
  QByteArray cut(full.Data(), static_cast<int>(full.Size() / 2));
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifySync(LoadRfc9580Vector("payload.txt"), GFBuffer(cut));
  if (dobj->Check<GpgVerifyResult, GFBuffer>()) {
    auto r = ExtractParams<GpgVerifyResult>(dobj, 0);
    for (const auto& s : r.GetSignature()) EXPECT_FALSE(SignatureIsValid(s));
  }
}

// --- malformed decrypt-verify inputs ----------------------------------------

TEST_F(RpgpCoreTest, Rfc9580DecryptVerifyGarbageFailsCleanly) {
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifySync(LoadRfc9580Vector("garbage.bin"));
  EXPECT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
}

TEST_F(RpgpCoreTest, Rfc9580DecryptVerifyLegacySedIsRejected) {
  // The SED must be refused on the combined path too (finding H2/H1).
  auto [err, dobj] =
      MessageCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifySync(LoadRfc9580Vector("enc_sed_tag9.pgp"));
  EXPECT_NE(CheckGpgError(err), GPG_ERR_NO_ERROR);
}

// --- malformed import inputs ------------------------------------------------

TEST_F(RpgpCoreTest, Rfc9580ImportGarbageDoesNotImportAnything) {
  auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                  .ImportKey(LoadRfc9580Vector("garbage.bin"));
  // Import of non-key data must import nothing (null info or zero imported).
  if (info != nullptr) {
    EXPECT_EQ(info->imported, 0);
  }
}

TEST_F(RpgpCoreTest, Rfc9580ImportEmptyDoesNotImportAnything) {
  auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                  .ImportKey(GFBuffer());
  if (info != nullptr) {
    EXPECT_EQ(info->imported, 0);
  }
}

// --- no-crash stress on repeated attacker input -----------------------------

TEST_F(RpgpCoreTest, Rfc9580RepeatedGarbageDecryptIsStable) {
  auto junk = LoadRfc9580Vector("garbage.bin");
  for (int i = 0; i < 50; ++i) {
    EXPECT_NE(CheckGpgError(Decrypt(junk)), GPG_ERR_NO_ERROR);
  }
}

}  // namespace GpgFrontend::Test
