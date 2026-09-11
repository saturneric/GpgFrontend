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

/**
 * @file SecureMemoryAllocator.h
 * @brief Application-wide memory allocation API with optional secure wiping and
 * libsodium guarded memory.
 *
 * Two allocation tiers are provided:
 * - Standard (SMAMalloc / SMARealloc / SMAFree): for ordinary allocations.
 *   Tracks allocations and wipes memory on free when the application secure
 *   level is >= 1; below that it is plain malloc and free, untracked.
 * - Secure (SMASecMalloc / SMASecRealloc / SMASecFree): for secrets. Always
 *   tracked and **always wiped on release, at every secure level**. Above
 *   level 2 it additionally uses libsodium guarded memory (sodium_malloc /
 *   sodium_free), which locks the pages against swap.
 *
 * The wipe is a property of the secure tier rather than of the caller, so a
 * secret does not depend on every release site remembering to zero it, nor on
 * a secure level the user has no reason to have raised. Growing a secure block
 * never uses plain realloc either: the contents are moved and the block left
 * behind is wiped, instead of being handed back to the heap intact.
 *
 * Memory allocated through these functions must be freed with the corresponding
 * SMA* free function and must not be passed to raw free(). Releasing a secure
 * block through SMAFree is recognised and still wipes it, but it is a mistake
 * worth fixing.
 */

namespace GpgFrontend {

/**
 * @brief Allocate @p size bytes of zeroed memory.
 *
 * @param size number of bytes to allocate
 * @return pointer to allocated memory, or nullptr if size is zero or allocation
 * fails
 */
auto GF_CORE_EXPORT SMAMalloc(size_t size) -> void*;

/**
 * @brief Resize a previously SMAMalloc-allocated block to @p size bytes.
 *
 * If @p ptr is nullptr, behaves like SMAMalloc. If @p size is zero, frees
 * the block and returns nullptr.
 *
 * @param ptr pointer to an existing SMAMalloc-allocated block, or nullptr
 * @param size new size in bytes
 * @return pointer to the resized block, or nullptr on failure
 */
auto GF_CORE_EXPORT SMARealloc(void* ptr, size_t size) -> void*;

/**
 * @brief Free a block allocated by SMAMalloc or SMARealloc.
 *
 * Wipes the memory before freeing when the application secure level is >= 1.
 *
 * @param ptr pointer to the block to free, or nullptr (no-op)
 */
void GF_CORE_EXPORT SMAFree(void* ptr);

/**
 * @brief Allocate @p size bytes of zeroed secure memory.
 *
 * Uses libsodium guarded memory (sodium_malloc) when the application secure
 * level is >= 2, and ordinary tracked memory below that. Either way the block
 * is registered, so its size is known when it is released and it can be wiped.
 *
 * @param size number of bytes to allocate
 * @return pointer to allocated secure memory, or nullptr if size is zero or
 * allocation fails
 */
auto GF_CORE_EXPORT SMASecMalloc(size_t size) -> void*;

/**
 * @brief Resize a previously SMASecMalloc-allocated block to @p size bytes.
 *
 * If @p ptr is nullptr, behaves like SMASecMalloc. If @p size is zero, frees
 * the block and returns nullptr. The contents are moved to the new block and
 * the old one is wiped before release; plain realloc is never used, as it
 * would leave a full copy of the secret in freed memory.
 *
 * @param ptr pointer to an existing SMASecMalloc-allocated block, or nullptr
 * @param size new size in bytes
 * @return pointer to the resized block, or nullptr on failure
 */
auto GF_CORE_EXPORT SMASecRealloc(void* ptr, size_t size) -> void*;

/**
 * @brief Free a block allocated by SMASecMalloc or SMASecRealloc.
 *
 * Always erases the contents first: sodium_free zeroes the guarded pages it
 * releases, and an ordinary block is wiped by hand before it is freed. This
 * holds at every secure level.
 *
 * @param ptr pointer to the block to free, or nullptr (no-op)
 */
void GF_CORE_EXPORT SMASecFree(void* ptr);

}  // namespace GpgFrontend
