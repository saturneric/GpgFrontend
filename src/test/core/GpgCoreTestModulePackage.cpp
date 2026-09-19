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

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QTemporaryDir>
#include <thread>

#include "GpgFrontendTest.h"
#include "core/ModuleTestPackages.h"
#include "core/function/ArchiveFileOperator.h"
#include "core/module/ModuleImageMapping.h"
#include "core/module/ModuleManifest.h"
#include "core/module/ModulePackageBuilder.h"
#include "core/module/ModulePackageVerifier.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/BuildInfoUtils.h"
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestModulePackage.cpp
 * @brief The integrity claims of `*.gfmodule`, as executable rules.
 *
 * Every negative case is one mutation away from a package that is known to
 * verify, which is the only way to be sure a refusal is about the mutation and
 * not about the fixture.
 */

namespace GpgFrontend::Test {

namespace {

/// A spec that verifies as-is on this machine. Tests mutate one thing.
auto GoodSpec(const QString& dir, const QString& payload_path)
    -> Module::ModulePackageBuildSpec {
  Module::ModulePackageBuildSpec spec;
  spec.module_id = "com.bktus.gpgfrontend.module.test";
  spec.version = "1.0.0";
  spec.sdk_abi = GF_SDK_ABI_VERSION;
  spec.min_host_version = "2.0.0";
  spec.capabilities = QStringList{"gpg", "ui"};
  spec.metadata = {{"Name", "Test Module"},
                   {"Description", "a module that exists to be packaged"},
                   {"Author", "Saturneric"}};
  spec.build_id = "test-build";
  spec.build_timestamp = "2026-09-15T00:00:00Z";
  spec.build_source_commit = "0000000000000000000000000000000000000000";
  spec.platform_os = Module::ManifestHostOsName();
  spec.platform_arch = QSysInfo::currentCpuArchitecture();
  spec.platform_qt = QT_VERSION_STR;
  spec.files = {{"bin/module.so", payload_path, {}},
                {"resources/note.txt", {}, QByteArray("a resource")}};
  spec.output_path = dir + "/test.gfmodule";
  return spec;
}

/// Read one member out of a package, without extracting anything.
auto ReadMember(const QString& package, const QString& member) -> QByteArray {
  QTemporaryDir nowhere;
  if (!nowhere.isValid()) return {};
  QByteArray found;
  ArchiveFileOperator::ExtractArchiveFromFileSync(
      package, nowhere.path(), ArchiveExtractPolicy::Permissive(),
      [](const QString&) { return true; },
      [&](const QString& path, const GFBuffer& bytes) {
        if (path == member) found = bytes.ConvertToQByteArray();
        return true;
      });
  return found;
}

/// List every member of a package.
auto ListMembers(const QString& package) -> QStringList {
  QTemporaryDir nowhere;
  if (!nowhere.isValid()) return {};
  QStringList members;
  ArchiveFileOperator::ExtractArchiveFromFileSync(
      package, nowhere.path(), ArchiveExtractPolicy::Permissive(),
      [](const QString&) { return true; },
      [&](const QString& path, const GFBuffer&) {
        members.append(path);
        return true;
      });
  return members;
}

/// Rewrite an existing package, leaving everything it does not touch alone.
///
/// Rebuilding through the builder would re-sign, which is the opposite of what
/// a tampering test needs. So this reads the members out, applies the one
/// change, and writes a new archive with no signing step at all.
auto RepackWith(const QString& source, const QString& destination,
                const QMap<QString, QByteArray>& replacements,
                const QStringList& removals = {},
                const QVector<QPair<QString, QByteArray>>& additions = {})
    -> bool {
  QTemporaryDir nowhere;
  if (!nowhere.isValid()) return false;

  QVector<QPair<QString, QByteArray>> members;
  const auto error = ArchiveFileOperator::ExtractArchiveFromFileSync(
      source, nowhere.path(), ArchiveExtractPolicy::Permissive(),
      [](const QString&) { return true; },
      [&](const QString& path, const GFBuffer& bytes) {
        if (removals.contains(path)) return true;
        const auto it = replacements.constFind(path);
        members.append({path, it == replacements.constEnd()
                                  ? bytes.ConvertToQByteArray()
                                  : *it});
        return true;
      });
  if (error != 0) return false;

  members.append(additions);

  QFile out(destination);
  if (!out.open(QIODevice::WriteOnly)) return false;

  auto exchanger = CreateStandardGFDataExchanger();
  GFError archive_error = 0;
  std::thread producer([&]() {
    qsizetype index = 0;
    archive_error = ArchiveFileOperator::NewArchiveFromMembersSync(
        [&](ArchiveMemberEntry& entry) {
          if (index >= members.size()) return false;
          const auto& m = members.at(index++);
          entry.relative_path = m.first;
          entry.bytes = GFBuffer(m.second);
          return true;
        },
        exchanger, ArchiveCompression::kNONE, ArchiveFormat::kZIP);
  });

  std::array<std::byte, 64 * 1024> chunk{};
  while (true) {
    const auto n = exchanger->Read(chunk.data(), chunk.size());
    if (n <= 0) break;
    out.write(reinterpret_cast<const char*>(chunk.data()), n);
  }
  producer.join();
  out.close();
  return archive_error == 0;
}

/// A temporary directory holding one package that is known to verify.
class ModulePackageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(dir_.isValid());
    payload_ = dir_.path() + "/payload.bin";
    QFile f(payload_);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(4096, 'm'));
    f.close();

