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

#include "KeyPackageOperator.h"

#include "core/function/KeyPackageOperator.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

namespace GpgFrontend {

KeyPackageOperator::KeyPackageOperator(int channel)
    : GpgFrontend::SingletonFunctionObject<KeyPackageOperator>(channel) {}

void KeyPackageOperator::GenerateKeyPackage(const QString& key_package_path,
                                            const QString& key_path,
                                            const GpgAbstractKeyPtrList& keys,
                                            const GFBuffer& pin, bool secret,
                                            const OperationCallback& cb) {
  KeyImportExportOperation::GetInstance(GetChannel())
      .ExportAllKeys(
          keys, secret, true, [=](GpgError err, const DataObjectPtr& data_obj) {
            LOG_D() << "ExportAllKeys callback called, err: "
                    << DescribeGpgErrCode(err).second;
            if (CheckGpgError(err) != GPG_ERR_NO_ERROR) {
              cb(-1, TransferParams(QString{"Export Key Data Failed: "} +
                                    DescribeGpgErrCode(err).second));
              return;
            }

            if (data_obj == nullptr || !data_obj->Check<GFBuffer>()) {
              cb(-1, TransferParams(QString{"Fetch Key Data Failed"}));
              return;
            }

            auto gf_buffer = ExtractParams<GFBuffer>(data_obj, 0);

            auto p = gb_fac_.RandomGpgPassphase(256);
            if (!p) {
              cb(-1, TransferParams(QString{"Generate Passphase Failed"}));
              return;
            }

            // AES encrypt
            auto encrypted = GFBufferFactory::Encrypt(*p, gf_buffer);
            if (!encrypted) {
              cb(-1, TransferParams(QString{"Encrypt Key Data Failed"}));
              return;
            }

            auto encrypted_phrase = GFBufferFactory::Encrypt(pin, *p);
            if (!encrypted_phrase) {
              cb(-1, TransferParams(QString{"Encrypt Passphase Failed"}));
              return;
            }

            auto succ = WriteFileGFBuffer(key_package_path, *encrypted);
            if (!succ) {
              cb(-1, TransferParams(QString{"Write Key Package Failed"}));
              return;
            }

            succ = WriteFileGFBuffer(key_path, *encrypted_phrase);
            if (!succ) {
              // A package without its key cannot be opened by anyone.
              QFile::remove(key_package_path);
              cb(-1, TransferParams(
                         QString{"Write the Key of Key Package Failed"}));
              return;
            }

            cb(0, TransferParams(QString{}));
            return;
          });
}

auto KeyPackageOperator::ImportKeyPackage(const QString& key_package_path,
                                          const QString& key_path,
                                          const GFBuffer& pin)
    -> std::tuple<QString, QSharedPointer<GpgImportInformation>> {
  auto [err, key_data] = OpenKeyPackage(key_package_path, key_path, pin);
  if (!err.isEmpty()) return {err, {}};

  auto p_info =
      KeyImportExportOperation::GetInstance(GetChannel()).ImportKey(key_data);
  if (!p_info) return {{"Import Key Data in the Key Package Failed"}, {}};

  return {QString{}, p_info};
}

auto KeyPackageOperator::OpenKeyPackage(const QString& key_package_path,
                                        const QString& key_path,
                                        const GFBuffer& pin)
    -> std::tuple<QString, GFBuffer> {
  auto [succ_e, encrypted] = ReadFileGFBuffer(key_package_path);
  if (!succ_e || encrypted.Empty()) return {{"Read Key Package Failed"}, {}};

  auto [succ_p, encrypted_passphrase] = ReadFileGFBuffer(key_path);
  if (!succ_p || encrypted_passphrase.Empty()) {
    return {{"Read the Key of Key Package Failed"}, {}};
  }

  auto passphrase = GFBufferFactory::Decrypt(pin, encrypted_passphrase);
  if (!passphrase) return {{"Decrypt the Key of Key Package Failed"}, {}};

  auto key_data = GFBufferFactory::Decrypt(*passphrase, encrypted);
  if (!key_data) return {{"Decrypt the Key Package Failed"}, {}};

  return {QString{}, *key_data};
}

auto KeyPackageOperator::ListKeyPackageKeys(const GFBuffer& key_data)
    -> QList<KeyPackageKey> {
  QList<KeyPackageKey> keys;

  for (const auto& gf_key : GetGFKeysFromKeyBlock(key_data)) {
    const auto& meta = gf_key.metadata;
    if (meta.fpr.isEmpty()) continue;

    auto it = std::find_if(keys.begin(), keys.end(), [&](const auto& key) {
      return key.fpr.compare(meta.fpr, Qt::CaseInsensitive) == 0;
    });
    if (it != keys.end()) {
      it->has_secret = it->has_secret || meta.has_secret;
      continue;
    }

    KeyPackageKey key;
    key.fpr = meta.fpr.toUpper();
    key.key_id = meta.key_id.toUpper();
    key.has_secret = meta.has_secret;

    // The primary user ID names the key; the first stands in without one.
    const auto primary =
        std::find_if(meta.user_ids.begin(), meta.user_ids.end(),
                     [](const auto& uid) { return uid.is_primary; });
    const auto* uid = primary != meta.user_ids.end() ? &*primary
                      : meta.user_ids.isEmpty()      ? nullptr
                                                     : &meta.user_ids.front();
    if (uid != nullptr) {
      key.name = uid->name;
      key.email = uid->email;
    }

    keys.append(key);
  }

  return keys;
}

auto KeyPackageOperator::ImportKeyPackageKeys(const GFBuffer& key_data,
                                              const QStringList& fprs)
    -> std::tuple<QString, QSharedPointer<GpgImportInformation>> {
  // Every block of every chosen key, public and secret, joined into one
  // armored key block that either engine imports as usual. Blocks of keys
  // left out never reach the engine.
  GFBuffer selected;
  for (const auto& gf_key : GetGFKeysFromKeyBlock(key_data)) {
    if (!fprs.contains(gf_key.metadata.fpr, Qt::CaseInsensitive)) continue;

    for (const auto* block :
         {&gf_key.blocks.public_key, &gf_key.blocks.secret_key}) {
      if (block->Empty()) continue;
      selected.Append(*block);
      selected.Append("\n", 1);
    }
  }
  if (selected.Empty())
    return {{"No Selected Key Found in the Key Package"}, {}};

  auto p_info =
      KeyImportExportOperation::GetInstance(GetChannel()).ImportKey(selected);
  if (!p_info) return {{"Import Key Data in the Key Package Failed"}, {}};

  return {QString{}, p_info};
}

auto KeyPackageOperator::GenerateKeyPackageName() -> QString {
  return generate_key_package_name();
}

/**
 * @brief generate key package name
 *
 * @return QString key package name
 */
auto KeyPackageOperator::generate_key_package_name() -> QString {
  auto random = gb_fac_.RandomGpgZBasePassphase(31);
  if (!random) {
    LOG_E()
        << "generate random passphrase failed when generating key package name";
    return QString{"KeyPackage"};
  }

  return QString("KeyPackage_%1").arg(random->Left(8).ConvertToQString());
}

}  // namespace GpgFrontend
