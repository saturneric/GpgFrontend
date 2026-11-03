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

#include "core/module/ModuleLogCategory.h"
#include "core/module/ModuleNamespace.h"

namespace GpgFrontend::Test {

using Module::ModuleDirectoryKey;
using Module::ModuleIdLeaf;
using Module::ModuleLogCategory;
using Module::ModuleLogCategoryName;
using Module::ModuleTraceLogCategory;

namespace {

constexpr auto kEmailId = "com.bktus.gpgfrontend.module.email";
constexpr auto kKeyServerId = "com.bktus.gpgfrontend.module.key_server_sync";

}  // namespace

TEST(ModuleLogCategoryTest, ACategoryNameMatchesTheNamespaceDirectoryLeaf) {
  // The point of reusing ModuleIdLeaf rather than writing a second normaliser:
  // the category a reader filters on is the same string as the directory they
  // are already looking at. If these two ever drift, this fails.
  for (const auto* id : {kEmailId, kKeyServerId,
                         "com.bktus.gpgfrontend.module.gnupg_info_gathering",
                         "com.bktus.gpgfrontend.module.version_checking"}) {
    const auto leaf = ModuleIdLeaf(id);
    EXPECT_EQ(ModuleLogCategoryName(id), QString("module.") + leaf);
    EXPECT_TRUE(ModuleDirectoryKey(id).startsWith(leaf + "-"));
  }
}

TEST(ModuleLogCategoryTest, AnUnderscoreBecomesADashRatherThanDisappearing) {
  // `key_server_sync` must not read as `keyserversync`.
  EXPECT_EQ(ModuleLogCategoryName(kKeyServerId), "module.key-server-sync");
}

TEST(ModuleLogCategoryTest, AnEmptyIdIsUnknownRatherThanAOneLetterModule) {
  // ModuleIdLeaf("") answers "m", which as a category would claim a module
  // called `m` rather than admitting the message could not be attributed.
  EXPECT_EQ(ModuleLogCategoryName(""), "module.unknown");
  EXPECT_EQ(ModuleLogCategoryName("   "), "module.unknown");
}

TEST(ModuleLogCategoryTest, TraceIsAChildCategoryRatherThanASeparateOne) {
  // A child, so that a Qt rule on the parent can be written without silencing
  // trace by accident and vice versa.
  EXPECT_EQ(QString(ModuleTraceLogCategory(kEmailId).categoryName()),
            QString(ModuleLogCategory(kEmailId).categoryName()) + ".trace");
}

TEST(ModuleLogCategoryTest, ACategoryIsStableAcrossCallsRatherThanReallocated) {
  // QLoggingCategory does not copy the name it is given, so a category handed
  // out twice must be the SAME object -- a second object with an equal name
  // would be a second registration, and a filter rule's effect would depend on
  // which one a caller happened to hold.
  const auto& first = ModuleLogCategory(kEmailId);
  const auto& second = ModuleLogCategory(kEmailId);
  EXPECT_EQ(&first, &second);

  const auto& trace_first = ModuleTraceLogCategory(kEmailId);
  const auto& trace_second = ModuleTraceLogCategory(kEmailId);
  EXPECT_EQ(&trace_first, &trace_second);
  EXPECT_NE(&first, &trace_first);
}

TEST(ModuleLogCategoryTest, TwoModulesDoNotShareACategoryRatherThanBeingOne) {
  EXPECT_NE(&ModuleLogCategory(kEmailId), &ModuleLogCategory(kKeyServerId));
  EXPECT_NE(ModuleLogCategoryName(kEmailId),
            ModuleLogCategoryName(kKeyServerId));
}

TEST(ModuleLogCategoryTest, TwoIdsSharingALeafShareACategoryAsDocumented) {
  // Stated rather than defended against: the category is a filter handle, not
  // an identity. ModuleDirectoryKey is what stays unique, and this pins the
  // difference so nobody later reads the category as identifying.
  const auto* a = "com.bktus.gpgfrontend.module.email";
  const auto* b = "org.example.other.email";

  EXPECT_EQ(ModuleLogCategoryName(a), ModuleLogCategoryName(b));
  EXPECT_NE(ModuleDirectoryKey(a), ModuleDirectoryKey(b));
}

TEST(ModuleLogCategoryTest, TheNameIsAlwaysAUsableQtCategory) {
  // Qt matches rules on dotted components; a name with a space, a slash or an
  // empty component would silently never match one.
  for (const auto* id :
       {kEmailId, "weird..id", "9leading", "_", "UPPER.CASE", "trailing_"}) {
    const auto name = ModuleLogCategoryName(id);
    EXPECT_TRUE(name.startsWith("module."));
    EXPECT_FALSE(name.contains(' '));
    EXPECT_FALSE(name.contains("..")) << name.toStdString();
    EXPECT_FALSE(name.endsWith('.')) << name.toStdString();
  }
}

}  // namespace GpgFrontend::Test