    spec_ = GoodSpec(dir_.path(), payload_);
    const auto built = Module::BuildModulePackage(spec_);
    ASSERT_TRUE(built.ok) << built.reason.toStdString();
    manifest_bytes_ = built.manifest_bytes;
    public_key_ = built.build_public_key;
  }

  [[nodiscard]] auto Package() const -> QString { return spec_.output_path; }
  [[nodiscard]] auto Path(const QString& name) const -> QString {
    return dir_.path() + "/" + name;
  }

  QTemporaryDir dir_;
  QString payload_;
  Module::ModulePackageBuildSpec spec_;
  QByteArray manifest_bytes_;
  QByteArray public_key_;
};

}  // namespace

// --------------------------------------------------------------- happy path

TEST_F(ModulePackageTest, AValidPackageVerifies) {
  const auto v = Module::VerifyModulePackage(Package());
  ASSERT_TRUE(v.ok) << v.reason.toStdString();
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kOK);
  EXPECT_EQ(v.manifest.id, spec_.module_id);
  EXPECT_EQ(v.manifest.version, "1.0.0");
  EXPECT_EQ(v.manifest.sdk_abi, GF_SDK_ABI_VERSION);
  EXPECT_EQ(v.manifest.capabilities, (QStringList{"gpg", "ui"}));
  EXPECT_EQ(v.manifest.metadata.value("Name"), "Test Module");
  EXPECT_EQ(v.manifest.files.size(), 2);
  EXPECT_EQ(v.build_public_key, public_key_);

  // The verifier located the sole bin/ entry while checking it, and publishes
  // its signed digest. Everything downstream consumes this rather than walking
  // manifest.files again -- so it must be the manifest's own value, not merely
  // some 64-character string.
  QString declared;
  for (const auto& f : v.manifest.files) {
    if (f.path.startsWith(Module::kModulePackageBinaryDir)) declared = f.sha256;
  }
  ASSERT_EQ(declared.size(), 64);
  EXPECT_EQ(v.library_sha256, declared);
}

TEST_F(ModulePackageTest, TheSignedBytesAreTheStoredBytes) {
  // The signature covers manifest.json as stored, so what the builder reports
  // signing has to be byte-identical to what the package carries. If these
  // ever diverge, every verification still passes and the format has quietly
  // acquired a canonicalisation step on the reading side that nobody wrote.
  EXPECT_EQ(ReadMember(Package(), Module::kModulePackageManifestPath),
            manifest_bytes_);
}

