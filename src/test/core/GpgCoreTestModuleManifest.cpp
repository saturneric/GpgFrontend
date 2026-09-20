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
#include <array>

#include "GpgFrontendTest.h"
#include "core/module/ModuleDescriptorBuilder.h"
#include "core/module/ModuleManifest.h"
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestModuleManifest.cpp
 * @brief The manifest parser, and the canonical JSON the signature covers.
 *
 * Split out of GpgCoreTestModuleDescriptor.cpp, which had grown to 846 lines
 * and two subjects. These need no package, no archive and no key: they are
 * about whether a string of JSON is accepted, and about the exact bytes a
 * signature is computed over. Keeping them beside a fixture that builds and
 * signs a real archive made both harder to read than either needs to be.
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
      {"events", QJsonArray{"APPLICATION_LOADED"}},
      {"translation_context", "ModuleTest"},
      {"metadata", QJsonObject{{"Name", "Test"}}},
      {"build",
       QJsonObject{{"id", "b"}, {"timestamp", "t"}, {"source_commit", "c"}}},
      {"platform",
       QJsonObject{{"os", "linux"}, {"arch", "x86_64"}, {"qt", "6.6"}}},
      {"entry_native",
       QJsonObject{{"name", "gf_mod_test"},
                   {"verification", QJsonObject{{"mode", "file-sha256"},
                                                {"value", QString(64, 'a')}}},
                   {"size", 4096}}},
      {"resources", QJsonArray{}},
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
  EXPECT_EQ(r.manifest.entry_native.name, "gf_mod_test");
  EXPECT_TRUE(r.manifest.resources.isEmpty());
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
                                    "events",
                                    "translation_context",
                                    "metadata",
                                    "build",
                                    "platform",
                                    "entry_native",
                                    "resources"};
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
  o3["resources"] = QJsonObject{};
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
  o["resources"] =
      QJsonArray{QJsonObject{{"path", "icons/a.svg"}, {"sha256", "ABC"}}};
  EXPECT_FALSE(ParseObject(o).ok);

  // Upper case is refused too: a digest compared as a string needs one
  // spelling, and the verifier produces the lower-case one.
  auto o2 = GoodManifestObject();
  o2["resources"] = QJsonArray{
      QJsonObject{{"path", "icons/a.svg"}, {"sha256", QString(64, 'A')}}};
  EXPECT_FALSE(ParseObject(o2).ok);
}

