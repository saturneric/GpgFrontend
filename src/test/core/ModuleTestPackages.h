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

/// Every module descriptor the build produced, in a stable order.
///
/// One namespace per module -- modules/<key>/module.gfmodule -- so this walks
/// directories rather than globbing, which is also what the Host does.
inline auto BuiltModulePackages() -> QFileInfoList {
  const QDir root(QCoreApplication::applicationDirPath() + "/modules");

  QFileInfoList found;
  for (const auto& candidate :
       root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
    const QFileInfo descriptor(candidate.absoluteFilePath() +
                               "/module.gfmodule");
    if (descriptor.isFile()) found.append(descriptor);
  }
  return found;
}

/// The one whose entry native is biggest, or empty if this build made none.
///
/// Size rather than name, and for a stated reason: a per-byte cost shows up on
/// the largest module and hides on the smallest. It is the NATIVE that is
/// measured now -- every descriptor is a few kilobytes of metadata, so
/// comparing those would pick one essentially at random.
inline auto LargestBuiltModulePackage() -> QString {
  const auto built = BuiltModulePackages();
  if (built.isEmpty()) return {};

  const auto native_size = [](const QFileInfo& descriptor) -> qint64 {
    const QDir native(descriptor.absolutePath() + "/native");
    qint64 total = 0;
    for (const auto& file : native.entryInfoList(QDir::Files)) {
      total += file.size();
    }
    return total;
  };

  const auto* largest = &built.first();
  for (const auto& info : built) {
    if (native_size(info) > native_size(*largest)) largest = &info;
  }
  return largest->absoluteFilePath();
}

/// The seed this build signs module descriptors with.
///
/// Tests use the real one rather than a key of their own, and they have no
/// choice: BuildModuleDescriptor() refuses a seed that does not derive the public
/// key compiled into gf_core, precisely so a descriptor the Host could not
/// load cannot be produced. That makes every packaging test exercise the
/// signing path a build actually takes.
///
/// It lives beside the build tree, not inside the artifacts directory, because
/// nothing that stages or installs artifacts should be able to sweep it up.
inline auto BuildSigningSeed() -> QByteArray {
  const QDir artifacts(QCoreApplication::applicationDirPath());
  QFile seed(
      artifacts.absoluteFilePath("../.module-build-key/module-build.seed"));
  if (!seed.open(QIODevice::ReadOnly)) return {};
  const auto bytes = seed.readAll();
  seed.close();
  return bytes;
}

}  // namespace GpgFrontend::Test
