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
#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleDescriptorBuilder.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleManifest.h"
#include "core/module/ModuleTrustRoot.h"
#include "core/utils/AsyncUtils.h"
#include "core/utils/BuildInfoUtils.h"
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestModuleDescriptor.cpp
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
    -> Module::ModuleDescriptorBuildSpec {
  Module::ModuleDescriptorBuildSpec spec;
  spec.module_id = "com.bktus.gpgfrontend.module.test";
  spec.version = "1.0.0";
  spec.sdk_abi = GF_SDK_ABI_VERSION;
  spec.min_host_version = "2.0.0";
  spec.capabilities = QStringList{"gpg", "ui"};
  spec.events = QStringList{"APPLICATION_LOADED", "MAINWINDOW_MENU_MOUNTED"};
  spec.translation_context = "ModuleTest";
  spec.metadata = {{"Name", "Test Module"},
                   {"Description", "a module that exists to be packaged"},
                   {"Author", "Saturneric"}};
  spec.build_id = Module::ModuleBuildId();
  spec.build_timestamp = "2026-09-15T00:00:00Z";
  spec.build_source_commit = "0000000000000000000000000000000000000000";
  spec.platform_os = Module::ManifestHostOsName();
  spec.platform_arch = QSysInfo::currentCpuArchitecture();
  spec.platform_qt = QT_VERSION_STR;
  // The entry native is bound, not packaged: the descriptor records a value
  // computed from these bytes and the file stays where it is.
  spec.signing_seed = BuildSigningSeed();
  spec.build_id = Module::ModuleBuildId();
  spec.entry_native_name = "gf_mod_test";
  spec.entry_native_file = payload_path;
  spec.resources = {{"resources/note.txt", {}, QByteArray("a resource")}};
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
class ModuleDescriptorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(dir_.isValid());
    payload_ = dir_.path() + "/payload.bin";
    QFile f(payload_);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(4096, 'm'));
    f.close();

    spec_ = GoodSpec(dir_.path(), payload_);
    const auto built = Module::BuildModuleDescriptor(spec_);
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
  Module::ModuleDescriptorBuildSpec spec_;
  QByteArray manifest_bytes_;
  QByteArray public_key_;
};

}  // namespace

// --------------------------------------------------------------- happy path

TEST_F(ModuleDescriptorTest, AValidPackageVerifies) {
  const auto v = Module::VerifyModuleDescriptor(Package());
  ASSERT_TRUE(v.ok) << v.reason.toStdString();
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kOK);
  EXPECT_EQ(v.manifest.id, spec_.module_id);
  EXPECT_EQ(v.manifest.version, "1.0.0");
  EXPECT_EQ(v.manifest.sdk_abi, GF_SDK_ABI_VERSION);
  EXPECT_EQ(v.manifest.capabilities, (QStringList{"gpg", "ui"}));
  EXPECT_EQ(v.manifest.metadata.value("Name"), "Test Module");
  EXPECT_EQ(v.manifest.resources.size(), 1);
  EXPECT_EQ(v.build_public_key, public_key_);

  // The entry native is named logically and bound by value. The name is not a
  // filename and the value is not a path: turning the first into the second is
  // the Host's job, one layer up.
  EXPECT_EQ(v.manifest.entry_native.name, "gf_mod_test");
  EXPECT_EQ(v.manifest.entry_native.mode,
            Module::ModuleEntryVerificationMode::kFILE_SHA256);
  EXPECT_EQ(v.manifest.entry_native.value.size(), 64);
}

TEST_F(ModuleDescriptorTest, TheSignedBytesAreTheStoredBytes) {
  // The signature covers manifest.json as stored, so what the builder reports
  // signing has to be byte-identical to what the package carries. If these
  // ever diverge, every verification still passes and the format has quietly
  // acquired a canonicalisation step on the reading side that nobody wrote.
  EXPECT_EQ(ReadMember(Package(), Module::kModuleDescriptorManifestPath),
            manifest_bytes_);
}

