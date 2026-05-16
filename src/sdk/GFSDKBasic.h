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

#include <cstddef>
#include <cstdint>

#include "GFSDKBasicModel.h"

extern "C" {

/**
 * @brief Allocates a block of memory from the module allocator.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated memory, or nullptr on failure.
 */
auto GF_SDK_EXPORT GFAllocateMemory(uint32_t size) -> void*;

/**
 * @brief Resizes a previously allocated memory block.
 * @param ptr  Pointer returned by GFAllocateMemory or GFReallocateMemory.
 * @param size New size in bytes.
 * @return Pointer to the reallocated memory, or nullptr on failure.
 */
auto GF_SDK_EXPORT GFReallocateMemory(void* ptr, uint32_t size) -> void*;

/**
 * @brief Frees a block allocated by GFAllocateMemory or GFReallocateMemory.
 * @param ptr Pointer to the memory block to free.
 */
void GF_SDK_EXPORT GFFreeMemory(void* ptr);

/**
 * @brief Allocates a block of secure memory that is zeroed before release.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated secure memory, or nullptr on failure.
 */
auto GF_SDK_EXPORT GFSecAllocateMemory(uint32_t size) -> void*;

/**
 * @brief Resizes a previously allocated secure memory block.
 * @param ptr  Pointer returned by GFSecAllocateMemory or GFSecReallocateMemory.
 * @param size New size in bytes.
 * @return Pointer to the reallocated secure memory, or nullptr on failure.
 */
auto GF_SDK_EXPORT GFSecReallocateMemory(void* ptr, uint32_t size) -> void*;

/**
 * @brief Frees a block allocated by GFSecAllocateMemory or GFSecReallocateMemory.
 *
 * The memory is zeroed before being released to prevent sensitive data leaks.
 * @param ptr Pointer to the secure memory block to free.
 */
void GF_SDK_EXPORT GFSecFreeMemory(void* ptr);

/**
 * @brief Returns the GpgFrontend application version string (e.g. "2.1.0").
 * @return Caller-owned string; free with GFFreeMemory.
 */
auto GF_SDK_EXPORT GFProjectVersion() -> const char*;

/**
 * @brief Returns the abbreviated git commit hash of the current build.
 * @return Caller-owned string; free with GFFreeMemory.
 */
auto GF_SDK_EXPORT GFProjectGitCommitHash() -> const char*;

/**
 * @brief Returns the Qt version string the application was built against
 *        (e.g. "6.6.1").
 * @return Caller-owned string; free with GFFreeMemory.
 */
auto GF_SDK_EXPORT GFQtEnvVersion() -> const char*;

/**
 * @brief Executes an external command synchronously on a worker thread.
 *
 * Blocks until the command exits, then invokes @p cb with the exit code,
 * stdout, and stderr output.
 *
 * @param cmd  Path or name of the command to run.
 * @param argc Number of arguments in @p argv.
 * @param argv Argument array of length @p argc.
 * @param cb   Callback invoked with (data, exit_code, stdout, stderr).
 * @param data User context pointer forwarded to @p cb.
 */
void GF_SDK_EXPORT GFExecuteCommandSync(const char* cmd, int32_t argc,
                                        char** argv,
                                        GFCommandExecuteCallback cb,
                                        void* data);

/**
 * @brief Executes multiple commands concurrently and waits for all to finish.
 *
 * Each context's callback is invoked as its command completes.
 *
 * @param contexts      Array of pointers to GFCommandExecuteContext structures.
 * @param contexts_size Number of entries in @p contexts.
 */
void GF_SDK_EXPORT GFExecuteCommandBatchSync(GFCommandExecuteContext** contexts,
                                             int32_t contexts_size);

/**
 * @brief Duplicates a string using the module allocator.
 *
 * The returned pointer must be freed with GFFreeMemory.
 *
 * @param src Null-terminated source string (max length kGfStrlenMax).
 * @return Caller-owned copy, or nullptr if @p src exceeds kGfStrlenMax.
 */
auto GF_SDK_EXPORT GFModuleStrDup(const char* src) -> char*;

/**
 * @brief Duplicates a string using the secure allocator.
 *
 * The memory is zeroed when freed. Use for sensitive strings such as
 * passphrases. The returned pointer must be freed with GFSecFreeMemory.
 *
 * @param src Null-terminated source string (max length kGfStrlenMax).
 * @return Caller-owned secure copy, or nullptr if @p src exceeds kGfStrlenMax.
 */
auto GF_SDK_EXPORT GFModuleSecStrDup(const char* src) -> char*;

/**
 * @brief Returns the active locale name of the application (e.g. "en_US").
 * @return Caller-owned string; free with GFFreeMemory.
 */
auto GF_SDK_EXPORT GFAppActiveLocale() -> char*;

/**
 * @brief Registers a translator data reader callback for a given module.
 *
 * The reader is called when the UI needs translation data for the active
 * locale. It must fill @p data with a caller-allocated buffer containing the
 * raw translation data for the requested locale.
 *
 * @param id     Unique string identifier for this translator reader.
 * @param reader Callback that supplies translation data for a locale.
 * @return 0 on success, -1 on failure.
 */
auto GF_SDK_EXPORT GFAppRegisterTranslatorReader(const char* id,
                                                 GFTranslatorDataReader reader)
    -> int;

/**
 * @brief Saves a string value to the in-memory cache.
 * @param key   Cache key.
 * @param value Value to store.
 * @return 0 on success.
 */
auto GF_SDK_EXPORT GFCacheSave(const char* key, const char* value) -> int;

/**
 * @brief Saves a string value to the in-memory cache with an expiry time.
 * @param key   Cache key.
 * @param value Value to store.
 * @param ttl   Time-to-live in seconds; the entry expires after this duration.
 * @return 0 on success.
 */
auto GF_SDK_EXPORT GFCacheSaveWithTTL(const char* key, const char* value,
                                      int ttl) -> int;

/**
 * @brief Retrieves a value from the persistent (durable) cache.
 *
 * The value is returned as a JSON string. The key is automatically namespaced
 * under the module prefix.
 *
 * @param key Cache key.
 * @return Caller-owned JSON string, or nullptr if not found.
 *         Free with GFFreeMemory.
 */
auto GF_SDK_EXPORT GFDurableCacheGet(const char* key) -> const char*;

/**
 * @brief Saves a JSON string value to the persistent (durable) cache.
 *
 * The key is automatically namespaced under the module prefix. The value must
 * be a valid JSON document.
 *
 * @param key   Cache key.
 * @param value JSON-encoded value to store.
 * @return 0 on success.
 */
auto GF_SDK_EXPORT GFDurableCacheSave(const char* key, const char* value)
    -> int;

/**
 * @brief Retrieves a value from the in-memory cache.
 * @param key Cache key.
 * @return Caller-owned string, or nullptr if the key is not present.
 *         Free with GFFreeMemory.
 */
auto GF_SDK_EXPORT GFCacheGet(const char* key) -> const char*;

/**
 * @brief Returns whether the application is running inside a Flatpak sandbox.
 * @return true if running under Flatpak, false otherwise.
 */
auto GF_SDK_EXPORT GFIsFlatpakENV() -> bool;
}