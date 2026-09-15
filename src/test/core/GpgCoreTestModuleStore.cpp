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
#include <QTemporaryDir>

#include "GpgFrontendTest.h"
#include "core/module/ModulePackageBuilder.h"
#include "core/module/ModuleStore.h"
#include "sdk/GFSDKBuildInfo.h"

/**
 * @file GpgCoreTestModuleStore.cpp
 * @brief Install, update, rollback and immutability, as executable rules.
 *
 * The store's job is that what gets loaded is a tree whose digests were
 * re-checked against a signed manifest a moment ago -- so most of these are
 * about what happens when that stops being true.
 */

namespace GpgFrontend::Test {

namespace {

auto HostOs() -> QString {
#if defined(Q_OS_WIN)
  return "windows";
#elif defined(Q_OS_MACOS)
  return "macos";
#else
  return "linux";
#endif
}

/// A package that installs cleanly. `body` distinguishes two builds of the
/// same version, which is exactly the case the store keys on a digest for.
auto BuildPackage(const QString& dir, const QString& name,
                  const QString& version, const QByteArray& body)
    -> QString {
  Module::ModulePackageBuildSpec spec;
  spec.module_id = "com.bktus.gpgfrontend.module.store_test";
  spec.version = version;
  spec.sdk_abi = GF_SDK_ABI_VERSION;
  spec.min_host_version = "2.0.0";
  spec.capabilities = QStringList{"gpg"};
  spec.metadata = {{"Name", "Store Test"}};
  spec.build_id = "test";
  spec.build_timestamp = "2026-09-15T00:00:00Z";
  spec.build_source_commit = "0";
  spec.platform_os = HostOs();
  spec.platform_arch = QSysInfo::currentCpuArchitecture();
  spec.platform_qt = QT_VERSION_STR;
  spec.files = {{"bin/module.so", {}, body},
                {"resources/note.txt", {}, QByteArray("note")}};
  spec.output_path = dir + "/" + name;

  const auto built = Module::BuildModulePackage(spec);
  return built.ok ? spec.output_path : QString();
}

class ModuleStoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(dir_.isValid());
    store_ = Module::ModuleStoreRoot(dir_.path() + "/mods");
    ASSERT_FALSE(store_.isEmpty());
  }

  [[nodiscard]] auto Pkg(const QString& name, const QString& version,
                         const QByteArray& body) const -> QString {
    return BuildPackage(dir_.path(), name, version, body);
  }

  static constexpr auto* kId = "com.bktus.gpgfrontend.module.store_test";

  QTemporaryDir dir_;
  QString store_;
};

}  // namespace

TEST_F(ModuleStoreTest, InstallsAndResolves) {
  const auto pkg = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  ASSERT_FALSE(pkg.isEmpty());

  const auto installed = Module::InstallModulePackage(pkg, store_);
  ASSERT_TRUE(installed.ok) << installed.reason.toStdString();
  EXPECT_FALSE(installed.already_installed);
  EXPECT_EQ(installed.manifest.id, kId);
  EXPECT_TRUE(QFile::exists(installed.library_path));

  const auto resolved = Module::ResolveInstalledModule(store_, kId);
  ASSERT_TRUE(resolved.ok) << resolved.reason.toStdString();
  EXPECT_EQ(resolved.install_dir, installed.install_dir);
  EXPECT_EQ(resolved.manifest.version, "1.0.0");

  EXPECT_EQ(Module::ListInstalledModules(store_), QStringList{kId});
}

TEST_F(ModuleStoreTest, InstallingTheSameBytesTwiceChangesNothing) {
  const auto pkg = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  const auto first = Module::InstallModulePackage(pkg, store_);
  ASSERT_TRUE(first.ok);

  const auto second = Module::InstallModulePackage(pkg, store_);
  ASSERT_TRUE(second.ok);
  EXPECT_TRUE(second.already_installed);
  EXPECT_EQ(second.install_dir, first.install_dir);
}

TEST_F(ModuleStoreTest, TwoBuildsOfOneVersionAreTwoInstalls) {
  // The version string is what a human writes; the digest is what the bytes
  // are. Keying on the string alone would let a rebuilt package silently
  // become the installed one without anything being written.
  const auto one = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  const auto two = Pkg("b.gfmodule", "1.0.0", QByteArray(2048, 'b'));

  const auto first = Module::InstallModulePackage(one, store_);
  const auto second = Module::InstallModulePackage(two, store_);
  ASSERT_TRUE(first.ok);
  ASSERT_TRUE(second.ok);

  EXPECT_NE(first.install_dir, second.install_dir);
  EXPECT_FALSE(second.already_installed);
  EXPECT_EQ(Module::ResolveInstalledModule(store_, kId).install_dir,
            second.install_dir);
}

