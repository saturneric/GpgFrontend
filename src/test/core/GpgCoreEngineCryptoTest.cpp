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

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <algorithm>

#include "GpgCoreEngineTest.h"
#include "core/function/openpgp/FileCryptoOperation.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyGenerationOperation.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/openpgp/MessageCryptoOperation.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgKey.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"

namespace GpgFrontend::Test {

// ---------------------------------------------------------------------------
// Key generation
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, GenerateKeyWithSubkey) {
  auto key = GenerateFullKey("genkey");
  ASSERT_TRUE(key != nullptr);

  EXPECT_TRUE(key->IsGood());
  EXPECT_TRUE(key->IsPrivateKey());
  EXPECT_TRUE(key->IsHasActualSignCap());
  EXPECT_TRUE(key->IsHasActualEncrCap());
  EXPECT_FALSE(key->SubKeys().empty());

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, GenerateSubkey) {
  auto key = GenerateFullKey("addsubkey");
  ASSERT_TRUE(key != nullptr);
  auto original_size = key->SubKeys().size();

  auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);
  s_info->SetAlgo(PickEncryptionSubAlgo());
  s_info->SetNonExpired(true);
  s_info->SetNonPassPhrase(true);

  auto [err, data_object] =
      KeyGenerationOperation::GetInstance(Channel()).GenerateSubkeySync(key,
                                                                        s_info);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(data_object->Check<GpgGenerateKeyResult>());
  auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
  ASSERT_TRUE(result.IsGood());

  GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
  auto reloaded =
      GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(key->Fingerprint());
  ASSERT_TRUE(reloaded != nullptr);
  EXPECT_EQ(reloaded->SubKeys().size(), original_size + 1);

  DeleteKey(key);
}

// Coverage sweep: walk every primary-key algorithm the engine advertises
// (GetSupportedKeyAlgo, already filtered by each algorithm's per-engine
// SupportedVersion()) and try to generate it. A representative core set must
// always succeed; the rest are best-effort, since an engine may legitimately
// refuse an advertised algorithm in a given environment (e.g. a GnuPG build
// lacking ed448/libgcrypt support). Refusals are logged as coverage info, not
// failed, so the test stays stable across engine builds while still catching
// gross regressions. Adding a new algorithm grows this sweep automatically.
TEST_P(GpgCoreEngineTest, GenerateAllDeclaredPrimaryKeys) {
  // Generating every declared algorithm (RSA-4096, SLH-DSA, ...) across all
  // engines is expensive, so this exhaustive sweep is opt-in. Set
  // GF_RUN_ALGO_COVERAGE=1 to run it.
  if (qEnvironmentVariableIsEmpty("GF_RUN_ALGO_COVERAGE")) {
    GTEST_SKIP() << "set GF_RUN_ALGO_COVERAGE=1 to run the algorithm "
                    "generation coverage sweep";
  }

  // Algorithms every engine must always be able to issue as a primary key.
  static const QSet<QString> kCorePrimaryAlgos = {"rsa2048", "ed25519",
                                                  "nistp256"};

  const auto algos = KeyGenerateInfo::GetSupportedKeyAlgo(Channel());
  ASSERT_FALSE(algos.empty());

  auto try_generate_primary = [this](const KeyAlgo& algo) -> bool {
    auto p_info = QSharedPointer<KeyGenerateInfo>::create();
    p_info->SetName("declared_primary");
    p_info->SetEmail("declared@gpgfrontend.bktus.com");
    p_info->SetComment(algo.Id());
    p_info->SetAlgo(algo);
    p_info->SetNonExpired(true);
    p_info->SetNonPassPhrase(true);

    // Hybrid (PQC) primaries pair with a classical companion algorithm and are
    // only valid as v6 keys; mirror what the generate dialog sets up.
    const auto sub_algos = algo.SubAlgos(Channel());
    if (!sub_algos.empty()) p_info->SetSubAlgo(sub_algos.front());
    if (algo.IsPostQuantum()) p_info->SetKeyVersion(6);

    auto [err, data_object] =
        KeyGenerationOperation::GetInstance(Channel()).GenerateKeySync(p_info);
    if (CheckGpgError(err) != GPG_ERR_NO_ERROR) return false;
    if (!data_object->Check<GpgGenerateKeyResult>()) return false;

    auto result = ExtractParams<GpgGenerateKeyResult>(data_object, 0);
    if (!result.IsGood() || result.GetFingerprint().isEmpty()) return false;

    GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
    auto key = GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(
        result.GetFingerprint().toUpper());
    if (key == nullptr) return false;
    DeleteKey(key);
    return true;
  };

  int generated = 0;
  QStringList refused;
  for (const auto& algo : algos) {
    if (algo.Id() == "none") continue;  // sentinel, not a real algorithm

    if (try_generate_primary(algo)) {
      ++generated;
    } else if (kCorePrimaryAlgos.contains(algo.Id())) {
      ADD_FAILURE() << "core primary algorithm failed to generate: "
                    << algo.Id().toStdString();
    } else {
      refused << algo.Id();
    }
  }

  EXPECT_GT(generated, 0);
  if (!refused.empty()) {
    GTEST_LOG_(INFO) << "advertised primary algorithms not generated by engine "
                     << GetParam().name.toStdString() << ": "
                     << refused.join(", ").toStdString();
  }
}

