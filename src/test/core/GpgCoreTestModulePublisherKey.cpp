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

#include <gtest/gtest.h>
#include <sodium.h>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "GpgFrontendTest.h"
#include "core/ModuleTestPackages.h"
#include "core/module/ModulePublisherKey.h"
#include "core/module/ModuleTrustRoot.h"

/**
 * @file GpgCoreTestModulePublisherKey.cpp
 * @brief A publisher's identity, and the files it lives in.
 *
 * The format exists mostly for what it refuses: a raw 32-byte seed -- the
 * shape of this build's own `module-build.seed` -- must never be readable as
 * a publisher key.
 */

namespace GpgFrontend::Test {

namespace {

auto WriteFile(const QString& path, const QByteArray& bytes) -> bool {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) return false;
  return file.write(bytes) == bytes.size();
}

auto ReadFile(const QString& path) -> QByteArray {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  return file.readAll();
}

}  // namespace

TEST(ModulePublisherKeyTest, AGeneratedKeyRoundTrips) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto secret = dir.path() + "/publisher.key";
  const auto pub = secret + ".pub";

  QByteArray generated;
  QString reason;
  ASSERT_TRUE(
      Module::GenerateModulePublisherKeyFiles(secret, pub, generated, reason))
      << reason.toStdString();
  ASSERT_EQ(generated.size(), crypto_sign_PUBLICKEYBYTES);
  EXPECT_NE(generated, Module::ModuleBuildPublicKey());

  QByteArray seed;
  ASSERT_TRUE(Module::ReadModulePublisherSecretKey(secret, seed, reason))
      << reason.toStdString();
  EXPECT_EQ(Module::ModulePublisherPublicKeyFromSeed(seed), generated);

  // Either file names the same identity.
  QByteArray from_public;
  QByteArray from_secret;
  ASSERT_TRUE(Module::ReadModulePublisherPublicKey(pub, from_public, reason));
  ASSERT_TRUE(
      Module::ReadModulePublisherPublicKey(secret, from_secret, reason));
  EXPECT_EQ(from_public, generated);
  EXPECT_EQ(from_secret, generated);

  EXPECT_TRUE(ReadFile(secret).startsWith(
      QByteArray(Module::kModulePublisherSecretKeyHeader) + '\n'));
  EXPECT_TRUE(ReadFile(pub).startsWith(
      QByteArray(Module::kModulePublisherPublicKeyHeader) + '\n'));
}

#ifndef Q_OS_WIN
TEST(ModulePublisherKeyTest, TheSecretFileIsOwnerOnly) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto secret = dir.path() + "/publisher.key";

  QByteArray key;
  QString reason;
  ASSERT_TRUE(Module::GenerateModulePublisherKeyFiles(secret, secret + ".pub",
                                                      key, reason));

  const auto permissions = QFileInfo(secret).permissions();
  EXPECT_FALSE(permissions & (QFile::ReadGroup | QFile::WriteGroup |
                              QFile::ReadOther | QFile::WriteOther));
  EXPECT_FALSE(Module::IsModulePublisherSecretKeyExposed(secret));

  ASSERT_TRUE(QFile::setPermissions(
      secret, permissions | QFile::ReadGroup | QFile::ReadOther));
  EXPECT_TRUE(Module::IsModulePublisherSecretKeyExposed(secret));
}
#endif

TEST(ModulePublisherKeyTest, AKeyIsNeverReplaced) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto secret = dir.path() + "/publisher.key";

  QByteArray first;
  QString reason;
  ASSERT_TRUE(Module::GenerateModulePublisherKeyFiles(secret, secret + ".pub",
                                                      first, reason));
  const auto before = ReadFile(secret);

  QByteArray second;
  EXPECT_FALSE(Module::GenerateModulePublisherKeyFiles(secret, secret + ".pub",
                                                       second, reason));
  EXPECT_TRUE(reason.contains("never replaced")) << reason.toStdString();
  EXPECT_EQ(ReadFile(secret), before);
}

TEST(ModulePublisherKeyTest, ARawSeedIsRefused) {
  // What `module-build.seed` is: 32 raw bytes. The one mistake the typed
  // format exists to make impossible.
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto raw = dir.path() + "/module-build.seed";
  ASSERT_TRUE(WriteFile(raw, QByteArray(32, '\x42')));

  QByteArray seed;
  QString reason;
  EXPECT_FALSE(Module::ReadModulePublisherSecretKey(raw, seed, reason));
  EXPECT_TRUE(seed.isEmpty());
  EXPECT_TRUE(reason.contains("never a publisher identity"))
      << reason.toStdString();

  // And the real one, when this build has it.
  const auto build_seed = BuildSigningSeed();
  if (!build_seed.isEmpty()) {
    const auto copied = dir.path() + "/real.seed";
    ASSERT_TRUE(WriteFile(copied, build_seed));
    EXPECT_FALSE(Module::ReadModulePublisherSecretKey(copied, seed, reason));
  }
}

TEST(ModulePublisherKeyTest, MalformedFilesAreRefused) {
  const QByteArray header(Module::kModulePublisherSecretKeyHeader);
  const QByteArray good_hex(64, 'a');

  const QList<QByteArray> bad{
      {},
      good_hex,                                      // no header
      "gf-module-publisher-secret-v2\n" + good_hex,  // wrong version
      QByteArray(Module::kModulePublisherPublicKeyHeader) + '\n' +
          good_hex,                         // the other kind
      header + '\n' + QByteArray(63, 'a'),  // short
      header + '\n' + QByteArray(66, 'a'),  // long
      header + '\n' + QByteArray(64, 'A'),  // upper case
      header + '\n' + QByteArray(64, 'g'),  // not hex
      header + '\n' + good_hex + "\n\n",    // trailing junk
      header + "\r\n" + good_hex,           // header with CR
  };
  for (const auto& contents : bad) {
    QByteArray seed;
    QString reason;
    EXPECT_FALSE(Module::ParseModulePublisherSecretKey(contents, seed, reason))
        << contents.toStdString();
    EXPECT_FALSE(reason.isEmpty());
  }

  QByteArray seed;
  QString reason;
  EXPECT_TRUE(Module::ParseModulePublisherSecretKey(header + '\n' + good_hex,
                                                    seed, reason));
  EXPECT_TRUE(Module::ParseModulePublisherSecretKey(
      header + '\n' + good_hex + "\r\n", seed, reason));
  EXPECT_EQ(seed, QByteArray(32, '\xaa'));
}

TEST(ModulePublisherKeyTest, TheCanonicalTextIsWhatTheTrustStoreRecords) {
  const QByteArray key(32, '\x5c');
  EXPECT_EQ(Module::ModulePublisherKeyText(key),
            QString::fromLatin1(key.toHex()));
  EXPECT_TRUE(Module::ModulePublisherKeyText(QByteArray(31, '\x5c')).isEmpty());

  auto grouped = Module::ModulePublisherKeyFingerprint(key);
  grouped.remove(' ');
  EXPECT_EQ(grouped.toLower(), Module::ModulePublisherKeyText(key));
}

}  // namespace GpgFrontend::Test
