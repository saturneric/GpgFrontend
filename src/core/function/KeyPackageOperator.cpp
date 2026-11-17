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

#include "core/function/GlobalSettingStation.h"
#include "core/function/KeyPackageOperator.h"
#include "core/function/openpgp/KeyImportExportOperation.h"
#include "core/function/rpgp/KeyStorage.h"
#include "core/model/GpgImportInformation.h"
#include "core/utils/GpgUtils.h"
#include "core/utils/IOUtils.h"

#ifdef HAS_RUST_SUPPORT
#include "core/GFCoreRust.h"
#include "core/function/rpgp/RustEngineCallback.h"
#include "core/utils/AsyncUtils.h"
#endif

namespace GpgFrontend {

namespace {

#ifdef HAS_RUST_SUPPORT

auto AsRust(const GFBuffer& buffer) -> Rust::GfrBuffer {
  return {reinterpret_cast<const uint8_t*>(buffer.Data()),
          static_cast<uintptr_t>(buffer.Size())};
}

/// The engine's own reason for the last failure on this thread, if it gave
/// one.
auto RustErrorDetail(const QString& fallback) -> QString {
  char* msg = Rust::gfr_get_last_error_msg();
  if (msg == nullptr) return fallback;
  auto detail = QString::fromUtf8(msg);
  Rust::gfr_crypto_free_string(msg);
  return detail.isEmpty() ? fallback : detail;
}

auto InfoFromHeader(const Rust::GfrKeypackHeaderC& header) -> KeyPackageInfo {
  KeyPackageInfo info;
  info.version = static_cast<int>(header.version);
  switch (header.protection) {
    case Rust::GfrKeypackProtection::Plain:
      info.protection = KeyPackageProtection::kPLAIN;
      break;
    case Rust::GfrKeypackProtection::PinFile:
    default:
      info.protection = KeyPackageProtection::kPIN_FILE;
      break;
  }
  info.secrets_unprotected = header.secrets_unprotected;
  info.package_id =
      QByteArray(reinterpret_cast<const char*>(header.package_id),
                 static_cast<qsizetype>(sizeof(header.package_id)))
          .toHex();
  info.generated_at = static_cast<qint64>(header.generated_at);
  info.key_count = static_cast<int>(header.key_count);
  return info;
}

/// Serves the engine's reads of a key package from the host's file: the
/// engine asks for the header, the index and then single key blocks, so the
/// file is never loaded whole.
auto ReadKeyPackage(void* user_data, uint64_t offset, uint8_t* buf,
                    uintptr_t len) -> int64_t {
  auto* file = static_cast<QFile*>(user_data);
  if (!file->seek(static_cast<qint64>(offset))) return -1;
  return file->read(reinterpret_cast<char*>(buf), static_cast<qint64>(len));
}

/// The header of a version 3 package, read from its first bytes only;
/// nullopt when the file is something else.
auto ReadV3Header(const QString& path) -> std::optional<KeyPackageInfo> {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  Rust::GfrKeypackHeaderC header{};
  const auto status =
      Rust::gfr_keypack_read_header(ReadKeyPackage, &file, &header);
  RustErrorDetail({});
  if (status == Rust::GfrStatus::ErrorUnsupportedFeature) {
    KeyPackageInfo info;
    info.version = 3;
    info.too_new = true;
    return info;
  }
  if (status != Rust::GfrStatus::Success) return {};
  return InfoFromHeader(header);
}

/// Open a version 3 package. Exactly one of (pin, key_file) and secret is
/// given. The engine reads the header and index, then one key at a time.
auto OpenV3(const QString& path, const GFBuffer& pin, const GFBuffer& key_file,
            const GFBuffer& secret) -> std::tuple<QString, GFBuffer> {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return {{"Read Key Package Failed"}, {}};
  }

  Rust::GfrKeypackHeaderC header{};
  Rust::GfrKeypack* pack = nullptr;
  const auto status =
      Rust::gfr_keypack_open(ReadKeyPackage, &file, AsRust(pin),
                             AsRust(key_file), AsRust(secret), &header, &pack);
  // Closed before the file it reads from.
  const auto close = qScopeGuard([&pack]() { Rust::gfr_keypack_close(pack); });
  if (status != Rust::GfrStatus::Success) {
    auto detail = RustErrorDetail(u"Decrypt the Key Package Failed"_s);
    // A wrong PIN or key reads the same as with version 2, so the import
    // wizard asks for it again where it was typed.
    if (status == Rust::GfrStatus::ErrorWrongPassword) {
      detail = u"Decrypt the Key of Key Package Failed"_s;
    }
    return {detail, {}};
  }