TEST_F(ModuleDescriptorTest, NothingIsWrittenWhileVerifying) {
  // The safety property, asserted rather than assumed: verification reads a
  // package and leaves nothing behind that a later step could execute.
  const auto before =
      QDir(dir_.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
  ASSERT_TRUE(Module::VerifyModuleDescriptor(Package()).ok);
  EXPECT_EQ(
      QDir(dir_.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot),
      before);
}

// ---------------------------------------------------------------- signature

TEST_F(ModuleDescriptorTest, AModifiedManifestFails) {
  auto altered = manifest_bytes_;
  altered.replace("1.0.0", "9.9.9");
  ASSERT_NE(altered, manifest_bytes_);

  const auto out = Path("tampered.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out,
                         {{Module::kModuleDescriptorManifestPath, altered}}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kUNTRUSTED_BUILD_KEY);
}

TEST_F(ModuleDescriptorTest, AModifiedSignatureFails) {
  auto signature =
      ReadMember(Package(), Module::kModuleDescriptorSignaturePath);
  ASSERT_EQ(signature.size(), 64);
  signature[0] = static_cast<char>(signature[0] ^ 0xFF);

  const auto out = Path("badsig.gfmodule");
  ASSERT_TRUE(RepackWith(
      Package(), out, {{Module::kModuleDescriptorSignaturePath, signature}}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kUNTRUSTED_BUILD_KEY);
}

TEST_F(ModuleDescriptorTest, ADescriptorCarryingABuildKeyIsRefused) {
  // The key used to travel inside the package, which established that the
  // package agreed with itself and nothing else: anyone able to replace it
  // could also mint a keypair, re-sign an altered manifest and ship the
  // matching key. A descriptor still carrying one is not a descriptor with an
  // extra file -- it is one from before the trust root moved into the Host.
  const auto out = Path("carrieskey.gfmodule");
  ASSERT_TRUE(RepackWith(
      Package(), out, {}, {},
      {{Module::kModuleDescriptorBuildKeyPath, QByteArray(32, '\x01')}}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kMALFORMED);
  EXPECT_TRUE(v.reason.contains("trust root")) << v.reason.toStdString();
}

TEST_F(ModuleDescriptorTest, ADescriptorIsVerifiedWithTheHostsOwnKey) {
  // The default is not "no key" but "this Host's key". There is no longer a
  // way to ask for a verification that passes on the descriptor's own terms.
  ASSERT_TRUE(Module::VerifyModuleDescriptor(Package()).ok);
  EXPECT_TRUE(
      Module::VerifyModuleDescriptor(Package(), Module::ModuleBuildPublicKey())
          .ok);

  QByteArray wrong(Module::ModuleBuildPublicKey());
  wrong[0] = static_cast<char>(wrong[0] ^ 0xFF);

  const auto v = Module::VerifyModuleDescriptor(Package(), wrong);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kUNTRUSTED_BUILD_KEY);
}

TEST_F(ModuleDescriptorTest, ADescriptorFromAnotherBuildIsRefusedByBuildId) {
  // Right key, wrong build: reachable only from the same tree, which is what
  // makes it worth a separate status. "Not ours" and "ours, but from a
  // different build" are different problems, and only the second is one a
  // rebuild fixes.
  auto spec = spec_;
  spec.build_id = "gfb1-00000000000000000000000000000000";
  spec.output_path = Path("otherbuild.gfmodule");
  ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);

  const auto v = Module::VerifyModuleDescriptor(spec.output_path);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kWRONG_BUILD);
  EXPECT_TRUE(v.reason.contains(Module::ModuleBuildId()))
      << v.reason.toStdString();
}

TEST_F(ModuleDescriptorTest, TheBuilderRefusesASeedThatIsNotThisBuilds) {
  // Structural, and the point of it: gf_module_tool links gf_core, so it
  // carries the very trust root the Host does. A descriptor signed with some
  // other key would be one no Host could load, so it cannot be produced at
  // all rather than produced and discovered later.
  auto spec = spec_;
  spec.signing_seed = QByteArray(32, '\x07');
  spec.output_path = Path("wrongseed.gfmodule");

  const auto result = Module::BuildModuleDescriptor(spec);
  EXPECT_FALSE(result.ok);
  EXPECT_TRUE(result.reason.contains("module-build key"))
      << result.reason.toStdString();
  EXPECT_FALSE(QFile::exists(spec.output_path))
      << "nothing should have been written";
}

