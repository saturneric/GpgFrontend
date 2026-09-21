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

#include "GFSDKVisibility.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief How loud a module's message is.
 *
 * Passed across the ABI as a plain `int`, not as this type: a C enum's
 * underlying type is implementation-defined, and this SDK already passes
 * enumerated arguments as `int` everywhere else. The enum names the values; it
 * is not itself an ABI type.
 */
typedef enum GFLogSeverity {
  GF_LOG_TRACE = 0,
  GF_LOG_DEBUG = 1,
  GF_LOG_INFO = 2,
  GF_LOG_WARN = 3,
  GF_LOG_ERROR = 4,
} GFLogSeverity;

/**
 * @brief Emit one log message, carrying who wrote it and where from.
 *
 * ## What this fixes
 *
 * The older entry points below take a message and nothing else, so the host had
 * only two things to say about any module line: a category of `module`, shared
 * by every module at once, and a source location pointing at this file. Both
 * fields answered a question about the SDK shim rather than about the module.
 * This one carries the module's own identity and its own `__FILE__`/`__LINE__`.
 *
 * ## Attribution, and what it is worth
 *
 * The host prefers its own thread-local record of which module it called into
 * (`GFSdkCurrentModule()`), and falls back to @p module_id only where that does
 * not exist -- which is exactly the case this argument is for: a module's own
 * worker thread, which the host never entered.
 *
 * So on its own thread a module names itself, and could name itself wrongly.
 * That is accepted and bounded: **the log category is a diagnostic, not a trust
 * boundary.** A module can already write arbitrary text into a log line, so
 * misattributing its own output is a cosmetic lie rather than an escalation.
 * The half the module does not control is preferred wherever it exists.
 *
 * ## Ownership
 *
 * Every pointer is BORROWED for the duration of the call. The SDK copies what
 * it needs and frees nothing.
 *
 * @param module_id the caller's canonical module id, or NULL to let the host
 * decide; an id it cannot place is filed under `module.unknown`
 * @param severity one of @ref GFLogSeverity, as an int
 * @param file `__FILE__`, or NULL to emit without a source location
 * @param line `__LINE__`, ignored when @p file is NULL
 * @param function `Q_FUNC_INFO` or `__func__`, or NULL
 * @param msg the message; NULL is treated as empty
 */
GF_SDK_EXPORT void GFModuleLogAt(const char* module_id, int severity,
                                 const char* file, int line,
                                 const char* function, const char* msg);

/**
 * @brief Whether a message at @p severity would be emitted at all.
 *
 * For skipping the work of building a message that is about to be discarded.
 * A module's log macros format through `QString::arg`, which is not free, and
 * the level was previously consulted only after that had already happened.
 *
 * @param module_id as for @ref GFModuleLogAt
 * @param severity one of @ref GFLogSeverity, as an int
 * @return non-zero if the message would be emitted
 */
GF_SDK_EXPORT int GFModuleLogEnabled(const char* module_id, int severity);

/**
 * @brief Emits a trace-level log message from a module.
 *
 * Unattributed and without a source location; @ref GFModuleLogAt is the entry
 * point that carries both. Retained so that anything already linking these
 * keeps working.
 *
 * @param msg Null-terminated message string.
 */
GF_SDK_EXPORT void GFModuleLogTrace(const char* msg);

/**
 * @brief Emits a debug-level log message from a module.
 * @param msg Null-terminated message string.
 */
GF_SDK_EXPORT void GFModuleLogDebug(const char* msg);

/**
 * @brief Emits an info-level log message from a module.
 * @param msg Null-terminated message string.
 */
GF_SDK_EXPORT void GFModuleLogInfo(const char* msg);

/**
 * @brief Emits a warning-level log message from a module.
 * @param msg Null-terminated message string.
 */
GF_SDK_EXPORT void GFModuleLogWarn(const char* msg);

/**
 * @brief Emits an error-level log message from a module.
 * @param msg Null-terminated message string.
 */
GF_SDK_EXPORT void GFModuleLogError(const char* msg);

#ifdef __cplusplus
}
#endif
