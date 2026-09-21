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
#include <QFile>
#include <QSysInfo>
#include <QTemporaryDir>

#include "GpgFrontendTest.h"
#include "core/ModuleDescriptorArchive.h"
#include "core/ModuleTestPackages.h"
#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleDescriptorBuilder.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleExternalTrust.h"
#include "core/module/ModuleHostPolicy.h"
#include "core/module/ModuleNamespace.h"
#include "core/module/ModuleTrustRoot.h"
#include "core/utils/CommonUtils.h"
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestModuleExternalTrust.cpp
 * @brief The boundary a third-party module crosses, and who decides.
 *
 * None of this is reachable from CI, and saying so is the point of writing it
 * down here: the only signer in the pipeline is this build's own ephemeral
 * key, so no workflow can produce an external module and no gate exercises
 * these paths. They are covered here, by constructing the descriptors
 * directly with libsodium -- which is also why the packer's refusal to sign
 * with a foreign seed does not have to be loosened to test this.
 */

namespace GpgFrontend::Test {

namespace {

constexpr auto kManifestPath = "META-INF/manifest.json";
constexpr auto kSignaturePath = "META-INF/manifest.sig";
constexpr auto kBuildKeyPath = "META-INF/build-key.pub";

struct ForeignKeypair {
  QByteArray public_key;
  QByteArray secret_key;
};

/// A build key that is emphatically not this build's.
auto MakeForeignKeypair() -> ForeignKeypair {
  EnsureSodiumInit();
  ForeignKeypair kp;
  kp.public_key.resize(crypto_sign_PUBLICKEYBYTES);
  kp.secret_key.resize(crypto_sign_SECRETKEYBYTES);
  crypto_sign_keypair(reinterpret_cast<unsigned char*>(kp.public_key.data()),
                      reinterpret_cast<unsigned char*>(kp.secret_key.data()));
  return kp;
}

auto SignWith(const QByteArray& secret_key, const QByteArray& message)
    -> QByteArray {
  QByteArray signature(crypto_sign_BYTES, '\0');
  crypto_sign_detached(
      reinterpret_cast<unsigned char*>(signature.data()), nullptr,
      reinterpret_cast<const unsigned char*>(message.constData()),
      static_cast<unsigned long long>(message.size()),
      reinterpret_cast<const unsigned char*>(secret_key.constData()));
  return signature;
}

}  // namespace

/**
 * A descriptor of each shape, built from one honest manifest.
 *
 * The manifest bytes are produced by the real builder, so the JSON under test
 * is the JSON this project actually emits rather than something hand-rolled
 * to suit the test.
 */
class ModuleExternalTrustTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(dir_.isValid());
    ASSERT_TRUE(QDir(dir_.path()).mkpath("native"));
    native_root_.path = dir_.path() + "/native";

    entry_path_ = native_root_.path + "/" +
                  Module::ModuleNativeFileName("gf_mod_test_sentinel");
    ASSERT_TRUE(
        QFile::copy(QString::fromUtf8(GF_TEST_SENTINEL_FILE), entry_path_));

    // An ordinary integrated descriptor first: correct in every way, signed
    // by this build, carrying no key of its own.
    Module::ModuleDescriptorBuildSpec spec;
    spec.module_id = "com.example.module.external";
    spec.version = "1.0.0";
    spec.sdk_abi = GF_SDK_ABI_VERSION;
    spec.min_host_version = "2.0.0";
    spec.events = QStringList{"APPLICATION_LOADED"};
    spec.translation_context = "ModuleExternal";
    spec.metadata = {{"Name", "External"},
                     {"Description", "d"},
                     {"Author", "somebody else"}};
    spec.build_id = Module::ModuleBuildId();
    spec.build_timestamp = "2026-09-15T00:00:00Z";
    spec.build_source_commit = QString(40, '0');
    spec.platform_os = Module::ManifestHostOsName();
    spec.platform_arch = QSysInfo::currentCpuArchitecture();
    spec.platform_qt = QT_VERSION_STR;
    spec.entry_native_name = "gf_mod_test_sentinel";
    spec.entry_native_file = entry_path_;
    // External modules always require a binding, so an external descriptor
    // that omits one is refused later on those grounds rather than these.
    spec.entry_binding = Module::ModuleBindingRequirement::kREQUIRED;
    spec.signing_seed = BuildSigningSeed();
    spec.output_path = dir_.path() + "/integrated.gfmodule";

    const auto built = Module::BuildModuleDescriptor(spec);
    ASSERT_TRUE(built.ok) << built.reason.toStdString();
    manifest_bytes_ = built.manifest_bytes;
    module_id_ = spec.module_id;
    integrated_path_ = spec.output_path;

