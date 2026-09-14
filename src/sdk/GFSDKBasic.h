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

#include "GFSDKBasicModel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocates a block of memory from the module allocator.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated memory, or nullptr on failure.
 */
GF_SDK_EXPORT void* GFAllocateMemory(uint32_t size);

/**
 * @brief Resizes a previously allocated memory block.
 * @param ptr  Pointer returned by GFAllocateMemory or GFReallocateMemory.
 * @param size New size in bytes.
 * @return Pointer to the reallocated memory, or nullptr on failure.
 */
GF_SDK_EXPORT void* GFReallocateMemory(void* ptr, uint32_t size);

/**
 * @brief Frees a block allocated by GFAllocateMemory or GFReallocateMemory.
 * @param ptr Pointer to the memory block to free.
 */
GF_SDK_EXPORT void GFFreeMemory(void* ptr);

/**
 * @brief Allocates a block of secure memory that is zeroed before release.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated secure memory, or nullptr on failure.
 */
GF_SDK_EXPORT void* GFSecAllocateMemory(uint32_t size);

/**
 * @brief Resizes a previously allocated secure memory block.
 * @param ptr  Pointer returned by GFSecAllocateMemory or GFSecReallocateMemory.
 * @param size New size in bytes.
 * @return Pointer to the reallocated secure memory, or nullptr on failure.
 */
GF_SDK_EXPORT void* GFSecReallocateMemory(void* ptr, uint32_t size);

/**
 * @brief Frees a block allocated by GFSecAllocateMemory or
 * GFSecReallocateMemory.
 *
 * The memory is zeroed before being released to prevent sensitive data leaks.
 * @param ptr Pointer to the secure memory block to free.
 */
GF_SDK_EXPORT void GFSecFreeMemory(void* ptr);

/**
 * @brief Returns the GpgFrontend application version string (e.g. "2.1.0").
 * @return Borrowed string with process lifetime. Do NOT free.
 */
GF_SDK_EXPORT const char* GFProjectVersion();

/**
 * @brief Returns the abbreviated git commit hash of the current build.
 * @return Borrowed string with process lifetime. Do NOT free.
 */
GF_SDK_EXPORT const char* GFProjectGitCommitHash();

/**
 * @brief Returns the Qt version string the application was built against
 *        (e.g. "6.6.1").
 * @return Borrowed string with process lifetime. Do NOT free.
 */
GF_SDK_EXPORT const char* GFQtEnvVersion();

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
GF_SDK_EXPORT void GFExecuteCommandSync(const char* cmd, int32_t argc,
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
GF_SDK_EXPORT void GFExecuteCommandBatchSync(GFCommandExecuteContext** contexts,
                                             int32_t contexts_size);

/**
 * @brief Duplicates a string using the module allocator.
 *
 * The returned pointer must be freed with GFFreeMemory.
 *
 * @param src Null-terminated source string (max length kGfStrlenMax).
 * @return Caller-owned copy, or nullptr if @p src exceeds kGfStrlenMax.
 */
GF_SDK_EXPORT char* GFModuleStrDup(const char* src);

/**
 * @brief Duplicates a string using the secure allocator.
 *
 * The memory is zeroed when freed. Use for sensitive strings such as
 * passphrases. The returned pointer must be freed with GFSecFreeMemory.
 *
 * @param src Null-terminated source string (max length kGfStrlenMax).
 * @return Caller-owned secure copy, or nullptr if @p src exceeds kGfStrlenMax.
 */
GF_SDK_EXPORT char* GFModuleSecStrDup(const char* src);

/**
 * @brief Returns the active locale name of the application (e.g. "en_US").
 * @return Caller-owned string; free with GFFreeMemory.
 */
GF_SDK_EXPORT char* GFAppActiveLocale();

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
GF_SDK_EXPORT int GFAppRegisterTranslatorReader(const char* id,
                                                GFTranslatorDataReader reader);

/**
 * @brief Saves a string value to the in-memory cache.
 * @param key   Cache key.
 * @param value Value to store.
 * @return 0 on success.
 */
GF_SDK_EXPORT int GFCacheSave(const char* key, const char* value);

/**
 * @brief Saves a string value to the in-memory cache with an expiry time.
 * @param key   Cache key.
 * @param value Value to store.
 * @param ttl   Time-to-live in seconds; the entry expires after this duration.
 * @return 0 on success.
 */
GF_SDK_EXPORT int GFCacheSaveWithTTL(const char* key, const char* value,
                                     int ttl);

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
GF_SDK_EXPORT const char* GFDurableCacheGet(const char* key);

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
GF_SDK_EXPORT int GFDurableCacheSave(const char* key, const char* value);

/**
 * @brief Retrieves a secret from the durable cache's secure tier.
 *
 * Same encrypted-at-rest store as GFDurableCacheGet and namespaced the same
 * way, but the value travels through zeroizing memory on both sides, so a
 * password does not linger in a freed heap block. Use this, never
 * GFDurableCacheGet, for anything the user would call a secret.
 *
 * @param key Cache key.
 * @return Caller-owned string, or nullptr if not found.
 *         Free with GFSecFreeMemory, NOT GFFreeMemory.
 */
GF_SDK_EXPORT char* GFSecDurableCacheGet(const char* key);

/**
 * @brief Stores a secret in the durable cache's secure tier.
 *
 * Written through immediately rather than waiting for the periodic flush: a
 * credential the user just entered must survive a crash before the next tick.
 *
 * Both arguments are taken by value in the SDK sense: this call frees them.
 * @p key must come from GFModuleStrDup (or QDUP) and @p value from
 * GFModuleSecStrDup (or QSECDUP) -- passing a pointer owned by anything else,
 * such as a QByteArray's internal buffer, aborts the process.
 *
 * @param key   Cache key.
 * @param value Secret to store. Wiped and released by this call.
 * @return 0 on success.
 */
GF_SDK_EXPORT int GFSecDurableCacheSave(const char* key, const char* value);

/**
 * @brief Removes a secret from the durable cache's secure tier.
 *
 * @param key Cache key.
 * @return 0 on success, including when nothing was stored under @p key.
 */
GF_SDK_EXPORT int GFSecDurableCacheRemove(const char* key);

/**
 * @brief How the application secure key itself is protected.
 *
 * Everything in the durable cache is encrypted under that key, so this is the
 * ceiling on how well a module can protect anything it stores. A module that
 * persists a credential is expected to consult this and decline to do so
 * silently when the answer is 0.
 *
 * @return 0 = unprotected (key file is plaintext), 1 = system keychain,
 *         2 = user PIN, -1 = unknown.
 */
GF_SDK_EXPORT int GFAppKeyProtectionLevel();

/**
 * @brief Retrieves a value from the in-memory cache.
 * @param key Cache key.
 * @return Caller-owned string, or nullptr if the key is not present.
 *         Free with GFFreeMemory.
 */
GF_SDK_EXPORT const char* GFCacheGet(const char* key);

/**
 * @brief Returns whether the application is running inside a Flatpak sandbox.
 * @return non-zero if running under Flatpak, 0 otherwise.
 */
GF_SDK_EXPORT int GFIsFlatpakENV();
#ifdef __cplusplus
}
#endif