TEST_F(ModuleDescriptorTest, TheBuilderRefusesAnEmptySeed) {
  auto spec = spec_;
  spec.signing_seed.clear();
  spec.output_path = Path("noseed.gfmodule");

  const auto result = Module::BuildModuleDescriptor(spec);
  EXPECT_FALSE(result.ok);
  EXPECT_FALSE(QFile::exists(spec.output_path));
}

// ----------------------------------------------------------- file integrity

TEST_F(ModuleDescriptorTest, AnAppendedMemberIsRefused) {
  // There is no executable member to modify any more, which is the change.
  // What remains, and matters more, is that a member the manifest does not
  // cover cannot be smuggled in: the signature says nothing about it, so the
  // package must refuse rather than carry it.
  const auto out = Path("appended.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out, {}, {},
                         {{"resources/extra.txt", QByteArray("smuggled")}}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kUNDECLARED_RESOURCE);
}

TEST_F(ModuleDescriptorTest, AModifiedResourceFails) {
  const auto out = Path("badres.gfmodule");
  ASSERT_TRUE(RepackWith(
      Package(), out, {{"resources/note.txt", QByteArray("not a resource")}}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status,
            Module::ModuleDescriptorStatus::kRESOURCE_DIGEST_MISMATCH);
}

TEST_F(ModuleDescriptorTest, AMissingDeclaredFileFails) {
  const auto out = Path("missing.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out, {}, {"resources/note.txt"}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status,
            Module::ModuleDescriptorStatus::kMISSING_DECLARED_RESOURCE);
}

TEST_F(ModuleDescriptorTest, AnUndeclaredExtraFileFails) {
  // The appended-payload case: everything the manifest covers is intact, and
  // the package carries one more thing the signature says nothing about.
  const auto out = Path("extra.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out, {}, {},
                         {{"bin/extra.so", QByteArray("payload")}}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kUNDECLARED_RESOURCE);
}

// -------------------------------------------------------------- meta members

TEST_F(ModuleDescriptorTest, ADuplicateManifestFails) {
  // Two members under one name: one reader takes the first and another takes
  // the last, and they disagree about what was signed.
  const auto out = Path("dupmanifest.gfmodule");
  ASSERT_TRUE(
      RepackWith(Package(), out, {}, {},
                 {{Module::kModuleDescriptorManifestPath, manifest_bytes_}}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kNOT_A_PACKAGE);
}

TEST_F(ModuleDescriptorTest, AMissingSignatureFails) {
  const auto out = Path("nosig.gfmodule");
  ASSERT_TRUE(
      RepackWith(Package(), out, {}, {Module::kModuleDescriptorSignaturePath}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kMALFORMED);
}

TEST_F(ModuleDescriptorTest, ACaseCollidingEntryFails) {
  // `RESOURCES/note.txt` and `resources/note.txt` are two members here and one
  // on macOS or Windows, so the package would mean different things depending
  // on who read it. Refused at the archive walk, before any of it is trusted.
  const auto out = Path("casecollide.gfmodule");
  ASSERT_TRUE(RepackWith(Package(), out, {}, {},
                         {{"RESOURCES/note.txt", QByteArray("payload")}}));

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kNOT_A_PACKAGE);
}

// ------------------------------------------------------------------- policy

TEST_F(ModuleDescriptorTest, AWrongPlatformIsRejected) {
  auto spec = spec_;
  spec.platform_arch = "pdp11";
  spec.output_path = Path("wrongarch.gfmodule");
  ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);

  const auto v = Module::VerifyModuleDescriptor(spec.output_path);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kWRONG_PLATFORM);
}

