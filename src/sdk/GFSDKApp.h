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

#include <stdint.h>

#include "GFSDKContext.h"

/**
 * @file GFSDKApp.h
 * @brief Facts about the running application, and the one helper that needs
 *        none of them.
 *
 * Every fact here is read-only and always available: a module that could not
 * ask its own version is not usefully restricted, it is just harder to
 * support. They are separate functions rather than a `get_fact(int)` because
 * `version` and `is_flatpak` have nothing in common but their caller, and a
 * lookup by code would only have moved the type confusion into a cast.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Borrowed, static for the life of the process. */
const char* GFAppVersion(GFSDKContext* ctx);
const char* GFAppGitCommitHash(GFSDKContext* ctx);
const char* GFAppQtVersion(GFSDKContext* ctx);

/**
 * @brief The User-Agent the application itself sends.
 *
 * The only network-adjacent thing the host offers. The requests themselves
 * are the module's own, through Qt; "network" is a declaration the host
 * records and shows to the user, not a capability it can withhold.
 */
const char* GFAppUserAgent(GFSDKContext* ctx);

/** @brief Active locale, e.g. "en_US". Owned; free with GFMemFree(NORMAL). */
char* GFAppLocale(GFSDKContext* ctx);

int GFAppIsFlatpak(GFSDKContext* ctx);

/**
 * @brief How the application secure key itself is protected.
 *
 * Everything in the durable cache is encrypted under that key, so this is the
 * ceiling on how well a module can protect anything it stores. A module that
 * persists a credential is expected to consult this and decline to do so
 * silently when the answer is 0.
 *
 * @return 0 unprotected, 1 system keychain, 2 user PIN, -1 unknown
 */
int GFAppKeyProtectionLevel(GFSDKContext* ctx);

/**
 * @brief Compare two version strings. PURE: takes no context.
 *
 * It needs nothing from the host, so it crosses no boundary and costs no ABI.
 * It is here rather than in a "misc" header because versions are an
 * application fact, and a reader looking for one looks here.
 *
 * Deliberately a faithful copy of the host's own ordering, quirks included: a
 * leading "v" is dropped, only the components both versions have are compared
 * numerically, and a version with MORE components then sorts after one with
 * fewer, so "2.1.0" is greater than "2.1" rather than equal to it. Modules
 * already depend on this; changing it while moving it would change update
 * prompts.
 *
 * @return negative if @p current < @p latest, 0 if equal, positive if greater
 */
int GFCompareSoftwareVersion(const char* current, const char* latest);

#ifdef __cplusplus
}
#endif
