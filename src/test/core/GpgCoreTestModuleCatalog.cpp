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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "GpgFrontendTest.h"
#include "core/module/ModuleCatalog.h"
#include "core/module/ModulePackageBuilder.h"

/**
 * @file GpgCoreTestModuleCatalog.cpp
 * @brief The signed catalog format, as executable rules.
 *
 * The catalog is NOT wired into loading: nothing in the application reads one
 * and no root key is embedded anywhere. These tests pin the format and its
 * reader so that the trust decision -- whose key, published where -- can be
 * taken later without the format being designed under pressure at the time.
 *
 * Note there is deliberately no function under test that WRITES a catalog.
 * Signing one needs the root secret, which is a publishing operation; the
 * tests sign with libsodium directly, which is the point being made.
 */

namespace GpgFrontend::Test {

namespace {

/// A root key pair, minted per test. There is no such thing as a test root
/// key that means anything, which is exactly why none is embedded.
struct RootKey {
  QByteArray public_key;
  QByteArray secret_key;
};

auto MakeRootKey() -> RootKey {
  RootKey key;
  key.public_key.resize(crypto_sign_PUBLICKEYBYTES);
  key.secret_key.resize(crypto_sign_SECRETKEYBYTES);
  crypto_sign_keypair(reinterpret_cast<unsigned char*>(key.public_key.data()),
                      reinterpret_cast<unsigned char*>(key.secret_key.data()));
  return key;
}

auto Sign(const QByteArray& bytes, const RootKey& key) -> QByteArray {
  QByteArray signature(crypto_sign_BYTES, Qt::Uninitialized);
  unsigned long long length = 0;
  crypto_sign_detached(
      reinterpret_cast<unsigned char*>(signature.data()), &length,
      reinterpret_cast<const unsigned char*>(bytes.constData()),
      static_cast<unsigned long long>(bytes.size()),
      reinterpret_cast<const unsigned char*>(key.secret_key.constData()));
  signature.resize(static_cast<qsizetype>(length));
  return signature;
}

auto Entry(const QString& id, const QString& version, char digest_fill,
           char key_fill, int epoch = 0, bool revoked = false) -> QJsonObject {
  return QJsonObject{
      {"module_id", id},
      {"version", version},
      {"package_sha256", QString(64, QChar(digest_fill))},
      {"build_public_key",
       QString(crypto_sign_PUBLICKEYBYTES * 2, QChar(key_fill))},
      {"security_epoch", epoch},
      {"revoked", revoked},
  };
}

auto GoodCatalogObject() -> QJsonObject {
  return QJsonObject{
      {"schema_version", Module::kModuleCatalogSchemaVersion},
      {"issued_at", "2026-09-15T00:00:00Z"},
      {"entries", QJsonArray{Entry("com.bktus.gpgfrontend.module.email",
                                   "2.0.0", 'a', 'b'),
                             Entry("com.bktus.gpgfrontend.module.email",
                                   "1.0.0", 'c', 'd', 0, true)}},
  };
}

auto Serialize(const QJsonObject& o) -> QByteArray {
  QByteArray bytes;
  Module::CanonicalJson(o, bytes);
  return bytes;
}

}  // namespace

TEST(ModuleCatalogTest, AProperlySignedCatalogReads) {
  const auto key = MakeRootKey();
  const auto bytes = Serialize(GoodCatalogObject());

  const auto result =
      Module::VerifyModuleCatalog(bytes, Sign(bytes, key), key.public_key);
  ASSERT_TRUE(result.ok) << result.reason.toStdString();
  EXPECT_EQ(result.catalog.entries.size(), 2);
  EXPECT_EQ(result.catalog.issued_at, "2026-09-15T00:00:00Z");
}

TEST(ModuleCatalogTest, ADifferentRootKeyIsRefused) {
  const auto key = MakeRootKey();
  const auto other = MakeRootKey();
  const auto bytes = Serialize(GoodCatalogObject());

  const auto result =
      Module::VerifyModuleCatalog(bytes, Sign(bytes, key), other.public_key);
  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.status, Module::ModuleCatalogStatus::kBAD_SIGNATURE);
}

