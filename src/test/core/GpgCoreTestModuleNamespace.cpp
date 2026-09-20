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
#include <QFile>
#include <QRegularExpression>
#include <QSet>

#include "GpgFrontendTest.h"
#include "ModuleTestPackages.h"
#include "core/module/ModuleNamespace.h"

/**
 * @file GpgCoreTestModuleNamespace.cpp
 * @brief The module directory key, and the two implementations of it.
 *
 * ModuleDirectoryKey() exists twice -- here in C++, and as
 * gf_module_directory_key() in cmake/ModuleRegistry.cmake -- because the two
 * halves run at different times: CMake names the output directory, and the
 * Host resolves it again at load. Nothing in a build failure would say so if
 * they drifted; the directory would simply be named something the Host later
 * refuses to find, at startup, on a user's machine.
 *
 * So the agreement is pinned here, against the file CMake writes, and the
 * derivation is pinned against golden values so that changing it is a
 * deliberate act rather than a side effect of tidying the string handling.
 */

namespace GpgFrontend::Test {

namespace {

constexpr auto kKeyPattern = "^[a-z][a-z0-9-]*-[0-9a-f]{20}$";

auto WellFormed(const QString& key) -> bool {
  static const QRegularExpression re(QString::fromLatin1(kKeyPattern));
  return re.match(key).hasMatch();
}

}  // namespace

// --------------------------------------------------------------------- shape

TEST(ModuleNamespaceTest, AKeyHasTheDocumentedShape) {
  const auto key =
      Module::ModuleDirectoryKey("com.bktus.gpgfrontend.module.email");
  EXPECT_TRUE(WellFormed(key)) << key.toStdString();
  EXPECT_TRUE(key.startsWith("email-")) << key.toStdString();
  EXPECT_LE(key.size(), 45);
}

TEST(ModuleNamespaceTest, AnEmptyIdHasNoKey) {
  EXPECT_TRUE(Module::ModuleDirectoryKey({}).isEmpty());
}

TEST(ModuleNamespaceTest, TheKeyIsDeterministic) {
  const auto a = Module::ModuleDirectoryKey("com.example.module.thing");
  const auto b = Module::ModuleDirectoryKey("com.example.module.thing");
  EXPECT_EQ(a, b);
}

TEST(ModuleNamespaceTest, GoldenKeysDoNotDriftSilently) {
  // Not golden for their own sake: these are the four ids this tree ships, and
  // a build whose key derivation changed would place every module somewhere
  // the previous Host cannot find. Changing these values is allowed; changing
  // them by accident is what this catches.
  const QMap<QString, QString> golden{
      {"com.bktus.gpgfrontend.module.email", "email-7752caa0259f59619e44"},
      {"com.bktus.gpgfrontend.module.gnupg_info_gathering",
       "gnupg-info-gathering-cdbefa3d0e760c00fd76"},
      {"com.bktus.gpgfrontend.module.key_server_sync",
       "key-server-sync-b59012630cec16920cc6"},
      {"com.bktus.gpgfrontend.module.version_checking",
       "version-checking-8a7b405b1f49a1c73296"},
  };

  for (auto it = golden.constBegin(); it != golden.constEnd(); ++it) {
    EXPECT_EQ(Module::ModuleDirectoryKey(it.key()), it.value())
        << "the derivation changed for " << it.key().toStdString();
  }
}

// ---------------------------------------------------------------- the leaf

TEST(ModuleNamespaceTest, UnderscoresBecomeDashesRatherThanVanishing) {
  // `ver_check` must not read as `vercheck`: the underscore is a word
  // separator in these ids, and deleting it changes the word.
  const auto key = Module::ModuleDirectoryKey("com.example.ver_check");
  EXPECT_TRUE(key.startsWith("ver-check-")) << key.toStdString();
}

TEST(ModuleNamespaceTest, AnIdWithNoDotsIsItsOwnLeaf) {
  const auto key = Module::ModuleDirectoryKey("standalone");
  EXPECT_TRUE(key.startsWith("standalone-")) << key.toStdString();
  EXPECT_TRUE(WellFormed(key));
}

TEST(ModuleNamespaceTest, ALeafThatWouldNotStartWithALetterIsPrefixed) {
  for (const auto* id : {"com.example.9lives", "com.example._leading",
                         "com.example.-dash", "com.example.___"}) {
    const auto key = Module::ModuleDirectoryKey(QString::fromLatin1(id));
    EXPECT_TRUE(WellFormed(key)) << id << " -> " << key.toStdString();
  }
}

TEST(ModuleNamespaceTest, AnOverLongLeafIsTruncatedAndStillWellFormed) {
  const auto key = Module::ModuleDirectoryKey(
      "com.example." + QString(80, u'a') + "_" + QString(80, u'b'));
  EXPECT_TRUE(WellFormed(key)) << key.toStdString();
  EXPECT_LE(key.size(), 45);
  // No doubled separator where the truncation landed.
  EXPECT_FALSE(key.contains("--")) << key.toStdString();
}

TEST(ModuleNamespaceTest, NonAsciiInTheLeafIsDroppedNotEncoded) {
  const auto key = Module::ModuleDirectoryKey(
      QString::fromUtf8("com.example.\xe9\x82\xae\xe4\xbb\xb6mail"));
  EXPECT_TRUE(WellFormed(key)) << key.toStdString();
  EXPECT_TRUE(key.startsWith("mail-")) << key.toStdString();
}

TEST(ModuleNamespaceTest, CaseIsFoldedInTheLeaf) {
  const auto a = Module::ModuleDirectoryKey("com.example.EMail");
  EXPECT_TRUE(a.startsWith("email-")) << a.toStdString();

  // But NOT in the hash: two ids differing only by case are different ids,
  // and must not share a directory.
  const auto b = Module::ModuleDirectoryKey("com.example.email");
  EXPECT_NE(a, b);
}

// ------------------------------------------------------------- distinctness

TEST(ModuleNamespaceTest, IdsSharingALeafStillGetDistinctKeys) {
  const auto a = Module::ModuleDirectoryKey("com.vendor.a.email");
  const auto b = Module::ModuleDirectoryKey("com.vendor.b.email");
  ASSERT_TRUE(a.startsWith("email-"));
  ASSERT_TRUE(b.startsWith("email-"));
  EXPECT_NE(a, b) << "the leaf is a label; the suffix is what separates them";
}

TEST(ModuleNamespaceTest, ManyNearbyIdsDoNotCollide) {
  QSet<QString> keys;
  for (int i = 0; i < 2000; ++i) {
    keys.insert(Module::ModuleDirectoryKey(
        QString("com.bktus.gpgfrontend.module.m%1").arg(i)));
  }
  EXPECT_EQ(keys.size(), 2000);
}

// ------------------------------------------------- CMake agrees with C++

TEST(ModuleNamespaceTest, CMakeDerivesTheSameKeysThisBuildDoes) {
  // Written by gf_add_module() during configure, one `id=key` per line. This
  // is the only place the CMake implementation and this one ever meet.
  //
  // The path comes from CMake, not from applicationDirPath(). The file is
  // always in artifacts/, but the test binary is only there under Ninja -- it
  // is artifacts/bin on MinGW, AppDir/usr/bin for an AppImage build and
  // build/<config> under Xcode. So on three of four layouts this test used to
  // skip itself, which meant the one check holding two implementations of
  // ModuleDirectoryKey together reported success without comparing anything.
  const auto path = QString::fromUtf8(GF_MODULE_DIRECTORY_KEYS_FILE);

  QFile file(path);
  if (GF_REGISTERED_MODULE_COUNT == 0) {
    GTEST_SKIP() << "this build registered no modules";
  }
  // A failure, not a skip: CMake registered modules, so it wrote this file.
  ASSERT_TRUE(file.exists())
      << GF_REGISTERED_MODULE_COUNT
      << " module(s) were registered but the key file is missing: "
      << path.toStdString();
  ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
  const auto text = QString::fromUtf8(file.readAll());
  file.close();

  int checked = 0;
  for (const auto& line : text.split(u'\n', Qt::SkipEmptyParts)) {
    const auto split = line.indexOf(u'=');
    ASSERT_GT(split, 0) << "malformed line: " << line.toStdString();

    const auto id = line.left(split);
    const auto cmake_key = line.mid(split + 1);

    EXPECT_EQ(Module::ModuleDirectoryKey(id), cmake_key)
        << "cmake/ModuleRegistry.cmake and ModuleNamespace.cpp disagree "
           "about "
        << id.toStdString();
    EXPECT_TRUE(WellFormed(cmake_key)) << cmake_key.toStdString();
    ++checked;
  }

  EXPECT_EQ(checked, GF_REGISTERED_MODULE_COUNT)
      << "the key file does not describe every module CMake registered";
}

// ------------------------------------------------- where the natives live

TEST(ModuleNamespaceTest, TheUnifiedLayoutPutsNativesBesideTheDescriptor) {
  const auto root = Module::ModuleNativeRootFor(
      "/opt/gpgfrontend/modules/email-0123456789abcdef0123/module.gfmodule");
  EXPECT_EQ(root, "/opt/gpgfrontend/modules/email-0123456789abcdef0123/native");
}

TEST(ModuleNamespaceTest, AMacOsBundleSplitsTheNamespaceAcrossTwoTrees) {
  // Apple wants data under Resources and executable code under Frameworks, so
  // this is the one layout where a namespace is not two levels of one tree.
  const auto root = Module::ModuleNativeRootFor(
      "/Applications/GpgFrontend.app/Contents/Resources/modules/"
      "email-0123456789abcdef0123/module.gfmodule");
  EXPECT_EQ(root.toStdString(),
            std::string("/Applications/GpgFrontend.app/Contents/Frameworks/"
                        "GpgFrontendModules/email-0123456789abcdef0123"));
}

TEST(ModuleNamespaceTest, AMacOsDevelopmentTreeTakesTheUnifiedAnswer) {
  // A macOS debug build uses the unified layout, and this decides from the
  // path's shape rather than from an #ifdef -- which is what makes both cases
  // testable on any host, and what keeps the two from needing separate code.
  const auto root = Module::ModuleNativeRootFor(
      "/home/dev/build/artifacts/modules/email-0123456789abcdef0123/"
      "module.gfmodule");
  EXPECT_EQ(root,
            "/home/dev/build/artifacts/modules/email-0123456789abcdef0123/"
            "native");
}

TEST(ModuleNamespaceTest, ResourcesAloneIsNotABundleLayout) {
  // Both levels have to match. A namespace that merely happens to sit under a
  // directory called Resources is not a bundle, and answering as though it
  // were would send the resolver somewhere that does not exist.
  const auto root = Module::ModuleNativeRootFor(
      "/srv/Resources/email-0123456789abcdef0123/module.gfmodule");
  EXPECT_EQ(root, "/srv/Resources/email-0123456789abcdef0123/native");
}

TEST(ModuleNamespaceTest, TheBuiltTreeResolvesToDirectoriesThatExist) {
  // The derivation above is about strings; this is the one case that checks
  // it against the tree this build actually produced.
  for (const auto& descriptor : BuiltModulePackages()) {
    const auto root =
        Module::ModuleNativeRootFor(descriptor.absoluteFilePath());
    EXPECT_TRUE(QFileInfo(root).isDir())
        << root.toStdString() << " is not a directory";
  }
}

}  // namespace GpgFrontend::Test
