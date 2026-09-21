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
#include <QTemporaryDir>
#include <algorithm>

#include "GpgFrontendTest.h"
#include "ModuleTestPackages.h"
#include "core/module/ModuleNamespace.h"
#include "core/module/ModuleSetVerification.h"

/**
 * @file GpgCoreTestModuleSetVerification.cpp
 * @brief The release gate, and the faults it exists to catch.
 *
 * Per-module verification cannot see any of these: they are properties of the
 * set, not of any one descriptor. Each case below is a tree that is internally
 * plausible -- every descriptor signed, every signature valid -- and wrong as
 * a whole.
 *
 * The clean tree is the build's own output rather than something synthesised
 * here, so the positive case is the one a release would actually ship.
 */

namespace GpgFrontend::Test {

namespace {

/// A writable copy of this build's real module tree.
///
/// Returns false ONLY when this build produced no modules. A copy that fails
/// halfway is reported as a failure here rather than folded into the same
/// answer: it used to return false for both, so a genuine I/O error read as
/// "no module tree to copy" and six negative tests skipped themselves.
auto CopyBuiltTree(const QString& into) -> bool {
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) return false;

  const QDir root(built.first().absolutePath() + "/..");
  for (const auto& ns : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    const auto target = into + "/" + ns.fileName();
    if (!QDir().mkpath(target + "/native")) {
      ADD_FAILURE() << "could not create " << target.toStdString();
      return true;
    }

    const QDir source(ns.absoluteFilePath());
    for (const auto& file : source.entryInfoList(QDir::Files)) {
      if (!QFile::copy(file.absoluteFilePath(),
                       target + "/" + file.fileName())) {
        ADD_FAILURE() << "could not copy " << file.fileName().toStdString();
        return true;
      }
    }
    const QDir native(ns.absoluteFilePath() + "/native");
    for (const auto& file : native.entryInfoList(QDir::Files)) {
      if (!QFile::copy(file.absoluteFilePath(),
                       target + "/native/" + file.fileName())) {
        ADD_FAILURE() << "could not copy " << file.fileName().toStdString();
        return true;
      }
    }
  }
  return true;
}

/// This build's modules, laid out the way a macOS bundle lays them out.
///
/// Descriptors under `Contents/Resources/modules/<key>/`, natives under
/// `Contents/Frameworks/GpgFrontendModules/<key>/` -- the one layout where a
/// namespace is not two levels of a single tree, because Apple wants data in
/// the first place and executable code in the second.
auto CopyBuiltTreeAsBundle(const QString& app) -> bool {
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) return false;

  const auto descriptors = app + "/Contents/Resources/modules";
  const auto natives = app + "/Contents/Frameworks/GpgFrontendModules";