  // Every key, binary and back to back, as one keyring: listing and
  // importing read it as they read a version 2 package's content.
  GFBuffer key_data;
  for (uint32_t i = 0; i < header.key_count; ++i) {
    Rust::GfrKeypackKeyC key{};
    const auto read = Rust::gfr_keypack_read_key(pack, i, &key);
    if (read == Rust::GfrStatus::Success) {
      key_data.Append(reinterpret_cast<const char*>(key.key_data),
                      static_cast<ssize_t>(key.key_data_len));
    }
    Rust::gfr_keypack_free_key(&key);
    if (read != Rust::GfrStatus::Success) {
      return {RustErrorDetail(u"Decrypt the Key Package Failed"_s), {}};
    }
  }
  return {QString{}, key_data};
}

/// Put gfpipe beside a written package, ready to run: the program, and a
/// plain package's key in `<name>.gfkey`, readable by its owner only. An
/// error message, or empty.
auto SetUpGfPipe(const KeyPackageV3Request& request, KeyPackageV3Written& out)
    -> QString {
  const QFileInfo package(request.package_path);
  const QDir folder = package.absoluteDir();

  if (!out.plain_key.Empty()) {
    out.key_text_path =
        folder.filePath(package.completeBaseName() + u".gfkey"_s);
    if (!WriteFileGFBuffer(out.key_text_path, out.plain_key) ||
        !QFile::setPermissions(out.key_text_path,
                               QFile::ReadOwner | QFile::WriteOwner)) {
      return u"Save the Key for gfpipe Failed"_s;
    }
  }

  const QFileInfo program(request.gfpipe_program);
  out.gfpipe_path = folder.filePath(program.fileName());
  if (QFileInfo(out.gfpipe_path).canonicalFilePath() !=
      program.canonicalFilePath()) {
    // QFile::copy() does not replace; the replacement was confirmed.
    QFile::remove(out.gfpipe_path);
    if (!QFile::copy(program.absoluteFilePath(), out.gfpipe_path)) {
      return u"Copy gfpipe Failed"_s;
    }
  }
  QFile::setPermissions(out.gfpipe_path,
                        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                            QFile::ReadGroup | QFile::ExeGroup |
                            QFile::ReadOther | QFile::ExeOther);
  return {};
}

/// One binary key each: the secret key where it goes into the package,
/// without its passphrase when asked, the public certificate otherwise.
auto ExportBlocksForPackage(int channel, const KeyPackageV3Request& request)
    -> std::tuple<QString, QList<GFBuffer>> {
  auto& exporter = KeyImportExportOperation::GetInstance(channel);

  QList<GFBuffer> blocks;
  for (const auto& key : request.keys) {
    if (key == nullptr) continue;
    const bool with_secret = request.secret && key->IsPrivateKey();

    // Binary: the package keeps keys as binary OpenPGP.
    auto [err, block] = exporter.ExportKey(key, with_secret, false, false);
    if (!err.Ok() || block.Empty()) {
      return {u"Export Key Data Failed: "_s + key->ID(), {}};
    }

    if (with_secret && request.remove_passphrase) {
      uint8_t* open = nullptr;
      uintptr_t open_len = 0;
      const auto status = Rust::gfr_keypack_remove_key_password(
          channel, AsRust(block), FetchPasswordCallback, &open, &open_len);
      if (status != Rust::GfrStatus::Success || open == nullptr) {
        Rust::gfr_crypto_free_buffer(open, open_len);
        return {
            RustErrorDetail(u"Remove the Passphrase Failed: "_s + key->ID()),
            {}};
      }
      block = GFBuffer(reinterpret_cast<const char*>(open),
                       static_cast<size_t>(open_len));
      Rust::gfr_crypto_free_buffer(open, open_len);
    }
    blocks.append(block);
  }
  return {QString{}, blocks};
}

#endif

}  // namespace

KeyPackageOperator::KeyPackageOperator(int channel)
    : GpgFrontend::SingletonFunctionObject<KeyPackageOperator>(channel) {}

