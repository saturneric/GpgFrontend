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

#include <QDir>
#include <QFile>
#include <QSysInfo>
#include <QTemporaryDir>

#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleDescriptorBuilder.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleHostPolicy.h"
#include "core/module/ModuleManifest.h"
#include "core/module/ModuleNamespace.h"
#include "core/module/ModuleTrustRoot.h"
#include "core/utils/BuildInfoUtils.h"
#include "sdk/GFSDKBuildInfo.h"
#include "test/core/ModuleTestPackages.h"

/**
 * @file GpgCoreTestModuleBindingPolicy.cpp
 * @brief Whether a descriptor must bind its entry, and who decides.
 *
 * Two questions this file keeps apart, because conflating them is the way
 * this design fails:
 *
 *   MAY a binding be absent?   origin + Host policy
 *   IS a present binding checked?  always, unconditionally
 *
 * Everything here drives the policy explicitly. Nothing inherits the build
 * default, because a test that inherits it stops asserting anything the
 * moment the default changes -- which is exactly what happened to the two
 * cases in the suites next door that now skip with a reason instead.
 */

namespace GpgFrontend::Test {

namespace {

constexpr Module::ModuleEntryTrustPolicy kIntegratedBound{
    Module::ModuleOrigin::kINTEGRATED,
    Module::ModuleBindingRequirement::kREQUIRED};

constexpr Module::ModuleEntryTrustPolicy kIntegratedUnbound{
    Module::ModuleOrigin::kINTEGRATED,
    Module::ModuleBindingRequirement::kNOT_REQUIRED};

/// An external module, asked for the weakest thing the type can express.
///
/// The second member says kNOT_REQUIRED and it makes no difference: external
/// modules always require a binding, and that is a property of
/// ModuleEntryTrustPolicy rather than of the call sites. Written this way on
/// purpose, so the test fails if anyone ever makes the knob apply to both.
constexpr Module::ModuleEntryTrustPolicy kExternalAskingForLess{
    Module::ModuleOrigin::kEXTERNAL,
    Module::ModuleBindingRequirement::kNOT_REQUIRED};

/// A descriptor plus a native directory, with the binding shape chosen here.
class ModuleBindingPolicyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(dir_.isValid());
    ASSERT_TRUE(QDir(dir_.path()).mkpath("native"));
    native_root_.path = dir_.path() + "/native";

    // A real loadable image, because the header check is one of the things
    // that must survive an absent binding -- and a file of 'm' bytes would be
    // refused by it for the wrong reason.
    source_native_ = QString::fromUtf8(GF_TEST_SENTINEL_FILE);
    ASSERT_TRUE(QFile::exists(source_native_))
        << "the sentinel library is missing: " << source_native_.toStdString();

    entry_path_ = native_root_.path + "/" +
                  Module::ModuleNativeFileName("gf_mod_test_sentinel");
    ASSERT_TRUE(QFile::copy(source_native_, entry_path_));
  }

  /// Build a descriptor for the native currently at @c entry_path_.
  [[nodiscard]] auto Build(Module::ModuleBindingRequirement binding)
      -> Module::ModuleManifest {
    Module::ModuleDescriptorBuildSpec spec;
    spec.module_id = "com.bktus.gpgfrontend.module.test";
    spec.version = "1.0.0";
    spec.sdk_abi = GF_SDK_ABI_VERSION;
    spec.min_host_version = "2.0.0";
    spec.events = QStringList{"APPLICATION_LOADED"};
    spec.translation_context = "ModuleTest";
    spec.metadata = {{"Name", "Test"}, {"Description", "d"}, {"Author", "a"}};
    spec.build_id = Module::ModuleBuildId();
    spec.build_timestamp = "2026-09-15T00:00:00Z";
    spec.build_source_commit = QString(40, '0');
    spec.platform_os = Module::ManifestHostOsName();
    spec.platform_arch = QSysInfo::currentCpuArchitecture();
    spec.platform_qt = QT_VERSION_STR;
    spec.entry_native_name = "gf_mod_test_sentinel";
    spec.entry_native_file = entry_path_;
    spec.entry_binding = binding;
    spec.signing_seed = BuildSigningSeed();
    spec.output_path = dir_.path() + "/module.gfmodule";

    descriptor_path_ = spec.output_path;

    const auto built = Module::BuildModuleDescriptor(spec);
    EXPECT_TRUE(built.ok) << built.reason.toStdString();

    const auto read = Module::VerifyModuleDescriptor(spec.output_path);
    EXPECT_TRUE(read.ok) << read.reason.toStdString();
    return read.manifest;
  }

