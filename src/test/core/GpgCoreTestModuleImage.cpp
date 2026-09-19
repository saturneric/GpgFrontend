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
#include <QDirIterator>
#include <QFile>
#include <QLibrary>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "core/ModuleTestPackages.h"
#include "core/module/ModuleImageMapping.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModulePackageVerifier.h"

namespace GpgFrontend::Test {

namespace {

/// Everything currently under that root, so a test can assert it is unchanged.
auto PrivateRootEntries() -> QStringList {
  const QDir root(Module::ModuleImageMapping::PrivateRoot());
  if (!root.exists()) return {};
  auto entries = root.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
  entries.sort();
  return entries;
}

class ModuleImageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    package_ = LargestBuiltModulePackage();
    if (package_.isEmpty()) GTEST_SKIP() << "no module packages in this build";
  }

  void TearDown() override {
    // An armed fault that never fired would leak into the next test.
    Module::SetModuleImageFaultForTesting(Module::ModuleImageFaultPoint::kNONE);
  }

  QString package_;
};

}  // namespace

TEST_F(ModuleImageTest, AVerifiedImageLoadsWithoutBeingInstalled) {
  const auto read = Module::ReadVerifiedModuleImage(package_);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();
  ASSERT_TRUE(read.image.IsValid());

  QString reason;
  auto mapping = Module::ModuleImageMapping::Create(read.image, &reason);
  ASSERT_TRUE(mapping) << reason.toStdString();

  QLibrary library(mapping->LoadPath());
  ASSERT_TRUE(library.load()) << library.errorString().toStdString();

  // The one entry point a module is required to have. Resolving it is what
  // distinguishes "the loader accepted these bytes" from "a file existed".
  EXPECT_NE(library.resolve("GFModuleGetApi"), nullptr);
  library.unload();
}

#ifdef Q_OS_LINUX
TEST_F(ModuleImageTest, OnLinuxTheImageNeverExistsInAnyDirectory) {
  const auto read = Module::ReadVerifiedModuleImage(package_);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  auto mapping = Module::ModuleImageMapping::Create(read.image);
  ASSERT_TRUE(mapping);

  // The whole claim, in one assertion: what the loader is given is a
  // descriptor, not a path anyone could open by name.
  EXPECT_TRUE(mapping->IsAnonymous());
  EXPECT_TRUE(mapping->LoadPath().startsWith("/proc/self/fd/"));

  QLibrary library(mapping->LoadPath());
  ASSERT_TRUE(library.load()) << library.errorString().toStdString();

  // And it stays anonymous once mapped: the kernel's own view of the mapping
  // names a deleted anonymous file, which is the strongest form of "this was
  // never installed" available.
  QFile maps("/proc/self/maps");
  ASSERT_TRUE(maps.open(QIODevice::ReadOnly));
  const auto mapped = QString::fromLatin1(maps.readAll());
  EXPECT_TRUE(mapped.contains("/memfd:"));
  library.unload();
}
#endif

#ifdef Q_OS_LINUX
TEST_F(ModuleImageTest, TwoMappingsNeverShareALoadPath) {
  // A regression test for a bug this design invites and which cost a real
  // failure to find. On the memfd path the descriptor number *is* the name the
  // loader is given, and glibc matches an already-loaded object by name before
  // it looks at an inode. Recycle the number and dlopen returns the previous
  // object: success reported, no initialisers run, the wrong module mapped.
  //
  // So a load path, once handed out, must never come back for a different
  // image -- including after the mapping that owned it has been destroyed,
  // which is what happens whenever a module loads and is then rejected.
  const auto read = Module::ReadVerifiedModuleImage(package_);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  QString first_path;
  {
    auto first = Module::ModuleImageMapping::Create(read.image);
    ASSERT_TRUE(first);
    first_path = first->LoadPath();

    QLibrary library(first_path);
    ASSERT_TRUE(library.load()) << library.errorString().toStdString();
    library.unload();
  }

  auto second = Module::ModuleImageMapping::Create(read.image);
  ASSERT_TRUE(second);
  EXPECT_NE(second->LoadPath(), first_path);
}
#endif

