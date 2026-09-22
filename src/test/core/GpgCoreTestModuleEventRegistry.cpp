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

#include <QProcess>
#include <QSet>

#include "GpgFrontendTest.h"
#include "core/module/ModuleEventRegistry.h"

/**
 * @file GpgCoreTestModuleEventRegistry.cpp
 * @brief The event catalogue as a contract, and the boundary as a fact.
 *
 * Two unrelated subjects share this file because they are the two halves of
 * the same question -- what may a module observe, and what may it do -- and
 * neither is large enough to be worth its own.
 */

namespace GpgFrontend::Test {

// ------------------------------------------------------------- the catalogue

TEST(ModuleEventRegistryTest, EveryEntryIsUsableAsAContract) {
  const auto& catalog = Module::ModuleEventCatalog();
  ASSERT_FALSE(catalog.isEmpty());

  QSet<QString> ids;
  for (const auto& spec : catalog) {
    const auto id = QString::fromLatin1(spec.id);
    EXPECT_FALSE(id.isEmpty());
    EXPECT_FALSE(ids.contains(id))
        << id.toStdString()
        << " is listed twice; a second entry silently "
           "loses to the first";
    ids.insert(id);

    // A summary is not decoration: it is what the controller shows a user
    // deciding whether to approve a module that subscribes to this.
    ASSERT_NE(spec.summary, nullptr) << id.toStdString();
    EXPECT_FALSE(QString::fromLatin1(spec.summary).isEmpty())
        << id.toStdString();

    if ((spec.flags & GF_EVENT_PATTERN) != 0) {
      EXPECT_TRUE(id.contains('<')) << id.toStdString()
                                    << " is flagged as a pattern but has no "
                                       "placeholder to substitute";
    } else {
      EXPECT_TRUE(QRegularExpression("^[A-Z][A-Z0-9_]*$").match(id).hasMatch())
          << id.toStdString()
          << " is not an upper-case identifier, which is "
             "the form the host dispatches on";
    }
  }
}

TEST(ModuleEventRegistryTest, AnEventTheHostNeverFiresIsNotKnown) {
  EXPECT_FALSE(Module::IsKnownModuleEvent("NOT_AN_EVENT"));
  EXPECT_FALSE(Module::IsKnownModuleEvent(""));
  // Near misses, because those are the ones a typo produces.
  EXPECT_FALSE(Module::IsKnownModuleEvent("APPLICATION_LOAD"));
  EXPECT_FALSE(Module::IsKnownModuleEvent("MAINWINDOW_MENU_MOUNT"));
}

TEST(ModuleEventRegistryTest, TheGeneratedFamiliesMatchThroughTheirPattern) {
  // These ids are built at run time from a tab type or a file extension, so
  // they cannot be listed. The pattern is what a subscription is checked
  // against.
  EXPECT_TRUE(Module::IsKnownModuleEvent("EDIT_TAB_TYPE_EMAIL_OP_DECRYPT"));
  EXPECT_TRUE(Module::IsKnownModuleEvent("EDIT_TAB_TYPE_EMAIL_OP_SAVE_FILE"));
  EXPECT_TRUE(Module::IsKnownModuleEvent("FILE_EXT_EMAIL_OP_OPEN_FILE"));

  // A pattern matches a SHAPE, not a prefix.
  EXPECT_FALSE(Module::IsKnownModuleEvent("EDIT_TAB_TYPE_EMAIL"));
  EXPECT_FALSE(Module::IsKnownModuleEvent("EDIT_TAB_TYPE__OP_DECRYPT"));
}

TEST(ModuleEventRegistryTest, ALiteralWinsOverAPatternItWouldFit) {
  // Nothing in the catalogue relies on this today, and the ordering rule is
  // worth pinning anyway: the literal carries the real contract, and a
  // pattern that happened to match it would replace that contract with a
  // more general one.
  for (const auto& spec : Module::ModuleEventCatalog()) {
    if ((spec.flags & GF_EVENT_PATTERN) != 0) continue;
    const auto id = QString::fromLatin1(spec.id);
    const auto* found = Module::FindModuleEventSpec(id);
    ASSERT_NE(found, nullptr) << id.toStdString();
    EXPECT_STREQ(found->id, spec.id);
  }
}

// The small, deliberate list. If this grows, it grew because somebody added
// a trigger site that reads a reply -- which is a decision, and this test is
// where it gets noticed.
TEST(ModuleEventRegistryTest, TheExtensionPointsAreTheOnesThatChangeBehaviour) {
  QStringList extend;
  for (const auto& spec : Module::ModuleEventCatalog()) {
    if (spec.semantics == Module::ModuleEventSemantics::kEXTEND) {
      extend.append(QString::fromLatin1(spec.id));
    }
  }
  extend.sort();

  EXPECT_EQ(extend, (QStringList{
                        "ABOUT_DIALOG_TABS_MOUNTED",
                        "EDIT_TAB_TYPE_<TYPE>_OP_<OP>",
                        "FILE_EXT_<PREFIX>_OP_<OP>",
                        "KEY_PAIR_OPERA_MENU_CREATED",
                        "MAINWINDOW_MENU_MOUNTED",
                        "NETWORK_SETTINGS_TAB_UI_CREATED",
                        "REQUEST_GET_PUBLIC_KEY_BY_FINGERPRINT",
                        "REQUEST_GET_PUBLIC_KEY_BY_KEY_ID",
                        "REQUEST_UPLOAD_PUBLIC_KEY",
                    }));

  // The observational majority, spot-checked at both ends.
  EXPECT_FALSE(Module::IsModuleEventExtensionPoint("APPLICATION_LOADED"));
  EXPECT_FALSE(Module::IsModuleEventExtensionPoint("TAB_ACTIVATED"));
  EXPECT_FALSE(Module::IsModuleEventExtensionPoint("DOCUMENT_SAVED"));
  EXPECT_FALSE(Module::IsModuleEventExtensionPoint("MAIN_WINDOW_CLOSING"));
}

// Every event the in-tree modules declare must be one the host fires.
// Otherwise the module builds, signs, loads -- and its handler never runs.
TEST(ModuleEventRegistryTest, EveryEventTheShippedModulesDeclareIsFired) {
  const QDir root(GF_TEST_SOURCE_DIR "/modules/src");
  if (!root.exists()) GTEST_SKIP() << "modules submodule is not checked out";

  int manifests = 0;
  for (const auto& entry :
       root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
    QFile file(root.filePath(entry + "/module.json"));
    if (!file.exists()) continue;
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    manifests++;

    const auto doc = QJsonDocument::fromJson(file.readAll());
    ASSERT_TRUE(doc.isObject()) << entry.toStdString();
    for (const auto& value : doc.object().value("events").toArray()) {
      const auto id = value.toString();
      EXPECT_TRUE(Module::IsKnownModuleEvent(id))
          << entry.toStdString() << " declares \"" << id.toStdString()
          << "\", which this host never fires; the subscription would be "
             "refused and the handler would never run";
    }
  }

  EXPECT_GT(manifests, 0) << "no module manifests were read, so this test "
                             "asserted nothing";
}

// ---------------------------------------------------------- the boundary

/**
 * @brief The built modules import nothing from the host.
 *
 * The link policy in gf_add_module() already forbids this, and this is the
 * check on the ARTEFACT rather than on the intent: a link line is a claim
 * and a binary is evidence.
 *
 * Shelling the script rather than reimplementing it, so there is one
 * definition of the rule and CI, the post-build step and this test all run
 * the same one.
 */
TEST(ModuleEventRegistryTest, NoBuiltModuleReachesTheHostDirectly) {
  const QString script = GF_TEST_MODULE_BOUNDARY_SCRIPT;
  const QString root = GF_TEST_MODULE_NAMESPACE_ROOT;

  if (!QFile::exists(script)) {
    GTEST_SKIP() << "boundary script missing: " << script.toStdString();
  }
  if (!QDir(root).exists()) {
    GTEST_SKIP() << "no module namespace root at " << root.toStdString();
  }

  QProcess proc;
  proc.start(script, {"--namespace-root", root, "--expect-count",
                      QString::number(GF_REGISTERED_MODULE_COUNT)});
  ASSERT_TRUE(proc.waitForStarted(10000));
  ASSERT_TRUE(proc.waitForFinished(120000));

  EXPECT_EQ(proc.exitCode(), 0)
      << QString::fromUtf8(proc.readAllStandardOutput()).toStdString()
      << QString::fromUtf8(proc.readAllStandardError()).toStdString();
}

}  // namespace GpgFrontend::Test
