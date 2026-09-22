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

#include <core/function/SecureMemoryAllocator.h>
#include <core/typedef/CoreTypedef.h>

#include "core/model/GFBuffer.h"

Q_DECLARE_LOGGING_CATEGORY(sdk)

#define LOG_D() qCDebug(sdk)
#define LOG_I() qCInfo(sdk)
#define LOG_W() qCWarning(sdk)
#define LOG_E() qCCritical(sdk)
#define LOG_F() qCFatal(sdk)

struct GFModuleEventParam;

/**
 * @brief Copy @p str into a new UTF-8 C string from the normal arena.
 *
 * @return caller-owned; free with GFFreeMemory
 */
auto GFStrDup(const QString &str) -> char *;

/**
 * @brief Copies raw octets into an SDK-allocated buffer, byte for byte.
 *
 * Unlike GFStrDup this performs no text conversion and stops at nothing: an
 * embedded NUL is copied like any other byte. The result is NUL-terminated one
 * past @p size so callers that still treat it as a C string see something
 * sane, but @p size is the authoritative length.
 *
 * @param bytes the octets to copy
 * @param[out] size receives the exact byte count (may be nullptr)
 * @return caller-owned buffer, free with GFFreeMemory
 */
auto GFBytesDup(const QByteArray &bytes, size_t *size) -> char *;

/**
 * @brief Read a C string the host OWNS, and free it.
 *
 * Only for strings whose ownership really was transferred to the host, such
 * as the fields of an event answer. Null-safe.
 */
auto GFUnStrDup(char *str) -> QString;

/// The same, for a transferred string typed as const.
auto GFUnStrDup(const char *str) -> QString;

/**
 * @brief Read a borrowed C string WITHOUT taking ownership of it.
 *
 * The counterpart to GFUnStrDup, and the one to reach for at a public entry
 * point. Under the SDK's ownership rule an ARGUMENT is always borrowed: the
 * caller created it and the caller releases it. GFUnStrDup frees what it is
 * handed, which is right only where the SDK genuinely took ownership.
 *
 * Getting this backwards is the mistake the whole redesign exists to remove:
 * freeing an argument means a module must pre-allocate every string it passes
 * (the DUP(...) wrapping at every call site), and forgetting to means the SDK
 * frees memory it never owned.
 *
 * Null-safe: a null pointer reads as an empty string.
 */
auto GFStrView(const char *str) -> QString;

/**
 * @brief Read an event answer's parameter list, and free all of it.
 *
 * The list is transferred: nodes and names come from the normal arena and
 * values from the secure arena (see GFModuleEventAnswer).
 */
auto ConvertEventParamsToMap(GFModuleEventParam *params)
    -> QMap<QString, GpgFrontend::GFBuffer>;
