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

#include "GpgFrontendResource.h"

#include <QtGlobal>  // Q_INIT_RESOURCE

// Q_INIT_RESOURCE(name) expands to `extern int qInitResources_<name>();
// qInitResources_<name>();` at block scope, so the declaration binds to the
// innermost enclosing namespace. It must therefore be invoked from the GLOBAL
// namespace: calling it inside namespace GpgFrontend would reference a
// non-existent GpgFrontend::qInitResources_* and fail to link.
//
// All three RCC objects are compiled into this same shared library, so their
// qInitResources_* symbols only need internal (hidden) visibility. Referencing
// them here also keeps the linker from garbage-collecting the resource data.
static void GfInitEmbeddedResources() {
  Q_INIT_RESOURCE(gpgfrontend);     // /icons, /test/*, TRANSLATORS
  Q_INIT_RESOURCE(gftranslations);  // /i18n
  Q_INIT_RESOURCE(qttranslations);  // /i18n_qt
}

namespace GpgFrontend {

void InitResources() { GfInitEmbeddedResources(); }

}  // namespace GpgFrontend
