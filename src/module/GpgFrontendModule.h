/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

/**
 * Project internal dependencies
 */
#include "GpgFrontend.h"
#include "GpgFrontendModuleExport.h"
#include "core/GpgFrontendCore.h"

/**
 * @brief logger for inner
 *
 */
#define MODULE_LOG_TRACE(...) GF_LOG_TRACE("module", __VA_ARGS__)
#define MODULE_LOG_DEBUG(...) GF_LOG_DEBUG("module", __VA_ARGS__)
#define MODULE_LOG_INFO(...) GF_LOG_INFO("module", __VA_ARGS__)
#define MODULE_LOG_WARN(...) GF_LOG_WARN("module", __VA_ARGS__)
#define MODULE_LOG_ERROR(...) GF_LOG_ERROR("module", __VA_ARGS__)