TEST_F(ModuleDescriptorTest, AnIncompatibleAbiIsRejectedInBothDirections) {
  auto too_old = spec_;
  too_old.sdk_abi = GF_SDK_ABI_MIN_SUPPORTED - 1;
  too_old.output_path = Path("tooold.gfmodule");
  ASSERT_TRUE(Module::BuildModuleDescriptor(too_old).ok);
  EXPECT_EQ(Module::VerifyModuleDescriptor(too_old.output_path).status,
            Module::ModuleDescriptorStatus::kINCOMPATIBLE_ABI);

  auto too_new = spec_;
  too_new.sdk_abi = GF_SDK_ABI_VERSION + 1;
  too_new.output_path = Path("toonew.gfmodule");
  ASSERT_TRUE(Module::BuildModuleDescriptor(too_new).ok);
  EXPECT_EQ(Module::VerifyModuleDescriptor(too_new.output_path).status,
            Module::ModuleDescriptorStatus::kINCOMPATIBLE_ABI);
}

TEST_F(ModuleDescriptorTest, AHostTooOldIsRejected) {
  auto spec = spec_;
  spec.min_host_version = "99.0.0";
  spec.output_path = Path("needsnewer.gfmodule");
  ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);
  EXPECT_EQ(Module::VerifyModuleDescriptor(spec.output_path).status,
            Module::ModuleDescriptorStatus::kINCOMPATIBLE_ABI);
}

TEST_F(ModuleDescriptorTest, AFileThatIsNotAPackageIsRefused) {
  const auto out = Path("garbage.gfmodule");
  QFile f(out);
  ASSERT_TRUE(f.open(QIODevice::WriteOnly));
  f.write(QByteArray(1024, 'z'));
  f.close();

  const auto v = Module::VerifyModuleDescriptor(out);
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kNOT_A_PACKAGE);
}

TEST_F(ModuleDescriptorTest, AMissingFileIsRefused) {
  const auto v = Module::VerifyModuleDescriptor(Path("nope.gfmodule"));
  EXPECT_FALSE(v.ok);
  EXPECT_EQ(v.status, Module::ModuleDescriptorStatus::kIO_FAILED);
}

// --------------------------------------------------------------- key hygiene

TEST_F(ModuleDescriptorTest, NoPrivateKeyMaterialIsLeftAnywhere) {
  // The signing key is the build's, derived from a seed that stays in the
  // build tree; the expanded secret is wiped before the builder returns and
  // neither half travels in the descriptor. What is assertable from outside is
  // that the package carries exactly two META-INF members -- no key among
  // them -- and that the build left nothing beside it.
  auto meta_members = ListMembers(Package()).filter(QString("META-INF/"));
  meta_members.sort();
  EXPECT_EQ(meta_members, (QStringList{Module::kModuleDescriptorManifestPath,
                                       Module::kModuleDescriptorSignaturePath}))
      << "a descriptor carries a manifest and a signature, and nothing else";

  const auto stray =
      QDir(dir_.path())
          .entryList(QStringList{"*.key", "*.sec", "*.pem"}, QDir::Files);
  EXPECT_TRUE(stray.isEmpty());
}

// ------------------------------------------------------------- the builder

// ------------------------------------------------- reading resources back

TEST_F(ModuleDescriptorTest, ResourcesCanBeReadBackForRegeneration) {
  // `reseal` rewrites a descriptor in place against a native that deployment
  // has since rewritten. It must carry the resources forward EXACTLY, and the
  // only trustworthy source for them is the signed original.
  QMap<QString, QByteArray> resources;
  QString why;
  ASSERT_TRUE(Module::ReadModuleDescriptorResources(Package(), resources, why))
      << why.toStdString();

  ASSERT_EQ(resources.size(), 1);
  EXPECT_EQ(resources.value("resources/note.txt"), QByteArray("a resource"));
}

TEST_F(ModuleDescriptorTest, ReadingResourcesSkipsTheDescriptorsOwnMachinery) {
  // META-INF is regenerated, never carried forward. Carrying the old manifest
  // or signature into a new descriptor would put two of each in the archive,
  // which the verifier refuses -- correctly, and confusingly.
  QMap<QString, QByteArray> resources;
  QString why;
  ASSERT_TRUE(Module::ReadModuleDescriptorResources(Package(), resources, why));

  for (const auto& name : resources.keys()) {
    EXPECT_FALSE(name.startsWith("META-INF/")) << name.toStdString();
  }
}

