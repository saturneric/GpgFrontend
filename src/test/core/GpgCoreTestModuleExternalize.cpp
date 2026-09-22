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

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSysInfo>
#include <QTemporaryDir>

#include "GpgFrontendTest.h"
#include "core/ModuleDescriptorArchive.h"
#include "core/ModuleTestPackages.h"
#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleDescriptorBuilder.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleExternalTrust.h"
#include "core/module/ModuleExternalize.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleNamespace.h"
#include "core/module/ModulePublisherKey.h"
#include "core/module/ModuleTrustRoot.h"
#include "core/utils/CommonUtils.h"
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestModuleExternalize.cpp
 * @brief Integrated in, publisher-signed external out, and nothing else.
 *
 * Every input here is a real integrated namespace, signed by this build and
 * binding the real sentinel library, so a refusal is about the one thing each
 * case changes -- and the happy path is the artifact a publisher would ship.
 */

namespace GpgFrontend::Test {

namespace {

auto RandomSeed() -> QByteArray {
  EnsureSodiumInit();
  QByteArray seed(crypto_sign_SEEDBYTES, '\0');
  randombytes_buf(seed.data(), static_cast<size_t>(seed.size()));
  return seed;
}

auto ReadFile(const QString& path) -> QByteArray {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  return file.readAll();
}

/// Everything under @p root, hidden entries included, relative.
auto ListTree(const QString& root) -> QStringList {
  QStringList out;
  QDirIterator it(root, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) out.append(QDir(root).relativeFilePath(it.next()));
  out.sort();
  return out;
}

/// The contract row a key path falls under, spelled as the row is.
auto RowFor(const QString& path,
            const QVector<Module::ModuleExternalizeField>& contract)
    -> QString {
  for (const auto& row : contract) {
    if (row.path == path) return row.path;
  }
  for (const auto& row : contract) {
    if (!row.path.endsWith(".*")) continue;
    if (path.startsWith(row.path.chopped(1))) return row.path;
  }
  return {};
}

}  // namespace

class ModuleExternalizeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!Module::ModuleEntryVerificationModeFor(Module::ManifestHostOsName())
             .mode.has_value()) {
      GTEST_SKIP() << "this platform has no entry binding, so nothing on it "
                      "can be externalized";
    }
    ASSERT_TRUE(dir_.isValid());
    input_root_ = dir_.path() + "/integrated";
    output_root_ = dir_.path() + "/external";
    publisher_seed_ = RandomSeed();
    publisher_key_ = Module::ModulePublisherPublicKeyFromSeed(publisher_seed_);

    spec_.module_id = "com.example.module.externalize";
    spec_.version = "1.2.3";
    spec_.sdk_abi = GF_SDK_ABI_VERSION;
    spec_.min_host_version = "2.0.0";
    spec_.capabilities = QStringList{"gpg", "ui"};
    spec_.events = QStringList{"APPLICATION_LOADED"};
    spec_.translation_context = "ModuleExternalize";
    spec_.metadata = {{"Name", "Externalize"},
                      {"Description", "a module on its way out"},
                      {"Author", "Saturneric"}};
    spec_.build_id = Module::ModuleBuildId();
    spec_.build_timestamp = "2026-09-22T00:00:00Z";
    spec_.build_source_commit = QString(40, '1');
    spec_.platform_os = Module::ManifestHostOsName();
    spec_.platform_arch = QSysInfo::currentCpuArchitecture();
    spec_.platform_qt = QT_VERSION_STR;
    spec_.entry_native_name = "gf_mod_test_sentinel";
    spec_.entry_binding = Module::ModuleBindingRequirement::kREQUIRED;
    spec_.signing_seed = BuildSigningSeed();
    spec_.resources = {{"resources/a.txt", {}, QByteArray("first")},
                       {"resources/b.txt", {}, QByteArray("second")}};
    ASSERT_TRUE(BuildIntegrated(spec_));
  }

  void TearDown() override {
    if (publisher_key_.isEmpty()) return;
    Module::RevokeModulePublisherKey(publisher_key_);
    Module::SetExternalModuleEnabled(spec_.module_id, publisher_key_, false);
  }

  [[nodiscard]] auto Namespace() const -> QString {
    return input_root_ + "/" + Module::ModuleDirectoryKey(spec_.module_id);
  }
  [[nodiscard]] auto Native() const -> QString {
    return Namespace() + "/native/" +
           Module::ModuleNativeFileName(spec_.entry_native_name);
  }

  /// Lay @p spec out as an integrated namespace under input_root_.
  auto BuildIntegrated(Module::ModuleDescriptorBuildSpec& spec) -> bool {
    if (!QDir().mkpath(Namespace() + "/native")) return false;
    if (!QFile::exists(Native()) &&
        !QFile::copy(QString::fromUtf8(GF_TEST_SENTINEL_FILE), Native())) {
      return false;
    }
    spec.entry_native_file = Native();
    spec.output_path = Namespace() + "/module.gfmodule";
    const auto built = Module::BuildModuleDescriptor(spec);
    EXPECT_TRUE(built.ok) << built.reason.toStdString();
    return built.ok;
  }

  auto Externalize(const QString& output_root, const QString& name = {},
                   const QString& url = {}) const
      -> Module::ModuleExternalizeResult {
    Module::ModuleExternalizeSpec spec;
    spec.input = Namespace();
    spec.publisher_seed = publisher_seed_;
    spec.output_root = output_root;
    spec.publisher_name = name;
    spec.publisher_url = url;
    return Module::ExternalizeModule(spec);
  }

  /// Refused, for @p because, and leaving nothing behind.
  void ExpectRefused(const Module::ModuleExternalizeResult& result,
                     const QString& because) const {
    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(result.reason.contains(because)) << result.reason.toStdString();
    EXPECT_TRUE(ListTree(output_root_).isEmpty())
        << "a refusal left something behind: "
        << ListTree(output_root_).join(", ").toStdString();
  }

  QTemporaryDir dir_;
  QString input_root_;
  QString output_root_;
  QByteArray publisher_seed_;
  QByteArray publisher_key_;
  Module::ModuleDescriptorBuildSpec spec_;
};