  const QDir root(built.first().absolutePath() + "/..");
  for (const auto& ns : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    const QDir source(ns.absoluteFilePath());
    if (!QFileInfo(source.absoluteFilePath("module.gfmodule")).isFile()) {
      continue;
    }

    if (!QDir().mkpath(descriptors + "/" + ns.fileName()) ||
        !QDir().mkpath(natives + "/" + ns.fileName())) {
      return false;
    }
    if (!QFile::copy(source.absoluteFilePath("module.gfmodule"),
                     descriptors + "/" + ns.fileName() + "/module.gfmodule")) {
      return false;
    }
    const QDir native(source.absoluteFilePath("native"));
    for (const auto& file : native.entryInfoList(QDir::Files)) {
      if (!QFile::copy(file.absoluteFilePath(),
                       natives + "/" + ns.fileName() + "/" + file.fileName())) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace

namespace {

/// What this build configured: the answer the Host itself will give.
///
/// Most cases here are about the set -- namespaces, counts, descriptors -- and
/// want the real policy so they exercise what actually ships.
auto HostPolicy() -> Module::ModuleEntryTrustPolicy {
  return {Module::ModuleOrigin::kINTEGRATED,
          Module::HostIntegratedBindingRequirement()};
}

/// Binding demanded, whatever this build defaults to.
///
/// Used by the cases that assert tamper DETECTION. They must never inherit
/// the build default: with binding off those descriptors carry no claim about
/// their natives, the mutation is not caught, and the obvious repair is to
/// relax the assertion -- which is how a gate stops checking anything.
constexpr Module::ModuleEntryTrustPolicy kBoundPolicy{
    Module::ModuleOrigin::kINTEGRATED,
    Module::ModuleBindingRequirement::kREQUIRED};

}  // namespace

TEST(ModuleSetVerificationTest, ABundleLayoutResolvesAcrossItsTwoTrees) {
  // The macOS shipping layout, checked on any host.
  //
  // ModuleNativeRootFor() decides the split from the path's shape rather than
  // from an #ifdef, precisely so this is testable here -- and the test is
  // worth having because the unit tests for that function only ever fed it
  // synthetic strings. VerifyModuleSet had its own hardcoded "<ns>/native"
  // and never called it, so a correctly assembled bundle reported every module
  // as missing its native. Covering the function and not its caller is how
  // that survived.
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) {
    ASSERT_EQ(GF_REGISTERED_MODULE_COUNT, 0)
        << "modules were registered but none reached the build tree";
    GTEST_SKIP() << "this build produced no modules";
  }

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto app = dir.path() + "/GpgFrontend.app";
  ASSERT_TRUE(CopyBuiltTreeAsBundle(app));

  const auto result =
      Module::VerifyModuleSet(HostPolicy(), app + "/Contents/Resources/modules",
                              static_cast<int>(built.size()));

  for (const auto& problem : result.problems) {
    ADD_FAILURE() << problem.where.toStdString() << ": "
                  << problem.reason.toStdString();
  }
  EXPECT_TRUE(result.ok);
  EXPECT_EQ(result.verified.size(), built.size());
}

TEST(ModuleSetVerificationTest, ABundleMissingItsFrameworksHalfIsRefused) {
  // Half a namespace is not a namespace. If the descriptors ship and the
  // natives do not -- the exact shape of the bug this layout work fixed --
  // every module must be refused rather than quietly absent.
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) {
    ASSERT_EQ(GF_REGISTERED_MODULE_COUNT, 0)
        << "modules were registered but none reached the build tree";
    GTEST_SKIP() << "this build produced no modules";
  }

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const auto app = dir.path() + "/GpgFrontend.app";
  ASSERT_TRUE(CopyBuiltTreeAsBundle(app));

  QDir(app + "/Contents/Frameworks").removeRecursively();

  const auto result =
      Module::VerifyModuleSet(HostPolicy(), app + "/Contents/Resources/modules",
                              static_cast<int>(built.size()));
  EXPECT_FALSE(result.ok);
  EXPECT_EQ(result.problems.size(), built.size() + 1)
      << "one refusal per module, plus the count mismatch";
}

TEST(ModuleSetVerificationTest, ThisBuildsOwnTreeVerifies) {
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) {
    ASSERT_EQ(GF_REGISTERED_MODULE_COUNT, 0)
        << "modules were registered but none reached the build tree";
    GTEST_SKIP() << "this build produced no modules";
  }

  // The expectation comes from CMake, not from what the walk happened to
  // find. Counting the directories and then checking that many were verified
  // is circular: a module that never reached the tree lowers both sides.
  const auto root = built.first().absolutePath() + "/..";
  const auto result = Module::VerifyModuleSet(
      HostPolicy(), QDir(root).absolutePath(), GF_REGISTERED_MODULE_COUNT);

  for (const auto& problem : result.problems) {
    ADD_FAILURE() << problem.where.toStdString() << ": "
                  << problem.reason.toStdString();
  }
  EXPECT_TRUE(result.ok);
  EXPECT_EQ(result.verified.size(), built.size());
}