TEST_F(ModuleImageTest, TheFileBackedPathWritesExactlyTheVerifiedBytes) {
  // macOS and Windows have no choice about this, and on Linux it is what a
  // developer gets when they ask for symbols. The bytes must be the same
  // verified bytes either way -- the file is transport, not a second source of
  // truth.
  qputenv("GPGFRONTEND_MODULE_FILE_BACKED", "1");

  const auto read = Module::ReadVerifiedModuleImage(package_);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  QString reason;
  auto mapping = Module::ModuleImageMapping::Create(read.image, &reason);
  ASSERT_TRUE(mapping) << reason.toStdString();
  EXPECT_FALSE(mapping->IsAnonymous());

  QFile written(mapping->LoadPath());
  ASSERT_TRUE(written.open(QIODevice::ReadOnly));
  EXPECT_EQ(written.readAll(), read.image.Bytes());
  written.close();

  QLibrary library(mapping->LoadPath());
  ASSERT_TRUE(library.load()) << library.errorString().toStdString();
  library.unload();

  qunsetenv("GPGFRONTEND_MODULE_FILE_BACKED");
}

TEST_F(ModuleImageTest, AnUnverifiedImageCannotBeMaterialized) {
  // A default-constructed image is the only one anything outside the verifier
  // can make, and it is worth nothing. There is deliberately no way to build a
  // populated one from bytes somebody read off a disk.
  const Module::VerifiedModuleImage nothing;
  EXPECT_FALSE(nothing.IsValid());

  QString reason;
  EXPECT_FALSE(Module::ModuleImageMapping::Create(nothing, &reason));
  EXPECT_FALSE(reason.isEmpty());
}

// ---------------------------------------------------- materialisation faults

class ModuleImageFaultTest
    : public ModuleImageTest,
      public ::testing::WithParamInterface<Module::ModuleImageFaultPoint> {};

TEST_P(ModuleImageFaultTest, AFailedMaterializationNeverReachesTheLoader) {
  // The property that matters is not that Create() returned an error. It is
  // that a caller has nothing to load afterwards -- no path, no file, nothing
  // half-written left where a later start might find it and map it.
  qputenv("GPGFRONTEND_MODULE_FILE_BACKED", "1");

  const auto read = Module::ReadVerifiedModuleImage(package_);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  Module::SetModuleImageFaultForTesting(GetParam());

  QString reason;
  auto mapping = Module::ModuleImageMapping::Create(read.image, &reason);

  ASSERT_FALSE(mapping);
  EXPECT_FALSE(reason.isEmpty());

  qunsetenv("GPGFRONTEND_MODULE_FILE_BACKED");
}

INSTANTIATE_TEST_SUITE_P(
    EveryWayItCanGoWrong, ModuleImageFaultTest,
    ::testing::Values(Module::ModuleImageFaultPoint::kCREATE_BACKING,
                      Module::ModuleImageFaultPoint::kSHORT_WRITE,
                      Module::ModuleImageFaultPoint::kWRITE_FAILS,
                      Module::ModuleImageFaultPoint::kREADBACK_DIFFERS));

TEST_F(ModuleImageTest, AHalfWrittenImageIsNotLeftBehind) {
  qputenv("GPGFRONTEND_MODULE_FILE_BACKED", "1");

  const auto read = Module::ReadVerifiedModuleImage(package_);
  ASSERT_TRUE(read.ok);

  // The claim is that a truncated image leaves NO file a loader could later be
  // pointed at. This used to end on EXPECT_GE(swept, 0), which is true of any
  // int -- so the thing it names was never actually checked. Compare what is
  // on disk instead.
  const auto before = PrivateRootEntries();

  Module::SetModuleImageFaultForTesting(
      Module::ModuleImageFaultPoint::kSHORT_WRITE);
  EXPECT_FALSE(Module::ModuleImageMapping::Create(read.image));

  Module::ModuleImageMapping::SweepAbandonedDirectories();

  EXPECT_EQ(PrivateRootEntries(), before)
      << "a failed materialization left something behind";

  // And specifically nothing named like the image, anywhere beneath it.
  const QDir root(Module::ModuleImageMapping::PrivateRoot());
  QDirIterator it(root.absolutePath(), QStringList{read.image.LibraryName()},
                  QDir::Files, QDirIterator::Subdirectories);
  QStringList survivors;
  while (it.hasNext()) survivors << it.next();
  EXPECT_TRUE(survivors.isEmpty()) << "half-written image survived at: "
                                   << survivors.join(", ").toStdString();

  qunsetenv("GPGFRONTEND_MODULE_FILE_BACKED");
}

