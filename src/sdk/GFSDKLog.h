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

#include "GFSDKContext.h"

/**
 * @file GFSDKLog.h
 * @brief One log entry point, and a level predicate.
 *
 * Five severity-named functions used to exist here. They differed by an
 * integer, so they are now one call and a set of C++ macros in GFModuleLog.h
 * that read better at a call site than a constant does.
 *
 * The module's identity is NOT an argument. The host reads it from the
 * context it validates, which is the only version of it a module cannot get
 * wrong. Every pointer below is borrowed for the duration of the call.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Emit one message, carrying where it came from.
 *
 * @param severity one of @ref GFLogSeverity, as an int
 * @param file `__FILE__`, or NULL to emit without a source location
 * @param line `__LINE__`, ignored when @p file is NULL
 * @param function `Q_FUNC_INFO` or `__func__`, or NULL
 * @param msg the message; NULL is treated as empty
 */
void GFLogAt(GFSDKContext* ctx, int severity, const char* file, int line,
             const char* function, const char* msg);

/**
 * @brief Whether a message at @p severity would be emitted at all.
 *
 * For skipping the work of building a message about to be discarded. The log
 * macros format through QString::arg, which is not free.
 */
int GFLogEnabled(GFSDKContext* ctx, int severity);

#ifdef __cplusplus
}
#endif