TEST(ModuleSetVerificationTest, AWrongCountIsARefusal) {
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) {
    ASSERT_EQ(GF_REGISTERED_MODULE_COUNT, 0)
        << "modules were registered but none reached the build tree";
    GTEST_SKIP() << "this build produced no modules";
  }

  const auto root = QDir(built.first().absolutePath() + "/..").absolutePath();
  const auto result = Module::VerifyModuleSet(
      HostPolicy(), root, static_cast<int>(built.size()) + 1);

  EXPECT_FALSE(result.ok)
      << "a release that shipped fewer modules than it meant to is a release "
         "with a module missing, and nothing else would say so";
}

TEST(ModuleSetVerificationTest, AMutatedEntryNativeIsCaught) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  if (!CopyBuiltTree(tree.path())) {
    ASSERT_EQ(GF_REGISTERED_MODULE_COUNT, 0)
        << "modules were registered but none reached the build tree";
    GTEST_SKIP() << "this build produced no modules";
  }

  // One byte, in the middle, in a library that still loads perfectly well. It
  // is simply not the one its descriptor was signed for.
  //
  // The file is the one the DESCRIPTOR BINDS, asked of the verifier, not the
  // first file the directory happens to list. A namespace holds private
  // dependencies too -- on macOS these directories carry libssl and libcrypto
  // since dependency bundling landed -- and flipping a byte in one of those
  // would prove nothing about the entry binding while still turning the tree
  // red, for a reason the assertion below does not describe.
  //
  // Verified under kBoundPolicy rather than this build's own, and that is the
  // whole point: with integrated binding OFF these descriptors record no
  // claim about their natives, so the flipped byte would go undetected and
  // this test would pass while asserting nothing. If the tree really is
  // unbound the case skips, loudly, instead of quietly succeeding.
  const auto before = Module::VerifyModuleSet(kBoundPolicy, tree.path());
  if (!before.ok) {
    GTEST_SKIP() << "this build's descriptors carry no entry binding "
                    "(GPGFRONTEND_INTEGRATED_MODULE_NATIVE_BINDING=OFF), so "
                    "there is no binding here to detect a mutation with";
  }
  ASSERT_FALSE(before.entries.isEmpty());

  const auto entry_path = before.entries.constBegin().value();
  {
    QFile file(entry_path);
    ASSERT_TRUE(file.open(QIODevice::ReadWrite));
    ASSERT_TRUE(file.seek(file.size() / 2));
    const auto before = file.read(1);
    ASSERT_TRUE(file.seek(file.size() / 2));
    const char flipped = static_cast<char>(before.at(0) ^ 1);
    ASSERT_EQ(file.write(&flipped, 1), 1);
  }

  const auto result = Module::VerifyModuleSet(kBoundPolicy, tree.path());
  EXPECT_FALSE(result.ok);

  const auto mutated = before.entries.constBegin().key();
  const auto caught = std::any_of(
      result.problems.cbegin(), result.problems.cend(), [&](const auto& p) {
        return p.where == mutated && p.reason.contains("entry native");
      });
  EXPECT_TRUE(caught) << "the mutated module was not the one reported";
}

TEST(ModuleSetVerificationTest, ARenamedNamespaceIsCaught) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  if (!CopyBuiltTree(tree.path())) {
    ASSERT_EQ(GF_REGISTERED_MODULE_COUNT, 0)
        << "modules were registered but none reached the build tree";
    GTEST_SKIP() << "this build produced no modules";
  }

  const QDir root(tree.path());
  const auto ns = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).first();
  ASSERT_TRUE(QDir().rename(ns.absoluteFilePath(),
                            tree.path() + "/not-the-derived-key"));

  const auto result = Module::VerifyModuleSet(HostPolicy(), tree.path());
  EXPECT_FALSE(result.ok);
  ASSERT_FALSE(result.problems.isEmpty());
  EXPECT_TRUE(result.problems.first().reason.contains("whose namespace is"))
      << result.problems.first().reason.toStdString();
}