TEST(ModuleCatalogTest, AnAlteredCatalogIsRefused) {
  const auto key = MakeRootKey();
  const auto bytes = Serialize(GoodCatalogObject());
  const auto signature = Sign(bytes, key);

  auto altered = bytes;
  altered.replace("2.0.0", "9.9.9");
  ASSERT_NE(altered, bytes);

  const auto result =
      Module::VerifyModuleCatalog(altered, signature, key.public_key);
  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.status, Module::ModuleCatalogStatus::kBAD_SIGNATURE);
}

TEST(ModuleCatalogTest, WithoutARootKeyNothingIsRead) {
  // The property that stops this being an unauthenticated source of policy: a
  // catalog with no key to check it against is not read at all, rather than
  // parsed and then distrusted.
  const auto key = MakeRootKey();
  const auto bytes = Serialize(GoodCatalogObject());
  const auto signature = Sign(bytes, key);

  for (const auto& no_key : {QByteArray(), QByteArray("short")}) {
    const auto result = Module::VerifyModuleCatalog(bytes, signature, no_key);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.status, Module::ModuleCatalogStatus::kBAD_SIGNATURE);
    EXPECT_TRUE(result.catalog.entries.isEmpty());
  }
}

TEST(ModuleCatalogTest, AWrongSizedSignatureIsRefused) {
  const auto key = MakeRootKey();
  const auto bytes = Serialize(GoodCatalogObject());

  const auto result =
      Module::VerifyModuleCatalog(bytes, QByteArray("nope"), key.public_key);
  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.status, Module::ModuleCatalogStatus::kBAD_SIGNATURE);
}

TEST(ModuleCatalogTest, NothingIsParsedBeforeTheSignatureIsChecked) {
  // Bytes that are not JSON at all still come back as a signature failure
  // rather than a parse failure, which is how you can tell the ordering is
  // what it claims to be.
  const auto key = MakeRootKey();
  const QByteArray garbage = "{not json at all";

  const auto result = Module::VerifyModuleCatalog(
      garbage, Sign(Serialize(GoodCatalogObject()), key), key.public_key);
  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.status, Module::ModuleCatalogStatus::kBAD_SIGNATURE);
}

namespace {

/// Sign whatever object is given, then read it back with the right key, so a
/// schema test is only ever about the schema.
auto ReadSigned(const QJsonObject& o) -> Module::ModuleCatalogVerification {
  const auto key = MakeRootKey();
  const auto bytes = Serialize(o);
  return Module::VerifyModuleCatalog(bytes, Sign(bytes, key), key.public_key);
}

}  // namespace

TEST(ModuleCatalogTest, RejectsAnUnsupportedSchemaVersion) {
  auto o = GoodCatalogObject();
  o["schema_version"] = Module::kModuleCatalogSchemaVersion + 1;
  const auto result = ReadSigned(o);
  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.status, Module::ModuleCatalogStatus::kTOO_NEW);
}

TEST(ModuleCatalogTest, RejectsEveryMissingRequiredField) {
  for (const auto& key :
       QStringList{"schema_version", "issued_at", "entries"}) {
    auto o = GoodCatalogObject();
    o.remove(key);
    EXPECT_FALSE(ReadSigned(o).ok)
        << "removing " << key.toStdString() << " was tolerated";
  }

  for (const auto& key :
       QStringList{"module_id", "version", "package_sha256", "build_public_key",
                   "security_epoch", "revoked"}) {
    auto entry = Entry("com.example.m", "1.0.0", 'a', 'b');
    entry.remove(key);
    auto o = GoodCatalogObject();
    o["entries"] = QJsonArray{entry};
    EXPECT_FALSE(ReadSigned(o).ok)
        << "removing entries." << key.toStdString() << " was tolerated";
  }
}

