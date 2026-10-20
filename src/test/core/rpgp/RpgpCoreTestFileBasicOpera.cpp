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

#include "RpgpCoreTest.h"
#include "core/function/openpgp/FileCryptoOperation.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgSignResult.h"
#include "core/model/GpgVerifyResult.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

TEST_F(RpgpCoreTest, CoreFileEncryptDecryptTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP File!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptFileSync({encrypt_key}, input_file, true, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptFileSync(output_file, decrypt_output_file);

  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_FALSE(decr_result.Recipients().empty());

  const auto [read_success, out_buffer] = ReadFileGFBuffer(decrypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(RpgpCoreTest, CoreFileEncryptDecryptBinaryTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP File Binary!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptFileSync({encrypt_key}, input_file, false, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptFileSync(output_file, decrypt_output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult>()));
  auto decr_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_FALSE(decr_result.Recipients().empty());

  const auto [read_success, out_buffer] = ReadFileGFBuffer(decrypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(RpgpCoreTest, CoreFileEncryptSymmetricDecryptTest) {
  auto buffer = GFBuffer(QString("Hello RPGP Symmetric File!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptFileSymmetricSync(input_file, true, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgEncryptResult>()));
  auto result = ExtractParams<GpgEncryptResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidRecipients().empty());

  auto decrypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptFileSync(output_file, decrypt_output_file);

  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult>()));
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  ASSERT_TRUE(decrypt_result.Recipients().empty());

  const auto [read_success, out_buffer] = ReadFileGFBuffer(decrypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

TEST_F(RpgpCoreTest, CoreFileSignVerifyNormalTest) {
  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);

  auto input_file = CreateTempFileAndWriteData("Hello RPGP File Sign!");
  auto output_file = GetTempFilePath();

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .SignFileSync({sign_key}, input_file, true, output_file);

  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object->Check<GpgSignResult>()));
  auto result = ExtractParams<GpgSignResult>(data_object, 0);
  ASSERT_TRUE(result.InvalidSigners().empty());

  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .VerifyFileSync(input_file, output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgVerifyResult>()));
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 0);
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "575572EF0DF799AB884EC6C114C6B0B1596A2755");
}

TEST_F(RpgpCoreTest, CoreFileEncryptSignDecryptVerifyTest) {
  auto encrypt_key =
      GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
          .GetPubkeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(encrypt_key != nullptr);

  auto sign_key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                      .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(sign_key != nullptr);

  auto buffer = GFBuffer(QString("Hello RPGP File Encrypt Sign!"));
  auto input_file = CreateTempFileAndWriteData(buffer);
  auto output_file = GetTempFilePath();

  ASSERT_TRUE(sign_key->IsPrivateKey());
  ASSERT_TRUE(sign_key->IsHasActualSignCap());

  auto [err, data_object] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .EncryptSignFileSync({encrypt_key}, {sign_key}, input_file, true,
                               output_file);

  ASSERT_TRUE((data_object->Check<GpgEncryptResult, GpgSignResult>()));
  auto encr_result = ExtractParams<GpgEncryptResult>(data_object, 0);
  auto sign_result = ExtractParams<GpgSignResult>(data_object, 1);
  ASSERT_TRUE(encr_result.InvalidRecipients().empty());
  ASSERT_TRUE(sign_result.InvalidSigners().empty());
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);

  auto decrypt_output_file = GetTempFilePath();
  auto [err_0, data_object_0] =
      FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest)
          .DecryptVerifyFileSync(output_file, decrypt_output_file);

  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  ASSERT_TRUE((data_object_0->Check<GpgDecryptResult, GpgVerifyResult>()));
  auto decrypt_result = ExtractParams<GpgDecryptResult>(data_object_0, 0);
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 1);

  ASSERT_FALSE(decrypt_result.Recipients().empty());
  ASSERT_FALSE(verify_result.GetSignature().empty());
  ASSERT_EQ(verify_result.GetSignature().at(0).GetFingerprint(),
            "575572EF0DF799AB884EC6C114C6B0B1596A2755");

  const auto [read_success, out_buffer] = ReadFileGFBuffer(decrypt_output_file);
  ASSERT_TRUE(read_success);
  ASSERT_EQ(buffer, out_buffer);
}

namespace {

constexpr auto kLargeFileEnv = "GF_RUN_LARGE_FILE_TESTS";

// Flip the last byte of `path`: inside the SEIPD v1 MDC, or the final SEIPD v2
// authentication tag.
void FlipLastByte(const QString& path) {
  QFile f(path);
  ASSERT_TRUE(f.open(QIODevice::ReadWrite));
  ASSERT_TRUE(f.seek(f.size() - 1));
  char c = 0;
  ASSERT_TRUE(f.getChar(&c));
  ASSERT_TRUE(f.seek(f.size() - 1));
  ASSERT_TRUE(f.putChar(static_cast<char>(c ^ 0xFF)));
}

auto EncryptForTest(const GFBuffer& plain, bool sign) -> QString {
  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  EXPECT_TRUE(key != nullptr);
  auto input_file = CreateTempFileAndWriteData(plain);
  auto output_file = GetTempFilePath();
  auto& op = FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest);
  auto [err, data_object] =
      sign
          ? op.EncryptSignFileSync({key}, {key}, input_file, false, output_file)
          : op.EncryptFileSync({key}, input_file, false, output_file);
  EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  return output_file;
}

}  // namespace

