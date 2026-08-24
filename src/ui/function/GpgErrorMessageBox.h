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

#include "core/typedef/GpgTypedef.h"

namespace GpgFrontend::UI {

/**
 * @brief Report the outcome of a gpg operation, whichever way it went.
 *
 * A no-error code produces a plain success box; anything else is handed to
 * RaiseFailureMessageBox() so a failure always carries its source and
 * description rather than a bare "it did not work".
 *
 * @param parent dialog parent
 * @param err the error returned by the operation
 */
void GF_UI_EXPORT RaiseMessageBox(QWidget* parent, GpgError err);

/**
 * @brief Report a failed gpg operation, with the engine's own diagnosis.
 *
 * @param parent dialog parent
 * @param err the error returned by the operation
 * @param msg extra detail from the caller, often the engine's status line
 */
void GF_UI_EXPORT RaiseFailureMessageBox(QWidget* parent, GpgError err,
                                         const QString& msg = {});

}  // namespace GpgFrontend::UI