TEST_F(ModuleDescriptorTest, ARegeneratedDescriptorKeepsEveryResource) {
  // The round trip `reseal` performs, without the tool: read the resources
  // out, build a new descriptor from the same manifest, read them back.
  QMap<QString, QByteArray> original;
  QString why;
  ASSERT_TRUE(Module::ReadModuleDescriptorResources(Package(), original, why));

  auto spec = GoodSpec(dir_.path(), payload_);
  spec.output_path = Path("regenerated.gfmodule");
  spec.resources.clear();
  for (auto it = original.constBegin(); it != original.constEnd(); ++it) {
    spec.resources.append({it.key(), {}, it.value()});
  }

  const auto built = Module::BuildModuleDescriptor(spec);
  ASSERT_TRUE(built.ok) << built.reason.toStdString();
  ASSERT_TRUE(Module::VerifyModuleDescriptor(spec.output_path).ok);

  QMap<QString, QByteArray> again;
  ASSERT_TRUE(
      Module::ReadModuleDescriptorResources(spec.output_path, again, why));
  EXPECT_EQ(again, original);
}

TEST_F(ModuleDescriptorTest, ReadingResourcesFromANonDescriptorFails) {
  QMap<QString, QByteArray> resources;
  QString why;
  EXPECT_FALSE(Module::ReadModuleDescriptorResources(Path("nope.gfmodule"),
                                                     resources, why));
  EXPECT_FALSE(why.isEmpty());
}

// ------------------------------------------------- the preparation seal

TEST_F(ModuleDescriptorTest, ASealedValueThatStillMatchesChangesNothing) {
  auto spec = GoodSpec(dir_.path(), payload_);
  spec.output_path = Path("sealed.gfmodule");

  // What seal-prepared would have recorded, taken from the descriptor the
  // fixture already built against the same unchanged file.
  const auto v = Module::VerifyModuleDescriptor(Package());
  ASSERT_TRUE(v.ok) << v.reason.toStdString();
  spec.expected_entry_value = v.manifest.entry_native.value;

  const auto built = Module::BuildModuleDescriptor(spec);
  EXPECT_TRUE(built.ok) << built.reason.toStdString();
  EXPECT_TRUE(Module::VerifyModuleDescriptor(spec.output_path).ok);
}

TEST_F(ModuleDescriptorTest, AnEntryRewrittenAfterSealingIsRefused) {
  // The case the seal exists for: preparation finished, something wrote to the
  // native afterwards, and the descriptor about to be written would describe
  // the file as it used to be.
  const auto v = Module::VerifyModuleDescriptor(Package());
  ASSERT_TRUE(v.ok) << v.reason.toStdString();
  const auto sealed_value = v.manifest.entry_native.value;

  QFile payload(payload_);
  ASSERT_TRUE(payload.open(QIODevice::WriteOnly | QIODevice::Truncate));
  payload.write(QByteArray(4096, 'n'));
  payload.close();

  auto spec = GoodSpec(dir_.path(), payload_);
  spec.output_path = Path("stale.gfmodule");
  spec.expected_entry_value = sealed_value;

  const auto built = Module::BuildModuleDescriptor(spec);
  EXPECT_FALSE(built.ok);
  EXPECT_TRUE(built.reason.contains("sealed")) << built.reason.toStdString();
  EXPECT_FALSE(QFileInfo::exists(spec.output_path))
      << "a refused build must not leave a descriptor behind";
}

TEST_F(ModuleDescriptorTest, NoSealMeansNoCheckRatherThanAFailedOne) {
  // A release that skips seal-prepared loses the check and nothing else.
  auto spec = GoodSpec(dir_.path(), payload_);
  spec.output_path = Path("unsealed.gfmodule");
  ASSERT_TRUE(spec.expected_entry_value.isEmpty());

  EXPECT_TRUE(Module::BuildModuleDescriptor(spec).ok);
}

