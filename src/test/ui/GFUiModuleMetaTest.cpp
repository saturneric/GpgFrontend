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

#include <optional>

#include "GpgFrontendTest.h"
#include "sdk/GFSDKBuildInfo.h"
#include "ui/dialog/controller/ModuleMeta.h"

namespace GpgFrontend::UI::Test {

namespace {

auto PackagedView() -> Module::ModuleProvenance {
  Module::ModuleManifest manifest;
  manifest.id = "com.example.thing";
  manifest.version = "1.4.0";
  manifest.capabilities = QStringList{"gpg", "ui"};
  manifest.platform_os = "linux";
  manifest.platform_arch = "x86_64";
  manifest.platform_qt = "6.8.1";
  manifest.metadata = {{"Name", "Thing"},
                       {"Description", "Does a thing."},
                       {"Author", "Someone"}};

  Module::ModuleProvenance view;
  view.identifier = "com.example.thing";
  view.version = "1.4.0";
  view.packaged = true;
  view.source_package_path = "/opt/gpgfrontend/modules/thing.gfmodule";
  view.manifest = manifest;
  view.metadata = manifest.metadata;
  view.sdk_abi = GF_SDK_ABI_VERSION;
  view.hash = QString(64, 'a');
  return view;
}

auto LooseView() -> Module::ModuleProvenance {
  Module::ModuleProvenance view;
  view.identifier = "com.example.loose";
  view.version = "0.9.0";
  view.packaged = false;
  view.source_package_path = "/home/dev/build/libgf_mod_loose.so";
  view.sdk_abi = GF_SDK_ABI_VERSION;
  view.hash = QString(64, 'b');
  return view;
}

auto Find(const QVector<MetaListRow>& rows, const QString& caption)
    -> std::optional<MetaListRow> {
  for (const auto& row : rows) {
    if (row.caption == caption) return row;
  }
  return std::nullopt;
}

auto IndexOf(const QVector<MetaListRow>& rows, const QString& caption) -> int {
  for (int i = 0; i < rows.size(); ++i) {
    if (rows.at(i).caption == caption) return i;
  }
  return -1;
}

}  // namespace

TEST(ModuleMetaTest, APackagedModuleNamesThePackageItCameFrom) {
  const auto rows = BuildModuleRows(PackagedView());

  const auto origin = Find(rows, QObject::tr("Origin"));
  ASSERT_TRUE(origin.has_value());
  EXPECT_EQ(origin->value, QObject::tr("Signed package"));

  const auto package = Find(rows, QObject::tr("Package"));
  ASSERT_TRUE(package.has_value());
  EXPECT_TRUE(package->value.endsWith("thing.gfmodule"));
  EXPECT_TRUE(package->path);
}

TEST(ModuleMetaTest, TheLoadPathNeverReachesTheUser) {
  // The point of showing the package rather than the load path. On Linux the
  // load path is a descriptor in /proc/self/fd, which names nothing a user
  // could open; elsewhere it is a temporary file that is already gone.
  auto view = PackagedView();
  const auto rows = BuildModuleRows(view);

  for (const auto& row : rows) {
    EXPECT_FALSE(row.value.contains("/proc/self/fd/"));
    EXPECT_FALSE(row.value.contains("gpgfrontend-modules"));
  }
}

TEST(ModuleMetaTest, FactsComeBeforeClaims) {
  // A fact and a claim shown as one list read as two facts, so the headings
  // and their order are the substance of this panel rather than decoration.
  const auto rows = BuildModuleRows(PackagedView());

  const auto verified = IndexOf(rows, QObject::tr("Verified by GpgFrontend"));
  const auto described = IndexOf(rows, QObject::tr("Described by its package"));
  ASSERT_GE(verified, 0);
  ASSERT_GE(described, 0);
  EXPECT_LT(verified, described);

  EXPECT_EQ(rows.at(verified).kind, MetaRowKind::kSection);
  EXPECT_EQ(rows.at(described).kind, MetaRowKind::kSection);
}

TEST(ModuleMetaTest, ASignatureDoesNotClaimToSayWhoBuiltIt) {
  const auto rows = BuildModuleRows(PackagedView());

  const auto origin = Find(rows, QObject::tr("Origin"));
  ASSERT_TRUE(origin.has_value());
  // Compared against the exact string, not searched for a fragment of it: a
  // substring of tr() output need not be a substring of the translation, so
  // the old form asserted nothing outside English.
  EXPECT_EQ(origin->detail, UnattributedSignatureCaveat());
  EXPECT_FALSE(origin->detail.isEmpty());
}

TEST(ModuleMetaTest, PackagedMetadataIsNotMarkedUnverified) {
  // It was covered by a signature the host checked before any of this
  // module's code ran, so marking it as a claim would be the wrong answer in
  // the other direction.
  const auto rows = BuildModuleRows(PackagedView());

  const auto name = Find(rows, QObject::tr("Name"));
  ASSERT_TRUE(name.has_value());
  EXPECT_EQ(name->value, "Thing");
  EXPECT_FALSE(name->unverified);
}

TEST(ModuleMetaTest, ALooseLibrarySaysSoAndOffersNoMetadata) {
  const auto rows = BuildModuleRows(LooseView());

  const auto origin = Find(rows, QObject::tr("Origin"));
  ASSERT_TRUE(origin.has_value());
  EXPECT_EQ(origin->value, QObject::tr("Unsigned library"));
  EXPECT_TRUE(origin->degraded);

  // Absent rather than filled with the dotted identifier: there is nothing
  // behind them, and a title that is really an id is worse than no title.
  EXPECT_FALSE(Find(rows, QObject::tr("Name")).has_value());
  EXPECT_FALSE(Find(rows, QObject::tr("Author")).has_value());
  EXPECT_FALSE(Find(rows, QObject::tr("Description")).has_value());

  // And no claims heading, because there are no claims to head.
  EXPECT_EQ(IndexOf(rows, QObject::tr("Claimed by the module")), -1);
}

TEST(ModuleMetaTest, TheSdkAbiIsTheModulesOwnNotTheHosts) {
  // These two rows used to show the host's SDK and Qt versions, which are
  // identical for every module -- the panel appeared to say something about a
  // module while saying nothing at all.
  // Deliberately a number this host CANNOT have supplied: previously the test
  // set the same ABI the host has, so it could not distinguish the module's
  // value from the host's and could not fail.
  auto view = PackagedView();
  view.sdk_abi = GF_SDK_ABI_VERSION + 1;
  const auto rows = BuildModuleRows(view);

  const auto abi = Find(rows, QObject::tr("SDK ABI"));
  ASSERT_TRUE(abi.has_value());
  EXPECT_EQ(abi->value, QString::number(GF_SDK_ABI_VERSION + 1));

  const auto qt = Find(rows, QObject::tr("Built against Qt"));
  ASSERT_TRUE(qt.has_value());
  EXPECT_EQ(qt->value, "6.8.1");
}

TEST(ModuleMetaTest, AnIntegratedModuleIsNeitherSignedNorUnsigned) {
  Module::ModuleProvenance view;
  view.identifier = "com.example.builtin";
  view.version = "2.0.0";
  view.integrated = true;

  const auto rows = BuildModuleRows(view);
  const auto origin = Find(rows, QObject::tr("Origin"));
  ASSERT_TRUE(origin.has_value());
  EXPECT_EQ(origin->value, QObject::tr("Built into this application"));
  EXPECT_FALSE(origin->degraded);
}

}  // namespace GpgFrontend::UI::Test
