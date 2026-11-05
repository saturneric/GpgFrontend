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

#include "GFShellExport.h"

// The Status Panel's value types, now the core's (InfoBoardCard.h), which the
// phone shares; the names stay where the desktop has always found them.

#include "shell/result_analyse/InfoBoardCard.h"

namespace GpgFrontend {

using GpgFrontend::InfoBoardCard;
using GpgFrontend::InfoBoardCardKind;
using GpgFrontend::InfoBoardFieldGroup;
using GpgFrontend::InfoBoardFieldMeta;
using GpgFrontend::InfoBoardFieldRole;
using GpgFrontend::InfoBoardOutcome;
using GpgFrontend::InfoBoardSource;
using GpgFrontend::InfoBoardStatus;
using GpgFrontend::kINFO_ERROR_CRITICAL;
using GpgFrontend::kINFO_ERROR_NEUTRAL;
using GpgFrontend::kINFO_ERROR_OK;
using GpgFrontend::kINFO_ERROR_WARN;

}  // namespace GpgFrontend