  QTemporaryDir dir_;
  Module::ModuleNativeRoot native_root_;
  QString source_native_;
  QString entry_path_;
  QString descriptor_path_;
};

}  // namespace

// ---------------------------------------------------------------------------
// What "not required" does and does not mean
// ---------------------------------------------------------------------------

TEST_F(ModuleBindingPolicyTest, AnOmittedBindingIsAcceptedWhenNotRequired) {
  const auto manifest = Build(Module::ModuleBindingRequirement::kNOT_REQUIRED);
  ASSERT_FALSE(manifest.entry_native.verification.has_value())
      << "the builder wrote a binding nobody asked for";

  const auto entry = Module::ResolveAndVerifyNativeEntry(manifest, native_root_,
                                                         kIntegratedUnbound);
  EXPECT_TRUE(entry.ok) << entry.reason.toStdString();
  EXPECT_EQ(entry.path, QFileInfo(entry_path_).canonicalFilePath());
}

TEST_F(ModuleBindingPolicyTest, AnOmittedBindingIsRefusedWhenRequired) {
  const auto manifest = Build(Module::ModuleBindingRequirement::kNOT_REQUIRED);

  const auto entry = Module::ResolveAndVerifyNativeEntry(manifest, native_root_,
                                                         kIntegratedBound);
  EXPECT_FALSE(entry.ok);
  EXPECT_EQ(entry.status, Module::ModuleEntryStatus::kENTRY_BINDING_ABSENT);
  EXPECT_TRUE(entry.path.isEmpty())
      << "no path may come back, because a path is all a loader is given";
}

TEST_F(ModuleBindingPolicyTest, APresentBindingIsCheckedEvenWhenNotRequired) {
  // The load-bearing case. kNOT_REQUIRED says a descriptor MAY omit a
  // binding; it must never say a binding that is there can be ignored. If
  // this ever fails, "optional" has quietly come to mean "advisory".
  const auto manifest = Build(Module::ModuleBindingRequirement::kREQUIRED);
  ASSERT_TRUE(manifest.entry_native.verification.has_value());

  {
    QFile file(entry_path_);
    ASSERT_TRUE(file.open(QIODevice::ReadWrite));
    ASSERT_TRUE(file.seek(file.size() / 2));
    const auto original = file.read(1);
    ASSERT_TRUE(file.seek(file.size() / 2));
    const char flipped = static_cast<char>(original.at(0) ^ 1);
    ASSERT_EQ(file.write(&flipped, 1), 1);
  }

  const auto entry = Module::ResolveAndVerifyNativeEntry(manifest, native_root_,
                                                         kIntegratedUnbound);
  EXPECT_FALSE(entry.ok) << "a present binding was not checked";
  EXPECT_EQ(entry.status,
            Module::ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH);
}

// ---------------------------------------------------------------------------
// What survives an absent binding, and what does not
// ---------------------------------------------------------------------------

TEST_F(ModuleBindingPolicyTest, AMissingEntryIsStillRefusedWithNoBinding) {
  const auto manifest = Build(Module::ModuleBindingRequirement::kNOT_REQUIRED);
  ASSERT_TRUE(QFile::remove(entry_path_));

  const auto entry = Module::ResolveAndVerifyNativeEntry(manifest, native_root_,
                                                         kIntegratedUnbound);
  EXPECT_FALSE(entry.ok);
  EXPECT_EQ(entry.status, Module::ModuleEntryStatus::kMISSING_ENTRY_NATIVE);
}

TEST_F(ModuleBindingPolicyTest, ASymlinkedEntryIsStillRefusedWithNoBinding) {
  const auto manifest = Build(Module::ModuleBindingRequirement::kNOT_REQUIRED);

  // Pointing at the CORRECT library, so nothing but the link itself is wrong.
  const auto elsewhere = dir_.path() + "/elsewhere.so";
  ASSERT_TRUE(QFile::copy(entry_path_, elsewhere));
  ASSERT_TRUE(QFile::remove(entry_path_));
  ASSERT_TRUE(QFile::link(elsewhere, entry_path_));

  const auto entry = Module::ResolveAndVerifyNativeEntry(manifest, native_root_,
                                                         kIntegratedUnbound);
  EXPECT_FALSE(entry.ok);
  EXPECT_EQ(entry.status, Module::ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE);
  EXPECT_TRUE(entry.reason.contains("symlink")) << entry.reason.toStdString();
}

