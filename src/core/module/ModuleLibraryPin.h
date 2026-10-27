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

#include "GFCoreExport.h"

namespace GpgFrontend::Module {

/**
 * @file ModuleLibraryPin.h
 * @brief Modules whose library stays mapped until the process exits.
 *
 * A module that provides an HTML renderer has loaded an engine -- Chromium,
 * under QtWebEngine -- whose threads and statics cannot be torn out of a
 * live process. Deactivating such a module withdraws everything it provides
 * and closes its gate as usual; it only skips the unmap. A later activation
 * maps the same image again, which the loader treats as a no-op.
 *
 * Process-wide and append-only: a pin is never lifted.
 */

/// Keep @p module_id's library mapped until exit. Thread-safe.
void GF_CORE_EXPORT PinModuleLibrary(const QString& module_id);

/// Whether @p module_id was pinned. Thread-safe.
auto GF_CORE_EXPORT IsModuleLibraryPinned(const QString& module_id) -> bool;

}  // namespace GpgFrontend::Module