// A tampered file fails as an integrity problem through the same API the GUI
// uses, and the engine keeps no output: neither a new file nor a change to an
// existing one.
TEST_F(RpgpCoreTest, CoreFileDecryptTamperedFileCommitsNothingTest) {
  for (const bool verify : {false, true}) {
    auto cipher =
        EncryptForTest(GFBuffer(QString("tamper me").repeated(4096)), verify);
    FlipLastByte(cipher);

    auto& op = FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest);
    auto fresh = GetTempFilePath();
    auto [err, data_object] = verify ? op.DecryptVerifyFileSync(cipher, fresh)
                                     : op.DecryptFileSync(cipher, fresh);
    EXPECT_EQ(CheckGpgError(err), GPG_ERR_BAD_DATA) << "verify=" << verify;
    EXPECT_FALSE(QFileInfo::exists(fresh)) << "verify=" << verify;

    auto existing = CreateTempFileAndWriteData(GFBuffer(QString("previous")));
    auto [err_1, data_object_1] =
        verify ? op.DecryptVerifyFileSync(cipher, existing)
               : op.DecryptFileSync(cipher, existing);
    EXPECT_EQ(CheckGpgError(err_1), GPG_ERR_BAD_DATA);
    const auto [ok, kept] = ReadFileGFBuffer(existing);
    ASSERT_TRUE(ok);
    EXPECT_EQ(kept, GFBuffer(QString("previous")));
  }
}

// The large-file tier (GF_RUN_LARGE_FILE_TESTS=1, run by
// scripts/run_tests.sh --large-file-only): a SEIPD file over the former 1 GiB
// limit, encrypted, signed, decrypted and verified end to end through the C++
// engine API, compared by hash, with bounded peak memory on Linux.
TEST_F(RpgpCoreTest, LargeFileEncryptSignDecryptVerifyAboveFormerLimitTest) {
  if (qEnvironmentVariable(kLargeFileEnv) != "1") {
    GTEST_SKIP() << "large-file tier: set " << kLargeFileEnv << "=1";
  }
  constexpr qint64 kSize = (qint64{1} << 30) + (qint64{1} << 20);

  auto input_file = GetTempFilePath();
  QCryptographicHash want(QCryptographicHash::Sha256);
  {
    QFile f(input_file);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    QByteArray chunk(1 << 20, Qt::Uninitialized);
    quint64 state = 0x5E1D0001;
    for (qint64 written = 0; written < kSize; written += chunk.size()) {
      for (auto& byte : chunk) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        byte = static_cast<char>(state >> 56);
      }
      want.addData(chunk);
      ASSERT_EQ(f.write(chunk), chunk.size());
    }
  }

  auto key = GpgKeyRepository::GetInstance(kRpgpChannelForUnitTest)
                 .GetKeyPtr("3B20B337A988D2C9917D0F33BDB8BB6BDDFA8497");
  ASSERT_TRUE(key != nullptr);
  auto& op = FileCryptoOperation::GetInstance(kRpgpChannelForUnitTest);
  auto cipher = GetTempFilePath();
  auto [err, data_object] =
      op.EncryptSignFileSync({key}, {key}, input_file, false, cipher);
  ASSERT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  QFile::remove(input_file);

#if defined(__linux__)
  // Reset the peak-RSS mark so only the decryption is measured.
  QFile clear_refs("/proc/self/clear_refs");
  if (clear_refs.open(QIODevice::WriteOnly)) clear_refs.write("5");
  auto status_kib = [](const char* field) -> qint64 {
    QFile status("/proc/self/status");
    if (!status.open(QIODevice::ReadOnly)) return -1;
    for (const auto& line : status.readAll().split('\n')) {
      if (line.startsWith(field)) {
        return line.mid(qstrlen(field))
            .trimmed()
            .split(' ')
            .front()
            .toLongLong();
      }
    }
    return -1;
  };
  const auto baseline_kib = status_kib("VmRSS:");
#endif

  auto output = GetTempFilePath();
  auto [err_0, data_object_0] = op.DecryptVerifyFileSync(cipher, output);
  ASSERT_EQ(CheckGpgError(err_0), GPG_ERR_NO_ERROR);
  auto verify_result = ExtractParams<GpgVerifyResult>(data_object_0, 1);
  ASSERT_FALSE(verify_result.GetSignature().empty());

#if defined(__linux__)
  const auto growth_mib = (status_kib("VmHWM:") - baseline_kib) / 1024;
  EXPECT_LT(growth_mib, 256)
      << "peak RSS grew by " << growth_mib << " MiB decrypting a 1 GiB file";
#endif

  QFile out(output);
  ASSERT_TRUE(out.open(QIODevice::ReadOnly));
  QCryptographicHash got(QCryptographicHash::Sha256);
  ASSERT_TRUE(got.addData(&out));
  EXPECT_EQ(out.size(), kSize);
  EXPECT_EQ(got.result(), want.result());
  out.close();
  QFile::remove(output);

  FlipLastByte(cipher);
  auto [err_1, data_object_1] = op.DecryptVerifyFileSync(cipher, output);
  EXPECT_EQ(CheckGpgError(err_1), GPG_ERR_BAD_DATA);
  EXPECT_FALSE(QFileInfo::exists(output));
  QFile::remove(cipher);
}

}  // namespace GpgFrontend::Test
