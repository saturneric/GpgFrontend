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

#include <QHash>
#include <QMutex>
#include <QMutexLocker>

#include "core/module/ModuleNamespace.h"

namespace GpgFrontend::Module {

namespace {

/// What an unattributable message is filed under.
///
/// Spelled out rather than left to `ModuleIdLeaf("")`, which answers "m": a
/// message nobody could attribute must not look like one from a module called
/// `m`.
constexpr auto kUnknownModuleCategory = "module.unknown";

/// One category, and the storage keeping its name alive.
///
/// `QLoggingCategory` takes a `const char*` it does not copy, so the QByteArray
/// has to outlive it. Keeping the two together is what makes that guarantee a
/// property of the type rather than a rule somebody has to remember.
struct OwnedCategory {
  QByteArray name;
  QLoggingCategory* category;
};

/// Held by pointer, and never deleted.
///
/// Not an oversight and not a QHash workaround: a category has to outlive every
/// log statement that could name it, which is to say the process. Owning them
/// through a smart pointer would promise a teardown that must never happen.
auto Registry() -> QHash<QString, OwnedCategory*>& {
  static QHash<QString, OwnedCategory*> registry;
  return registry;
}

auto RegistryMutex() -> QMutex& {
  static QMutex mutex;
  return mutex;
}

/// Resolve a category name to the single category object bearing it.
///
/// Keyed by the NAME, not by the module id, so the two ids that share a leaf
/// share the one category object rather than getting two objects with equal
/// names -- which Qt would treat as two registrations of the same category and
/// which would make a filter rule's effect depend on which object a caller
/// happened to hold.
auto CategoryNamed(const QString& name) -> const QLoggingCategory& {
  const QMutexLocker locker(&RegistryMutex());

  auto& registry = Registry();
  auto it = registry.find(name);
  if (it == registry.end()) {
    auto* owned = new OwnedCategory;
    owned->name = name.toUtf8();
    // constData() rather than the QByteArray, and the QByteArray is never
    // reassigned afterwards: the category holds this exact pointer for the
    // life of the process.
    owned->category = new QLoggingCategory(owned->name.constData());
    it = registry.insert(name, owned);
  }

  return *(*it)->category;
}

}  // namespace

auto ModuleLogCategoryName(const QString& module_id) -> QString {
  if (module_id.trimmed().isEmpty()) return kUnknownModuleCategory;
  return QStringLiteral("module.") + ModuleIdLeaf(module_id);
}

auto ModuleLogCategory(const QString& module_id) -> const QLoggingCategory& {
  return CategoryNamed(ModuleLogCategoryName(module_id));
}

auto ModuleTraceLogCategory(const QString& module_id)
    -> const QLoggingCategory& {
  return CategoryNamed(ModuleLogCategoryName(module_id) + ".trace");
}

}  // namespace GpgFrontend::Module
