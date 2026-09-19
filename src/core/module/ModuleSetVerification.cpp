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

#include "core/module/ModuleSetVerification.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include "core/module/ModuleDescriptor.h"
#include "core/module/ModuleEntryBinding.h"
#include "core/module/ModuleManager.h"
#include "core/module/ModuleNamespace.h"
#include "core/module/ModulePreparedEntry.h"
#include "core/module/ModuleTrustRoot.h"

namespace GpgFrontend::Module {

auto VerifyModuleSet(const QString& root, int expected_count)
    -> ModuleSetVerification {
  ModuleSetVerification result;

  const QDir dir(root);
  if (!dir.exists()) {
    result.problems.append({root, "this directory does not exist"});
    return result;
  }

  // Which namespace declared which id, so a duplicate can name both sides.
  QMap<QString, QString> claimed_by;

  for (const auto& ns :
       dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
    const auto descriptor =
        ns.absoluteFilePath() + "/" + kModuleDescriptorFileName;

    if (!QFileInfo(descriptor).isFile()) {
      // Not fatal on its own: a tree may legitimately hold something else.
      // It becomes fatal below only if it holds native libraries, which would
      // make it a module namespace with no descriptor to bind them.
      const QDir native(ns.absoluteFilePath() + "/native");
      if (native.exists() && !native.entryInfoList(QDir::Files).isEmpty()) {
        result.problems.append(
            {ns.fileName(),
             "it holds native libraries but no module.gfmodule, so nothing "
             "vouches for them"});
      } else {
        result.warnings.append(
            {ns.fileName(), "not a module namespace; it holds no descriptor"});
      }
      continue;
    }

    const auto verdict = VerifyModuleDescriptor(descriptor);
    if (!verdict.ok) {
      result.problems.append(
          {ns.fileName(),
           QString("its descriptor was refused: %1 (%2)")
               .arg(verdict.reason,
                    QString::fromUtf8(
                        ModuleDescriptorStatusToString(verdict.status)))});
      continue;
    }

    const auto& id = verdict.manifest.id;

    const auto expected_key = ModuleDirectoryKey(id);
    if (ns.fileName() != expected_key) {
      result.problems.append(
          {ns.fileName(), QString("it declares \"%1\", whose namespace is "
                                  "\"%2\"")
                              .arg(id, expected_key)});
      continue;
    }

    if (claimed_by.contains(id)) {
      result.problems.append(
          {ns.fileName(), QString("\"%1\" is already claimed by \"%2\"")
                              .arg(id, claimed_by.value(id))});
      continue;
    }
    claimed_by.insert(id, ns.fileName());

    const ModuleNativeRoot native{ns.absoluteFilePath() + "/native"};
    const auto entry = ResolveAndVerifyNativeEntry(verdict.manifest, native);
    if (!entry.ok) {
      result.problems.append(
          {ns.fileName(),
           QString("its entry native was refused: %1 (%2)")
               .arg(entry.reason, QString::fromUtf8(ModuleEntryStatusToString(
                                      entry.status)))});
      continue;
    }

    // Anything else in native/ is a private helper: legitimate, unlisted by
    // design, and the platform loader's business rather than the descriptor's.
    // A stale one left by a rename is worth saying out loud, and nothing more:
    // making it fatal would invent a requirement the format does not have.
    const QDir native_dir(native.path);
    for (const auto& file : native_dir.entryInfoList(QDir::Files)) {
      if (file.absoluteFilePath() == entry.path) continue;
      // The preparation seal is a build-tree note to the finalize step, not a
      // shipped artifact and not a helper. Staging drops it; reporting it here
      // would train a reader to ignore this warning.
      if (file.fileName() == kPreparedEntrySealFileName) continue;
      result.warnings.append(
          {ns.fileName() + "/native/" + file.fileName(),
           "a private helper the descriptor does not bind; the platform "
           "loader resolves it, and nothing here vouches for it"});
    }

    result.verified.insert(id, descriptor);
    result.entries.insert(ns.fileName(), entry.path);
  }

  if (expected_count >= 0 && result.verified.size() != expected_count) {
    result.problems.append(
        {root, QString("%1 modules verified, and %2 were expected")
                   .arg(result.verified.size())
                   .arg(expected_count)});
  }

  result.ok = result.problems.isEmpty();
  return result;
}

auto FindNativeModuleBinariesOutside(const QString& root,
                                     const QString& namespace_root)
    -> QStringList {
  QStringList found;

  const auto allowed = QFileInfo(namespace_root).canonicalFilePath();

  QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QFileInfo info(it.next());

    // By name rather than by header: a module library that failed to link, or
    // was truncated by a staging step, is exactly as much of a leak as one
    // that would load.
    if (!IsModuleLibraryFileName(info.fileName())) continue;

    const auto path = info.canonicalFilePath();
    if (!allowed.isEmpty() && path.startsWith(allowed + "/")) continue;

    found.append(path);
  }

  found.sort();
  return found;
}

}  // namespace GpgFrontend::Module