// ---------------------------------------------------------------- happy path

TEST_F(ModuleExternalizeTest, AnIntegratedModuleBecomesAnExternalOne) {
  const auto result =
      Externalize(output_root_, "Example Publisher", "https://example.com");
  ASSERT_TRUE(result.ok) << result.reason.toStdString();

  const auto key = Module::ModuleDirectoryKey(spec_.module_id);
  EXPECT_EQ(result.output_namespace, output_root_ + "/" + key);
  EXPECT_EQ(result.publisher_key, publisher_key_);

  // Exactly a descriptor and its entry: no staging, no helpers, no seed.
  const auto native_name =
      Module::ModuleNativeFileName(spec_.entry_native_name);
  EXPECT_EQ(ListTree(output_root_),
            (QStringList{key, key + "/module.gfmodule", key + "/native",
                         key + "/native/" + native_name}));
  EXPECT_EQ(ReadFile(result.output_namespace + "/native/" + native_name),
            ReadFile(Native()));

  const auto descriptor = result.output_namespace + "/module.gfmodule";
  const auto external = Module::VerifyExternalModuleDescriptor(descriptor);
  ASSERT_TRUE(external.ok) << external.reason.toStdString();
  EXPECT_EQ(external.signer_public_key, publisher_key_);
  EXPECT_FALSE(Module::VerifyModuleDescriptor(descriptor).ok)
      << "the output verified as integrated";

  const auto input =
      Module::VerifyModuleDescriptor(Namespace() + "/module.gfmodule");
  ASSERT_TRUE(input.ok);
  const auto& in = input.manifest;
  const auto& out = external.manifest;
  EXPECT_EQ(out.id, in.id);
  EXPECT_EQ(out.version, in.version);
  EXPECT_EQ(out.sdk_abi, in.sdk_abi);
  EXPECT_EQ(out.min_host_version, in.min_host_version);
  EXPECT_EQ(out.capabilities, in.capabilities);
  EXPECT_EQ(out.events, in.events);
  EXPECT_EQ(out.translation_context, in.translation_context);
  EXPECT_EQ(out.platform_os, in.platform_os);
  EXPECT_EQ(out.platform_arch, in.platform_arch);
  EXPECT_EQ(out.build_id, in.build_id);
  EXPECT_EQ(out.entry_native.name, in.entry_native.name);
  ASSERT_TRUE(out.entry_native.verification.has_value());
  EXPECT_EQ(out.entry_native.verification->value,
            in.entry_native.verification->value);
  EXPECT_EQ(out.resources.size(), 2);
  EXPECT_EQ(out.metadata.value("Name"), "Externalize");
  EXPECT_EQ(out.metadata.value(Module::kModuleMetadataPublisher),
            "Example Publisher");
  EXPECT_EQ(out.metadata.value(Module::kModuleMetadataPublisherUrl),
            "https://example.com");

  // The external entry policy, the one the Host applies.
  const auto entry = Module::ResolveAndVerifyNativeEntry(
      out, Module::ModuleNativeRoot{result.output_namespace + "/native"},
      {Module::ModuleOrigin::kEXTERNAL,
       Module::ModuleBindingRequirement::kREQUIRED});
  EXPECT_TRUE(entry.ok) << entry.reason.toStdString();
}

