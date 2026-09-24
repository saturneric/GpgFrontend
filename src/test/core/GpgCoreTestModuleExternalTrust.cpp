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
#include "core/module/ModuleManager.h"
#include "core/module/ModuleNamespace.h"
#include "core/module/ModulePublisherKey.h"
#include "core/module/ModuleTrustRoot.h"
#include "core/utils/CommonUtils.h"
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestModuleExternalTrust.cpp
 * @brief The boundary a third-party module crosses, and who decides.
 *
 * External descriptors are produced here the way gf_module_externalize
 * produces them -- by the real builder with origin kEXTERNAL and a publisher
 * seed -- so what is verified is what the tool actually emits. Only the
 * deliberately malformed shapes are assembled by hand.
 */

namespace GpgFrontend::Test {

namespace {

constexpr auto kManifestPath = "META-INF/manifest.json";
constexpr auto kSignaturePath = "META-INF/manifest.sig";
constexpr auto kPublisherKeyPath = "META-INF/publisher.pub";

/// A build id that is emphatically not this build's.
constexpr auto kForeignBuildId = "gfb1-ffffffffffffffffffffffffffffffff";

struct ForeignKeypair {
  QByteArray seed;
  QByteArray public_key;
  QByteArray secret_key;
};

auto KeypairFromSeed(const QByteArray& seed) -> ForeignKeypair {
  EnsureSodiumInit();
  ForeignKeypair kp;
  kp.seed = seed;
  kp.public_key.resize(crypto_sign_PUBLICKEYBYTES);
  kp.secret_key.resize(crypto_sign_SECRETKEYBYTES);
  crypto_sign_seed_keypair(
      reinterpret_cast<unsigned char*>(kp.public_key.data()),
      reinterpret_cast<unsigned char*>(kp.secret_key.data()),
      reinterpret_cast<const unsigned char*>(seed.constData()));
  return kp;
}

/// A publisher key that is emphatically not this build's.
auto MakeForeignKeypair() -> ForeignKeypair {
  EnsureSodiumInit();
  QByteArray seed(crypto_sign_SEEDBYTES, '\0');
  randombytes_buf(seed.data(), static_cast<size_t>(seed.size()));
  return KeypairFromSeed(seed);
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
 * A descriptor of each shape, both from the real builder.
 *
 * The external one names a build other than this one, as any real external
 * module does; the build id is provenance there, and must not matter.
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
    spec_ = spec;

    foreign_ = MakeForeignKeypair();
    external_path_ = dir_.path() + "/external.gfmodule";
    ASSERT_TRUE(BuildExternal(foreign_, kForeignBuildId, external_path_));
  }

  void TearDown() override {
    // Trust is persisted, so a case that grants it would otherwise decide the
    // next one. Withdrawn explicitly rather than left to ordering.
    Module::RevokeModulePublisherKey(foreign_.public_key);
    Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, false);
  }

  /// The fixture's module as an external descriptor, publisher-signed.
  [[nodiscard]] auto BuildExternal(const ForeignKeypair& kp,
                                   const QString& build_id,
                                   const QString& out) const -> bool {
    auto spec = spec_;
    spec.origin = Module::ModuleOrigin::kEXTERNAL;
    spec.build_id = build_id;
    spec.signing_seed = kp.seed;
    spec.output_path = out;
    const auto built = Module::BuildModuleDescriptor(spec);
    EXPECT_TRUE(built.ok) << built.reason.toStdString();
    return built.ok;
  }

  /// The integrated manifest, signed by @p kp, carrying @p key_member at
  /// @p key_path -- the shapes the builder refuses to produce.
  [[nodiscard]] auto WriteHandSigned(const ForeignKeypair& kp,
                                     const QString& key_path,
                                     const QByteArray& key_member,
                                     const QString& out) const -> bool {
    const QVector<QPair<QString, QByteArray>> members{
        {kManifestPath, manifest_bytes_},
        {kSignaturePath, SignWith(kp.secret_key, manifest_bytes_)},
        {key_path, key_member},
    };
    return WriteDescriptorArchive(out, members);
  }