TEST(ModuleManifestTest, AnEmptyResourceListIsFine) {
  // The inversion of the old rule, and deliberate. `files` had to be non-empty
  // because it covered the module binary, and a manifest covering nothing
  // would have verified trivially. `resources` covers only non-executable
  // members, and carrying none is the normal case: the thing that must not be
  // missing is `entry_native`, which is a separate, required field.
  auto o = GoodManifestObject();
  o["resources"] = QJsonArray{};
  EXPECT_TRUE(ParseObject(o).ok);
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
// The subscription allowlist is only worth signing if it is well formed: the
// runtime matches these against its handler table by exact upper-case id.
TEST(ModuleManifestTest, RejectsAMalformedEventList) {
  auto o = GoodManifestObject();
  o["events"] = "APPLICATION_LOADED";
  EXPECT_FALSE(ParseObject(o).ok) << "a bare string is not an event list";

  o = GoodManifestObject();
  o["events"] = QJsonArray{"APPLICATION_LOADED", 7};
  EXPECT_FALSE(ParseObject(o).ok) << "a non-string event id";

  o = GoodManifestObject();
  o["events"] = QJsonArray{"application_loaded"};
  EXPECT_FALSE(ParseObject(o).ok)
      << "lower case would never match what the host dispatches";

  o = GoodManifestObject();
  o["events"] = QJsonArray{"APPLICATION_LOADED", "APPLICATION_LOADED"};
  EXPECT_FALSE(ParseObject(o).ok) << "a duplicate declaration";

  o = GoodManifestObject();
  o["events"] = QJsonArray{""};
  EXPECT_FALSE(ParseObject(o).ok) << "an empty event id";

  // A module that subscribes to nothing is a legitimate thing to say.
  o = GoodManifestObject();
  o["events"] = QJsonArray{};
  EXPECT_TRUE(ParseObject(o).ok);
}

TEST(ModuleManifestTest, RejectsAnEmptyTranslationContext) {
  auto o = GoodManifestObject();
  o["translation_context"] = "";
  EXPECT_FALSE(ParseObject(o).ok);

  o = GoodManifestObject();
  o["translation_context"] = 7;
  EXPECT_FALSE(ParseObject(o).ok);
}

// A package from before schema 3 carries its executable payload inside itself,
// which this build has no path for at all. Refusing it by version, with the
// reason said out loud, beats refusing it later on a field it does not have.
TEST(ModuleManifestTest, AnOlderSchemaIsRefusedByVersionWithAReason) {
  for (const auto older : {1, 2}) {
    auto o = GoodManifestObject();
    o["schema_version"] = older;

    const auto r = ParseObject(o);
    EXPECT_FALSE(r.ok) << "schema " << older << " was tolerated";
    EXPECT_EQ(r.status, Module::ModuleManifestStatus::kMALFORMED);
    EXPECT_TRUE(r.reason.contains("schema_version")) << r.reason.toStdString();
  }
}

// ------------------------------------------------- entry_native, section 7a

TEST(ModuleManifestTest, RejectsAnEntryNameThatIsNotLogical) {
  // Each of these is a path, or could become one. A logical name cannot
  // express a path at all, which is why the descriptor needs no rule about
  // traversal: there is nothing to traverse with.
  for (const auto* bad : {"../x", "/x", "C:\\x", "a/b", "a.b", "libgf_mod_x",
                          "Gf_Mod_X", "", "9lives"}) {
    auto o = GoodManifestObject();
    auto entry = o["entry_native"].toObject();
    entry["name"] = QString::fromLatin1(bad);
    o["entry_native"] = entry;
    EXPECT_FALSE(ParseObject(o).ok)
        << "\"" << bad << "\" was accepted as a logical native name";
  }
}

TEST(ModuleManifestTest, RejectsAnEntryNameOfSixtyFivePlusCharacters) {
  auto o = GoodManifestObject();
  auto entry = o["entry_native"].toObject();
  entry["name"] = "a" + QString(64, u'b');
  o["entry_native"] = entry;
  EXPECT_FALSE(ParseObject(o).ok);
}

TEST(ModuleManifestTest, TheVerificationModeMustBeTheOneThePlatformMandates) {
  // The module author does not choose this. Every wrong pairing is refused,
  // and a descriptor cannot select a weaker mode by claiming a platform --
  // the platform claim is checked against the host before any of this matters.
  // A LIST, not a map keyed by os. This was a QMap, and two of its four
  // entries were keyed "linux" -- so the second silently replaced the first,
  // the loop ran three times, and `linux` + `apple-binding-id`, the pairing
  // named first above, was never tested at all. A container that dedupes its
  // own test cases is the wrong container for a table of test cases.
  const std::array<std::pair<QString, QString>, 4> wrong{{
      {"linux", "apple-binding-id"},
      {"linux", "pe-authenticode-sha256"},
      {"macos", "file-sha256"},
      {"windows", "file-sha256"},
  }};

  auto checked = 0;
  for (const auto& [os, mode] : wrong) {
    auto o = GoodManifestObject();
    auto platform = o["platform"].toObject();
    platform["os"] = os;
    o["platform"] = platform;

    auto entry = o["entry_native"].toObject();
    auto verification = entry["verification"].toObject();
    verification["mode"] = mode;
    entry["verification"] = verification;
    // size is only legal under file-sha256; drop it so the mode is what fails
    entry.remove("size");
    o["entry_native"] = entry;

    const auto r = ParseObject(o);
    ASSERT_FALSE(r.ok) << os.toStdString() << " accepted "
                       << mode.toStdString();
    // The reason, not just the refusal: every other field in this object is
    // valid, but asserting only `!ok` would be satisfied by any future check
    // that happened to fire earlier.
    EXPECT_TRUE(r.reason.contains("must use"))
        << os.toStdString() << " + " << mode.toStdString()
        << " was refused for a different reason: " << r.reason.toStdString();
    ++checked;
  }

  EXPECT_EQ(checked, wrong.size()) << "the table stopped being fully walked";
}

TEST(ModuleManifestTest, RejectsAnUnknownOrMissingVerificationMode) {
  auto o = GoodManifestObject();
  auto entry = o["entry_native"].toObject();
  auto verification = entry["verification"].toObject();
  verification["mode"] = "sha1-of-something";
  entry["verification"] = verification;
  o["entry_native"] = entry;
  EXPECT_FALSE(ParseObject(o).ok);

  o = GoodManifestObject();
  entry = o["entry_native"].toObject();
  verification = entry["verification"].toObject();
  verification.remove("mode");
  entry["verification"] = verification;
  o["entry_native"] = entry;
  EXPECT_FALSE(ParseObject(o).ok);

  o = GoodManifestObject();
  entry = o["entry_native"].toObject();
  verification = entry["verification"].toObject();
  verification["value"] = "";
  entry["verification"] = verification;
  o["entry_native"] = entry;
  EXPECT_FALSE(ParseObject(o).ok) << "an empty value is not a skipped check";
}

TEST(ModuleManifestTest, SizeIsOnlyAllowedWhereItMeansAnything) {
  // Windows Authenticode signing appends a certificate table and macOS signing
  // rewrites __LINKEDIT; a size recorded under either would be an invariant
  // that legitimately breaks. Refusing it in the schema beats leaving a trap.
  auto o = GoodManifestObject();
  auto platform = o["platform"].toObject();
  platform["os"] = "macos";
  o["platform"] = platform;

  auto entry = o["entry_native"].toObject();
  auto verification = entry["verification"].toObject();
  verification["mode"] = "apple-binding-id";
  entry["verification"] = verification;
  entry["size"] = 4096;
  o["entry_native"] = entry;

  const auto r = ParseObject(o);
  EXPECT_FALSE(r.ok);
  EXPECT_TRUE(r.reason.contains("size")) << r.reason.toStdString();
}

TEST(ModuleManifestTest, SizeIsOptionalAndTypeChecked) {
  auto o = GoodManifestObject();
  auto entry = o["entry_native"].toObject();
  entry.remove("size");
  o["entry_native"] = entry;
  EXPECT_TRUE(ParseObject(o).ok) << "size is an optimisation, not a field";

  for (const auto bad : {QJsonValue("4096"), QJsonValue(-1), QJsonValue(1.5)}) {
    o = GoodManifestObject();
    entry = o["entry_native"].toObject();
    entry["size"] = bad;
    o["entry_native"] = entry;
    EXPECT_FALSE(ParseObject(o).ok);
  }
}

// ------------------------------------------------- architecture spelling

TEST(ModuleManifestTest, OneMachineHasOneCanonicalArchitectureName) {
  // CMake's CMAKE_SYSTEM_PROCESSOR says `aarch64`; Qt's
  // currentCpuArchitecture() says `arm64`. The descriptor was stamped by the
  // first and verified against the second, so every module was refused on ARM
  // Linux -- for a disagreement between two names for one machine.
  EXPECT_EQ(Module::NormalizeManifestArch("aarch64"), "arm64");
  EXPECT_EQ(Module::NormalizeManifestArch("arm64"), "arm64");
  EXPECT_EQ(Module::NormalizeManifestArch("ARM64"), "arm64");

  EXPECT_EQ(Module::NormalizeManifestArch("x86_64"), "x86_64");
  EXPECT_EQ(Module::NormalizeManifestArch("amd64"), "x86_64");
  EXPECT_EQ(Module::NormalizeManifestArch("AMD64"), "x86_64");
  EXPECT_EQ(Module::NormalizeManifestArch("x64"), "x86_64");

  EXPECT_EQ(Module::NormalizeManifestArch("i686"), "i386");
  EXPECT_EQ(Module::NormalizeManifestArch("i386"), "i386");
}

TEST(ModuleManifestTest, AnUnknownArchitectureIsPassedThroughNotMangled) {
  // An architecture nobody anticipated still compares equal to itself, which
  // is all the check needs. One this function rewrote would fail in a way
  // nothing here could explain.
  EXPECT_EQ(Module::NormalizeManifestArch("riscv64"), "riscv64");
  EXPECT_EQ(Module::NormalizeManifestArch("s390x"), "s390x");
  EXPECT_EQ(Module::NormalizeManifestArch("  PPC64LE  "), "ppc64le");
}

TEST(ModuleManifestTest, TheHostArchNameIsAlreadyCanonical) {
  const auto host = Module::ManifestHostArchName();
  EXPECT_EQ(host, Module::NormalizeManifestArch(host))
      << "normalising the host's own name must be a no-op, or the verifier "
         "compares a canonical value against a non-canonical one";
  EXPECT_FALSE(host.isEmpty());
}

TEST(ModuleManifestTest, NormalisationIsIdempotent) {
  for (const auto* name : {"aarch64", "amd64", "i686", "riscv64", "arm64"}) {
    const auto once = Module::NormalizeManifestArch(name);
    EXPECT_EQ(Module::NormalizeManifestArch(once), once) << name;
  }
}

}  // namespace GpgFrontend::Test
