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
#include <QDirIterator>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>

#include "GpgFrontendTest.h"

/**
 * @file GpgCoreTestSdkLayering.cpp
 * @brief The three-layer rule, checked on the sources and on the binaries.
 *
 * ## Why these are tests rather than review notes
 *
 * The arrangement this file pins is:
 *
 * ```
 *   module -> gf_sdk (public SDK, stateless) -> GFHostApi -> gf_host_api
 * ```
 *
 * Both halves of it are already enforced by the build -- `src/sdk/api/` clears
 * the inherited include path, so `#include "core/..."` there fails to compile,
 * and hidden visibility keeps the public SDK out of every host artefact's
 * dynamic symbol table. Neither of those is self-announcing, though: one is a
 * `set_property(DIRECTORY ...)` line and the other a default that a single
 * stray `__attribute__((visibility("default")))` would undo. Somebody restoring
 * `include_directories()` "to fix an include error" would silently reopen the
 * door, and nothing would say so.
 *
 * So the rules are asserted on the artefacts: the sources for the first, and
 * the built binaries for the second. A link line is a claim; a binary is
 * evidence.
 */

namespace GpgFrontend::Test {

namespace {

auto SdkApiDir() -> QString {
  return QString(GF_TEST_SOURCE_DIR) + "/src/sdk/api";
}

}  // namespace

// ------------------------------------------------------- layer separation

/**
 * @brief Nothing under src/sdk/api/ may reach into the host.
 *
 * The public SDK is compiled into every module. If one of its translation
 * units included `core/`, the module would need `gf_core` to link, and the
 * capability table would stop being the only way in.
 *
 * `<gpgme.h>` is listed alongside them for a different reason: it is the
 * clearest sign that a wrapper has started doing crypto itself instead of
 * handing the work to a primitive.
 */
TEST(SdkLayeringTest, ThePublicSdkIncludesNothingFromTheHost) {
  const QDir dir(SdkApiDir());
  ASSERT_TRUE(dir.exists()) << SdkApiDir().toStdString();

  static const QRegularExpression kInclude(
      R"(^\s*#\s*include\s*[<"]([^>"]+)[>"])");

  QStringList offences;
  int scanned = 0;

  QDirIterator it(dir.absolutePath(), {"*.h", "*.hpp", "*.c", "*.cpp"},
                  QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const auto path = it.next();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
    ++scanned;

    int line_no = 0;
    while (!file.atEnd()) {
      const auto line = QString::fromUtf8(file.readLine());
      ++line_no;
      const auto m = kInclude.match(line);
      if (!m.hasMatch()) continue;

      const auto header = m.captured(1);
      if (header.startsWith("core/") || header.startsWith("ui/") ||
          header == "gpgme.h") {
        offences << QString("%1:%2: %3")
                        .arg(dir.relativeFilePath(path))
                        .arg(line_no)
                        .arg(header);
      }
    }
  }

  EXPECT_GT(scanned, 0) << "the scan found no sources, so it proved nothing";
  EXPECT_TRUE(offences.isEmpty())
      << "the module-side SDK reached into the host:\n"
      << offences.join("\n").toStdString();
}

// ------------------------------------------------------- one host path

/**
 * @brief No host artefact exports a public SDK entry point.
 *
 * This is the mirror of `NoBuiltModuleReachesTheHostDirectly`, which checks
 * the same boundary from the module's side. Together they say the whole thing:
 * modules do not import these names, and the host does not offer them.
 *
 * `GFHostApiInstallBridge` is checked here too. Caller and implementation are
 * in the same final link unit, so an exported one would be a second way to
 * install the bridge, reachable by anything that can dlsym the application.
 *
 * Shelling the script for the same reason the module-side test does: one
 * definition of the rule, run by CI, by the post-build step and by this test.
 */
TEST(SdkLayeringTest, NoHostArtefactExportsThePublicSdk) {
  const QString script = GF_TEST_MODULE_BOUNDARY_SCRIPT;
  const QString artifacts = GF_TEST_ARTIFACT_DIR;

  if (!QFile::exists(script)) {
    GTEST_SKIP() << "boundary script missing: " << script.toStdString();
  }
  if (!QDir(artifacts).exists()) {
    GTEST_SKIP() << "no artifact dir at " << artifacts.toStdString();
  }

  QProcess proc;
  proc.start(script, {"--host", "--artifact-dir", artifacts});
  ASSERT_TRUE(proc.waitForStarted(10000));
  ASSERT_TRUE(proc.waitForFinished(120000));

  EXPECT_EQ(proc.exitCode(), 0)
      << QString::fromUtf8(proc.readAllStandardOutput()).toStdString()
      << QString::fromUtf8(proc.readAllStandardError()).toStdString();
}

}  // namespace GpgFrontend::Test