TEST_F(ModuleStoreTest, RollbackReturnsToThePreviousVersion) {
  const auto v1 = Pkg("v1.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  const auto v2 = Pkg("v2.gfmodule", "2.0.0", QByteArray(2048, 'b'));

  const auto first = Module::InstallModulePackage(v1, store_);
  ASSERT_TRUE(first.ok);
  const auto second = Module::InstallModulePackage(v2, store_);
  ASSERT_TRUE(second.ok);
  ASSERT_EQ(Module::ResolveInstalledModule(store_, kId).manifest.version,
            "2.0.0");

  const auto back = Module::RollbackInstalledModule(store_, kId);
  ASSERT_TRUE(back.ok) << back.reason.toStdString();
  EXPECT_EQ(back.install_dir, first.install_dir);
  EXPECT_EQ(Module::ResolveInstalledModule(store_, kId).manifest.version,
            "1.0.0");

  // And a rollback can itself be undone, which matters when the version you
  // rolled back from was not the problem after all.
  const auto forward = Module::RollbackInstalledModule(store_, kId);
  ASSERT_TRUE(forward.ok);
  EXPECT_EQ(forward.install_dir, second.install_dir);
}

TEST_F(ModuleStoreTest, ThereIsNothingToRollBackToAtFirst) {
  const auto pkg = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  ASSERT_TRUE(Module::InstallModulePackage(pkg, store_).ok);

  const auto back = Module::RollbackInstalledModule(store_, kId);
  EXPECT_FALSE(back.ok);
  EXPECT_EQ(back.status, Module::ModulePackageStatus::kNOT_INSTALLED);

  // And the installed version is untouched by the attempt.
  EXPECT_EQ(Module::ResolveInstalledModule(store_, kId).manifest.version,
            "1.0.0");
}

TEST_F(ModuleStoreTest, AModuleThatIsNotInstalledResolvesToNothing) {
  const auto resolved = Module::ResolveInstalledModule(store_, kId);
  EXPECT_FALSE(resolved.ok);
  EXPECT_EQ(resolved.status, Module::ModulePackageStatus::kNOT_INSTALLED);
  EXPECT_TRUE(Module::ListInstalledModules(store_).isEmpty());
}

// ------------------------------------------------------------ immutability

TEST_F(ModuleStoreTest, InstalledFilesAreReadOnly) {
  // Advisory, and only that: the user owns these files. What it prevents is an
  // accident, not an adversary. The test below is the part that holds.
  const auto pkg = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  const auto installed = Module::InstallModulePackage(pkg, store_);
  ASSERT_TRUE(installed.ok);

  auto checked = 0;
  QDirIterator it(installed.install_dir, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const auto path = it.next();
    EXPECT_FALSE(QFile::permissions(path).testFlag(QFileDevice::WriteOwner))
        << path.toStdString();
    ++checked;
  }
  EXPECT_GT(checked, 0);
}

TEST_F(ModuleStoreTest, ATamperedInstallIsRefusedBeforeItCanBeLoaded) {
  // The enforceable half of immutability: a change to an installed module is
  // detected before anything loads it. Note the test has to put the write
  // permission back first -- which is the point about read-only being
  // advisory, demonstrated rather than argued.
  const auto pkg = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  const auto installed = Module::InstallModulePackage(pkg, store_);
  ASSERT_TRUE(installed.ok);
  ASSERT_TRUE(Module::ResolveInstalledModule(store_, kId).ok);

  const auto binary = installed.library_path;
  ASSERT_TRUE(QFile::setPermissions(
      binary, QFile::permissions(binary) | QFileDevice::WriteOwner));
  {
    QFile f(binary);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(2048, 'z'));
  }

  const auto resolved = Module::ResolveInstalledModule(store_, kId);
  EXPECT_FALSE(resolved.ok);
  EXPECT_EQ(resolved.status,
            Module::ModulePackageStatus::kFILE_DIGEST_MISMATCH);
  EXPECT_TRUE(resolved.library_path.isEmpty());
}

TEST_F(ModuleStoreTest, AFileAddedToAnInstallIsRefused) {
  // The appended-payload case, after installation rather than inside the
  // package: a file nothing in the signed manifest vouches for.
  const auto pkg = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  const auto installed = Module::InstallModulePackage(pkg, store_);
  ASSERT_TRUE(installed.ok);

  QFile extra(installed.install_dir + "/bin/extra.so");
  ASSERT_TRUE(extra.open(QIODevice::WriteOnly));
  extra.write("payload");
  extra.close();

  const auto resolved = Module::ResolveInstalledModule(store_, kId);
  EXPECT_FALSE(resolved.ok);
  EXPECT_EQ(resolved.status, Module::ModulePackageStatus::kUNDECLARED_FILE);
}