TEST_F(ModuleExternalizeTest, PublisherMetadataIsOptional) {
  const auto result = Externalize(output_root_);
  ASSERT_TRUE(result.ok) << result.reason.toStdString();
  EXPECT_FALSE(
      result.manifest.metadata.contains(Module::kModuleMetadataPublisher));
  EXPECT_FALSE(
      result.manifest.metadata.contains(Module::kModuleMetadataPublisherUrl));
}

TEST_F(ModuleExternalizeTest, ExternalizationIsDeterministic) {
  const auto first = Externalize(dir_.path() + "/one", "P");
  const auto second = Externalize(dir_.path() + "/two", "P");
  ASSERT_TRUE(first.ok) << first.reason.toStdString();
  ASSERT_TRUE(second.ok) << second.reason.toStdString();
  EXPECT_EQ(ReadFile(first.output_namespace + "/module.gfmodule"),
            ReadFile(second.output_namespace + "/module.gfmodule"));
}

// ------------------------------------------------------------- the contract

TEST_F(ModuleExternalizeTest, TheContractCoversExactlyWhatTheBuilderWrites) {
  // The guard against a field passing through by default. If the builder
  // gains a field, this fails until the contract gains a row deciding its
  // fate -- and a row with nothing behind it fails too.
  const auto result = Externalize(output_root_, "P", "https://p");
  ASSERT_TRUE(result.ok) << result.reason.toStdString();

  const auto external = Module::VerifyExternalModuleDescriptor(
      result.output_namespace + "/module.gfmodule");
  ASSERT_TRUE(external.ok);

  const auto contract = Module::ModuleExternalizeContract();
  QSet<QString> used;
  for (const auto& path :
       Module::ModuleManifestKeyPaths(external.manifest_bytes)) {
    const auto row = RowFor(path, contract);
    EXPECT_FALSE(row.isEmpty()) << "the builder writes \"" << path.toStdString()
                                << "\" and the contract has no decision for it";
    used.insert(row);
  }

  QSet<QString> rows;
  for (const auto& row : contract) rows.insert(row.path);
  // `size` is recorded only where the binding mode is a whole-file digest.
  const auto rule =
      Module::ModuleEntryVerificationModeFor(Module::ManifestHostOsName());
  if (rule.mode != Module::ModuleEntryVerificationMode::kFILE_SHA256) {
    rows.remove("entry_native.size");
  }
  EXPECT_EQ(used, rows) << "a contract row matches nothing the builder writes";
}

