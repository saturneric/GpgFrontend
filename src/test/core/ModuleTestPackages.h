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

#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfoList>

/**
 * @file ModuleTestPackages.h
 * @brief Finding the module packages this build produced.
 *
 * Four test files located the same directory in four slightly different ways,
 * two of them sorting differently for reasons neither stated. The directory is
 * one fact about the build layout; if it moves again -- it already did once,
 * from artifacts/module-packages to artifacts/modules -- it should move here.
 */

namespace GpgFrontend::Test {

/// Every `*.gfmodule` the build produced, in a stable order.
inline auto BuiltModulePackages() -> QFileInfoList {
  const QDir packages(QCoreApplication::applicationDirPath() + "/modules");
  return packages.entryInfoList(QStringList{"*.gfmodule"}, QDir::Files,
                                QDir::Name);
}

/// The biggest one, or empty if this build made none.
///
/// Size rather than name, and for a stated reason: a 46 MiB module is where a
/// per-byte cost or a second read shows up, and a 3 MiB one hides it.
inline auto LargestBuiltModulePackage() -> QString {
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) return {};

  const auto* largest = &built.first();
  for (const auto& info : built) {
    if (info.size() > largest->size()) largest = &info;
  }
  return largest->absoluteFilePath();
}

/// The seed this build signs module descriptors with.
///
/// Tests use the real one rather than a key of their own, and they have no
/// choice: BuildModulePackage() refuses a seed that does not derive the public
/// key compiled into gf_core, precisely so a descriptor the Host could not
/// load cannot be produced. That makes every packaging test exercise the
/// signing path a build actually takes.
///
/// It lives beside the build tree, not inside the artifacts directory, because
/// nothing that stages or installs artifacts should be able to sweep it up.
inline auto BuildSigningSeed() -> QByteArray {
  const QDir artifacts(QCoreApplication::applicationDirPath());
  QFile seed(artifacts.absoluteFilePath("../.module-build-key/module-build.seed"));
  if (!seed.open(QIODevice::ReadOnly)) return {};
  const auto bytes = seed.readAll();
  seed.close();
  return bytes;
}

}  // namespace GpgFrontend::Test
