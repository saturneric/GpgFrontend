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

extern "C" {

/**
 * @brief Emits a trace-level log message from a module.
 * @param msg Null-terminated message string.
 */
void GF_SDK_EXPORT GFModuleLogTrace(const char* msg);

/**
 * @brief Emits a debug-level log message from a module.
 * @param msg Null-terminated message string.
 */
void GF_SDK_EXPORT GFModuleLogDebug(const char* msg);

/**
 * @brief Emits an info-level log message from a module.
 * @param msg Null-terminated message string.
 */
void GF_SDK_EXPORT GFModuleLogInfo(const char* msg);

/**
 * @brief Emits a warning-level log message from a module.
 * @param msg Null-terminated message string.
 */
void GF_SDK_EXPORT GFModuleLogWarn(const char* msg);

/**
 * @brief Emits an error-level log message from a module.
 * @param msg Null-terminated message string.
 */
void GF_SDK_EXPORT GFModuleLogError(const char* msg);
}