// Coverage sweep for subkeys: for a freshly generated base key, walk every
// algorithm the generate-subkey dialog would offer for it — exactly
// GetSupportedSubkeyAlgo(channel, key) — and try to add it. Same policy as the
// primary sweep: a core set is strict, the rest best-effort and logged. The
// base key uses the engine's default version, so the offered set matches what a
// user actually sees for such a key.
TEST_P(GpgCoreEngineTest, GenerateAllDeclaredSubkeys) {
  // Exhaustive and slow; opt-in via GF_RUN_ALGO_COVERAGE=1 (see the primary-key
  // sweep above).
  if (qEnvironmentVariableIsEmpty("GF_RUN_ALGO_COVERAGE")) {
    GTEST_SKIP() << "set GF_RUN_ALGO_COVERAGE=1 to run the algorithm "
                    "generation coverage sweep";
  }

  // Subkey algorithms every engine must always be able to add to a key.
  static const QSet<QString> kCoreSubkeyAlgos = {"rsa2048", "cv25519",
                                                 "ed25519"};

  // Use the engine's default key version (via the shared helper), so the
  // offered subkey set — and therefore what is asserted — matches what a user
  // actually sees in the dialog for such a key.
  auto base = GenerateFullKey("declared_subkey_base");
  ASSERT_TRUE(base != nullptr);
  const auto base_fpr = base->Fingerprint();

  const auto sub_algos =
      KeyGenerateInfo::GetSupportedSubkeyAlgo(Channel(), *base);
  ASSERT_FALSE(sub_algos.empty());

  auto try_add_subkey = [this, &base_fpr](const KeyAlgo& algo) -> bool {
    // Re-fetch the primary each round so the operation never works off a key
    // snapshot made stale by the previously added subkey.
    GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
    auto p_key = GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(base_fpr);
    if (p_key == nullptr) return false;

    auto s_info = QSharedPointer<KeyGenerateInfo>::create(true);
    s_info->SetAlgo(algo);
    s_info->SetNonExpired(true);
    s_info->SetNonPassPhrase(true);

    const auto hybrid_sub_algos = algo.SubAlgos(Channel());
    if (!hybrid_sub_algos.empty()) s_info->SetSubAlgo(hybrid_sub_algos.front());

    auto [err, data_object] =
        KeyGenerationOperation::GetInstance(Channel()).GenerateSubkeySync(
            p_key, s_info);
    return CheckGpgError(err) == GPG_ERR_NO_ERROR &&
           data_object->Check<GpgGenerateKeyResult>();
  };

  int generated = 0;
  QStringList refused;
  for (const auto& algo : sub_algos) {
    if (algo.Id() == "none") continue;  // sentinel, not a real algorithm

    if (try_add_subkey(algo)) {
      ++generated;
    } else if (kCoreSubkeyAlgos.contains(algo.Id())) {
      ADD_FAILURE() << "core subkey algorithm failed to generate: "
                    << algo.Id().toStdString();
    } else {
      refused << algo.Id();
    }
  }

  EXPECT_GT(generated, 0);
  if (!refused.empty()) {
    GTEST_LOG_(INFO) << "advertised subkey algorithms not generated by engine "
                     << GetParam().name.toStdString() << ": "
                     << refused.join(", ").toStdString();
  }

  GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
  DeleteKey(GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(base_fpr));
}