void KeyPackageOperator::GenerateKeyPackage(const QString& key_package_path,
                                            const QString& key_path,
                                            const GpgAbstractKeyPtrList& keys,
                                            const GFBuffer& pin, bool secret,
                                            const OperationCallback& cb) {
  KeyImportExportOperation::GetInstance(GetChannel())
      .ExportAllKeys(
          keys, secret, true, [=](GFError err, const DataObjectPtr& data_obj) {
            LOG_D() << "ExportAllKeys callback called, err: "
                    << DescribeGFError(err).second;
            if (!err.Ok()) {
              cb(-1, TransferParams(QString{"Export Key Data Failed: "} +
                                    DescribeGFError(err).second));
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
  auto [succ_p, encrypted_passphrase] = ReadFileGFBuffer(key_path);
  if (!succ_p || encrypted_passphrase.Empty()) {
    return {{"Read the Key of Key Package Failed"}, {}};
  }

#ifdef HAS_RUST_SUPPORT
  if (ReadV3Header(key_package_path)) {
    return OpenV3(key_package_path, pin, encrypted_passphrase, {});
  }
#endif

  auto [succ_e, encrypted] = ReadFileGFBuffer(key_package_path);
  if (!succ_e || encrypted.Empty()) return {{"Read Key Package Failed"}, {}};

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

auto KeyPackageOperator::GenerateKeyPackageV3Sync(
    const KeyPackageV3Request& request) -> KeyPackageV3Written {
#ifndef HAS_RUST_SUPPORT
  (void)request;
  return {u"Key package version 3 needs the rPGP engine"_s, {}, {}};
#else
  KeyPackageV3Written out;
  if (request.keys.empty()) {
    out.error = u"No Key to Export"_s;
    return out;
  }

  auto [export_error, blocks] = ExportBlocksForPackage(GetChannel(), request);
  if (!export_error.isEmpty()) {
    out.error = export_error;
    return out;
  }

  std::vector<Rust::GfrBuffer> rust_blocks;
  rust_blocks.reserve(blocks.size());
  for (const auto& block : blocks) rust_blocks.push_back(AsRust(block));

  const auto protection = [&]() {
    switch (request.protection) {
      case KeyPackageProtection::kPLAIN:
        return Rust::GfrKeypackProtection::Plain;
      case KeyPackageProtection::kPIN_FILE:
      default:
        return Rust::GfrKeypackProtection::PinFile;
    }
  }();

  Rust::GfrKeypackWriteResultC written{};
  const auto status = Rust::gfr_keypack_write(
      rust_blocks.data(), rust_blocks.size(), protection, AsRust(request.pin),
      static_cast<uint64_t>(QDateTime::currentSecsSinceEpoch()), &written);
  const auto release = qScopeGuard(
      [&written]() { Rust::gfr_keypack_free_write_result(&written); });
  if (status != Rust::GfrStatus::Success) {
    out.error = RustErrorDetail(u"Write the Key Package Failed"_s);
    return out;
  }

  const auto info = InfoFromHeader(written.header);
  out.package_id = info.package_id;

  if (!WriteFileGFBuffer(
          request.package_path,
          GFBuffer(reinterpret_cast<const char*>(written.package),
                   written.package_len))) {
    out.error = u"Write Key Package Failed"_s;
    return out;
  }
  if (written.key_file != nullptr &&
      !WriteFileGFBuffer(
          request.key_path,
          GFBuffer(reinterpret_cast<const char*>(written.key_file),
                   written.key_file_len))) {
    out.error = u"Write the Key of Key Package Failed"_s;
    return out;
  }

  if (written.plain_key != nullptr) {
    out.plain_key = GFBuffer(written.plain_key, std::strlen(written.plain_key));
  }

  if (!request.gfpipe_program.isEmpty()) {
    out.error = SetUpGfPipe(request, out);
  }
  return out;
#endif
}

void KeyPackageOperator::GenerateKeyPackageV3(
    const KeyPackageV3Request& request,
    const std::function<void(KeyPackageV3Written)>& cb) {
  auto result = QSharedPointer<KeyPackageV3Written>::create();
  RunOperaAsync(
      [this, request, result](const DataObjectPtr&) -> GFOperaStatus {
        *result = GenerateKeyPackageV3Sync(request);
        return 0;
      },
      [cb, result](GFOperaStatus, const DataObjectPtr&) { cb(*result); },
      u"generate_key_package_v3"_s);
}

auto KeyPackageOperator::InspectKeyPackage(const QString& key_package_path)
    -> KeyPackageInfo {
#ifdef HAS_RUST_SUPPORT
  if (auto info = ReadV3Header(key_package_path)) return *info;
#endif

  QFile file(key_package_path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  if (file.read(6) == "GFSEC2") {
    KeyPackageInfo info;
    info.version = 2;
    return info;
  }
  return {};
}

auto KeyPackageOperator::BundledGfPipe() -> QString {
  const QDir app_dir(GetGSS().GetAppDir());
#if defined(Q_OS_MACOS)
  // Contents/MacOS/GpgFrontend -> Contents/Helpers/gfpipe
  const auto path = app_dir.filePath(u"../Helpers/gfpipe"_s);
#elif defined(Q_OS_WINDOWS)
  const auto path = app_dir.filePath(u"gfpipe.exe"_s);
#else
  // bin/, $APPDIR/usr/bin and /app/bin: beside the application.
  const auto path = app_dir.filePath(u"gfpipe"_s);
#endif
  const QFileInfo info(path);
  return info.isFile() && info.isExecutable() ? info.canonicalFilePath()
                                              : QString{};
}

auto KeyPackageOperator::OpenKeyPackageWithSecret(
    const QString& key_package_path, const GFBuffer& secret)
    -> std::tuple<QString, GFBuffer> {
#ifndef HAS_RUST_SUPPORT
  (void)key_package_path;
  (void)secret;
  return {u"Key package version 3 needs the rPGP engine"_s, {}};
#else
  if (secret.Empty()) return {u"The Key Package Needs Its Key"_s, {}};
  return OpenV3(key_package_path, {}, {}, secret);
#endif
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