  QTemporaryDir dir_;
  Module::ModuleDescriptorBuildSpec spec_;
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
  EXPECT_TRUE(verdict.reason.contains("exactly one publisher key"))
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
  EXPECT_EQ(verdict.signer_public_key, foreign_.public_key);
  EXPECT_NE(verdict.signer_public_key, Module::ModuleBuildPublicKey());

  // Verifying it changed nothing about whether it is trusted.
  EXPECT_FALSE(Module::IsModulePublisherKeyTrusted(verdict.signer_public_key));
  EXPECT_EQ(Module::ExternalModuleAuthorization(verdict.manifest.id,
                                                verdict.signer_public_key),
            Module::ModuleAuthorizationState::kPUBLISHER_UNTRUSTED);
}

TEST_F(ModuleExternalTrustTest, ABuildIdFromAnotherBuildIsNotAnObstacle) {
  // build_id names the build tree that produced the module, which is somebody
  // else's. Comparing it to ours would refuse every external module there
  // could ever be, while asserting nothing.
  const auto verdict = Module::VerifyExternalModuleDescriptor(external_path_);
  ASSERT_TRUE(verdict.ok) << verdict.reason.toStdString();
  EXPECT_EQ(verdict.manifest.build_id, kForeignBuildId);
  EXPECT_NE(verdict.status, Module::ModuleDescriptorStatus::kWRONG_BUILD);
}

TEST_F(ModuleExternalTrustTest,
       NamingThisBuildsIdGainsAnExternalModuleNothing) {
  // Provenance only, in both directions: a publisher cannot borrow this
  // Host's integrated trust by stamping its build id. The module still waits
  // for both of the user's decisions, exactly as a foreign-id one does.
  const auto ours = dir_.path() + "/ours.gfmodule";
  ASSERT_TRUE(BuildExternal(foreign_, Module::ModuleBuildId(), ours));

  const auto verdict = Module::VerifyExternalModuleDescriptor(ours);
  ASSERT_TRUE(verdict.ok) << verdict.reason.toStdString();
  EXPECT_EQ(verdict.manifest.build_id, Module::ModuleBuildId());
  EXPECT_EQ(Module::ExternalModuleAuthorization(verdict.manifest.id,
                                                verdict.signer_public_key),
            Module::ModuleAuthorizationState::kPUBLISHER_UNTRUSTED);
  EXPECT_FALSE(Module::VerifyModuleDescriptor(ours).ok)
      << "an external descriptor verified as integrated";
}

TEST_F(ModuleExternalTrustTest, TheBuildKeyIsNeverAcceptedAsAPublisher) {
  // An integrated manifest, signed by this build, with this build's key
  // added as the "publisher". Structurally external, cryptographically
  // sound -- and refused, because the build key is never a publisher
  // identity and must not be presentable as one by adding a file.
  const auto build = KeypairFromSeed(BuildSigningSeed());
  ASSERT_EQ(build.public_key, Module::ModuleBuildPublicKey());

  const auto out = dir_.path() + "/buildkey.gfmodule";
  ASSERT_TRUE(WriteHandSigned(build, kPublisherKeyPath, build.public_key, out));

  const auto verdict = Module::VerifyExternalModuleDescriptor(out);
  EXPECT_FALSE(verdict.ok);
  EXPECT_EQ(verdict.status, Module::ModuleDescriptorStatus::kMALFORMED);
  EXPECT_TRUE(verdict.reason.contains("never a publisher identity"))
      << verdict.reason.toStdString();
}

TEST_F(ModuleExternalTrustTest, TheBuilderRefusesTheBuildSeedAsAPublisher) {
  auto spec = spec_;
  spec.origin = Module::ModuleOrigin::kEXTERNAL;
  spec.signing_seed = BuildSigningSeed();
  spec.output_path = dir_.path() + "/buildseed.gfmodule";

  const auto built = Module::BuildModuleDescriptor(spec);
  EXPECT_FALSE(built.ok);
  EXPECT_TRUE(built.reason.contains("never a publisher identity"))
      << built.reason.toStdString();
  EXPECT_FALSE(QFile::exists(spec.output_path));
}