TEST_F(ModuleExternalizeTest, BuildIdIsCarriedAsProvenanceOnly) {
  const auto result = Externalize(output_root_);
  ASSERT_TRUE(result.ok) << result.reason.toStdString();

  // Preserved -- it names the build that produced these binaries -- and
  // worth nothing: equal to this Host's own id, and still waiting on both of
  // the user's decisions.
  EXPECT_EQ(result.manifest.build_id, Module::ModuleBuildId());
  EXPECT_EQ(Module::ExternalModuleAuthorization(result.manifest.id,
                                                result.publisher_key),
            Module::ModuleAuthorizationState::kPUBLISHER_UNTRUSTED);
}

TEST_F(ModuleExternalizeTest, AFieldWithNoDecisionStopsExternalization) {
  // A manifest carrying a field the contract has never heard of, signed by
  // this build. The parser tolerates unknown fields, so it verifies as
  // integrated -- and must still not cross, because nobody decided what
  // crossing does to it.
  const auto descriptor = Namespace() + "/module.gfmodule";
  QVector<QPair<QString, QByteArray>> members;
  ASSERT_TRUE(ReadDescriptorMembers(descriptor, members));

  QByteArray manifest;
  for (const auto& m : members) {
    if (m.first == Module::kModuleDescriptorManifestPath) manifest = m.second;
  }
  auto object = QJsonDocument::fromJson(manifest).object();
  object.insert("future_field", "something new");
  QByteArray extended;
  ASSERT_TRUE(Module::CanonicalJson(object, extended));

  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> pk{};
  std::array<unsigned char, crypto_sign_SECRETKEYBYTES> sk{};
  const auto seed = BuildSigningSeed();
  crypto_sign_seed_keypair(
      pk.data(), sk.data(),
      reinterpret_cast<const unsigned char*>(seed.constData()));
  QByteArray signature(crypto_sign_BYTES, '\0');
  crypto_sign_detached(
      reinterpret_cast<unsigned char*>(signature.data()), nullptr,
      reinterpret_cast<const unsigned char*>(extended.constData()),
      static_cast<unsigned long long>(extended.size()), sk.data());

  for (auto& m : members) {
    if (m.first == Module::kModuleDescriptorManifestPath) m.second = extended;
    if (m.first == Module::kModuleDescriptorSignaturePath) m.second = signature;
  }
  ASSERT_TRUE(QFile::remove(descriptor));
  ASSERT_TRUE(WriteDescriptorArchive(descriptor, members));
  ASSERT_TRUE(Module::VerifyModuleDescriptor(descriptor).ok)
      << "the fixture must verify, or this proves nothing";

  ExpectRefused(Externalize(output_root_), "future_field");
}

// ------------------------------------------------------------- the input

TEST_F(ModuleExternalizeTest, AnExternalModuleIsNotAnInput) {
  const auto first = Externalize(output_root_);
  ASSERT_TRUE(first.ok) << first.reason.toStdString();

  Module::ModuleExternalizeSpec spec;
  spec.input = first.output_namespace;
  spec.publisher_seed = RandomSeed();
  spec.output_root = dir_.path() + "/again";
  const auto again = Module::ExternalizeModule(spec);
  EXPECT_FALSE(again.ok);
  EXPECT_TRUE(again.reason.contains("not a verified integrated module"))
      << again.reason.toStdString();
}

TEST_F(ModuleExternalizeTest, AnUnboundInputIsRefused) {
  // Built with integrated binding off: the descriptor makes no claim about
  // its native, so there is nothing verified to carry forward.
  auto unbound = spec_;
  unbound.entry_binding = Module::ModuleBindingRequirement::kNOT_REQUIRED;
  ASSERT_TRUE(QFile::remove(Namespace() + "/module.gfmodule"));
  ASSERT_TRUE(BuildIntegrated(unbound));

  ExpectRefused(Externalize(output_root_), "not a verified integrated module");
}

TEST_F(ModuleExternalizeTest, AModifiedNativeIsRefused) {
  QFile native(Native());
  ASSERT_TRUE(native.open(QIODevice::Append));
  native.write("appended after sealing");
  native.close();

  ExpectRefused(Externalize(output_root_), "not a verified integrated module");
}