TEST_F(ModulePackageTest, NothingIsWrittenWhileVerifying) {
  // The safety property, asserted rather than assumed: verification reads a
  // package and leaves nothing behind that a later step could execute.
  const auto before =
      QDir(dir_.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
  ASSERT_TRUE(Module::VerifyModulePackage(Package()).ok);
  EXPECT_EQ(
      QDir(dir_.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot),
      before);
}

// ---------------------------------------------------------------- signature

TEST_F(ModulePackageTest, AModifiedManifestFails) {
  auto altered = manifest_bytes_;
  altered.replace("1.0.0", "9.9.9");
  ASSERT_NE(altered, manifest_bytes_);

  const auto out = Path("tampered.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out,
                         {{Module::kModulePackageManifestPath, altered}}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kBAD_SIGNATURE);
}

TEST_F(ModulePackageTest, AModifiedSignatureFails) {
  auto signature = ReadMember(Package(), Module::kModulePackageSignaturePath);
  ASSERT_EQ(signature.size(), 64);
  signature[0] = static_cast<char>(signature[0] ^ 0xFF);

  const auto out = Path("badsig.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out,
                         {{Module::kModulePackageSignaturePath, signature}}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kBAD_SIGNATURE);
}

TEST_F(ModulePackageTest, AMismatchedBuildKeyFails) {
  // A second build's key: a perfectly valid Ed25519 public key, and not the
  // one that signed this manifest.
  auto other = spec_;
  other.output_path = Path("other.gfmodule");
  const auto second = Module::BuildModulePackage(other);
  ASSERT_TRUE(second.ok);
  ASSERT_NE(second.build_public_key, public_key_);

  const auto out = Path("wrongkey.gfmodule");
  ASSERT_TRUE(RepackWith(
      Package(), out,
      {{Module::kModulePackageBuildKeyPath, second.build_public_key}}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kBAD_SIGNATURE);
}

TEST_F(ModulePackageTest, AnUnexpectedBuildKeyFailsWhenOneIsExpected) {
  // The seam publisher trust will use. With no expected key a package verifies
  // on its own terms; with one that does not match, it does not. That is the
  // whole of what a catalog would have to change.
  ASSERT_TRUE(Module::VerifyModulePackage(Package()).ok);

  QByteArray wrong(public_key_);
  wrong[0] = static_cast<char>(wrong[0] ^ 0xFF);

  const auto v = Module::VerifyModulePackage(Package(), wrong);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kBAD_SIGNATURE);

  EXPECT_TRUE(Module::VerifyModulePackage(Package(), public_key_).ok);
}

// ----------------------------------------------------------- file integrity

TEST_F(ModulePackageTest, AModifiedBinaryFails) {
  const auto out = Path("badbin.gfmodule");
  ASSERT_TRUE(
      RepackWith(Package(), out, {{"bin/module.so", QByteArray(4096, 'x')}}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kFILE_DIGEST_MISMATCH);
}

TEST_F(ModulePackageTest, AModifiedResourceFails) {
  const auto out = Path("badres.gfmodule");
  ASSERT_TRUE(RepackWith(
      Package(), out, {{"resources/note.txt", QByteArray("not a resource")}}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kFILE_DIGEST_MISMATCH);
}

TEST_F(ModulePackageTest, AMissingDeclaredFileFails) {
  const auto out = Path("missing.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out, {}, {"resources/note.txt"}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kMISSING_DECLARED_FILE);
}

TEST_F(ModulePackageTest, AnUndeclaredExtraFileFails) {
  // The appended-payload case: everything the manifest covers is intact, and
  // the package carries one more thing the signature says nothing about.
  const auto out = Path("extra.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out, {}, {},
                         {{"bin/extra.so", QByteArray("payload")}}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kUNDECLARED_FILE);
}

// -------------------------------------------------------------- meta members

TEST_F(ModulePackageTest, ADuplicateManifestFails) {
  // Two members under one name: one reader takes the first and another takes
  // the last, and they disagree about what was signed.
  const auto out = Path("dupmanifest.gfmodule");
  ASSERT_TRUE(
      RepackWith(Package(), out, {}, {},
                 {{Module::kModulePackageManifestPath, manifest_bytes_}}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kNOT_A_PACKAGE);
}

TEST_F(ModulePackageTest, AMissingSignatureFails) {
  const auto out = Path("nosig.gfmodule");
  ASSERT_TRUE(
      RepackWith(Package(), out, {}, {Module::kModulePackageSignaturePath}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kMALFORMED);
}

TEST_F(ModulePackageTest, ACaseCollidingEntryFails) {
  // `BIN/module.so` and `bin/module.so` are two files here and one file on
  // macOS or Windows, so the package extracts to a different tree depending on
  // who unpacks it.
  const auto out = Path("casecollide.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out, {}, {},
                         {{"BIN/module.so", QByteArray("payload")}}));

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kNOT_A_PACKAGE);
}

// ------------------------------------------------------------------- policy

TEST_F(ModulePackageTest, AWrongPlatformIsRejected) {
  auto spec = spec_;
  spec.platform_arch = "pdp11";
  spec.output_path = Path("wrongarch.gfmodule");
  ASSERT_TRUE(Module::BuildModulePackage(spec).ok);

  const auto v = Module::VerifyModulePackage(spec.output_path);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kWRONG_PLATFORM);
}

TEST_F(ModulePackageTest, AnIncompatibleAbiIsRejectedInBothDirections) {
  auto too_old = spec_;
  too_old.sdk_abi = GF_SDK_ABI_MIN_SUPPORTED - 1;
  too_old.output_path = Path("tooold.gfmodule");
  ASSERT_TRUE(Module::BuildModulePackage(too_old).ok);
  EXPECT_EQ(Module::VerifyModulePackage(too_old.output_path).status,
            Module::ModulePackageStatus::kINCOMPATIBLE_ABI);

  auto too_new = spec_;
  too_new.sdk_abi = GF_SDK_ABI_VERSION + 1;
  too_new.output_path = Path("toonew.gfmodule");
  ASSERT_TRUE(Module::BuildModulePackage(too_new).ok);
  EXPECT_EQ(Module::VerifyModulePackage(too_new.output_path).status,
            Module::ModulePackageStatus::kINCOMPATIBLE_ABI);
}

TEST_F(ModulePackageTest, AHostTooOldIsRejected) {
  auto spec = spec_;
  spec.min_host_version = "99.0.0";
  spec.output_path = Path("needsnewer.gfmodule");
  ASSERT_TRUE(Module::BuildModulePackage(spec).ok);
  EXPECT_EQ(Module::VerifyModulePackage(spec.output_path).status,
            Module::ModulePackageStatus::kINCOMPATIBLE_ABI);
}

TEST_F(ModulePackageTest, AFileThatIsNotAPackageIsRefused) {
  const auto out = Path("garbage.gfmodule");
  QFile f(out);
  ASSERT_TRUE(f.open(QIODevice::WriteOnly));
  f.write(QByteArray(1024, 'z'));
  f.close();

  const auto v = Module::VerifyModulePackage(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kNOT_A_PACKAGE);
}

TEST_F(ModulePackageTest, AMissingFileIsRefused) {
  const auto v = Module::VerifyModulePackage(Path("nope.gfmodule"));
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModulePackageStatus::kIO_FAILED);
}

// --------------------------------------------------------------- key hygiene

TEST_F(ModulePackageTest, NoPrivateKeyMaterialIsLeftAnywhere) {
  // The signing key is generated per build and wiped before the builder
  // returns; the public half travels in the package and nothing else does.
  // What is assertable from outside is that the package carries exactly the
  // three META-INF members, and that the build left nothing beside it.
  auto meta_members = ListMembers(Package()).filter(QString("META-INF/"));
  meta_members.sort();
  EXPECT_EQ(meta_members, (QStringList{Module::kModulePackageBuildKeyPath,
                                       Module::kModulePackageManifestPath,
                                       Module::kModulePackageSignaturePath}));

  const auto stray =
      QDir(dir_.path())
          .entryList(QStringList{"*.key", "*.sec", "*.pem"}, QDir::Files);
  EXPECT_TRUE(stray.isEmpty());
}

// ------------------------------------------------------------- the builder

TEST_F(ModulePackageTest, TheBuilderRefusesAReservedPath) {
  auto spec = spec_;
  spec.files.append({"META-INF/manifest.json", {}, QByteArray("mine")});
  spec.output_path = Path("reserved.gfmodule");
  EXPECT_FALSE(Module::BuildModulePackage(spec).ok);
}

TEST_F(ModulePackageTest, TheBuilderRefusesAnEscapingPath) {
  auto spec = spec_;
  spec.files.append({"../outside.so", {}, QByteArray("mine")});
  spec.output_path = Path("escape.gfmodule");
  EXPECT_FALSE(Module::BuildModulePackage(spec).ok);
}

TEST_F(ModulePackageTest, TheBuilderRefusesACaseCollision) {
  auto spec = spec_;
  spec.files.append({"bin/MODULE.so", {}, QByteArray("mine")});
  spec.output_path = Path("collide.gfmodule");
  EXPECT_FALSE(Module::BuildModulePackage(spec).ok);
}

TEST_F(ModulePackageTest, TheBuilderRefusesAnUnreadableSource) {
  auto spec = spec_;
  spec.files = {{"bin/module.so", Path("does-not-exist"), {}}};
  spec.output_path = Path("unreadable.gfmodule");
  EXPECT_FALSE(Module::BuildModulePackage(spec).ok);
}

// ------------------------------------------------- nothing runs unverified

namespace {

/// The library that writes a file when it is mapped, if this build made one.
auto SentinelLibrary() -> QString {
  const auto path = QCoreApplication::applicationDirPath() +
                    "/test-modules/libgf_mod_test_sentinel" +
#if defined(Q_OS_WIN)
                    ".dll";
#elif defined(Q_OS_MACOS)
                    ".dylib";
#else
                    ".so";
#endif
  return QFile::exists(path) ? path : QString();
}

}  // namespace

TEST_F(ModulePackageTest, AVerifiedPackageDoesLoadItsCode) {
  // The positive control, and the test above it is worthless without it: a
  // refusal that runs no code proves nothing if a success would not have run
  // any either.
  const auto sentinel_library = SentinelLibrary();
  if (sentinel_library.isEmpty()) {
    GTEST_SKIP() << "this build has no sentinel library";
  }

  auto spec = spec_;
  spec.files = {{"bin/libgf_mod_test_sentinel.so", sentinel_library, {}}};
  spec.output_path = Path("sentinel.gfmodule");
  ASSERT_TRUE(Module::BuildModulePackage(spec).ok);

  const auto sentinel = Path("it-ran");
  ASSERT_FALSE(QFile::exists(sentinel));
  qputenv("GPGFRONTEND_TEST_SENTINEL", sentinel.toUtf8());

  const auto read = Module::ReadVerifiedModuleImage(spec.output_path);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();
  ASSERT_TRUE(read.image.IsValid());

  QString reason;
  auto mapping = Module::ModuleImageMapping::Create(read.image, &reason);
  ASSERT_TRUE(mapping) << reason.toStdString();

  QLibrary library(mapping->LoadPath());
  ASSERT_TRUE(library.load()) << library.errorString().toStdString();
  EXPECT_TRUE(QFile::exists(sentinel));

  library.unload();
  qunsetenv("GPGFRONTEND_TEST_SENTINEL");
}

TEST_F(ModulePackageTest, ARefusedPackageNeverRunsItsCode) {
  // The claim, asserted the only way it can be: not that loading returned an
  // error, but that the code never executed. By the time an error is returned
  // an image may already be mapped and its initialisers run, so the assertion
  // has to be about an observable side effect that never happened.
  //
  // Here it is stronger still. The refused package's binary is never written
  // anywhere at all, so there is nothing on disk for anything to map, whether
  // deliberately or by mistake.
  const auto sentinel_library = SentinelLibrary();
  if (sentinel_library.isEmpty()) {
    GTEST_SKIP() << "this build has no sentinel library";
  }

  auto spec = spec_;
  spec.files = {{"bin/libgf_mod_test_sentinel.so", sentinel_library, {}}};
  spec.output_path = Path("sentinel2.gfmodule");
  ASSERT_TRUE(Module::BuildModulePackage(spec).ok);

  // One mutation: the binary no longer matches the digest the manifest was
  // signed for.
  QFile library_file(sentinel_library);
  ASSERT_TRUE(library_file.open(QIODevice::ReadOnly));
  auto image = library_file.readAll();
  library_file.close();
  ASSERT_GT(image.size(), 1024);
  image[image.size() - 1] = static_cast<char>(image.at(image.size() - 1) ^ 1);

  const auto tampered = Path("sentinel-tampered.gfmodule");
  ASSERT_TRUE(RepackWith(spec.output_path, tampered,
                         {{"bin/libgf_mod_test_sentinel.so", image}}));

  const auto sentinel = Path("it-ran-anyway");
  qputenv("GPGFRONTEND_TEST_SENTINEL", sentinel.toUtf8());

  const auto read = Module::ReadVerifiedModuleImage(tampered);

  EXPECT_FALSE(read.ok);
  EXPECT_EQ(read.status, Module::ModulePackageStatus::kFILE_DIGEST_MISMATCH);

  // The refusal does not merely report an error: it hands back no image at
  // all, and an image is the only thing materialisation accepts. There is
  // therefore no path in existence for anything to load, deliberately or by
  // mistake -- which is the property the type is for.
  EXPECT_FALSE(read.image.IsValid());

  QString reason;
  EXPECT_FALSE(Module::ModuleImageMapping::Create(read.image, &reason));

  EXPECT_FALSE(QFile::exists(sentinel));

  qunsetenv("GPGFRONTEND_TEST_SENTINEL");
}

// ------------------------------------------------------ producer / verifier

TEST(ModulePackageSmokeTest, APackageBuiltByTheBuildVerifies) {
  // End to end, against the artifact CMake actually produced: the packaging
  // tool ran, with the arguments gf_add_module_package chose, and what came
  // out is fed to the verifier.
  //
  // This is the test that catches producer and verifier drifting apart, which
  // is the failure mode every unit test on either side individually survives.
  // Skipped rather than failed when the package is absent, since a build that
  // did not ask for module packages is a legitimate one.
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) {
    GTEST_SKIP() << "no module packages in this build";
  }

  // Every package the build produced, not just one: a packaging recipe that
  // only works for the smallest module is a recipe that has not been tested.
  for (const auto& info : built) {
    const auto v = Module::VerifyModulePackage(info.absoluteFilePath());
    EXPECT_TRUE(v.ok) << info.fileName().toStdString() << ": "
                      << v.reason.toStdString();
    EXPECT_TRUE(v.manifest.id.startsWith("com.bktus.gpgfrontend.module."));
    EXPECT_EQ(v.manifest.sdk_abi, GF_SDK_ABI_VERSION);
    ASSERT_EQ(v.manifest.files.size(), 1);
    EXPECT_TRUE(v.manifest.files.at(0).path.startsWith(
        Module::kModulePackageBinaryDir));

    // Producer and verifier agree on how to spell this platform. They used to
    // reach the string by different routes -- the packager took it from its
    // command line, the verifier computed it -- so a packaging recipe could
    // stamp a spelling that nothing would ever accept.
    EXPECT_EQ(v.manifest.platform_os, Module::ManifestHostOsName());
    EXPECT_EQ(v.manifest.platform_arch, QSysInfo::currentCpuArchitecture());

    // And the fact the loader consumes is published, for every package.
    EXPECT_EQ(v.library_sha256, v.manifest.files.at(0).sha256);
  }

  const QDir packages(QCoreApplication::applicationDirPath() + "/modules");
  const auto package = packages.absoluteFilePath("gpg_info.gfmodule");
  if (!QFile::exists(package)) return;

  const auto v = Module::VerifyModulePackage(package);
  ASSERT_TRUE(v.ok) << v.reason.toStdString();
  EXPECT_EQ(v.manifest.id, "com.bktus.gpgfrontend.module.gnupg_info_gathering");
  EXPECT_EQ(v.manifest.sdk_abi, GF_SDK_ABI_VERSION);
  EXPECT_EQ(v.manifest.metadata.value("Name"), "GatherGnupgInfo");
  EXPECT_EQ(v.manifest.metadata.value("Author"), "Saturneric");
  EXPECT_EQ(v.manifest.capabilities, QStringList{"gpg"});
}

}  // namespace GpgFrontend::Test