TEST_F(ModuleDescriptorTest, TheBuilderRefusesAReservedPath) {
  auto spec = spec_;
  spec.resources.append({"META-INF/manifest.json", {}, QByteArray("mine")});
  spec.output_path = Path("reserved.gfmodule");
  EXPECT_FALSE(Module::BuildModuleDescriptor(spec).ok);
}

TEST_F(ModuleDescriptorTest, TheBuilderRefusesAnEscapingPath) {
  auto spec = spec_;
  spec.resources.append({"../outside.so", {}, QByteArray("mine")});
  spec.output_path = Path("escape.gfmodule");
  EXPECT_FALSE(Module::BuildModuleDescriptor(spec).ok);
}

TEST_F(ModuleDescriptorTest, TheBuilderRefusesACaseCollision) {
  auto spec = spec_;
  spec.resources.append({"resources/NOTE.txt", {}, QByteArray("mine")});
  spec.output_path = Path("collide.gfmodule");
  EXPECT_FALSE(Module::BuildModuleDescriptor(spec).ok);
}

TEST_F(ModuleDescriptorTest, TheBuilderRefusesAnUnreadableSource) {
  auto spec = spec_;
  spec.resources = {{"resources/x.txt", Path("does-not-exist"), {}}};
  spec.output_path = Path("unreadable.gfmodule");
  EXPECT_FALSE(Module::BuildModuleDescriptor(spec).ok);
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

TEST_F(ModuleDescriptorTest, AVerifiedDescriptorDoesLoadTheCodeItBinds) {
  // The positive control, and the test below it is worthless without it: a
  // refusal that runs no code proves nothing if a success would not have run
  // any either.
  const auto sentinel_library = SentinelLibrary();
  if (sentinel_library.isEmpty()) {
    GTEST_SKIP() << "this build has no sentinel library";
  }

  // Beside the descriptor, because that is where the Host looks. The
  // descriptor names `gf_mod_test_sentinel` and never says where it lives;
  // turning that into `libgf_mod_test_sentinel.so` in this directory is the
  // whole of what ResolveAndVerifyNativeEntry() does.
  const auto native =
      Path(Module::ModuleNativeFileName("gf_mod_test_sentinel"));
  ASSERT_TRUE(QFile::copy(sentinel_library, native));

  auto spec = spec_;
  spec.entry_native_name = "gf_mod_test_sentinel";
  spec.entry_native_file = native;
  spec.resources.clear();
  spec.output_path = Path("sentinel.gfmodule");
  ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);

  const auto sentinel = Path("it-ran");
  ASSERT_FALSE(QFile::exists(sentinel));
  qputenv("GPGFRONTEND_TEST_SENTINEL", sentinel.toUtf8());

  const auto read = Module::VerifyModuleDescriptor(spec.output_path);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  const Module::ModuleNativeRoot root{Path("")};
  const auto entry = Module::ResolveAndVerifyNativeEntry(read.manifest, root);
  ASSERT_TRUE(entry.ok) << entry.reason.toStdString();

  QLibrary library(entry.path);
  ASSERT_TRUE(library.load()) << library.errorString().toStdString();
  EXPECT_TRUE(QFile::exists(sentinel));

  library.unload();
  qunsetenv("GPGFRONTEND_TEST_SENTINEL");
}

