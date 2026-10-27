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

#include "ModuleLibraryPin.h"

#include <QMutex>
#include <QSet>

namespace GpgFrontend::Module {

namespace {

struct Pins {
  QMutex mutex;
  QSet<QString> modules;
};

auto ThePins() -> Pins& {
  static Pins pins;
  return pins;
}

}  // namespace

void PinModuleLibrary(const QString& module_id) {
  if (module_id.isEmpty()) return;
  auto& pins = ThePins();
  const QMutexLocker lock(&pins.mutex);
  pins.modules.insert(module_id);
}

auto IsModuleLibraryPinned(const QString& module_id) -> bool {
  auto& pins = ThePins();
  const QMutexLocker lock(&pins.mutex);
  return pins.modules.contains(module_id);
}

}  // namespace GpgFrontend::Module
