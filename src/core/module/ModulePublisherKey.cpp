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

#include "core/module/ModulePublisherKey.h"

#include <sodium.h>

#include <QFile>
#include <QFileInfo>
#include <array>

#include "core/utils/CommonUtils.h"

namespace GpgFrontend::Module {

namespace {

constexpr qsizetype kKeyHexLength = 64;

/// Anything bigger is not a key file, and is not read whole to find out.
constexpr qint64 kMaxKeyFileBytes = 256;

/// Wipe a buffer this code owns. detach() first, so a shared copy is not
/// the one left holding the bytes.
void Wipe(QByteArray& bytes) {
  if (bytes.isEmpty()) return;
  bytes.detach();
  sodium_memzero(bytes.data(), static_cast<size_t>(bytes.size()));
  bytes.clear();
}

auto IsLowerHex(const QByteArray& text) -> bool {
  for (const auto c : text) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}

/// `header \n hex [\n | \r\n]`, and nothing else.
auto ParseTyped(const QByteArray& contents, const char* header,
                const char* what, QByteArray& out, QString& reason) -> bool {
  const QByteArray expected_header(header);

  if (contents.size() == 32 && !contents.startsWith(expected_header)) {
    // Named, because it is the one mistake this format exists to catch.
    reason = QString(
                 "this is a raw 32-byte key, not a %1 file; a raw seed "
                 "is what a build key looks like, and a build key is "
                 "never a publisher identity")
                 .arg(what);
    return false;
  }

  const auto newline = contents.indexOf('\n');
  if (newline < 0 || contents.left(newline) != expected_header) {
    reason = QString("this is not a %1 file: it must begin with \"%2\"")
                 .arg(what, QString::fromLatin1(header));
    return false;
  }

  auto body = contents.mid(newline + 1);
  if (body.endsWith("\r\n")) {
    body.chop(2);
  } else if (body.endsWith('\n')) {
    body.chop(1);
  }

  const auto well_formed = body.size() == kKeyHexLength && IsLowerHex(body);
  if (well_formed) out = QByteArray::fromHex(body);
  Wipe(body);

  if (!well_formed) {
    reason = QString(
                 "the key in this %1 file must be exactly %2 lower-case "
                 "hex characters")
                 .arg(what)
                 .arg(kKeyHexLength);
    return false;
  }
  return true;
}

auto ReadSmallFile(const QString& path, QByteArray& out, QString& reason)
    -> bool {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    reason = QString("\"%1\" could not be read").arg(path);
    return false;
  }
  if (file.size() > kMaxKeyFileBytes) {
    reason = QString("\"%1\" is too large to be a key file").arg(path);
    return false;
  }
  out = file.read(kMaxKeyFileBytes + 1);
  return true;
}

/// Create @p path, which must not exist, owner-only before anything is in it.
auto WriteNewFile(const QString& path, const QByteArray& bytes,
                  QFile::Permissions permissions, QString& reason) -> bool {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
    reason = QFileInfo::exists(path)
                 ? QString(
                       "\"%1\" already exists; a publisher key is never "
                       "replaced")
                       .arg(path)
                 : QString("\"%1\" could not be created").arg(path);
    return false;
  }
  // Narrowed while the file is still empty, so the secret is never on disk
  // under the default mask.
  if (!file.setPermissions(permissions) || file.write(bytes) != bytes.size() ||
      !file.flush()) {
    file.close();
    file.remove();
    reason = QString("\"%1\" could not be written").arg(path);
    return false;
  }
  file.close();
  return true;
}

}  // namespace

auto ModulePublisherKeyText(const QByteArray& public_key) -> QString {
  if (public_key.size() != crypto_sign_PUBLICKEYBYTES) return {};
  return QString::fromLatin1(public_key.toHex());
}

auto ModulePublisherKeyFingerprint(const QByteArray& public_key) -> QString {
  if (public_key.isEmpty()) return {};
  return BeautifyFingerprint(QString::fromLatin1(public_key.toHex().toUpper()));
}

auto ModulePublisherPublicKeyFromSeed(const QByteArray& seed) -> QByteArray {
  if (seed.size() != crypto_sign_SEEDBYTES || !EnsureSodiumInit()) return {};

  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> public_key{};
  std::array<unsigned char, crypto_sign_SECRETKEYBYTES> secret_key{};
  const auto ok =
      crypto_sign_seed_keypair(
          public_key.data(), secret_key.data(),
          reinterpret_cast<const unsigned char*>(seed.constData())) == 0;
  sodium_memzero(secret_key.data(), secret_key.size());
  if (!ok) return {};

  return {reinterpret_cast<const char*>(public_key.data()),
          static_cast<qsizetype>(public_key.size())};
}