TEST_F(ModuleDescriptorTest, ATamperedEntryNativeIsRefusedAndNeverRuns) {
  // The claim, asserted the only way it can be: not that resolution returned
  // an error, but that the code never executed. By the time an error comes
  // back a library could already have been mapped and its initialisers run,
  // so the assertion has to be about a side effect that never happened.
  //
  // This is also where the model's one real cost is visible, and worth being
  // precise about. The entry native now lives on disk before it is checked,
  // so what is proven here is that it is checked BEFORE anything opens it --
  // not that it could not have been swapped by another process in between.
  // That window is documented on ResolveAndVerifyNativeEntry() and is outside
  // the threat model; this test is about the ordering, which is inside it.
  const auto sentinel_library = SentinelLibrary();
  if (sentinel_library.isEmpty()) {
    GTEST_SKIP() << "this build has no sentinel library";
  }

  const auto native =
      Path(Module::ModuleNativeFileName("gf_mod_test_sentinel"));
  ASSERT_TRUE(QFile::copy(sentinel_library, native));

  auto spec = spec_;
  spec.entry_native_name = "gf_mod_test_sentinel";
  spec.entry_native_file = native;
  spec.resources.clear();
  spec.output_path = Path("sentinel2.gfmodule");
  ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);

  // One mutation, AFTER the descriptor was signed for these bytes. The file
  // still loads perfectly well; it is simply no longer the one this
  // descriptor binds.
  {
    QFile file(native);
    ASSERT_TRUE(file.open(QIODevice::ReadWrite));
    ASSERT_TRUE(file.seek(file.size() - 1));
    const auto last = file.read(1);
    ASSERT_EQ(last.size(), 1);
    ASSERT_TRUE(file.seek(file.size() - 1));
    const char flipped = static_cast<char>(last.at(0) ^ 1);
    ASSERT_EQ(file.write(&flipped, 1), 1);
    file.close();
  }

  const auto sentinel = Path("it-ran-anyway");
  qputenv("GPGFRONTEND_TEST_SENTINEL", sentinel.toUtf8());

  // The descriptor itself is untouched and still verifies: it is a separate
  // artifact from what it binds, which is the point of splitting them.
  const auto read = Module::VerifyModuleDescriptor(spec.output_path);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  const Module::ModuleNativeRoot root{Path("")};
  const auto entry = Module::ResolveAndVerifyNativeEntry(read.manifest, root);

  EXPECT_FALSE(entry.ok);
  EXPECT_EQ(entry.status,
            Module::ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH);

  // No path came back, and a path is the only thing a loader is ever given.
  EXPECT_TRUE(entry.path.isEmpty());
  EXPECT_FALSE(QFile::exists(sentinel));

  qunsetenv("GPGFRONTEND_TEST_SENTINEL");
}

TEST_F(ModuleDescriptorTest, AMissingEntryNativeIsItsOwnRefusal) {
  // A descriptor whose library was never installed, or was removed, is a
  // different problem from one whose library was altered -- and says so.
  auto spec = spec_;
  spec.entry_native_name = "gf_mod_test";
  spec.resources.clear();
  spec.output_path = Path("absent.gfmodule");
  ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);

  const Module::ModuleNativeRoot root{Path("no-such-directory")};
  const auto read = Module::VerifyModuleDescriptor(spec.output_path);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  const auto entry = Module::ResolveAndVerifyNativeEntry(read.manifest, root);
  EXPECT_FALSE(entry.ok);
  EXPECT_EQ(entry.status, Module::ModuleEntryStatus::kMISSING_ENTRY_NATIVE);
}

TEST_F(ModuleDescriptorTest, ASymlinkedEntryNativeIsRefusedRatherThanFollowed) {
  // The descriptor names a file in this directory. A link wearing that name
  // is a different file, and following it would be the Host choosing to load
  // something the descriptor did not bind.
  const auto sentinel_library = SentinelLibrary();
  if (sentinel_library.isEmpty()) {
    GTEST_SKIP() << "this build has no sentinel library";
  }

  const auto native =
      Path(Module::ModuleNativeFileName("gf_mod_test_sentinel"));
  auto spec = spec_;
  spec.entry_native_name = "gf_mod_test_sentinel";
  spec.entry_native_file = sentinel_library;
  spec.resources.clear();
  spec.output_path = Path("linked.gfmodule");
  ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);

  // Points at the very library the descriptor was signed for, so the digest
  // would match if it were followed. Only the file type refuses it.
  ASSERT_TRUE(QFile::link(sentinel_library, native));

  const Module::ModuleNativeRoot root{Path("")};
  const auto read = Module::VerifyModuleDescriptor(spec.output_path);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  const auto entry = Module::ResolveAndVerifyNativeEntry(read.manifest, root);
  EXPECT_FALSE(entry.ok);
  EXPECT_EQ(entry.status, Module::ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE);
}