    foreign_ = MakeForeignKeypair();
    external_path_ = dir_.path() + "/external.gfmodule";
    ASSERT_TRUE(WriteExternal(foreign_, external_path_));
  }

  void TearDown() override {
    // Trust is persisted, so a case that grants it would otherwise decide the
    // next one. Withdrawn explicitly rather than left to ordering.
    Module::RevokeModuleBuildKey(foreign_.public_key);
    Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, false);
  }

  /// The same manifest, signed by @p kp, carrying @p kp's public half.
  [[nodiscard]] auto WriteExternal(const ForeignKeypair& kp,
                                   const QString& out) const -> bool {
    const QVector<QPair<QString, QByteArray>> members{
        {kManifestPath, manifest_bytes_},
        {kSignaturePath, SignWith(kp.secret_key, manifest_bytes_)},
        {kBuildKeyPath, kp.public_key},
    };
    return WriteDescriptorArchive(out, members);
  }

  QTemporaryDir dir_;
  Module::ModuleNativeRoot native_root_;
  QString entry_path_;
  QByteArray manifest_bytes_;
  QString module_id_;
  QString integrated_path_;
  QString external_path_;
  ForeignKeypair foreign_;
};

// ---------------------------------------------------------------------------
// The two shapes are disjoint, so origin cannot be spoofed by moving a file
// ---------------------------------------------------------------------------

TEST_F(ModuleExternalTrustTest, AnExternalDescriptorIsRefusedAsIntegrated) {
  // Dropping a third party's module into the application's own namespace must
  // not grant it the application's trust. It is refused by STRUCTURE -- it
  // carries a key, and an integrated descriptor may not -- so no policy
  // comparison has to remember to happen.
  const auto verdict = Module::VerifyModuleDescriptor(external_path_);
  EXPECT_FALSE(verdict.ok);
  EXPECT_EQ(verdict.status, Module::ModuleDescriptorStatus::kMALFORMED);
  EXPECT_TRUE(verdict.reason.contains("trust root"))
      << verdict.reason.toStdString();
}

TEST_F(ModuleExternalTrustTest, AnIntegratedDescriptorIsRefusedAsExternal) {
  // And the other direction, which matters just as much: an integrated
  // descriptor in the user's own directory carries no key, so there is
  // nothing to show anyone and nothing to decide about.
  const auto verdict = Module::VerifyExternalModuleDescriptor(integrated_path_);
  EXPECT_FALSE(verdict.ok);
  EXPECT_EQ(verdict.status, Module::ModuleDescriptorStatus::kMALFORMED);
  EXPECT_TRUE(verdict.reason.contains("exactly one build key"))
      << verdict.reason.toStdString();
}

// ---------------------------------------------------------------------------
// Verification says who signed it, and nothing about whether to run it
// ---------------------------------------------------------------------------

TEST_F(ModuleExternalTrustTest, VerificationNamesTheKeyWithoutTrustingIt) {
  const auto verdict = Module::VerifyExternalModuleDescriptor(external_path_);
  ASSERT_TRUE(verdict.ok) << verdict.reason.toStdString();

  // The key is reported so a person can be asked about it -- which is the
  // whole reason an external descriptor carries one.
  EXPECT_EQ(verdict.build_public_key, foreign_.public_key);
  EXPECT_NE(verdict.build_public_key, Module::ModuleBuildPublicKey());

  // Verifying it changed nothing about whether it is trusted.
  EXPECT_FALSE(Module::IsModuleBuildKeyTrusted(verdict.build_public_key));
  EXPECT_EQ(Module::ExternalModuleAuthorization(verdict.manifest.id,
                                                verdict.build_public_key),
            Module::ModuleAuthorizationState::kKEY_UNTRUSTED);
}

TEST_F(ModuleExternalTrustTest, ABuildIdFromAnotherBuildIsNotAnObstacle) {
  // build_id names the build tree that produced the module, which is somebody
  // else's. Comparing it to ours would refuse every external module there
  // could ever be, while asserting nothing.
  const auto verdict = Module::VerifyExternalModuleDescriptor(external_path_);
  ASSERT_TRUE(verdict.ok) << verdict.reason.toStdString();
  EXPECT_NE(verdict.status, Module::ModuleDescriptorStatus::kWRONG_BUILD);
}

TEST_F(ModuleExternalTrustTest, ATamperedExternalDescriptorIsRefused) {
  // Self-signed does not mean unchecked: the signature still has to match the
  // key that is sitting next to it.
  QVector<QPair<QString, QByteArray>> members;
  ASSERT_TRUE(ReadDescriptorMembers(external_path_, members));
  for (auto& m : members) {
    if (m.first == kManifestPath) m.second.replace("1.0.0", "9.9.9");
  }
  const auto tampered = dir_.path() + "/tampered.gfmodule";
  ASSERT_TRUE(WriteDescriptorArchive(tampered, members));

  const auto verdict = Module::VerifyExternalModuleDescriptor(tampered);
  EXPECT_FALSE(verdict.ok);
  EXPECT_EQ(verdict.status,
            Module::ModuleDescriptorStatus::kUNTRUSTED_BUILD_KEY);
}