// ---------------------------------------------------------------------------
// Encrypt / Decrypt
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, EncryptDecrypt) {
  auto key = GenerateFullKey("encdec");
  ASSERT_TRUE(key != nullptr);

  auto plain = GFBuffer(QString("Hello GpgFrontend engine independent test!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(Channel()).EncryptSync({key}, plain,
                                                                 true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GFBuffer>()));
  auto enc_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto cipher = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(enc_result.InvalidRecipients().empty());
  ASSERT_FALSE(cipher.Empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(Channel()).DecryptSync(cipher);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GFBuffer>()));
  auto out = ExtractParams<GFBuffer>(data_object_0, 1);
  ASSERT_EQ(out, plain);

  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Sign / Verify
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, SignVerifyDetached) {
  auto key = GenerateFullKey("signverify");
  ASSERT_TRUE(key != nullptr);

  auto text = GFBuffer(QString("Hello GpgFrontend engine independent test!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(Channel()).SignSync(
          {key}, text, GPGME_SIG_MODE_DETACH, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 0);
  auto signature = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(sign_result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(Channel()).VerifySync(text,
                                                                signature);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  EXPECT_EQ(verify_result.GetSignature().at(0).GetFingerprint().toUpper(),
            key->Fingerprint().toUpper());

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, SignVerifyInline) {
  auto key = GenerateFullKey("signverify_inline");
  ASSERT_TRUE(key != nullptr);

  auto text = GFBuffer(QString("Hello GpgFrontend engine independent test!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(Channel()).SignSync(
          {key}, text, GPGME_SIG_MODE_NORMAL, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult, GFBuffer>()));
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 0);
  auto signed_buffer = ExtractParams<GFBuffer>(data_object, 1);
  ASSERT_TRUE(sign_result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(Channel()).VerifySync(signed_buffer,
                                                                GFBuffer());
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  EXPECT_EQ(verify_result.GetSignature().at(0).GetFingerprint().toUpper(),
            key->Fingerprint().toUpper());

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, EncryptSignDecryptVerify) {
  auto key = GenerateFullKey("encsign");
  ASSERT_TRUE(key != nullptr);

  auto plain = GFBuffer(QString("Hello GpgFrontend engine independent test!"));

  auto [err, data_object] =
      MessageCryptoOperation::GetInstance(Channel()).EncryptSignSync(
          {key}, {key}, plain, true);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object->Check<GpgEncryptResult, GpgSignResult, GFBuffer>()));
  auto cipher = ExtractParams<GFBuffer>(data_object, 2);

  auto [err_0, data_object_0] =
      MessageCryptoOperation::GetInstance(Channel()).DecryptVerifySync(cipher);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(
      (data_object_0->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 1);
  auto out = ExtractParams<GFBuffer>(data_object_0, 2);
  ASSERT_EQ(out, plain);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  EXPECT_EQ(verify_result.GetSignature().at(0).GetFingerprint().toUpper(),
            key->Fingerprint().toUpper());

  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Directory encrypt -> decrypt round-trip.
//
// Regression for a race in the GnuPG archive-decrypt path: the extraction task
// (which drains the decrypted plaintext from the exchanger and writes the files
// to disk) runs on a separate task runner from gpgme. The opera must not report
// completion until extraction has fully finished, otherwise the trailing files
// of the archive can be lost. We pack several files (mirroring the original
// report of "A.txt", "A.txt.gpg", "A.txt.sig") and assert every one survives
// the round-trip with identical content.
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, EncryptDecryptDirectoryRoundTrip) {
  auto key = GenerateFullKey("dirroundtrip");
  ASSERT_TRUE(key != nullptr);

  QTemporaryDir work_dir;
  ASSERT_TRUE(work_dir.isValid());

  const QString src_dir = work_dir.path() + "/test";
  ASSERT_TRUE(QDir().mkpath(src_dir));

  // Keep the total decrypted size below the exchanger queue capacity so gpgme
  // can dump the whole plaintext archive into the queue and return without
  // waiting for the extractor. That maximizes the window in which the extractor
  // is still writing files to disk after the gpg opera reports completion --
  // exactly the window the race exploited. A few MB of payload makes that
  // window large enough to catch deterministically.
  const QContainer<QPair<QString, QByteArray>> files = {
      {"A.txt", "plain content"},
      {"A.txt.gpg", "pretend ciphertext"},
      {"A.txt.sig", "pretend signature"},
      {"B.bin", QByteArray(qsizetype{3} * 1024 * 1024, '\x42')},
  };

  for (const auto& [name, content] : files) {
    QFile f(src_dir + "/" + name);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    ASSERT_EQ(f.write(content), content.size());
    f.close();
  }

  const QString archive_path = work_dir.path() + "/test.tar.gpg";

  bool encrypt_done = false;
  GpgError encrypt_err = GPG_ERR_GENERAL;
  FileCryptoOperation::GetInstance(Channel()).EncryptDirectory(
      {key}, src_dir, false, archive_path,
      [&](GpgError err, const DataObjectPtr&) {
        encrypt_err = err;
        encrypt_done = true;
      });
  WAIT_FOR_TRUE(encrypt_done, 10000);
  ASSERT_EQ(CheckGpgError(encrypt_err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE(QFileInfo::exists(archive_path));

  const QString out_dir = work_dir.path() + "/out";
  ASSERT_TRUE(QDir().mkpath(out_dir));

  bool decrypt_done = false;
  GpgError decrypt_err = GPG_ERR_GENERAL;
  FileCryptoOperation::GetInstance(Channel()).DecryptArchive(
      archive_path, out_dir, [&](GpgError err, const DataObjectPtr&) {
        decrypt_err = err;
        decrypt_done = true;
      });
  WAIT_FOR_TRUE(decrypt_done, 10000);
  ASSERT_EQ(CheckGpgError(decrypt_err), GPG_ERR_NO_ERROR);

  // Every packed file must reappear with identical content. A lost trailing
  // file (the original bug) fails here.
  for (const auto& [name, content] : files) {
    QFile f(out_dir + "/" + name);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly))
        << "missing extracted file: " << name.toStdString();
    EXPECT_EQ(f.readAll(), content)
        << "content mismatch for file: " << name.toStdString();
    f.close();
  }

  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Stress: repeat the full encrypt -> decrypt -> sign -> verify cycle.
// Bump the iteration count with the GF_STRESS_ITER environment variable, e.g.
//   GF_STRESS_ITER=10000 ./GpgFrontendTest --gtest_filter=*RPGP*Stress*
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, Stress) {
  auto key = GenerateFullKey("stress");
  ASSERT_TRUE(key != nullptr);

  const int iterations = StressIterations();
  for (int i = 0; i < iterations; ++i) {
    auto plain = GFBuffer(GenerateRandomString(256));

    auto [enc_err, enc_obj] =
        MessageCryptoOperation::GetInstance(Channel()).EncryptSync({key}, plain,
                                                                   true);
    ASSERT_EQ(CheckGpgError(enc_err), GPG_ERR_NO_ERROR)
        << "encrypt failed at iteration " << i;
    auto cipher = ExtractParams<GFBuffer>(enc_obj, 1);

    auto [dec_err, dec_obj] =
        MessageCryptoOperation::GetInstance(Channel()).DecryptSync(cipher);
    ASSERT_EQ(CheckGpgError(dec_err), GPG_ERR_NO_ERROR)
        << "decrypt failed at iteration " << i;
    auto decrypted = ExtractParams<GFBuffer>(dec_obj, 1);
    ASSERT_EQ(decrypted, plain) << "roundtrip mismatch at iteration " << i;

    auto [sign_err, sign_obj] =
        MessageCryptoOperation::GetInstance(Channel()).SignSync(
            {key}, plain, GPGME_SIG_MODE_DETACH, true);
    ASSERT_EQ(CheckGpgError(sign_err), GPG_ERR_NO_ERROR)
        << "sign failed at iteration " << i;
    auto signature = ExtractParams<GFBuffer>(sign_obj, 1);

    auto [verify_err, verify_obj] =
        MessageCryptoOperation::GetInstance(Channel()).VerifySync(plain,
                                                                  signature);
    ASSERT_EQ(CheckGpgError(verify_err), GPG_ERR_NO_ERROR)
        << "verify failed at iteration " << i;
  }

  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Stress: export -> import round-trips.
//
// The armored/binary buffer crosses the engine (and, for rPGP, the Rust FFI)
// boundary in both directions every iteration. This is a prime spot for buffer
// ownership / lifetime bugs (use-after-free, double-free, truncation), so it is
// worth hammering. Run under ASan/Valgrind for the most signal.
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, ImportExportRoundtripStress) {
  auto key = GenerateFullKey("import_export_stress");
  ASSERT_TRUE(key != nullptr);
  const auto fpr = key->Fingerprint().toUpper();

  // Export/import parses whole keys, so cap the churn to keep the default
  // stress phase bounded while still exercising the path thousands of times
  // across both engines.
  const int iterations = std::min(StressIterations(), 300);
  for (int i = 0; i < iterations; ++i) {
    auto [sec_err, secret_buf] =
        KeyImportExportOperation::GetInstance(Channel()).ExportKey(key, true,
                                                                   true, false);
    ASSERT_EQ(CheckGpgError(sec_err), GPG_ERR_NO_ERROR)
        << "export secret failed at iteration " << i;
    ASSERT_FALSE(secret_buf.Empty())
        << "empty secret export at iteration " << i;

    auto sec_info =
        KeyImportExportOperation::GetInstance(Channel()).ImportKey(secret_buf);
    ASSERT_TRUE(sec_info != nullptr)
        << "secret import failed at iteration " << i;

    auto [pub_err, public_buf] =
        KeyImportExportOperation::GetInstance(Channel()).ExportKey(key, false,
                                                                   true, false);
    ASSERT_EQ(CheckGpgError(pub_err), GPG_ERR_NO_ERROR)
        << "export public failed at iteration " << i;
    ASSERT_FALSE(public_buf.Empty())
        << "empty public export at iteration " << i;

    auto pub_info =
        KeyImportExportOperation::GetInstance(Channel()).ImportKey(public_buf);
    ASSERT_TRUE(pub_info != nullptr)
        << "public import failed at iteration " << i;
  }

  GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
  auto reloaded = GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(fpr);
  ASSERT_TRUE(reloaded != nullptr) << "key vanished after import/export churn";
  EXPECT_EQ(reloaded->Fingerprint().toUpper(), fpr);

  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Stress: rebuild the key model from the engine and walk every accessor.
//
// FlushKeyCache() forces the repository to re-read the key from the engine each
// iteration, then every getter (which, for rPGP, copies a string out of Rust)
// is exercised. A dangling pointer or stale buffer at the FFI boundary -- the
// exact class of bug behind the recent fingerprint-lifetime fix -- shows up
// here, especially under a sanitizer.
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, KeyModelTraversalStress) {
  auto key = GenerateFullKey("model_traversal_stress");
  ASSERT_TRUE(key != nullptr);
  const auto fpr = key->Fingerprint().toUpper();

  const int iterations = StressIterations();
  qsizetype sink = 0;  // observe the traversal so nothing is optimized away.
  for (int i = 0; i < iterations; ++i) {
    GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
    auto k = GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(fpr);
    ASSERT_TRUE(k != nullptr) << "key lookup failed at iteration " << i;

    sink += k->ID().size() + k->Name().size() + k->Email().size() +
            k->Fingerprint().size() + k->PublicKeyAlgo().size() +
            k->Algo().size();

    for (const auto& sub : k->SubKeys()) {
      sink += sub.ID().size() + sub.Fingerprint().size() + sub.Algo().size() +
              sub.PublicKeyAlgo().size();
      sink += static_cast<qsizetype>(sub.KeyLength());
      sink += static_cast<qsizetype>(sub.IsHasEncrCap());
      sink += static_cast<qsizetype>(sub.IsRevoked());
      sink += sub.CreationTime().toSecsSinceEpoch();
    }

    for (const auto& uid : k->UIDs()) {
      sink += uid.GetName().size() + uid.GetEmail().size() +
              uid.GetComment().size() + uid.GetUID().size();
      auto sigs = uid.GetSignatures();
      if (sigs != nullptr) sink += sigs->size();
    }
  }

  EXPECT_GT(sink, 0);
  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Stress: generate -> delete churn on the key database.
//
// Repeatedly inserting and removing whole keys exercises the engine's key-DB
// allocation/free paths. Key generation is comparatively expensive, so this is
// capped well below the other stress counts.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Stress: repeat the COMBINED encrypt+sign -> decrypt+verify cycle.
//
// This mirrors the exact user-reported crash path (`realloc(): invalid next
// size` during encrypt+sign / decrypt+verify). Unlike the plain Stress test,
// it drives EncryptSignSync/DecryptVerifySync, and varies the payload size so
// the output buffers cross the kSecBufferSize (4KB) boundary in Read2GFBuffer's
// realloc/append loop -- the most likely spot for a heap-buffer-overflow.
// Run under ASan for signal: scripts/run_tests_asan.sh -f
// '*EncryptSignDecryptVerifyStress*'
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, EncryptSignDecryptVerifyStress) {
  auto key = GenerateFullKey("encsign_stress");
  ASSERT_TRUE(key != nullptr);

  const int iterations = StressIterations();
  for (int i = 0; i < iterations; ++i) {
    // Vary the size across the 4KB secure-buffer boundary that Read2GFBuffer
    // chunks on, including sizes straddling multiples of it.
    const int size = 1 + (i * 257) % (32 * 1024);
    auto plain = GFBuffer(GenerateRandomString(size));

    auto [enc_err, enc_obj] =
        MessageCryptoOperation::GetInstance(Channel()).EncryptSignSync(
            {key}, {key}, plain, true);
    ASSERT_EQ(CheckGpgError(enc_err), GPG_ERR_NO_ERROR)
        << "encrypt+sign failed at iteration " << i << " size " << size;
    ASSERT_TRUE((enc_obj->Check<GpgEncryptResult, GpgSignResult, GFBuffer>()));
    auto cipher = ExtractParams<GFBuffer>(enc_obj, 2);

    auto [dec_err, dec_obj] =
        MessageCryptoOperation::GetInstance(Channel()).DecryptVerifySync(
            cipher);
    ASSERT_EQ(CheckGpgError(dec_err), GPG_ERR_NO_ERROR)
        << "decrypt+verify failed at iteration " << i << " size " << size;
    ASSERT_TRUE(
        (dec_obj->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));
    auto out = ExtractParams<GFBuffer>(dec_obj, 2);
    ASSERT_EQ(out, plain) << "roundtrip mismatch at iteration " << i;
  }

  DeleteKey(key);
}

// ---------------------------------------------------------------------------
// Regression: encrypt+sign / decrypt+verify with a PASSPHRASE-protected key.
//
// With a protected secret key the engine invokes the host passphrase-fetch
// callback. For rPGP, FetchPasswordCallback returns a length-delimited byte
// buffer that Rust then frees via gfc_secure_free_cstr() -> strlen(). The
// buffer used to be allocated without a NUL terminator, so strlen ran past it
// (heap-buffer-overflow), intermittently corrupting the heap and crashing with
// "realloc(): invalid next size". The other stress tests use NonPassPhrase keys
// and so never exercised this path. Run under ASan to catch a regression.
// ---------------------------------------------------------------------------

TEST_P(GpgCoreEngineTest, EncryptSignDecryptVerifyWithPassphrase) {
  auto key = GenerateFullKey("encsign_pass", /*with_passphrase=*/true);
  ASSERT_TRUE(key != nullptr);

  // A few rounds so the passphrase fetch + free path runs repeatedly.
  for (int i = 0; i < 5; ++i) {
    auto plain = GFBuffer(QString("passphrase path round %1").arg(i));

    auto [enc_err, enc_obj] =
        MessageCryptoOperation::GetInstance(Channel()).EncryptSignSync(
            {key}, {key}, plain, true);
    ASSERT_EQ(CheckGpgError(enc_err), GPG_ERR_NO_ERROR)
        << "encrypt+sign failed at round " << i;
    ASSERT_TRUE((enc_obj->Check<GpgEncryptResult, GpgSignResult, GFBuffer>()));
    auto cipher = ExtractParams<GFBuffer>(enc_obj, 2);

    auto [dec_err, dec_obj] =
        MessageCryptoOperation::GetInstance(Channel()).DecryptVerifySync(
            cipher);
    ASSERT_EQ(CheckGpgError(dec_err), GPG_ERR_NO_ERROR)
        << "decrypt+verify failed at round " << i;
    ASSERT_TRUE(
        (dec_obj->Check<GpgDecryptResult, GpgVerifyResult, GFBuffer>()));
    auto out = ExtractParams<GFBuffer>(dec_obj, 2);
    ASSERT_EQ(out, plain) << "roundtrip mismatch at round " << i;
  }

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, GenerateDeleteChurnStress) {
  const int iterations = std::min(StressIterations(), 30);
  for (int i = 0; i < iterations; ++i) {
    auto key = GenerateFullKey(QString("churn_%1").arg(i));
    ASSERT_TRUE(key != nullptr) << "key generation failed at iteration " << i;
    EXPECT_TRUE(key->IsGood());
    EXPECT_TRUE(key->IsPrivateKey());
    EXPECT_FALSE(key->SubKeys().empty());

    DeleteKey(key);
  }
}

}  // namespace GpgFrontend::Test
