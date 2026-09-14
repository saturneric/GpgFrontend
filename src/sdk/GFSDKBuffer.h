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

/**
 * @file GFSDKBuffer.h
 * @brief Opaque, always-wiped byte buffers that cross the module boundary.
 *
 * OWNERSHIP MODEL -- three rules, no exceptions anywhere in the SDK surface:
 *
 *   1. Arguments are ALWAYS borrowed. The SDK never frees an argument. Inputs
 *      are GFBufferView or `const char*`; the caller keeps and releases
 *      whatever it created.
 *   2. Owned handles are transferred ONLY by a return value or an out-param,
 *      and only from a `*New*` constructor or an explicit `*Take*` accessor.
 *      If the name contains neither, nothing was transferred.
 *   3. Accessors return BORROWED views whose lifetime is the owning object's.
 *      Nothing reachable from a handle is separately releasable, so there is
 *      exactly one release per owned object and no per-field teardown.
 *
 * Owned and borrowed are DIFFERENT TYPES, so the contract is visible in every
 * signature and a borrowed view cannot reach a release function at all:
 * passing an accessor's result to GFBufferRelease is a type error, not a
 * double free.
 *
 * The payload always lives on the application's secure allocator, which wipes
 * at every secure level and uses mlocked pages at secure level >= 2. There is
 * no tier flag and no way to ask for the ordinary allocator, so a module
 * cannot pick the wrong deallocator and decrypted plaintext cannot land in
 * unwiped memory.
 *
 * THREADING: a handle has a single owner. A borrowed view is valid only
 * within that owner's lifetime, on the owning thread. Sharing a handle across
 * threads requires the module's own synchronisation, exactly as any other
 * object it owns would. The live-handle registry detects double release and
 * same-thread use-after-release; it does NOT make concurrent release safe.
 */

#if defined(__GNUC__) || defined(__clang__)
#define GF_SDK_MUST_USE __attribute__((warn_unused_result))
#else
#define GF_SDK_MUST_USE
#endif

/* Clang's static analyser understands this ownership protocol directly, so
   clang-analyzer-unix.Malloc reports a leaked or double-released handle by
   path, at analysis time, with no runtime cost. */
#if defined(__clang__)
#define GF_SDK_OWNERSHIP_RETURNS __attribute__((ownership_returns(gfbuffer)))
#define GF_SDK_OWNERSHIP_TAKES __attribute__((ownership_takes(gfbuffer, 1)))
#else
#define GF_SDK_OWNERSHIP_RETURNS
#define GF_SDK_OWNERSHIP_TAKES
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque owning handle. Exactly one GFBufferRelease per handle. */
typedef struct GFBufferImpl* GFBufferRef;

/** Opaque borrowed view. Cannot be released; valid only while its owner is. */
typedef const struct GFBufferImpl* GFBufferView;

/**
 * @brief Copy @p size octets into a new buffer, verbatim.
 *
 * Embedded NUL bytes are copied like any other byte: @p size is
 * authoritative, and nothing stops at a terminator. This is the entry point
 * that makes the old NUL-truncation class of bug structurally impossible.
 *
 * There is deliberately no GFBufferNew(size) and no mutable-data accessor:
 * the backing GFBuffer detaches under copy-on-write when written through, so
 * a mutable accessor across the ABI could silently allocate a fresh copy of a
 * secret. Constructing in one shot removes the need, and the hazard with it.
 *
 * @param data source octets; may be NULL only when @p size is 0
 * @param size number of octets to copy
 * @return owned handle, or NULL on failure. Release with GFBufferRelease.
 */
GF_SDK_EXPORT GFBufferRef GF_SDK_MUST_USE GF_SDK_OWNERSHIP_RETURNS
GFBufferNewFromBytes(const void* data, size_t size);

/**
 * @brief Borrowed pointer to the octets. NULL for a NULL or stale handle.
 *
 * Valid until the owning handle is released. Never free this pointer.
 */
GF_SDK_EXPORT const void* GFBufferData(GFBufferView buf);

/** @brief Byte count. 0 for a NULL or stale handle. */
GF_SDK_EXPORT size_t GFBufferSize(GFBufferView buf);

/**
 * @brief Erase the contents now, before the handle dies.
 *
 * Wipes through every share of the same storage rather than detaching, since
 * this means "erase this secret from memory now" -- detaching would wipe a
 * fresh private copy and leave the original sitting in memory.
 */
GF_SDK_EXPORT void GFBufferZeroize(GFBufferRef buf);

/**
 * @brief Wipe and free. The only release function. NULL-safe.
 *
 * Releasing twice is detected and reported rather than corrupting the heap:
 * the handle is validated against the live-handle registry BEFORE it is
 * dereferenced, so no freed memory is ever read to make that decision.
 */
GF_SDK_EXPORT void GF_SDK_OWNERSHIP_TAKES GFBufferRelease(GFBufferRef buf);

/**
 * @brief How many buffers this module currently holds. Test/diagnostic hook.
 *
 * Lets a test assert the ledger balances across an operation, including its
 * error paths. Pass NULL for the process-wide total.
 */
GF_SDK_EXPORT size_t GFBufferOutstandingCount(const char* module_id);

#ifdef __cplusplus
}
#endif
