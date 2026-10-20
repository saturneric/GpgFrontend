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

#include "core/typedef/CoreTypedef.h"
#include "ui/struct/GpgOperaResult.h"

namespace GpgFrontend::UI {

/**
 * @brief Whether the staged output of a file or archive operation may be
 * committed to its final path.
 *
 * Status contract (see GpgResultAnalyse::GetStatus): > 0 is success, 0 is
 * valid output with non-fatal warnings (e.g. an unknown recipient or a missing
 * signer key) and is kept, < 0 is a failure and the output is discarded. A
 * successful staging and finalization step is still required on top of this.
 *
 * Exposed primarily for unit testing.
 *
 * @param results the results of every operation that wrote staged output
 * @return true when there is at least one result and none failed
 */
auto GF_UI_EXPORT
IsFileOperaSuccessful(const QContainer<GpgOperaResult>& results) -> bool;

}  // namespace GpgFrontend::UI
