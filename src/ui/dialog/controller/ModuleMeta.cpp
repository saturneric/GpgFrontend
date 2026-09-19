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

#include "ui/dialog/controller/ModuleMeta.h"

#include <QDir>
#include <QFileInfo>
#include <QObject>

namespace GpgFrontend::UI {

namespace {

/// How much of a digest is worth showing. A hash has no break opportunity, so
/// the choice is between shortening it and widening the whole panel.
constexpr int kHashDisplayLength = 16;

auto ShortHash(const QString &hash) -> QString {
  if (hash.isEmpty()) return QObject::tr("N/A");
  return hash.size() > kHashDisplayLength
             ? hash.left(kHashDisplayLength) + QStringLiteral("...")
             : hash;
}

}  // namespace

auto UnattributedSignatureCaveat() -> QString {
  return QObject::tr(
      "The signature shows this package was not altered after it was built. "
      "It does not show who built it: the key travels inside the package.");
}

auto BuildModuleRows(const Module::ModuleProvenance &module)
    -> QVector<MetaListRow> {
  QVector<MetaListRow> rows;
  // Established once, by the manager, from Module::IsPackaged(). This used to
  // be re-derived here as manifest.has_value() while ModuleListView asked
  // IsPackaged() -- two answers to one question.
  const auto packaged = module.packaged;

  // ---- what this application established -------------------------------

  rows.append({.kind = MetaRowKind::kSection,
               .caption = QObject::tr("Verified by GpgFrontend")});

  rows.append({.caption = QObject::tr("Identifier"),
               .value = module.identifier,
               .emphasis = true});

  rows.append({.caption = QObject::tr("Version"), .value = module.version});

  if (module.integrated) {
    rows.append({.caption = QObject::tr("Origin"),
                 .value = QObject::tr("Built into this application"),
                 .detail = QObject::tr(
                     "Ships with GpgFrontend and is not loaded from disk.")});
  } else if (packaged) {
    rows.append({.caption = QObject::tr("Origin"),
                 .value = QObject::tr("Signed package"),
                 .detail = UnattributedSignatureCaveat()});
    rows.append({.caption = QObject::tr("Package"),
                 .value = QDir::toNativeSeparators(module.source_package_path),
                 .path = true});
  } else {
    // Loose libraries are a development convenience. Saying so is the honest
    // way to explain why the rows below it carry a caveat.
    rows.append({.caption = QObject::tr("Origin"),
                 .value = QObject::tr("Unsigned library"),
                 .detail = QObject::tr(
                     "Loaded from a loose library, which nothing vouches for. "
                     "Released builds ship signed packages only."),
                 .degraded = true});
    if (!module.source_package_path.isEmpty()) {
      rows.append(
          {.caption = QObject::tr("File"),
           .value = QDir::toNativeSeparators(module.source_package_path),
           .path = true});
    }
  }

  // The module's own ABI, not the host's. These two rows used to show
  // GetModuleSDKVersion() and GetModuleQtEnvVersion(), both of which return
  // the HOST's values -- so every module displayed the same pair, and the
  // panel said nothing at all while appearing to say something.
  if (module.sdk_abi > 0) {
    rows.append({.caption = QObject::tr("SDK ABI"),
                 .value = QString::number(module.sdk_abi)});
  }

  if (packaged) {
    const auto &manifest = module.manifest.value();
    if (!manifest.platform_os.isEmpty()) {
      rows.append(
          {.caption = QObject::tr("Built for"),
           .value = manifest.platform_arch.isEmpty()
                        ? manifest.platform_os
                        : QStringLiteral("%1 %2").arg(manifest.platform_os,
                                                      manifest.platform_arch)});
    }
    if (!manifest.platform_qt.isEmpty()) {
      rows.append({.caption = QObject::tr("Built against Qt"),
                   .value = manifest.platform_qt});
    }
    if (!manifest.capabilities.isEmpty()) {
      rows.append({.caption = QObject::tr("Capabilities"),
                   .value = manifest.capabilities.join(QStringLiteral(", "))});
    }
  }

  if (!module.hash.isEmpty()) {
    rows.append({.caption = QObject::tr("Digest"),
                 .value = ShortHash(module.hash),
                 .detail = module.hash,
                 .path = true});
  }

  // ---- what the module says about itself --------------------------------

  // One source. The provenance carries the manifest's metadata for a packaged
  // module and the module's own for a loose one, so this no longer has to know
  // which to read -- only whether it was verified, which `packaged` says.
  const auto name = packaged ? module.metadata.value("Name") : QString();
  const auto description =
      packaged ? module.metadata.value("Description") : QString();
  const auto author = packaged ? module.metadata.value("Author") : QString();

  if (name.isEmpty() && description.isEmpty() && author.isEmpty()) {
    // Nothing to show, and nothing to pretend about. A loose library carries
    // no metadata at all: the identifier above is all there is, which is
    // exactly why the list stops here rather than repeating it as a title.
    return rows;
  }

  // Not claims when they are packaged: a signature the host checked before any
  // of this module's code ran covers them. That is the whole reason the
  // heading distinguishes the two halves rather than the rows being marked
  // individually.
  rows.append({.kind = MetaRowKind::kSection,
               .caption = packaged ? QObject::tr("Described by its package")
                                   : QObject::tr("Claimed by the module")});

  if (!name.isEmpty()) {
    rows.append({.caption = QObject::tr("Name"),
                 .value = name,
                 .unverified = !packaged});
  }
  if (!author.isEmpty()) {
    rows.append({.caption = QObject::tr("Author"),
                 .value = author,
                 .unverified = !packaged});
  }
  if (!description.isEmpty()) {
    rows.append({.caption = QObject::tr("Description"),
                 .value = description,
                 .unverified = !packaged});
  }

  return rows;
}

}  // namespace GpgFrontend::UI
