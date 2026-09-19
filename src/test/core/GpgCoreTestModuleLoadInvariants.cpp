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
#include <QFileInfo>
#include <QTemporaryDir>
#include <limits>

#include "GpgFrontendTest.h"
#include "core/ModuleTestPackages.h"
#include "core/module/Module.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleLoadStats.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModulePackageBuilder.h"
#include "core/module/ModulePackageVerifier.h"
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestModuleLoadInvariants.cpp
 * @brief Two properties of module loading that were previously held only by
 *        the shape of the code, and so could be lost silently.
 *
 * Both are about the WHOLE of one start rather than any one function, which is
 * why they read ModuleLoadStats -- the counters the loader already keeps --
 * instead of instrumenting the pipeline for the test's benefit.
 */

namespace GpgFrontend::Test {

namespace {

/// A real module library this build produced, which genuinely exports the
/// module api -- so it will load, and the only thing left to refuse it for is
/// what its manifest claims.
auto ARealModuleLibrary() -> QString {
  const QDir dir(QCoreApplication::applicationDirPath() + "/modules");
  const auto libs = dir.entryInfoList(
      QStringList{"libgf_mod_*.so", "libgf_mod_*.dylib", "gf_mod_*.dll"},
      QDir::Files, QDir::Size | QDir::Reversed);
  return libs.isEmpty() ? QString() : libs.first().absoluteFilePath();
}

}  // namespace

/**
 * A verified fact is established once and consumed, never worked out again.
 *
 * The general form of the rule rather than the one instance of it. The manager
 * used to re-scan manifest.files for the bin/ entry the verifier had already
 * located; instead of asserting that particular loop is gone, this asserts the
 * consequence any such re-derivation would have -- the package gets read more
 * than once.
 *
 * Measured as a delta around a single call, not as a startup total: the
 * counters are process-wide, and every other test that verifies a package of
 * its own contributes to them. A delta is attributable.
 */
TEST(ModuleLoadInvariantsTest, VerifyingAPackageReadsItExactlyOnce) {
  const auto packages = BuiltModulePackages();
  if (packages.isEmpty()) GTEST_SKIP() << "no module packages in this build";

  auto& stats = Module::ModuleLoadStats::GetInstance();

  // Startup hashes on this same counter, so let it finish before measuring.
  if (!stats.Summary().isEmpty()) WAIT_FOR_TRUE(stats.IsFinished(), 10000);

  // The measurement moved with the bytes. A descriptor no longer carries the
  // module binary, so verifying one hashes almost nothing; the entry native
  // is where the cost is, and resolving it is what gets measured.
  const auto largest = LargestBuiltModulePackage();
  ASSERT_FALSE(largest.isEmpty());

  const auto read = Module::VerifyModulePackage(largest);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  const Module::ModuleNativeRoot root{QFileInfo(largest).absolutePath()};
  const auto native = QDir(root.path).absoluteFilePath(
      Module::ModuleNativeFileName(read.manifest.entry_native.name));
  const auto size = QFileInfo(native).size();
  ASSERT_GT(size, 0);

  const auto before = stats.HashedBytes();
  const auto entry = Module::ResolveAndVerifyNativeEntry(read.manifest, root);
  ASSERT_TRUE(entry.ok) << entry.reason.toStdString();
  const auto delta = stats.HashedBytes() - before;

  // Exactly once over exactly the file. Not "at most", because there is no
  // container slack to allow for any more: the entry is one file, and hashing
  // it twice would double this precisely.
  EXPECT_EQ(delta, size) << "the entry native was not hashed exactly once";

  // And the fact itself is published, so nobody downstream needs to look for
  // it: this is what the manager consumes in place of its own search.
  EXPECT_EQ(read.manifest.entry_native.value.size(), 64);
  EXPECT_FALSE(read.manifest.entry_native.name.isEmpty());
}

/**
 * Native loading is serial, and now provably so.
 *
 * QLibrary::load() runs third-party static initialisers, and the host cannot
 * establish that one module's are safe against another's -- so phase two must
 * stay sequential. Nothing stopped a later refactor from parallelising that
 * loop, and nothing would have failed if it had.
 */
TEST(ModuleLoadInvariantsTest, NativeLoadingIsSerial) {
  const auto& stats = Module::ModuleLoadStats::GetInstance();
  if (stats.Summary().isEmpty()) {
    GTEST_SKIP() << "modules were not loaded in this run";
  }
  WAIT_FOR_TRUE(stats.IsFinished(), 10000);

  EXPECT_EQ(stats.PeakConcurrentNativeLoads(), 1)
      << "two modules were mapped at once; QLibrary::load() runs third-party "
         "static initialisers and phase two must stay serial";

  EXPECT_EQ(stats.NativeLoadThreadCount(), 1)
      << "native loads happened on more than one thread; even without overlap "
         "that gives a module's initialisers a different thread than the "
         "module runner they will be called on";
}

/**
 * A package cannot lie about which module it contains.
 *
 * The signature covers the manifest, so without this check it would cover a
 * NAME NOTHING ENFORCES: a package could say it is one module and carry
 * another, and everything downstream -- settings, activation, the module list,
 * any update decision -- would key off the binary's word for it instead.
 *
 * This refusal existed and was never exercised. The package below verifies
 * perfectly and materializes fine; the load is refused purely because the
 * binary introduces itself differently than the manifest does.
 */
TEST(ModuleLoadInvariantsTest, APackageCannotClaimAnIdentityItsBinaryDenies) {
  const auto library = ARealModuleLibrary();
  if (library.isEmpty()) GTEST_SKIP() << "this build has no module libraries";

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  Module::ModulePackageBuildSpec spec;
  // An id no module in this build reports, but a well-formed one -- so it is
  // the CROSS-CHECK that refuses this, not the identifier syntax rule.
  spec.module_id = "com.bktus.gpgfrontend.module.notwhatisinside";
  spec.version = "9.9.9";
  spec.sdk_abi = GF_SDK_ABI_VERSION;
  spec.min_host_version = "2.0.0";
  spec.capabilities = QStringList{"gpg"};
  // Required at schema 2. Empty is a legitimate statement and keeps this test
  // about identity rather than about subscriptions.
  spec.events = {};
  spec.translation_context = "ModuleImpostor";
  spec.metadata = {{"Name", "Impostor"}};
  spec.build_id = "test-build";
  spec.build_timestamp = "2026-09-15T00:00:00Z";
  spec.build_source_commit = QString(40, '0');
  spec.platform_arch = QSysInfo::currentCpuArchitecture();
  spec.platform_qt = QT_VERSION_STR;
  // Beside the descriptor, because that is where the Host resolves it from.
  const auto native =
      dir.path() + "/" + Module::ModuleNativeFileName("gf_mod_test_sentinel");
  ASSERT_TRUE(QFile::copy(library, native));
  spec.entry_native_name = "gf_mod_test_sentinel";
  spec.entry_native_file = native;
  spec.output_path = dir.path() + "/impostor.gfmodule";
  ASSERT_TRUE(Module::BuildModulePackage(spec).ok);

  auto& manager = Module::ModuleManager::GetInstance();

  // Phase one must SUCCEED: the package really is well-formed and correctly
  // signed. That is what makes this a test of the identity cross-check rather
  // than of verification.
  const auto candidate = manager.PrepareModule(spec.output_path, false);
  ASSERT_TRUE(candidate.ok)
      << "the impostor package should verify; only its claim is false";
  ASSERT_TRUE(candidate.manifest.has_value());
  EXPECT_EQ(candidate.manifest->id, spec.module_id);

  EXPECT_FALSE(manager.LoadPreparedModule(candidate))
      << "a package whose manifest names a different module than it carries "
         "must be refused";

  // And it must not have arrived under either name.
  EXPECT_EQ(manager.SearchModule(spec.module_id), nullptr);
}

/**
 * One set of facts, and in particular ONE answer to "is this packaged?".
 *
 * The controller dialog derived it from whether a manifest was present, while
 * the module list asked Module::IsPackaged(). Both were right, and they agreed
 * only because one setter call happened to bridge them -- so the day a module
 * acquired a manifest without being packaged, or the reverse, two panels in
 * the same dialog would have disagreed about the same module.
 */
TEST(ModuleLoadInvariantsTest, ProvenanceAnswersPackagedExactlyOnce) {
  auto& manager = Module::ModuleManager::GetInstance();

  const auto ids = manager.ListAllRegisteredModuleID();
  if (ids.isEmpty()) GTEST_SKIP() << "no modules registered in this run";

  for (const auto& id : ids) {
    const auto p = manager.GetModuleProvenance(id);
    ASSERT_EQ(p.identifier, id);

    auto module = manager.SearchModule(id);
    ASSERT_NE(module, nullptr);

    // The single source, and the two answers it replaced.
    EXPECT_EQ(p.packaged, module->IsPackaged()) << id.toStdString();
    EXPECT_EQ(p.packaged, p.manifest.has_value())
        << id.toStdString()
        << ": the two ways this used to be derived now have to agree";

    // A packaged module's metadata is the VERIFIED manifest's, which is what
    // lets the panel show it as fact rather than as the module's own claim.
    if (p.packaged) {
      EXPECT_EQ(p.metadata, p.manifest->metadata) << id.toStdString();
      EXPECT_TRUE(p.source_package_path.endsWith(Module::kModulePackageSuffix))
          << id.toStdString() << ": " << p.source_package_path.toStdString();
    }

    // Never the ephemeral path the image was mapped through.
    EXPECT_FALSE(p.source_package_path.startsWith("/proc/self/fd/"))
        << id.toStdString();

    // The module's own ABI, not the host's -- it is the number the loader
    // negotiated with this particular binary.
    EXPECT_GT(p.sdk_abi, 0) << id.toStdString();
  }
}

}  // namespace GpgFrontend::Test
