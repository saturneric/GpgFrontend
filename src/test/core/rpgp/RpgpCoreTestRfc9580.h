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

#pragma once

#include "RpgpCoreTest.h"
#include "core/function/openpgp/AbstractKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/GpgSignature.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

// ---------------------------------------------------------------------------
// Shared helpers for the RFC 9580 rPGP known-answer test vector suite.
//
// The vector corpus and the auxiliary "adversarial" signer keys are produced
// offline by scripts/gen_rpgp_test_vectors.sh and embedded as Qt resources
// (see resource/qrc/test.qrc, prefixes /test/rpgp_vectors and
// /test/rpgp_aux_keys).
//
// New tests reuse the existing RpgpCoreTest fixture: it already configures the
// rPGP context on kRpgpChannelForUnitTest, imports the fixture keys key1/2/3,
// and resets the keyring before every test (so any aux key imported inside a
// test body is isolated to that test). Tests that need an adversarial signer's
// public certificate for verification call ImportAuxKeys(...) at the top.
//
// IMPORTANT: the fingerprint constants below reflect the *currently committed*
// corpus. Regenerating the corpus mints fresh random keys; if you re-run the
// generation script, update these constants from resource/lfs/test/
// rpgp_vectors/MANIFEST.txt (and the issuer-fpr comments here).
// ---------------------------------------------------------------------------

namespace GpgFrontend::Test {

// The single known plaintext every signature covers / every ciphertext hides
// (payload.txt). Kept in sync with scripts/gen_rpgp_test_vectors.sh.
inline const char* const kRfc9580Payload =
    "GpgFrontend rPGP engine RFC 9580 known-answer test vector.\n";

// Fixture key1 (v4 Ed25519): primary fingerprint and signing-subkey fingerprint
// (the latter is what a data signature reports as its issuer).
inline const char* const kKey1PrimaryFpr =
    "3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497";
inline const char* const kKey1SignFpr =
    "575572EF0DF799AB884EC6C114C6B0B1596A2755";

// Auxiliary signer issuer fingerprints (signing-subkey where applicable).
// aux_good: v4, alive, SHA-512 signatures.
inline const char* const kAuxGoodSignFpr =
    "D3ABB909ADDE4A9D9EA30E0A8C871BB1D239C773";
// aux_v6: v6 (RFC 9580) key; SECRET committed (passphrase 123456).
inline const char* const kAuxV6PrimaryFpr =
    "10099B8E34D0B282DACC44E3CB23081B598F46794A5B5118C6185C3461FC0799";

// A signature counts as "Valid" only when the engine reports no error for it.
// The rPGP mapping (see ResultHandler.cpp / GpgSignature.cpp) yields
// GPG_ERR_NO_ERROR for kVALID and a non-zero code (bad sig / no pubkey /
// general) for every rejected signature, so this single predicate captures the
// weak-hash, expiration, and revocation gates uniformly.
inline auto SignatureIsValid(const GpgSignature& s) -> bool {
  return CheckGpgError(s.GetStatus()) == GPG_ERR_NO_ERROR;
}

// Load a committed test vector as a GFBuffer.
inline auto LoadRfc9580Vector(const QString& name) -> GFBuffer {
  auto [ok, buf] = ReadFileGFBuffer(QString(":/test/rpgp_vectors/") + name);
  EXPECT_TRUE(ok) << "failed to load test vector: " << name.toStdString();
  return buf;
}

// Import one or more auxiliary signer keys (by resource file name, e.g.
// "aux_good.asc") into the unit-test keyring so signature verification can
// resolve their public certificates. The keyring is reset before every test,
// so this must be called within each test that needs the key.
inline void ImportAuxKeys(std::initializer_list<QString> names) {
  for (const auto& name : names) {
    auto [ok, buf] = ReadFileGFBuffer(QString(":/test/rpgp_aux_keys/") + name);
    ASSERT_TRUE(ok) << "failed to load aux key: " << name.toStdString();
    auto info = KeyImportExportOperation::GetInstance(kRpgpChannelForUnitTest)
                    .ImportKey(buf);
    ASSERT_TRUE(info != nullptr)
        << "failed to import aux key: " << name.toStdString();
  }
  AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest).FlushCache();
  AbstractKeyRepository::GetInstance(kRpgpChannelForUnitTest).Fetch();
}

}  // namespace GpgFrontend::Test