TEST(ModuleCatalogTest, RejectsAWrongTypedField) {
  // Same inversion as the manifest: a catalog that quietly reads as "epoch 0,
  // not revoked" because a field was mistyped is worse than no catalog.
  auto entry = Entry("com.example.m", "1.0.0", 'a', 'b');
  entry["revoked"] = "yes";
  auto o = GoodCatalogObject();
  o["entries"] = QJsonArray{entry};
  EXPECT_FALSE(ReadSigned(o).ok);

  auto entry2 = Entry("com.example.m", "1.0.0", 'a', 'b');
  entry2["security_epoch"] = "one";
  auto o2 = GoodCatalogObject();
  o2["entries"] = QJsonArray{entry2};
  EXPECT_FALSE(ReadSigned(o2).ok);

  auto o3 = GoodCatalogObject();
  o3["entries"] = QJsonObject{};
  EXPECT_FALSE(ReadSigned(o3).ok);
}

TEST(ModuleCatalogTest, RejectsMalformedDigestsAndKeys) {
  auto o = GoodCatalogObject();
  o["entries"] = QJsonArray{Entry("com.example.m", "1.0.0", 'z', 'b')};
  EXPECT_FALSE(ReadSigned(o).ok) << "a non-hex digest was tolerated";

  auto o2 = GoodCatalogObject();
  o2["entries"] = QJsonArray{Entry("com.example.m", "1.0.0", 'a', 'z')};
  EXPECT_FALSE(ReadSigned(o2).ok) << "a non-hex build key was tolerated";

  auto entry = Entry("com.example.m", "1.0.0", 'a', 'b');
  entry["package_sha256"] = QString(63, 'a');
  auto o3 = GoodCatalogObject();
  o3["entries"] = QJsonArray{entry};
  EXPECT_FALSE(ReadSigned(o3).ok) << "a short digest was tolerated";
}

TEST(ModuleCatalogTest, RejectsTwoEntriesForOnePackage) {
  // A catalog saying two things about the same bytes is the split view the
  // package format refuses inside an archive, arriving one level up.
  auto o = GoodCatalogObject();
  o["entries"] = QJsonArray{Entry("com.example.m", "1.0.0", 'a', 'b'),
                            Entry("com.example.m", "1.0.1", 'a', 'c')};
  EXPECT_FALSE(ReadSigned(o).ok);
}

TEST(ModuleCatalogTest, AnEmptyCatalogIsValid) {
  // A catalog that vouches for nothing is a meaningful statement -- it is what
  // a publisher with nothing published would sign -- and must not be confused
  // with a broken one.
  auto o = GoodCatalogObject();
  o["entries"] = QJsonArray{};
  const auto result = ReadSigned(o);
  ASSERT_TRUE(result.ok) << result.reason.toStdString();
  EXPECT_TRUE(result.catalog.entries.isEmpty());
}

TEST(ModuleCatalogTest, ToleratesAnUnknownField) {
  auto o = GoodCatalogObject();
  o["published_at"] = "somewhere";
  EXPECT_TRUE(ReadSigned(o).ok);
}

// ------------------------------------------------------------------- lookup

TEST(ModuleCatalogTest, LooksUpByPackageDigestAndByModule) {
  const auto key = MakeRootKey();
  const auto bytes = Serialize(GoodCatalogObject());
  const auto result =
      Module::VerifyModuleCatalog(bytes, Sign(bytes, key), key.public_key);
  ASSERT_TRUE(result.ok);

  // Keyed by what the bytes are, not by what someone called them.
  const auto found = result.catalog.FindByDigest(QString(64, 'a'));
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->version, "2.0.0");
  EXPECT_FALSE(found->revoked);

  const auto revoked = result.catalog.FindByDigest(QString(64, 'c'));
  ASSERT_TRUE(revoked.has_value());
  EXPECT_TRUE(revoked->revoked);

  EXPECT_FALSE(result.catalog.FindByDigest(QString(64, 'f')).has_value());

  EXPECT_EQ(
      result.catalog.EntriesFor("com.bktus.gpgfrontend.module.email").size(),
      2);
  EXPECT_TRUE(result.catalog.EntriesFor("com.example.absent").isEmpty());
}

}  // namespace GpgFrontend::Test
