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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <iostream>

#include "GpgFrontendTest.h"
#include "core/module/ModuleCapability.h"
#include "ui/command/CommandRegistry.h"
#include "ui/lua/LuaModuleRuntime.h"

/**
 * @file GFUiBundledScriptTest.cpp
 * @brief Every bundled module's UI script loads as it does at startup.
 *
 * Scripts load when their module activates, before the Host's window exists
 * -- so before the Host's own commands (org.gpgfrontend.*) are registered.
 * A script that takes a Host command at load fails there, and the whole
 * script rolls back: every entry and page it would have added is gone. No
 * window is built in the tests either, which is exactly that condition.
 *
 * Each script is loaded into a fresh runtime under its module's id and
 * granted capabilities, against the commands and widgets the loaded module
 * registered; a module that is not loaded in this run is skipped.
 */

namespace GpgFrontend::Test {

namespace {

auto ModulesDir() -> QDir {
  return QDir(QString(GF_TEST_SOURCE_DIR) + "/modules/src");
}

struct Bundled {
  QString dir;
  QString id;
  uint32_t caps = 0;
  QByteArray script;
};

auto BundledScripts() -> QList<Bundled> {
  QList<Bundled> out;
  for (const auto& dir :
       ModulesDir().entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    QFile manifest(ModulesDir().filePath(dir + "/module.json"));
    QFile script(ModulesDir().filePath(dir + "/ui/main.lua"));
    if (!manifest.open(QIODevice::ReadOnly) ||
        !script.open(QIODevice::ReadOnly)) {
      continue;
    }
    const auto m = QJsonDocument::fromJson(manifest.readAll()).object();
    QStringList caps;
    for (const auto& c : m.value("capabilities").toArray()) {
      caps << c.toString();
    }
    out.append({dir, m.value("id").toString(),
                Module::ModuleCapabilityMask(caps), script.readAll()});
  }
  return out;
}

}  // namespace

TEST(BundledScriptTest, EveryBundledScriptLoadsWithoutTheHostsWindow) {
  const auto scripts = BundledScripts();
  ASSERT_FALSE(scripts.isEmpty()) << "no bundled module scripts found";

  static const QRegularExpression kCommandRef(
      QStringLiteral(R"re(commands\.get\("([^"]+)"\))re"));
  int loaded = 0;
  for (const auto& b : scripts) {
    SCOPED_TRACE(b.dir.toStdString());

    // Only a Host window registers org.gpgfrontend.*; a script must not
    // need one of those to load.
    bool module_loaded = true;
    for (auto it = kCommandRef.globalMatch(QString::fromUtf8(b.script));
         it.hasNext();) {
      const auto id = it.next().captured(1);
      EXPECT_FALSE(id.startsWith("org.gpgfrontend."))
          << "takes the Host command " << id.toStdString()
          << " at load, before the Host has registered it";
      if (!UI::CommandRegistry::Instance().Contains(id)) module_loaded = false;
    }
    if (!module_loaded) {
      std::cout << "[ SKIPPED ] " << b.dir.toStdString() << ": not loaded\n";
      continue;
    }

    UI::Lua::LuaModuleRuntime rt(b.id, b.caps);
    QString error;
    EXPECT_TRUE(rt.Load(b.script, "main.lua", &error)) << error.toStdString();
    std::cout << "[ LOADED  ] " << b.dir.toStdString() << ": "
              << rt.Actions().size() << " actions\n";
    ++loaded;
  }
  if (loaded == 0) GTEST_SKIP() << "no bundled module is loaded in this run";
}

}  // namespace GpgFrontend::Test