TEST_F(ModuleStoreTest, ARemovedFileIsRefused) {
  const auto pkg = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  const auto installed = Module::InstallModulePackage(pkg, store_);
  ASSERT_TRUE(installed.ok);

  const auto note = installed.install_dir + "/resources/note.txt";
  ASSERT_TRUE(QFile::setPermissions(
      note, QFile::permissions(note) | QFileDevice::WriteOwner));
  ASSERT_TRUE(QFile::remove(note));

  const auto resolved = Module::ResolveInstalledModule(store_, kId);
  EXPECT_FALSE(resolved.ok);
  EXPECT_EQ(resolved.status,
            Module::ModulePackageStatus::kMISSING_DECLARED_FILE);
}

// -------------------------------------------------------------------- sweep

TEST_F(ModuleStoreTest, TheSweepKeepsWhatIsReachableAndDropsTheRest) {
  const auto v1 = Pkg("v1.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  const auto v2 = Pkg("v2.gfmodule", "2.0.0", QByteArray(2048, 'b'));
  const auto v3 = Pkg("v3.gfmodule", "3.0.0", QByteArray(2048, 'c'));

  const auto first = Module::InstallModulePackage(v1, store_);
  const auto second = Module::InstallModulePackage(v2, store_);
  const auto third = Module::InstallModulePackage(v3, store_);
  ASSERT_TRUE(first.ok && second.ok && third.ok);

  // v1 is now superseded twice: nothing installed points at it, and no
  // rollback can reach it either.
  EXPECT_EQ(Module::SweepModuleStore(store_), 1);
  EXPECT_FALSE(QDir(first.install_dir).exists());
  EXPECT_TRUE(QDir(second.install_dir).exists());
  EXPECT_TRUE(QDir(third.install_dir).exists());

  // Still usable afterwards, and still able to roll back one step.
  EXPECT_TRUE(Module::ResolveInstalledModule(store_, kId).ok);
  EXPECT_TRUE(Module::RollbackInstalledModule(store_, kId).ok);

  // Idempotent.
  EXPECT_EQ(Module::SweepModuleStore(store_), 0);
}

TEST_F(ModuleStoreTest, TheSweepCollectsAbandonedStaging) {
  const auto pkg = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));
  const auto installed = Module::InstallModulePackage(pkg, store_);
  ASSERT_TRUE(installed.ok);

  // What a crash part-way through an install leaves behind. Dot-prefixed so
  // the version scan never mistook it for an installed version in the first
  // place -- which is also why the sweep has to ask for hidden entries.
  const auto versions = QFileInfo(installed.install_dir).absolutePath();
  ASSERT_TRUE(QDir().mkpath(versions + "/.staging-deadbeef/bin"));

  EXPECT_EQ(Module::SweepModuleStore(store_), 1);
  EXPECT_FALSE(QDir(versions + "/.staging-deadbeef").exists());
  EXPECT_TRUE(Module::ResolveInstalledModule(store_, kId).ok);
}

TEST_F(ModuleStoreTest, SweepingAnEmptyStoreIsHarmless) {
  EXPECT_EQ(Module::SweepModuleStore(store_), 0);
  EXPECT_EQ(Module::SweepModuleStore(dir_.path() + "/never-created"), 0);
}

// ------------------------------------------------------------------ refusal

TEST_F(ModuleStoreTest, ARefusedPackageInstallsNothing) {
  // The ordering the format exists for, stated at the store's boundary: a
  // package that fails verification leaves the store exactly as it was, so
  // there is no tree anywhere for anything to load.
  const auto bad = dir_.path() + "/broken.gfmodule";
  QFile f(bad);
  ASSERT_TRUE(f.open(QIODevice::WriteOnly));
  f.write(QByteArray(1024, 'x'));
  f.close();

  const auto installed = Module::InstallModulePackage(bad, store_);
  EXPECT_FALSE(installed.ok);
  EXPECT_TRUE(installed.install_dir.isEmpty());
  EXPECT_FALSE(QDir(store_).exists());
  EXPECT_TRUE(Module::ListInstalledModules(store_).isEmpty());
}

TEST_F(ModuleStoreTest, AnUnexpectedSigningKeyIsRefused) {
  // The same publisher-trust seam the verifier carries, at the store's door:
  // when a key is supplied, the package's own key becomes a value to check.
  const auto pkg = Pkg("a.gfmodule", "1.0.0", QByteArray(2048, 'a'));

  QByteArray wrong(crypto_sign_PUBLICKEYBYTES, '\x01');
  const auto installed = Module::InstallModulePackage(pkg, store_, wrong);
  EXPECT_FALSE(installed.ok);
  EXPECT_EQ(installed.status, Module::ModulePackageStatus::kBAD_SIGNATURE);
  EXPECT_TRUE(Module::ListInstalledModules(store_).isEmpty());
}

}  // namespace GpgFrontend::Test