TEST_F(ModuleExternalTrustTest, TheBuilderRefusesAnUnboundExternalEntry) {
  // The Host always demands a binding of an external module, so one without
  // it would verify and then never load. It is not produced at all.
  auto spec = spec_;
  spec.origin = Module::ModuleOrigin::kEXTERNAL;
  spec.entry_binding = Module::ModuleBindingRequirement::kNOT_REQUIRED;
  spec.signing_seed = foreign_.seed;
  spec.output_path = dir_.path() + "/unbound.gfmodule";

  const auto built = Module::BuildModuleDescriptor(spec);
  EXPECT_FALSE(built.ok);
  EXPECT_FALSE(QFile::exists(spec.output_path));
}

TEST_F(ModuleExternalTrustTest, TheExternalBuilderCarriesThePublisherKey) {
  QVector<QPair<QString, QByteArray>> members;
  ASSERT_TRUE(ReadDescriptorMembers(external_path_, members));

  QByteArray carried;
  for (const auto& m : members) {
    if (m.first == kPublisherKeyPath) carried = m.second;
    EXPECT_NE(m.first, QString("META-INF/build-key.pub"));
  }
  EXPECT_EQ(carried, foreign_.public_key);
}

TEST_F(ModuleExternalTrustTest, TheOldKeyMemberIsRefusedByName) {
  // The pre-rename spelling. Refused as a format error, by name, rather than
  // tolerated or surfacing as an anonymous undeclared member.
  const auto out = dir_.path() + "/legacy.gfmodule";
  ASSERT_TRUE(WriteHandSigned(foreign_, "META-INF/build-key.pub",
                              foreign_.public_key, out));

  for (const auto& verdict : {Module::VerifyExternalModuleDescriptor(out),
                              Module::VerifyModuleDescriptor(out)}) {
    EXPECT_FALSE(verdict.ok);
    EXPECT_EQ(verdict.status, Module::ModuleDescriptorStatus::kMALFORMED);
    EXPECT_TRUE(verdict.reason.contains("older format"))
        << verdict.reason.toStdString();
  }
}

TEST_F(ModuleExternalTrustTest, AnUnknownMetaInfMemberIsRefused) {
  QVector<QPair<QString, QByteArray>> members;
  ASSERT_TRUE(ReadDescriptorMembers(external_path_, members));
  members.append({"META-INF/extra", QByteArray("x")});
  const auto out = dir_.path() + "/extra.gfmodule";
  ASSERT_TRUE(WriteDescriptorArchive(out, members));

  const auto verdict = Module::VerifyExternalModuleDescriptor(out);
  EXPECT_FALSE(verdict.ok);
  EXPECT_EQ(verdict.status, Module::ModuleDescriptorStatus::kMALFORMED);
  EXPECT_TRUE(verdict.reason.contains("META-INF/extra"))
      << verdict.reason.toStdString();
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
    if (m.first == kPublisherKeyPath) m.second = other.public_key;
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
  ASSERT_TRUE(Module::TrustModulePublisherKey(foreign_.public_key, "a label"));
  EXPECT_TRUE(Module::IsModulePublisherKeyTrusted(foreign_.public_key));

  EXPECT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kNOT_ENABLED)
      << "trusting a publisher key enabled a module by itself";
}

TEST_F(ModuleExternalTrustTest, EnablingAModuleDoesNotTrustItsKey) {
  ASSERT_TRUE(
      Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, true));

  EXPECT_FALSE(Module::IsModulePublisherKeyTrusted(foreign_.public_key));
  EXPECT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kPUBLISHER_UNTRUSTED)
      << "enabling a module bypassed the key decision";
}

TEST_F(ModuleExternalTrustTest, BothDecisionsTogetherAreWhatAdmitIt) {
  ASSERT_TRUE(Module::TrustModulePublisherKey(foreign_.public_key, {}));
  ASSERT_TRUE(
      Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, true));

  EXPECT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kTRUSTED_AND_ENABLED);
}

TEST_F(ModuleExternalTrustTest, RevokingAKeyReturnsItsModulesToPending) {
  ASSERT_TRUE(Module::TrustModulePublisherKey(foreign_.public_key, {}));
  ASSERT_TRUE(
      Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, true));
  ASSERT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kTRUSTED_AND_ENABLED);

  ASSERT_TRUE(Module::RevokeModulePublisherKey(foreign_.public_key));

  EXPECT_EQ(
      Module::ExternalModuleAuthorization(module_id_, foreign_.public_key),
      Module::ModuleAuthorizationState::kPUBLISHER_UNTRUSTED)
      << "an approval outlived the key decision it rested on";
}

