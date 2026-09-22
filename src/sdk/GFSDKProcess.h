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

#include <stddef.h>

#include "GFSDKContext.h"

/**
 * @file GFSDKProcess.h
 * @brief Running an external program.
 *
 * Its own capability, and the most consequential thing in this SDK. One call,
 * because the single-command entry point was always the batch of one; running
 * one program is a convenience the C++ facade provides over this.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run every command in @p contexts and wait for all of them.
 *
 * Each context's callback fires as its command completes. The array and the
 * contexts are borrowed for the duration of the call, which is synchronous;
 * the `data` pointer inside each remains the caller's to free.
 *
 * @return 0 when the batch ran, negative on a bad request
 */
int GFProcessExecute(GFSDKContext* ctx, GFCommandExecuteContext** contexts,
                     size_t count);

#ifdef __cplusplus
}
#endif