auto ParseModulePublisherSecretKey(const QByteArray& contents, QByteArray& seed,
                                   QString& reason) -> bool {
  return ParseTyped(contents, kModulePublisherSecretKeyHeader,
                    "publisher secret key", seed, reason);
}

auto ParseModulePublisherPublicKey(const QByteArray& contents,
                                   QByteArray& public_key, QString& reason)
    -> bool {
  return ParseTyped(contents, kModulePublisherPublicKeyHeader,
                    "publisher public key", public_key, reason);
}

auto ReadModulePublisherSecretKey(const QString& path, QByteArray& seed,
                                  QString& reason) -> bool {
  QByteArray contents;
  if (!ReadSmallFile(path, contents, reason)) return false;
  const auto ok = ParseModulePublisherSecretKey(contents, seed, reason);
  Wipe(contents);
  return ok;
}

auto ReadModulePublisherPublicKey(const QString& path, QByteArray& public_key,
                                  QString& reason) -> bool {
  QByteArray contents;
  if (!ReadSmallFile(path, contents, reason)) return false;

  if (contents.startsWith(QByteArray(kModulePublisherSecretKeyHeader) + '\n')) {
    QByteArray seed;
    const auto parsed = ParseModulePublisherSecretKey(contents, seed, reason);
    Wipe(contents);
    if (!parsed) return false;
    public_key = ModulePublisherPublicKeyFromSeed(seed);
    Wipe(seed);
    if (public_key.isEmpty()) {
      reason = "the key in this file could not be used";
      return false;
    }
    return true;
  }

  return ParseModulePublisherPublicKey(contents, public_key, reason);
}

auto IsModulePublisherSecretKeyExposed(const QString& path) -> bool {
#ifdef Q_OS_WIN
  Q_UNUSED(path);
  return false;
#else
  const auto permissions = QFileInfo(path).permissions();
  return (permissions & (QFile::ReadGroup | QFile::WriteGroup |
                         QFile::ReadOther | QFile::WriteOther)) != 0;
#endif
}

auto GenerateModulePublisherKeyFiles(const QString& secret_path,
                                     const QString& public_path,
                                     QByteArray& public_key, QString& reason)
    -> bool {
  if (!EnsureSodiumInit()) {
    reason = "the cryptography library could not be started";
    return false;
  }
  if (QFileInfo::exists(secret_path) || QFileInfo::exists(public_path)) {
    reason =
        QString(
            "\"%1\" already exists; a publisher key is never "
            "replaced")
            .arg(QFileInfo::exists(secret_path) ? secret_path : public_path);
    return false;
  }

  QByteArray seed(crypto_sign_SEEDBYTES, '\0');
  randombytes_buf(seed.data(), static_cast<size_t>(seed.size()));
  public_key = ModulePublisherPublicKeyFromSeed(seed);
  if (public_key.isEmpty()) {
    Wipe(seed);
    reason = "a publisher key could not be generated";
    return false;
  }

  // Assembled in one reserved buffer, so no intermediate copy of the hex is
  // left behind unwiped by a reallocation.
  QByteArray hex(kKeyHexLength + 1, '\0');
  sodium_bin2hex(hex.data(), static_cast<size_t>(hex.size()),
                 reinterpret_cast<const unsigned char*>(seed.constData()),
                 static_cast<size_t>(seed.size()));
  hex.chop(1);
  Wipe(seed);

  const QByteArray header(kModulePublisherSecretKeyHeader);
  QByteArray secret_text;
  secret_text.reserve(header.size() + hex.size() + 2);
  secret_text.append(header).append('\n').append(hex).append('\n');
  Wipe(hex);
  const auto secret_ok = WriteNewFile(
      secret_path, secret_text, QFile::ReadOwner | QFile::WriteOwner, reason);
  Wipe(secret_text);
  if (!secret_ok) return false;

  const auto public_text = QByteArray(kModulePublisherPublicKeyHeader) + '\n' +
                           public_key.toHex() + '\n';
  if (!WriteNewFile(public_path, public_text,
                    QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup |
                        QFile::ReadOther,
                    reason)) {
    // Half a key pair is worse than none: the secret alone cannot be handed
    // to anyone to trust, and leaving it invites generating another.
    QFile::remove(secret_path);
    return false;
  }
  return true;
}

}  // namespace GpgFrontend::Module
