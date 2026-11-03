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

// gpgme's operation results, as the GnuPG backend turns them into models.

#include "core/function/gpg/GpgModelConvert.h"
#include "core/model/GpgDecryptResult.h"
#include "core/model/GpgEncryptResult.h"
#include "core/model/GpgGenerateKeyResult.h"
#include "core/model/GpgImportInformation.h"

namespace GpgFrontend::Test {

// No gpgme result means the operation produced none: never a good result.
TEST(GpgModelConvertTest, MissingGpgmeResultsAreNotGood) {
  EXPECT_FALSE(EncryptResultOf(nullptr).IsGood());
  EXPECT_FALSE(DecryptResultOf(nullptr).IsGood());
}

TEST(GpgModelConvertTest, GpgmeDecryptResultIsCarriedOver) {
  _gpgme_recipient recipient{};
  std::array<char, 17> keyid{"0123456789ABCDEF"};
  recipient.keyid = keyid.data();
  recipient.pubkey_algo = GPGME_PK_RSA;
  recipient.status = GPG_ERR_NO_SECKEY;

  std::array<char, 9> file_name{"note.txt"};
  _gpgme_op_decrypt_result raw{};
  raw.file_name = file_name.data();
  raw.is_mime = 1;
  raw.legacy_cipher_nomdc = 1;
  raw.recipients = &recipient;

  auto result = DecryptResultOf(&raw);
  ASSERT_TRUE(result.IsGood());
  EXPECT_EQ(result.Filename(), "note.txt");
  EXPECT_TRUE(result.MIME());
  EXPECT_FALSE(result.MessageIntegrityProtected());

  auto recipients = result.Recipients();
  ASSERT_EQ(recipients.size(), 1);
  EXPECT_EQ(recipients.front().keyid, "0123456789ABCDEF");
  EXPECT_EQ(recipients.front().status.code, GFErrorCode::kNO_SECRET_KEY);
}

TEST(GpgModelConvertTest, GpgmeInvalidRecipientsAreCarriedOver) {
  std::array<char, 5> fpr{"ABCD"};
  _gpgme_invalid_key invalid{};
  invalid.fpr = fpr.data();
  invalid.reason = GPG_ERR_UNUSABLE_PUBKEY;

  _gpgme_op_encrypt_result raw{};
  raw.invalid_recipients = &invalid;

  auto result = EncryptResultOf(&raw);
  ASSERT_TRUE(result.IsGood());
  auto invalids = result.InvalidRecipients();
  ASSERT_EQ(invalids.size(), 1);
  EXPECT_EQ(std::get<0>(invalids.front()), "ABCD");
  EXPECT_EQ(std::get<1>(invalids.front()).code,
            GFErrorCode::kUNUSABLE_PUBLIC_KEY);
}

TEST(GpgModelConvertTest, GpgmeGenerateKeyResultIsCarriedOver) {
  EXPECT_FALSE(GenerateKeyResultOf(nullptr).IsGood());
  EXPECT_TRUE(GpgGenerateKeyResult().GetFingerprint().isEmpty());

  std::array<char, 5> fpr{"ABCD"};
  _gpgme_op_genkey_result raw{};
  raw.fpr = fpr.data();

  auto result = GenerateKeyResultOf(&raw);
  EXPECT_TRUE(result.IsGood());
  EXPECT_EQ(result.GetFingerprint(), "ABCD");
}

TEST(GpgModelConvertTest, GpgmeImportResultIsCarriedOver) {
  EXPECT_EQ(ImportInformationOf(nullptr).considered, 0);

  std::array<char, 5> fpr{"ABCD"};
  _gpgme_import_status status{};
  status.fpr = fpr.data();
  status.status = GPGME_IMPORT_NEW | GPGME_IMPORT_SECRET;

  _gpgme_op_import_result raw{};
  raw.considered = 2;
  raw.imported = 1;
  raw.secret_unchanged = 1;
  raw.imports = &status;

  auto info = ImportInformationOf(&raw);
  EXPECT_EQ(info.considered, 2);
  EXPECT_EQ(info.imported, 1);
  EXPECT_EQ(info.secret_unchanged, 1);
  ASSERT_EQ(info.imported_keys.size(), 1U);
  EXPECT_EQ(info.imported_keys.front().fpr, "ABCD");
  EXPECT_EQ(info.imported_keys.front().import_status, 17);
}

}  // namespace GpgFrontend::Test
