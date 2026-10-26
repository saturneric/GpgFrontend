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

#include <QTemporaryDir>

#include "GpgCoreEngineTest.h"
#include "core/function/GFBufferFactory.h"
#include "core/function/KeyPackageOperator.h"
#include "core/function/openpgp/GpgKeyRepository.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/model/GpgImportInformation.h"
#include "core/model/GpgKey.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend::Test {

namespace {

/// Exports @p key from @p channel, armored, as the package writer does.
auto Export(int channel, const GpgKeyPtr& key, bool secret) -> GFBuffer {
  auto [err, buffer] = KeyImportExportOperation::GetInstance(channel).ExportKey(
      key, secret, true, false);
  EXPECT_EQ(CheckGpgError(err), GPG_ERR_NO_ERROR);
  return buffer;
}

/// Writes a key package the way KeyPackageOperator::GenerateKeyPackage()
/// does: the key data sealed with a passphrase, the passphrase sealed with
/// the PIN, each in a file of its own.
void WritePackage(const QString& package_path, const QString& key_path,
                  const GFBuffer& key_data, const GFBuffer& pin) {
  const GFBuffer passphrase(QString("a-random-package-passphrase"));
  auto sealed_data = GFBufferFactory::Encrypt(passphrase, key_data);
  auto sealed_passphrase = GFBufferFactory::Encrypt(pin, passphrase);
  ASSERT_TRUE(sealed_data && sealed_passphrase);
  ASSERT_TRUE(WriteFileGFBuffer(package_path, *sealed_data));
  ASSERT_TRUE(WriteFileGFBuffer(key_path, *sealed_passphrase));
}

auto Present(int channel, const QString& fpr) -> bool {
  GpgKeyRepository::GetInstance(channel).FlushKeyCache();
  const auto key = GpgKeyRepository::GetInstance(channel).GetKeyPtr(fpr);
  return key != nullptr && key->IsGood();
}

}  // namespace

TEST_P(GpgCoreEngineTest, KeyPackageOpensOnlyWithItsPin) {
  auto key = GenerateFullKey("key_package_pin");
  ASSERT_TRUE(key != nullptr);

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto package_path = dir.filePath("pkg.gfpack");
  const auto key_path = dir.filePath("pkg.key");
  const auto key_data = Export(Channel(), key, false);
  WritePackage(package_path, key_path, key_data, GFBuffer(QString("1234-pin")));

  auto& opera = KeyPackageOperator::GetInstance(Channel());

  // The PIN opens the key file; a wrong one fails there, and only there, so
  // the wizard can ask for it again.
  auto [wrong_err, wrong_data] =
      opera.OpenKeyPackage(package_path, key_path, GFBuffer(QString("nope")));
  EXPECT_EQ(wrong_err, "Decrypt the Key of Key Package Failed");
  EXPECT_TRUE(wrong_data.Empty());

  auto [err, data] = opera.OpenKeyPackage(package_path, key_path,
                                          GFBuffer(QString("1234-pin")));
  EXPECT_TRUE(err.isEmpty()) << err.toStdString();
  EXPECT_EQ(data.ConvertToQByteArray(), key_data.ConvertToQByteArray());

  DeleteKey(key);
}

TEST_P(GpgCoreEngineTest, KeyPackageListsEachKeyOnce) {
  auto with_secret = GenerateFullKey("key_package_list_a");
  auto public_only = GenerateFullKey("key_package_list_b");
  ASSERT_TRUE(with_secret != nullptr && public_only != nullptr);

  // Laid out as a package written with private keys: every key's public part,
  // then the private parts of the keys that have one.
  GFBuffer key_data = Export(Channel(), with_secret, false);
  key_data.Append(Export(Channel(), public_only, false));
  key_data.Append(Export(Channel(), with_secret, true));

  const auto keys = KeyPackageOperator::ListKeyPackageKeys(key_data);
  ASSERT_EQ(keys.size(), 2);

  const auto find = [&](const QString& fpr) {
    return std::find_if(keys.begin(), keys.end(), [&](const auto& k) {
      return k.fpr.compare(fpr, Qt::CaseInsensitive) == 0;
    });
  };
  const auto a = find(with_secret->Fingerprint());
  const auto b = find(public_only->Fingerprint());
  ASSERT_NE(a, keys.end());
  ASSERT_NE(b, keys.end());
  EXPECT_TRUE(a->has_secret);
  EXPECT_FALSE(b->has_secret);
  EXPECT_FALSE(a->key_id.isEmpty());
  EXPECT_TRUE(a->name.contains("key_package_list_a") ||
              a->email.contains("key_package_list_a"));

  DeleteKey(with_secret);
  DeleteKey(public_only);
}

TEST_P(GpgCoreEngineTest, KeyPackageImportsOnlyTheChosenKeys) {
  auto chosen = GenerateFullKey("key_package_chosen");
  auto left_out = GenerateFullKey("key_package_left_out");
  ASSERT_TRUE(chosen != nullptr && left_out != nullptr);
  const auto chosen_fpr = chosen->Fingerprint();
  const auto left_out_fpr = left_out->Fingerprint();

  GFBuffer key_data = Export(Channel(), chosen, false);
  key_data.Append(Export(Channel(), left_out, false));

  // Gone from the keyring, so the import alone decides whether they return.
  DeleteKey(chosen);
  DeleteKey(left_out);
  ASSERT_FALSE(Present(Channel(), chosen_fpr));
  ASSERT_FALSE(Present(Channel(), left_out_fpr));

  auto& opera = KeyPackageOperator::GetInstance(Channel());
  auto [err, info] = opera.ImportKeyPackageKeys(key_data, {chosen_fpr});
  EXPECT_TRUE(err.isEmpty()) << err.toStdString();
  ASSERT_TRUE(info != nullptr);

  EXPECT_TRUE(Present(Channel(), chosen_fpr));
  EXPECT_FALSE(Present(Channel(), left_out_fpr));

  // A fingerprint the package does not hold imports nothing, and says so.
  auto [none_err, none_info] =
      opera.ImportKeyPackageKeys(key_data, {QString(40, 'F')});
  EXPECT_EQ(none_err, "No Selected Key Found in the Key Package");
  EXPECT_TRUE(none_info == nullptr);

  GpgKeyRepository::GetInstance(Channel()).FlushKeyCache();
  DeleteKey(GpgKeyRepository::GetInstance(Channel()).GetKeyPtr(chosen_fpr));
}

}  // namespace GpgFrontend::Test
