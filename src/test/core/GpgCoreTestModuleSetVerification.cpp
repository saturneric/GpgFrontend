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
auto CopyBuiltTree(const QString& into) -> bool {
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) return false;

  const QDir root(built.first().absolutePath() + "/..");
  for (const auto& ns : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    const auto target = into + "/" + ns.fileName();
    if (!QDir().mkpath(target + "/native")) return false;

    const QDir source(ns.absoluteFilePath());
    for (const auto& file : source.entryInfoList(QDir::Files)) {
      if (!QFile::copy(file.absoluteFilePath(),
                       target + "/" + file.fileName())) {
        return false;
      }
    }
    const QDir native(ns.absoluteFilePath() + "/native");
    for (const auto& file : native.entryInfoList(QDir::Files)) {
      if (!QFile::copy(file.absoluteFilePath(),
                       target + "/native/" + file.fileName())) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace

TEST(ModuleSetVerificationTest, ThisBuildsOwnTreeVerifies) {
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) GTEST_SKIP() << "this build produced no modules";

  const auto root = built.first().absolutePath() + "/..";
  const auto result = Module::VerifyModuleSet(QDir(root).absolutePath(),
                                              static_cast<int>(built.size()));

  for (const auto& problem : result.problems) {
    ADD_FAILURE() << problem.where.toStdString() << ": "
                  << problem.reason.toStdString();
  }
  EXPECT_TRUE(result.ok);
  EXPECT_EQ(result.verified.size(), built.size());
}

TEST(ModuleSetVerificationTest, AWrongCountIsARefusal) {
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) GTEST_SKIP() << "this build produced no modules";

  const auto root = QDir(built.first().absolutePath() + "/..").absolutePath();
  const auto result =
      Module::VerifyModuleSet(root, static_cast<int>(built.size()) + 1);

  EXPECT_FALSE(result.ok)
      << "a release that shipped fewer modules than it meant to is a release "
         "with a module missing, and nothing else would say so";
}

TEST(ModuleSetVerificationTest, AMutatedEntryNativeIsCaught) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  if (!CopyBuiltTree(tree.path())) GTEST_SKIP() << "no module tree to copy";

  // One byte, in the middle, in a library that still loads perfectly well. It
  // is simply not the one its descriptor was signed for.
  const QDir root(tree.path());
  const auto ns = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).first();
  const QDir native(ns.absoluteFilePath() + "/native");
  const auto library = native.entryInfoList(QDir::Files).first();

  {
    QFile file(library.absoluteFilePath());
    ASSERT_TRUE(file.open(QIODevice::ReadWrite));
    ASSERT_TRUE(file.seek(file.size() / 2));
    const auto before = file.read(1);
    ASSERT_TRUE(file.seek(file.size() / 2));
    const char flipped = static_cast<char>(before.at(0) ^ 1);
    ASSERT_EQ(file.write(&flipped, 1), 1);
  }

  const auto result = Module::VerifyModuleSet(tree.path());
  EXPECT_FALSE(result.ok);
  ASSERT_FALSE(result.problems.isEmpty());
  EXPECT_TRUE(result.problems.first().reason.contains("entry native"))
      << result.problems.first().reason.toStdString();
}

TEST(ModuleSetVerificationTest, ARenamedNamespaceIsCaught) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  if (!CopyBuiltTree(tree.path())) GTEST_SKIP() << "no module tree to copy";

  const QDir root(tree.path());
  const auto ns = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).first();
  ASSERT_TRUE(QDir().rename(ns.absoluteFilePath(),
                            tree.path() + "/not-the-derived-key"));

  const auto result = Module::VerifyModuleSet(tree.path());
  EXPECT_FALSE(result.ok);
  ASSERT_FALSE(result.problems.isEmpty());
  EXPECT_TRUE(result.problems.first().reason.contains("whose namespace is"))
      << result.problems.first().reason.toStdString();
}

TEST(ModuleSetVerificationTest, TwoNamespacesClaimingOneModuleAreCaught) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  if (!CopyBuiltTree(tree.path())) GTEST_SKIP() << "no module tree to copy";

  // A copy under a different name: both descriptors verify, both entries
  // match, and the tree still cannot be shipped, because a module id has to
  // resolve to one place.
  const QDir root(tree.path());
  const auto ns = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).first();
  const auto duplicate = tree.path() + "/a-second-home";
  ASSERT_TRUE(QDir().mkpath(duplicate + "/native"));
  ASSERT_TRUE(QFile::copy(ns.absoluteFilePath() + "/module.gfmodule",
                          duplicate + "/module.gfmodule"));

  const auto result = Module::VerifyModuleSet(tree.path());
  EXPECT_FALSE(result.ok);
}

TEST(ModuleSetVerificationTest, ANamespaceWithNativesButNoDescriptorIsCaught) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  if (!CopyBuiltTree(tree.path())) GTEST_SKIP() << "no module tree to copy";

  const QDir root(tree.path());
  const auto ns = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot).first();
  ASSERT_TRUE(QFile::remove(ns.absoluteFilePath() + "/module.gfmodule"));

  const auto result = Module::VerifyModuleSet(tree.path());
  EXPECT_FALSE(result.ok) << "native libraries nothing vouches for";
  ASSERT_FALSE(result.problems.isEmpty());
  EXPECT_TRUE(result.problems.first().reason.contains("vouches"))
      << result.problems.first().reason.toStdString();
}

TEST(ModuleSetVerificationTest, AnEmptyDirectoryIsAWarningNotAFailure) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  if (!CopyBuiltTree(tree.path())) GTEST_SKIP() << "no module tree to copy";

  ASSERT_TRUE(QDir().mkpath(tree.path() + "/something-else"));

  const auto result = Module::VerifyModuleSet(tree.path());
  EXPECT_TRUE(result.ok) << "a directory that is not a module namespace is "
                            "not a reason to refuse a release";
  EXPECT_FALSE(result.warnings.isEmpty());
}

TEST(ModuleSetVerificationTest, ANativeOutsideAnyNamespaceIsFound) {
  QTemporaryDir tree;
  ASSERT_TRUE(tree.isValid());
  const auto modules = tree.path() + "/modules";
  ASSERT_TRUE(QDir().mkpath(modules));
  if (!CopyBuiltTree(modules)) GTEST_SKIP() << "no module tree to copy";

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