TEST_F(ModuleDescriptorTest, ATextFileWearingALibraryNameIsRefused) {
  auto spec = spec_;
  spec.entry_native_name = "gf_mod_test_sentinel";
  spec.resources.clear();
  spec.output_path = Path("textfile.gfmodule");

  const auto native =
      Path(Module::ModuleNativeFileName("gf_mod_test_sentinel"));
  {
    QFile file(native);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("#!/bin/sh\necho not a library\n");
    file.close();
  }
  spec.entry_native_file = native;
  ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);

  const Module::ModuleNativeRoot root{Path("")};
  const auto read = Module::VerifyModuleDescriptor(spec.output_path);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  // The digest matches -- it was computed from this very file. The refusal is
  // the file type, checked before anything is hashed or handed to a loader.
  const auto entry = Module::ResolveAndVerifyNativeEntry(read.manifest, root);
  EXPECT_FALSE(entry.ok);
  EXPECT_EQ(entry.status, Module::ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE);
}

// ------------------------------------------------------ producer / verifier

TEST(ModuleDescriptorSmokeTest, APackageBuiltByTheBuildVerifies) {
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
    const auto v = Module::VerifyModuleDescriptor(info.absoluteFilePath());
    EXPECT_TRUE(v.ok) << info.fileName().toStdString() << ": "
                      << v.reason.toStdString();
    EXPECT_TRUE(v.manifest.id.startsWith("com.bktus.gpgfrontend.module."));
    EXPECT_EQ(v.manifest.sdk_abi, GF_SDK_ABI_VERSION);
    // Nothing executable inside, and an entry bound from outside.
    EXPECT_TRUE(v.manifest.resources.isEmpty());
    EXPECT_TRUE(v.manifest.entry_native.name.startsWith("gf_mod_"));

    // Producer and verifier agree on how to spell this platform. They used to
    // reach the string by different routes -- the packager took it from its
    // command line, the verifier computed it -- so a packaging recipe could
    // stamp a spelling that nothing would ever accept.
    EXPECT_EQ(v.manifest.platform_os, Module::ManifestHostOsName());
    EXPECT_EQ(v.manifest.platform_arch, QSysInfo::currentCpuArchitecture());

    // And the entry it binds really is installed beside it, under the name
    // this platform spells it with. This is the end-to-end check that CMake's
    // placement and the Host's mapping agree.
    const Module::ModuleNativeRoot root{info.absolutePath() + "/native"};
    const auto entry = Module::ResolveAndVerifyNativeEntry(v.manifest, root);
    EXPECT_TRUE(entry.ok) << info.fileName().toStdString() << ": "
                          << entry.reason.toStdString();
  }

  // One module in detail, chosen by the identity it declares rather than by
  // where it sits: a namespace directory is named by a derived key, so
  // picking one by path would be picking whichever sorted first.
  constexpr auto kExpectedId =
      "com.bktus.gpgfrontend.module.gnupg_info_gathering";

  QString package;
  for (const auto& info : built) {
    const auto candidate =
        Module::VerifyModuleDescriptor(info.absoluteFilePath());
    if (candidate.ok && candidate.manifest.id == kExpectedId) {
      package = info.absoluteFilePath();
      break;
    }
  }
  if (package.isEmpty()) return;

  const auto v = Module::VerifyModuleDescriptor(package);
  ASSERT_TRUE(v.ok) << v.reason.toStdString();
  EXPECT_EQ(v.manifest.id, kExpectedId);
  EXPECT_EQ(v.manifest.sdk_abi, GF_SDK_ABI_VERSION);
  EXPECT_EQ(v.manifest.metadata.value("Name"), "GatherGnupgInfo");
  EXPECT_EQ(v.manifest.metadata.value("Author"), "Saturneric");
  EXPECT_EQ(v.manifest.capabilities, QStringList{"gpg"});
}

}  // namespace GpgFrontend::Test