TEST_F(ModuleBindingPolicyTest, ADirectoryWearingTheNameIsStillRefused) {
  const auto manifest = Build(Module::ModuleBindingRequirement::kNOT_REQUIRED);
  ASSERT_TRUE(QFile::remove(entry_path_));
  ASSERT_TRUE(QDir().mkpath(entry_path_));

  const auto entry = Module::ResolveAndVerifyNativeEntry(manifest, native_root_,
                                                         kIntegratedUnbound);
  EXPECT_FALSE(entry.ok);
  EXPECT_EQ(entry.status, Module::ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE);
}

TEST_F(ModuleBindingPolicyTest, ATextFileWearingTheNameIsStillRefused) {
  const auto manifest = Build(Module::ModuleBindingRequirement::kNOT_REQUIRED);
  ASSERT_TRUE(QFile::remove(entry_path_));
  {
    QFile f(entry_path_);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("#!/bin/sh\nexit 0\n");
  }

  const auto entry = Module::ResolveAndVerifyNativeEntry(manifest, native_root_,
                                                         kIntegratedUnbound);
  EXPECT_FALSE(entry.ok);
  EXPECT_EQ(entry.status, Module::ModuleEntryStatus::kBAD_NATIVE_FILE_TYPE);
}

TEST_F(ModuleBindingPolicyTest, AStructurallyValidWrongNativeIsAccepted) {
  // The cost of the relaxed policy, asserted rather than implied.
  //
  // Both halves are one test on purpose. Read alone, the first would look
  // like a gap somebody should close; read alone, the second would look like
  // the binding is always there. Together they say what is actually true:
  // with no binding the checks are structural, and a well-formed image under
  // the expected name is indistinguishable from the intended one.
  //
  // If this ever starts failing because the unbound case is refused, the
  // refusal is the bug -- something has begun inspecting bytes it was told
  // not to.
  const auto unbound = Build(Module::ModuleBindingRequirement::kNOT_REQUIRED);
  const auto bound = Build(Module::ModuleBindingRequirement::kREQUIRED);

  // A different, entirely legitimate library, put where the entry belongs.
  const auto intruder = QString::fromUtf8(GF_TEST_SENTINEL_FILE);
  QFile::remove(entry_path_);
  ASSERT_TRUE(QFile::copy(intruder, entry_path_));
  {
    // Make it genuinely a different file, while keeping it a valid image.
    QFile f(entry_path_);
    ASSERT_TRUE(f.open(QIODevice::ReadWrite));
    ASSERT_TRUE(f.seek(f.size() - 1));
    const char b = 0x2A;
    ASSERT_EQ(f.write(&b, 1), 1);
  }

  const auto without = Module::ResolveAndVerifyNativeEntry(
      unbound, native_root_, kIntegratedUnbound);
  EXPECT_TRUE(without.ok)
      << "an unbound descriptor cannot tell one valid image from another, "
         "and claiming otherwise would overstate what it proves: "
      << without.reason.toStdString();

  const auto with = Module::ResolveAndVerifyNativeEntry(bound, native_root_,
                                                        kIntegratedBound);
  EXPECT_FALSE(with.ok) << "a bound descriptor must catch exactly this";
  EXPECT_EQ(with.status,
            Module::ModuleEntryStatus::kENTRY_VERIFICATION_MISMATCH);
}

// ---------------------------------------------------------------------------
// The binding policy must never weaken descriptor authentication
// ---------------------------------------------------------------------------

TEST_F(ModuleBindingPolicyTest, TheBindingPolicyCannotWeakenDescriptorTrust) {
  // The invariant, asserted against the policy that relaxes the most.
  //
  // kNOT_REQUIRED governs exactly one thing: whether the descriptor must also
  // bind the bytes of its entry native. It has no bearing on what proves the
  // descriptor itself, and this case exists so that a future change which
  // quietly routes authentication through the policy fails here rather than
  // in a release.
  const auto manifest = Build(Module::ModuleBindingRequirement::kNOT_REQUIRED);

  // Still signed by this build's embedded key -- there is no argument that
  // could make it otherwise, which is the structural half of the invariant.
  const auto verdict = Module::VerifyModuleDescriptor(descriptor_path_);
  ASSERT_TRUE(verdict.ok) << verdict.reason.toStdString();
  EXPECT_EQ(verdict.signer_public_key, Module::ModuleBuildPublicKey());

  // Still bound to this build's identity.
  EXPECT_EQ(verdict.manifest.build_id, Module::ModuleBuildId());

  // And the structural checks still ran: the entry resolved, which it only
  // does after name validation, containment, file type and image header.
  const auto entry = Module::ResolveAndVerifyNativeEntry(manifest, native_root_,
                                                         kIntegratedUnbound);
  EXPECT_TRUE(entry.ok) << entry.reason.toStdString();
}