TEST_F(ModuleExternalizeTest, HelperNativesAreRefused) {
  const auto helper =
      Namespace() + "/native/" + Module::ModuleNativeFileName("gf_helper");
  ASSERT_TRUE(QFile::copy(Native(), helper));

  ExpectRefused(Externalize(output_root_), "does not bind");
}

TEST_F(ModuleExternalizeTest, AWrongNamespaceIsRefused) {
  const auto moved = input_root_ + "/" +
                     Module::ModuleDirectoryKey("com.example.somethingelse");
  ASSERT_TRUE(QDir().rename(Namespace(), moved));

  Module::ModuleExternalizeSpec spec;
  spec.input = moved;
  spec.publisher_seed = publisher_seed_;
  spec.output_root = output_root_;
  ExpectRefused(Module::ExternalizeModule(spec),
                "not a verified integrated module");
}

TEST_F(ModuleExternalizeTest, ATrailingSeparatorNamesTheSameNamespace) {
  // What shell completion produces. fileName() of `dir/` is empty, and the
  // namespace check used to refuse a correctly placed module over it.
  Module::ModuleExternalizeSpec spec;
  spec.input = Namespace() + "/";
  spec.publisher_seed = publisher_seed_;
  spec.output_root = output_root_ + "/";
  const auto result = Module::ExternalizeModule(spec);
  EXPECT_TRUE(result.ok) << result.reason.toStdString();
}

TEST_F(ModuleExternalizeTest, TheDescriptorPathIsAnInputToo) {
  Module::ModuleExternalizeSpec spec;
  spec.input = Namespace() + "/module.gfmodule";
  spec.publisher_seed = publisher_seed_;
  spec.output_root = output_root_;
  const auto result = Module::ExternalizeModule(spec);
  EXPECT_TRUE(result.ok) << result.reason.toStdString();
}

TEST_F(ModuleExternalizeTest, PublisherMetadataOnTheInputIsRefused) {
  // Only externalization sets it. An input already claiming a publisher is
  // not something to preserve or to overwrite.
  auto claimed = spec_;
  claimed.metadata.insert(Module::kModuleMetadataPublisher, "someone");
  ASSERT_TRUE(QFile::remove(Namespace() + "/module.gfmodule"));
  ASSERT_TRUE(BuildIntegrated(claimed));

  ExpectRefused(Externalize(output_root_, "me"), "only externalization");
}

// ------------------------------------------------------------- the key

TEST_F(ModuleExternalizeTest, TheBuildSeedIsNeverAPublisher) {
  Module::ModuleExternalizeSpec spec;
  spec.input = Namespace();
  spec.publisher_seed = BuildSigningSeed();
  spec.output_root = output_root_;
  ExpectRefused(Module::ExternalizeModule(spec), "never a publisher identity");
}

TEST_F(ModuleExternalizeTest, AnUnusableKeyIsRefused) {
  Module::ModuleExternalizeSpec spec;
  spec.input = Namespace();
  spec.publisher_seed = QByteArray(16, '\x01');
  spec.output_root = output_root_;
  ExpectRefused(Module::ExternalizeModule(spec), "no usable publisher key");
}

// ------------------------------------------------------------- the output

TEST_F(ModuleExternalizeTest, AnExistingOutputIsNeverOverwritten) {
  const auto first = Externalize(output_root_, "first");
  ASSERT_TRUE(first.ok) << first.reason.toStdString();
  const auto descriptor = first.output_namespace + "/module.gfmodule";
  const auto before = ReadFile(descriptor);

  const auto second = Externalize(output_root_, "second");
  EXPECT_FALSE(second.ok);
  EXPECT_TRUE(second.reason.contains("never overwritten"))
      << second.reason.toStdString();
  EXPECT_EQ(ReadFile(descriptor), before);
}

TEST_F(ModuleExternalizeTest, ALeftoverStagingDirectoryIsNotReused) {
  const auto staging = output_root_ + "/." +
                       Module::ModuleDirectoryKey(spec_.module_id) + ".staging";
  ASSERT_TRUE(QDir().mkpath(staging));

  const auto result = Externalize(output_root_);
  EXPECT_FALSE(result.ok);
  EXPECT_TRUE(result.reason.contains("left over"))
      << result.reason.toStdString();
  EXPECT_TRUE(QFileInfo::exists(staging))
      << "a directory this run did not create was removed";
}

