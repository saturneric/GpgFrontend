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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "GpgFrontendTest.h"
#include "core/module/ModuleManifest.h"
#include "core/module/ModulePackageBuilder.h"
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestModuleManifest.cpp
 * @brief The manifest parser, and the canonical JSON the signature covers.
 *
 * Split out of GpgCoreTestModulePackage.cpp, which had grown to 846 lines and
 * two subjects. These need no package, no archive and no key: they are about
 * whether a string of JSON is accepted, and about the exact bytes a signature
 * is computed over. Keeping them beside a fixture that builds and signs a real
 * archive made both harder to read than either needs to be.
 *
 * The strictness here is deliberate and is the opposite of the settings
 * layer's: a missing field, an unknown schema version or a field of the wrong
 * JSON type is a hard failure, because a manifest that parses leniently is a
 * manifest whose claims were never really checked.
 */

namespace GpgFrontend::Test {

namespace {

/// A manifest object that parses, so a test can break exactly one thing.
auto GoodManifestObject() -> QJsonObject {
  return QJsonObject{
      {"schema_version", Module::kModuleManifestSchemaVersion},
      {"id", "com.bktus.gpgfrontend.module.test"},
      {"version", "1.0.0"},
      {"sdk_abi", GF_SDK_ABI_VERSION},
      {"min_host_version", "2.0.0"},
      {"security_epoch", 0},
      {"capabilities", QJsonArray{"gpg"}},
      {"metadata", QJsonObject{{"Name", "Test"}}},
      {"build",
       QJsonObject{{"id", "b"}, {"timestamp", "t"}, {"source_commit", "c"}}},
      {"platform",
       QJsonObject{{"os", "linux"}, {"arch", "x86_64"}, {"qt", "6.6"}}},
      {"files", QJsonArray{QJsonObject{{"path", "bin/module.so"},
                                       {"sha256", QString(64, 'a')}}}},
  };
}

auto ParseObject(const QJsonObject& o) -> Module::ModuleManifestParseResult {
  return Module::ParseModuleManifest(QJsonDocument(o).toJson());
}

}  // namespace

TEST(ModuleManifestTest, RejectsMalformedJson) {
  const auto r = Module::ParseModuleManifest("{not json");
  EXPECT_FALSE(r.ok);
  EXPECT_EQ(r.status, Module::ModuleManifestStatus::kMALFORMED);
}

TEST(ModuleManifestTest, RejectsANonObject) {
  EXPECT_FALSE(Module::ParseModuleManifest("[1,2,3]").ok);
}

TEST(ModuleManifestTest, AcceptsAGoodManifest) {
  const auto r = ParseObject(GoodManifestObject());
  ASSERT_TRUE(r.ok) << r.reason.toStdString();
  EXPECT_EQ(r.manifest.id, "com.bktus.gpgfrontend.module.test");
  EXPECT_EQ(r.manifest.files.size(), 1);
}

TEST(ModuleManifestTest, RejectsAnUnsupportedSchemaVersion) {
  auto o = GoodManifestObject();
  o["schema_version"] = Module::kModuleManifestSchemaVersion + 1;
  const auto r = ParseObject(o);
  EXPECT_FALSE(r.ok);
  EXPECT_EQ(r.status, Module::ModuleManifestStatus::kTOO_NEW);
}

TEST(ModuleManifestTest, RejectsEveryMissingRequiredField) {
  const auto required = QStringList{"schema_version",
                                    "id",
                                    "version",
                                    "sdk_abi",
                                    "min_host_version",
                                    "security_epoch",
                                    "capabilities",
                                    "metadata",
                                    "build",
                                    "platform",
                                    "files"};
  for (const auto& key : required) {
    auto o = GoodManifestObject();
    o.remove(key);
    const auto r = ParseObject(o);
    EXPECT_FALSE(r.ok) << "removing " << key.toStdString() << " was tolerated";
    EXPECT_EQ(r.status, Module::ModuleManifestStatus::kMALFORMED);
  }
}

TEST(ModuleManifestTest, RejectsAWrongTypedField) {
  // The inversion of the settings layer's habit, which would keep its default
  // and say nothing. A mistyped sdk_abi has to be a refusal, not a zero.
  auto o = GoodManifestObject();
  o["sdk_abi"] = "three";
  const auto r = ParseObject(o);
  EXPECT_FALSE(r.ok);
  EXPECT_EQ(r.status, Module::ModuleManifestStatus::kMALFORMED);

  auto o2 = GoodManifestObject();
  o2["capabilities"] = "gpg";
  EXPECT_FALSE(ParseObject(o2).ok);

  auto o3 = GoodManifestObject();
  o3["files"] = QJsonObject{};
  EXPECT_FALSE(ParseObject(o3).ok);

  auto o4 = GoodManifestObject();
  o4["metadata"] = QJsonObject{{"Name", 7}};
  EXPECT_FALSE(ParseObject(o4).ok);
}

TEST(ModuleManifestTest, RejectsAFractionalVersionNumber) {
  auto o = GoodManifestObject();
  o["sdk_abi"] = 3.5;
  EXPECT_FALSE(ParseObject(o).ok);
}

TEST(ModuleManifestTest, RejectsABadDigest) {
  auto o = GoodManifestObject();
  o["files"] =
      QJsonArray{QJsonObject{{"path", "bin/module.so"}, {"sha256", "ABC"}}};
  EXPECT_FALSE(ParseObject(o).ok);

  // Upper case is refused too: a digest compared as a string needs one
  // spelling, and the verifier produces the lower-case one.
  auto o2 = GoodManifestObject();
  o2["files"] = QJsonArray{
      QJsonObject{{"path", "bin/module.so"}, {"sha256", QString(64, 'A')}}};
  EXPECT_FALSE(ParseObject(o2).ok);
}

TEST(ModuleManifestTest, RejectsAnEmptyFileList) {
  auto o = GoodManifestObject();
  o["files"] = QJsonArray{};
  EXPECT_FALSE(ParseObject(o).ok);
}

TEST(ModuleManifestTest, ToleratesAnUnknownField) {
  // Additive evolution inside a supported schema version. Nothing round-trips
  // it, because this manifest is never re-serialised.
  auto o = GoodManifestObject();
  o["something_from_the_future"] = "hello";
  EXPECT_TRUE(ParseObject(o).ok);
}

TEST(ModuleManifestTest, SecurityEpochIsTypeCheckedAndOtherwiseIgnored) {
  auto bad = GoodManifestObject();
  bad["security_epoch"] = "zero";
  EXPECT_FALSE(ParseObject(bad).ok);

  // A high one is carried through with no policy applied: nothing compares it
  // against a stored high-water mark, because nothing stores one yet.
  auto high = GoodManifestObject();
  high["security_epoch"] = 42;
  const auto r = ParseObject(high);
  ASSERT_TRUE(r.ok);
  EXPECT_EQ(r.manifest.security_epoch, 42);
}

// ----------------------------------------------------------------------- JCS

TEST(ModuleManifestTest, CanonicalJsonOrdersMembersAndOmitsSpace) {
  QByteArray out;
  ASSERT_TRUE(
      Module::CanonicalJson(QJsonObject{{"b", 2}, {"a", 1}, {"c", "x"}}, out));
  EXPECT_EQ(out, R"({"a":1,"b":2,"c":"x"})");
}

TEST(ModuleManifestTest, CanonicalJsonIsStableAcrossRuns) {
  QByteArray a;
  QByteArray b;
  ASSERT_TRUE(Module::CanonicalJson(GoodManifestObject(), a));
  ASSERT_TRUE(Module::CanonicalJson(GoodManifestObject(), b));
  EXPECT_EQ(a, b);
}

TEST(ModuleManifestTest, CanonicalJsonEscapesControlCharacters) {
  QByteArray out;
  const QString value =
      QString("a") + QChar(0x09) + "b" + QChar(0x0A) + "c" + QChar(0x01);
  ASSERT_TRUE(Module::CanonicalJson(QJsonObject{{"k", value}}, out));
  // Spelled by concatenation rather than as one raw string: gcc converts a
  // universal character name inside a raw string literal, so writing the
  // expected escape there produces the character it is supposed to describe.
  EXPECT_EQ(out, QByteArray(R"({"k":"a\tb\nc\u)") + "0001" + R"("})");
}

TEST(ModuleManifestTest, CanonicalJsonRefusesANonIntegralNumber) {
  QByteArray out;
  EXPECT_FALSE(Module::CanonicalJson(QJsonObject{{"k", 1.5}}, out));
}
}  // namespace GpgFrontend::Test