TEST_F(ModuleExternalTrustTest, AnApprovalIsNotInheritedByAReSignedModule) {
  // The same module id, signed under a different publisher key. Trust is
  // specific to one exact key: there is no rotation or succession, so a new
  // key is a new decision.
  ASSERT_TRUE(Module::TrustModulePublisherKey(foreign_.public_key, {}));
  ASSERT_TRUE(
      Module::SetExternalModuleEnabled(module_id_, foreign_.public_key, true));

  const auto rebuilt = MakeForeignKeypair();
  EXPECT_EQ(Module::ExternalModuleAuthorization(module_id_, rebuilt.public_key),
            Module::ModuleAuthorizationState::kPUBLISHER_UNTRUSTED)
      << "an approval granted for one publisher key covered another";

  Module::RevokeModulePublisherKey(rebuilt.public_key);
}

// ---------------------------------------------------------------------------
// Fingerprints are derived, never stored
// ---------------------------------------------------------------------------

TEST_F(ModuleExternalTrustTest, TheFingerprintComesFromTheKeyItself) {
  const auto fingerprint =
      Module::ModulePublisherKeyFingerprint(foreign_.public_key);
  EXPECT_FALSE(fingerprint.isEmpty());

  // Same key, same answer, every time and from nowhere else -- which is what
  // makes it safe to show one thing and compare another.
  EXPECT_EQ(fingerprint,
            Module::ModulePublisherKeyFingerprint(foreign_.public_key));

  const auto other = MakeForeignKeypair();
  EXPECT_NE(fingerprint,
            Module::ModulePublisherKeyFingerprint(other.public_key));

  // Every hex digit of the key is present, however it is grouped.
  auto ungrouped = fingerprint;
  ungrouped.remove(' ');
  EXPECT_EQ(ungrouped.toLower(),
            QString::fromLatin1(foreign_.public_key.toHex()));
}

// An id is owned by one trust origin. Everything keyed by it -- settings,
// the secure cache, commands -- would otherwise belong to whichever package
// was scanned first, and the scan order is a directory listing.
TEST_F(ModuleExternalTrustTest, AnExternalPackageMayNotClaimAnIntegratedId) {
  auto& manager = Module::ModuleManager::GetInstance();

  // An id an integrated module actually has...
  const auto shadowing = manager.PrepareModule(
      external_path_, Module::ModuleOrigin::kEXTERNAL, {module_id_});
  EXPECT_FALSE(shadowing.ok);

  // ...and the project's reserved namespace, whether or not anything
  // integrated uses the id today.
  auto spec = spec_;
  spec.module_id = "com.bktus.gpgfrontend.module.impostor";
  spec.origin = Module::ModuleOrigin::kEXTERNAL;
  spec.build_id = kForeignBuildId;
  spec.signing_seed = foreign_.seed;
  spec.output_path = dir_.path() + "/impostor.gfmodule";
  ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);
  const auto reserved = manager.PrepareModule(
      spec.output_path, Module::ModuleOrigin::kEXTERNAL, {});
  EXPECT_FALSE(reserved.ok);

  bool recorded = false;
  for (const auto& r : manager.ListModuleRefusals()) {
    if (r.descriptor_path == spec.output_path) {
      recorded = true;
      EXPECT_FALSE(r.pending_user_action) << "nothing the user can approve";
    }
  }
  EXPECT_TRUE(recorded);
}

TEST(ModuleExternalTrustRuleTest, AnEmptyKeyIsNeverTrusted) {
  // Without this, a descriptor whose key could not be read would compare
  // equal to a blank record and be admitted by whatever carried it.
  EXPECT_FALSE(Module::IsModulePublisherKeyTrusted({}));
  EXPECT_FALSE(Module::IsModulePublisherKeyTrusted(QByteArray(31, '\0')));
  EXPECT_FALSE(Module::TrustModulePublisherKey({}, "nothing"));
  EXPECT_TRUE(Module::ModulePublisherKeyFingerprint({}).isEmpty());
}

}  // namespace GpgFrontend::Test
