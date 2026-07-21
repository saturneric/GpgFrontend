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
  EXPECT_EQ(result.GetRaw(), nullptr);
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

  EXPECT_EQ(sig.GetValidity(), GPGME_VALIDITY_UNKNOWN);
}

TEST(SignatureModelTest, DefaultSignatureReportsFailureStatus) {
  GpgSignature sig;

  // an unset signature must never read as a good signature.
  EXPECT_NE(sig.GetStatus(), GPG_ERR_NO_ERROR);
  EXPECT_EQ((sig.GetSummary() & GPGME_SIGSUM_VALID), 0U);
  EXPECT_EQ((sig.GetSummary() & GPGME_SIGSUM_GREEN), 0U);
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

  EXPECT_EQ(copy.GetValidity(), GPGME_VALIDITY_UNKNOWN);
  EXPECT_TRUE(copy.GetFingerprint().isEmpty());
}

}  // namespace GpgFrontend::Test
