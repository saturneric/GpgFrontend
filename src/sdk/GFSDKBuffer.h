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
#include <stdint.h>

#include "GFSDKContext.h"

/**
 * @file GFSDKBuffer.h
 * @brief Octets, and the two allocators they can come from.
 *
 * OWNERSHIP, the three rules the whole SDK follows:
 *   - an argument is BORROWED. Pass a pointer straight through and keep
 *     owning it; nothing on the other side frees it.
 *   - an owned handle comes back only from an out-parameter or a name that
 *     says *Take*.
 *   - every accessor returns a BORROWED view valid until the owner is
 *     released. Nothing reachable from a handle is separately releasable.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Copy @p size octets into a new buffer, verbatim.
 *
 * Embedded NUL bytes are copied like any other byte: @p size is
 * authoritative and nothing stops at a terminator. This is the entry point
 * that makes the old NUL-truncation class of bug structurally impossible.
 *
 * There is deliberately no GFBufferNew(size) and no mutable-data accessor:
 * the backing buffer detaches under copy-on-write when written through, so a
 * mutable accessor across the ABI could silently allocate a fresh copy of a
 * secret. Constructing in one shot removes the need, and the hazard with it.
 */
GFBufferRef GFBufferNewFromBytes(GFSDKContext* ctx, const void* data,
                                 size_t size);

/** @brief Borrowed pointer to the octets. NULL for a NULL or stale handle. */
const void* GFBufferData(GFSDKContext* ctx, GFBufferView buf);

/** @brief Byte count. 0 for a NULL or stale handle. */
size_t GFBufferSize(GFSDKContext* ctx, GFBufferView buf);

/**
 * @brief Erase the contents now, before the handle dies.
 *
 * Wipes through every share of the same storage rather than detaching, since
 * this means "erase this secret from memory now"; detaching would wipe a
 * fresh private copy and leave the original sitting in memory.
 */
void GFBufferZeroize(GFSDKContext* ctx, GFBufferRef buf);

/** @brief Wipe and free. The only release function. NULL-safe. */
void GFBufferRelease(GFSDKContext* ctx, GFBufferRef buf);

/**
 * @brief How many buffers this module currently holds. Test hook.
 *
 * Answers about the module the context identifies, which is the only module
 * it could honestly answer about.
 */
size_t GFBufferOutstandingCount(GFSDKContext* ctx);

/* --- raw memory ----------------------------------------------------------
 *
 * One family with an arena, not two families. The ordinary and wiping
 * allocators differ in exactly one property, and the old split meant every
 * allocation function existed twice. Freeing from the wrong arena is still a
 * fatal mistake, so the arena travels with the call rather than being
 * remembered by the caller.
 *
 * Prefer a GFBufferRef to raw memory wherever the size is known: a buffer
 * wipes itself on release and is counted in the ledger above.
 */

/** @param arena one of @ref GFMemoryArena */
void* GFMemAlloc(GFSDKContext* ctx, int arena, uint32_t size);
void* GFMemRealloc(GFSDKContext* ctx, int arena, void* p, uint32_t size);
void GFMemFree(GFSDKContext* ctx, int arena, void* p);

/** @brief Copy a NUL-terminated string into @p arena. */
char* GFMemStrDup(GFSDKContext* ctx, int arena, const char* s);

#ifdef __cplusplus
}
#endif