TEST_F(ModuleBindingPolicyTest, AWrongBuildIdIsRefusedWhateverThePolicy) {
  // build_id equality is not negotiable for an integrated module, and is not
  // something the binding policy has an opinion about. Both policies, so a
  // future "OFF also relaxes this" cannot pass by only being tried one way.
  Module::ModuleDescriptorBuildSpec spec;
  spec.module_id = "com.bktus.gpgfrontend.module.test";
  spec.version = "1.0.0";
  spec.sdk_abi = GF_SDK_ABI_VERSION;
  spec.min_host_version = "2.0.0";
  spec.events = QStringList{"APPLICATION_LOADED"};
  spec.translation_context = "ModuleTest";
  spec.metadata = {{"Name", "T"}, {"Description", "d"}, {"Author", "a"}};
  spec.build_id = "gfb1-00000000000000000000000000000000";
  spec.build_timestamp = "2026-09-15T00:00:00Z";
  spec.build_source_commit = QString(40, '0');
  spec.platform_os = Module::ManifestHostOsName();
  spec.platform_arch = QSysInfo::currentCpuArchitecture();
  spec.platform_qt = QT_VERSION_STR;
  spec.entry_native_name = "gf_mod_test_sentinel";
  spec.entry_native_file = entry_path_;
  spec.signing_seed = BuildSigningSeed();

  for (const auto binding : {Module::ModuleBindingRequirement::kNOT_REQUIRED,
                             Module::ModuleBindingRequirement::kREQUIRED}) {
    spec.entry_binding = binding;
    spec.output_path = dir_.path() + "/otherbuild-" +
                       QString::number(static_cast<int>(binding)) + ".gfmodule";
    ASSERT_TRUE(Module::BuildModuleDescriptor(spec).ok);

    const auto verdict = Module::VerifyModuleDescriptor(spec.output_path);
    EXPECT_FALSE(verdict.ok);
    EXPECT_EQ(verdict.status, Module::ModuleDescriptorStatus::kWRONG_BUILD);
  }
}

// ---------------------------------------------------------------------------
// External modules
// ---------------------------------------------------------------------------

TEST_F(ModuleBindingPolicyTest, AnExternalModuleAlwaysRequiresABinding) {
  const auto manifest = Build(Module::ModuleBindingRequirement::kNOT_REQUIRED);

  const auto entry = Module::ResolveAndVerifyNativeEntry(
      manifest, native_root_, kExternalAskingForLess);
  EXPECT_FALSE(entry.ok)
      << "an external module was admitted with no binding at all";
  EXPECT_EQ(entry.status, Module::ModuleEntryStatus::kENTRY_BINDING_ABSENT);
  EXPECT_TRUE(entry.reason.contains("external")) << entry.reason.toStdString();
}

TEST(ModuleBindingPolicyRuleTest, ExternalOriginIgnoresTheIntegratedKnob) {
  // The rule lives in the type, not in its callers. If it ever moves out,
  // every call site becomes a place that can forget it.
  for (const auto requirement :
       {Module::ModuleBindingRequirement::kNOT_REQUIRED,
        Module::ModuleBindingRequirement::kREQUIRED}) {
    const Module::ModuleEntryTrustPolicy external{
        Module::ModuleOrigin::kEXTERNAL, requirement};
    EXPECT_TRUE(external.BindingRequired());
  }

  const Module::ModuleEntryTrustPolicy integrated_off{
      Module::ModuleOrigin::kINTEGRATED,
      Module::ModuleBindingRequirement::kNOT_REQUIRED};
  EXPECT_FALSE(integrated_off.BindingRequired());
}

TEST(ModuleBindingPolicyRuleTest, ADefaultConstructedPolicyIsTheStrictestOne) {
  // A call site that forgets to fill this in must be refused, not admitted.
  const Module::ModuleEntryTrustPolicy forgotten;
  EXPECT_TRUE(forgotten.BindingRequired());
  EXPECT_EQ(forgotten.origin, Module::ModuleOrigin::kEXTERNAL);
}

}  // namespace GpgFrontend::Test