// ------------------------------------------------------------------ sweeping

TEST_F(ModuleImageTest, TheSweepLeavesALiveMappingAlone) {
  qputenv("GPGFRONTEND_MODULE_FILE_BACKED", "1");

  const auto read = Module::ReadVerifiedModuleImage(package_);
  ASSERT_TRUE(read.ok);

  auto mapping = Module::ModuleImageMapping::Create(read.image);
  ASSERT_TRUE(mapping);
  const auto live_dir = QFileInfo(mapping->LoadPath()).absolutePath();
  ASSERT_TRUE(QDir(live_dir).exists());

  // Ownership is decided by a lock this process holds, not by a recorded
  // process id -- ids are reused, and a sweep that trusted one would sooner or
  // later delete a directory belonging to somebody else.
  Module::ModuleImageMapping::SweepAbandonedDirectories();
  EXPECT_TRUE(QDir(live_dir).exists());

  mapping.reset();
  qunsetenv("GPGFRONTEND_MODULE_FILE_BACKED");
}

TEST_F(ModuleImageTest, NothingSurvivesTheMapping) {
  qputenv("GPGFRONTEND_MODULE_FILE_BACKED", "1");

  const auto read = Module::ReadVerifiedModuleImage(package_);
  ASSERT_TRUE(read.ok);

  QString path;
  QString dir;
  {
    auto mapping = Module::ModuleImageMapping::Create(read.image);
    ASSERT_TRUE(mapping);
    path = mapping->LoadPath();
    dir = QFileInfo(path).absolutePath();

    QLibrary library(path);
    ASSERT_TRUE(library.load()) << library.errorString().toStdString();

    // Unlinked the moment it has been mapped, where the platform allows it.
    mapping->NotifyLoaded();
#ifdef Q_OS_UNIX
    EXPECT_FALSE(QFile::exists(path));
#endif
    library.unload();
  }

  // And once the mapping is gone, so is everything it made.
  EXPECT_FALSE(QFile::exists(path));
  EXPECT_FALSE(QDir(dir).exists());

  qunsetenv("GPGFRONTEND_MODULE_FILE_BACKED");
}

// ------------------------------------------------------- identity, not paths

TEST_F(ModuleImageTest, IdentityComesFromTheManifestNotTheLoadPath) {
  const auto read = Module::ReadVerifiedModuleImage(package_);
  ASSERT_TRUE(read.ok) << read.reason.toStdString();

  QString known_hash;
  for (const auto& file : read.manifest.files) {
    if (file.path.startsWith("bin/")) known_hash = file.sha256;
  }
  ASSERT_FALSE(known_hash.isEmpty());

  const auto inspection = Module::InspectModuleImage(read.image, known_hash);
  ASSERT_TRUE(inspection.ok) << inspection.reason.toStdString();

  // The signed name, which is the only place it can come from: the load path
  // on Linux is a number, and on the other platforms a temporary file that
  // nothing should be deriving identity from either.
  EXPECT_TRUE(read.image.LibraryName().startsWith("libgf_mod_"));
  EXPECT_EQ(inspection.hash, known_hash);

  auto mapping = Module::ModuleImageMapping::Create(read.image);
  ASSERT_TRUE(mapping);
  EXPECT_FALSE(Module::IsModuleLibraryFileName(
                   QFileInfo(mapping->LoadPath()).fileName()) &&
               mapping->IsAnonymous());
}

}  // namespace GpgFrontend::Test