TEST_F(ModuleExternalTrustTest, ASubstitutedKeyDoesNotRescueASignature) {
  // Swapping in a different key next to an unchanged signature must fail.
  // Otherwise "carries its own key" really would mean "verifies itself".
  const auto other = MakeForeignKeypair();
  QVector<QPair<QString, QByteArray>> members;
  ASSERT_TRUE(ReadDescriptorMembers(external_path_, members));
  for (auto& m : members) {
    if (m.first == kBuildKeyPath) m.second = other.public_key;
  }
  const auto swapped = dir_.path() + "/swapped.gfmodule";
  ASSERT_TRUE(WriteDescriptorArchive(swapped, members));

  const auto verdict = Module::VerifyExternalModuleDescriptor(swapped);
  EXPECT_FALSE(verdict.ok);
  EXPECT_EQ(verdict.status,
            Module::ModuleDescriptorStatus::kUNTRUSTED_BUILD_KEY);
}

// ---------------------------------------------------------------------------
// Two decisions, and neither one is the other
// ---------------------------------------------------------------------------

TEST_F(ModuleExternalTrustTest, TrustingAKeyDoesNotEnableAnyModule) {
  ASSERT_TRUE(Module::TrustModuleBuildKey(foreign_.public_key, "a label"));
  EXPECT_TRUE(Module::IsModuleBuildKeyTrusted(foreign_.public_key));

  EXPECT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kNOT_ENABLED)
      << "trusting a build key enabled a module by itself";
}

TEST_F(ModuleExternalTrustTest, EnablingAModuleDoesNotTrustItsKey) {
  ASSERT_TRUE(
      Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, true));

  EXPECT_FALSE(Module::IsModuleBuildKeyTrusted(foreign_.public_key));
  EXPECT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kKEY_UNTRUSTED)
      << "enabling a module bypassed the key decision";
}

TEST_F(ModuleExternalTrustTest, BothDecisionsTogetherAreWhatAdmitIt) {
  ASSERT_TRUE(Module::TrustModuleBuildKey(foreign_.public_key, {}));
  ASSERT_TRUE(
      Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, true));

  EXPECT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kTRUSTED_AND_ENABLED);
}

TEST_F(ModuleExternalTrustTest, RevokingAKeyReturnsItsModulesToPending) {
  ASSERT_TRUE(Module::TrustModuleBuildKey(foreign_.public_key, {}));
  ASSERT_TRUE(
      Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, true));
  ASSERT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kTRUSTED_AND_ENABLED);

  ASSERT_TRUE(Module::RevokeModuleBuildKey(foreign_.public_key));

  EXPECT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kKEY_UNTRUSTED)
      << "an approval outlived the key decision it rested on";
}

TEST_F(ModuleExternalTrustTest, AnApprovalIsNotInheritedByAReSignedModule) {
  // The same module id, signed under a different build key. Trust here is
  // build-key-specific on purpose: these keys are ephemeral and per build
  // tree, not durable identities, so a new one is a new decision.
  ASSERT_TRUE(Module::TrustModuleBuildKey(foreign_.public_key, {}));
  ASSERT_TRUE(
      Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, true));

  const auto rebuilt = MakeForeignKeypair();
  EXPECT_EQ(Module::ExternalModuleAuthorization(module_id_, rebuilt.public_key),
            Module::ModuleAuthorizationState::kKEY_UNTRUSTED)
      << "an approval granted for one build key covered another";

  Module::RevokeModuleBuildKey(rebuilt.public_key);
}

// ---------------------------------------------------------------------------
// Fingerprints are derived, never stored
// ---------------------------------------------------------------------------

TEST_F(ModuleExternalTrustTest, TheFingerprintComesFromTheKeyItself) {
  const auto fingerprint =
      Module::ModuleBuildKeyFingerprint(foreign_.public_key);
  EXPECT_FALSE(fingerprint.isEmpty());

  // Same key, same answer, every time and from nowhere else -- which is what
  // makes it safe to show one thing and compare another.
  EXPECT_EQ(fingerprint,
            Module::ModuleBuildKeyFingerprint(foreign_.public_key));

  const auto other = MakeForeignKeypair();
  EXPECT_NE(fingerprint, Module::ModuleBuildKeyFingerprint(other.public_key));

  // Every hex digit of the key is present, however it is grouped.
  auto ungrouped = fingerprint;
  ungrouped.remove(' ');
  EXPECT_EQ(ungrouped.toLower(),
            QString::fromLatin1(foreign_.public_key.toHex()));
}

TEST(ModuleExternalTrustRuleTest, AnEmptyKeyIsNeverTrusted) {
  // Without this, a descriptor whose key could not be read would compare
  // equal to a blank record and be admitted by whatever carried it.
  EXPECT_FALSE(Module::IsModuleBuildKeyTrusted({}));
  EXPECT_FALSE(Module::IsModuleBuildKeyTrusted(QByteArray(31, '\0')));
  EXPECT_FALSE(Module::TrustModuleBuildKey({}, "nothing"));
  EXPECT_TRUE(Module::ModuleBuildKeyFingerprint({}).isEmpty());
}

}  // namespace GpgFrontend::Test