TEST_F(ModuleExternalizeTest, TheOutputCannotBeInsideTheInput) {
  const auto result = Externalize(Namespace() + "/out");
  EXPECT_FALSE(result.ok);
  EXPECT_TRUE(result.reason.contains("inside the input"))
      << result.reason.toStdString();
  EXPECT_FALSE(QFileInfo::exists(Namespace() + "/out"))
      << "the refusal created a directory inside the input";
}

// ------------------------------------------------------------- end to end

TEST_F(ModuleExternalizeTest, TheHostAdmitsItOnlyAfterBothDecisions) {
  // The whole path a user's Host takes, short of mapping the library:
  // descriptor, publisher trust, per-module enable, namespace, binding.
  const auto result = Externalize(output_root_);
  ASSERT_TRUE(result.ok) << result.reason.toStdString();
  const auto descriptor = result.output_namespace + "/module.gfmodule";

  auto& manager = Module::ModuleManager::GetInstance();
  EXPECT_FALSE(
      manager.PrepareModule(descriptor, Module::ModuleOrigin::kEXTERNAL).ok)
      << "admitted before the publisher key was trusted";

  ASSERT_TRUE(Module::TrustModulePublisherKey(publisher_key_, {}));
  EXPECT_FALSE(
      manager.PrepareModule(descriptor, Module::ModuleOrigin::kEXTERNAL).ok)
      << "admitted before the module was enabled";

  ASSERT_TRUE(
      Module::SetExternalModuleEnabled(spec_.module_id, publisher_key_, true));
  const auto admitted =
      manager.PrepareModule(descriptor, Module::ModuleOrigin::kEXTERNAL);
  EXPECT_TRUE(admitted.ok) << "refused after both decisions were made";

  // And never as integrated, whatever the user decided.
  EXPECT_FALSE(
      manager.PrepareModule(descriptor, Module::ModuleOrigin::kINTEGRATED).ok);
}

// ------------------------------------------------------------- stateless

TEST(ModuleExternalizeBoundaryTest, TheTransformationReachesNoRuntimeState) {
  // A deterministic artifact transformation: no settings, no profile, no
  // trust store, no module runtime, no task system. Held on the sources,
  // because a link line cannot tell a call from a dependency.
  static const QRegularExpression kInclude(
      R"(^\s*#\s*include\s*[<"]([^>"]+)[>"])");
  const QStringList forbidden_prefixes{
      "core/function/GlobalSettingStation",
      "core/struct/settings_object/",
      "core/model/SettingsObject",
      "core/module/ModuleExternalTrust",
      "core/module/ModuleManager",
      "core/module/ModuleInit",
      "core/module/GlobalModuleContext",
      "core/thread/",
      "core/profile/",
  };
  const QStringList files{
      "src/core/module/ModuleExternalize.cpp",
      "src/core/module/ModuleExternalize.h",
      "src/core/module/ModulePublisherKey.cpp",
      "src/core/module/ModulePublisherKey.h",
      "src/tools/module_externalize/main.cpp",
  };

  QStringList offences;
  for (const auto& relative : files) {
    QFile file(QString(GF_TEST_SOURCE_DIR) + "/" + relative);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text))
        << relative.toStdString();
    int line_no = 0;
    while (!file.atEnd()) {
      const auto line = QString::fromUtf8(file.readLine());
      ++line_no;
      const auto m = kInclude.match(line);
      if (!m.hasMatch()) continue;
      for (const auto& prefix : forbidden_prefixes) {
        if (m.captured(1).startsWith(prefix)) {
          offences << QString("%1:%2: %3")
                          .arg(relative)
                          .arg(line_no)
                          .arg(m.captured(1));
        }
      }
    }
  }
  EXPECT_TRUE(offences.isEmpty())
      << "externalization reached into runtime state:\n"
      << offences.join("\n").toStdString();
}

}  // namespace GpgFrontend::Test