// A module cannot ship twice, and the namespace key is what makes that true.
//
// This used to be called "two namespaces claiming one module are caught" and
// asserted only that the tree was refused. It could not have been testing what
// its name said: a second namespace has to be named something, that name is
// not ModuleDirectoryKey(id), and the key rule refuses it before any
// duplicate-id rule could run. There was such a rule, it was unreachable for
// exactly this reason, and it has been deleted -- so this test now names the
// mechanism that actually does the work, and asserts its reason.
TEST(ModuleSetVerificationTest, AModuleCopiedIntoASecondNamespaceIsRefused) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  ASSERT_TRUE(CopyBuiltTree(tree.path())) << "no module tree to copy";

  const QDir root(tree.path());
  const auto ns = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).first();
  const auto duplicate = tree.path() + "/a-second-home";
  ASSERT_TRUE(QDir().mkpath(duplicate + "/native"));
  ASSERT_TRUE(QFile::copy(ns.absoluteFilePath() + "/module.gfmodule",
                          duplicate + "/module.gfmodule"));

  const auto result = Module::VerifyModuleSet(HostPolicy(), tree.path());
  EXPECT_FALSE(result.ok);

  const auto refused = std::any_of(
      result.problems.cbegin(), result.problems.cend(), [](const auto& p) {
        return p.where == "a-second-home" &&
               p.reason.contains("whose namespace");
      });
  EXPECT_TRUE(refused) << "the copy was not refused by the namespace key rule";
}

TEST(ModuleSetVerificationTest, ANamespaceWithNativesButNoDescriptorIsCaught) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  if (!CopyBuiltTree(tree.path())) {
    ASSERT_EQ(GF_REGISTERED_MODULE_COUNT, 0)
        << "modules were registered but none reached the build tree";
    GTEST_SKIP() << "this build produced no modules";
  }

  const QDir root(tree.path());
  const auto ns = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).first();
  ASSERT_TRUE(QFile::remove(ns.absoluteFilePath() + "/module.gfmodule"));

  const auto result = Module::VerifyModuleSet(HostPolicy(), tree.path());
  EXPECT_FALSE(result.ok) << "native libraries nothing vouches for";
  ASSERT_FALSE(result.problems.isEmpty());
  EXPECT_TRUE(result.problems.first().reason.contains("vouches"))
      << result.problems.first().reason.toStdString();
}

TEST(ModuleSetVerificationTest, AnEmptyDirectoryIsAWarningNotAFailure) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  if (!CopyBuiltTree(tree.path())) {
    ASSERT_EQ(GF_REGISTERED_MODULE_COUNT, 0)
        << "modules were registered but none reached the build tree";
    GTEST_SKIP() << "this build produced no modules";
  }

  ASSERT_TRUE(QDir().mkpath(tree.path() + "/something-else"));

  const auto result = Module::VerifyModuleSet(HostPolicy(), tree.path());
  EXPECT_TRUE(result.ok) << "a directory that is not a module namespace is "
                            "not a reason to refuse a release";
  EXPECT_FALSE(result.warnings.isEmpty());
}

TEST(ModuleSetVerificationTest, ANativeOutsideAnyNamespaceIsFound) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  const auto modules = tree.path() + "/modules";
  ASSERT_TRUE(QDir().mkpath(modules));
  if (!CopyBuiltTree(modules)) {
    ASSERT_EQ(GF_REGISTERED_MODULE_COUNT, 0)
        << "modules were registered but none reached the build tree";
    GTEST_SKIP() << "this build produced no modules";
  }

  const QDir root(modules);
  const auto ns = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).first();
  const QDir native(ns.absoluteFilePath() + "/native");
  const auto library = native.entryInfoList(QDir::Files).first();

  // The leak a staging step makes: the same library, one directory up.
  ASSERT_TRUE(QFile::copy(library.absoluteFilePath(),
                          tree.path() + "/" + library.fileName()));

  const auto leaked =
      Module::FindNativeModuleBinariesOutside(tree.path(), modules);
  ASSERT_EQ(leaked.size(), 1) << "exactly the copy, and not the original";
  EXPECT_TRUE(leaked.first().endsWith(library.fileName()));

  // And the ones inside their namespaces are not leaks.
  EXPECT_TRUE(
      Module::FindNativeModuleBinariesOutside(modules, modules).isEmpty());
}

}  // namespace GpgFrontend::Test
