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
 * @brief
 *
 * @param size
 * @return void*
 */
auto GF_SDK_EXPORT GFAllocateMemory(uint32_t size) -> void*;

/**
 * @brief
 *
 * @param ptr
 * @param size
 * @return void*
 */
auto GF_SDK_EXPORT GFReallocateMemory(void* ptr, uint32_t size) -> void*;

/**
 * @brief
 *
 */
void GF_SDK_EXPORT GFFreeMemory(void*);

/**
 * @brief
 *
 * @param size
 * @return void*
 */
auto GF_SDK_EXPORT GFSecAllocateMemory(uint32_t size) -> void*;

/**
 * @brief
 *
 * @param ptr
 * @param size
 * @return void*
 */
auto GF_SDK_EXPORT GFSecReallocateMemory(void* ptr, uint32_t size) -> void*;

/**
 * @brief
 *
 */
void GF_SDK_EXPORT GFSecFreeMemory(void*);

/**
 * @brief
 *
 * @return const char*
 */
auto GF_SDK_EXPORT GFProjectVersion() -> const char*;

/**
 * @brief
 *
 * @return const char*
 */
auto GF_SDK_EXPORT GFProjectGitCommitHash() -> const char*;

/**
 * @brief
 *
 * @return const char*
 */
auto GF_SDK_EXPORT GFQtEnvVersion() -> const char*;

/**
 * @brief
 *
 * @param cmd
 * @param argc
 * @param argv
 * @param cb
 * @param data
 */
void GF_SDK_EXPORT GFExecuteCommandSync(const char* cmd, int32_t argc,
                                        char** argv,
                                        GFCommandExecuteCallback cb,
                                        void* data);

/**
 * @brief
 *
 * @param context_size
 * @param context
 */
void GF_SDK_EXPORT GFExecuteCommandBatchSync(GFCommandExecuteContext** contexts,
                                             int32_t contexts_size);

/**
 * @brief
 *
 * @return char*
 */
auto GF_SDK_EXPORT GFModuleStrDup(const char*) -> char*;

/**
 * @brief
 *
 * @return char*
 */
auto GF_SDK_EXPORT GFModuleSecStrDup(const char*) -> char*;

/**
 * @brief
 *
 * @return char*
 */
auto GF_SDK_EXPORT GFAppActiveLocale() -> char*;

/**
 * @brief
 *
 * @param data
 * @param size
 * @return auto
 */
auto GF_SDK_EXPORT GFAppRegisterTranslatorReader(const char* id,
                                                 GFTranslatorDataReader reader)
    -> int;

/**
 * @brief
 *
 * @param key
 * @param value
 * @return auto
 */
auto GF_SDK_EXPORT GFCacheSave(const char* key, const char* value) -> int;

/**
 * @brief
 *
 * @param key
 * @param value
 * @return auto
 */
auto GF_SDK_EXPORT GFCacheSaveWithTTL(const char* key, const char* value,
                                      int ttl) -> int;

/**
 * @brief
 *
 * @param key
 * @param value
 * @return auto
 */
auto GF_SDK_EXPORT GFDurableCacheGet(const char* key) -> const char*;

/**
 * @brief
 *
 * @param key
 * @param value
 * @return auto
 */
auto GF_SDK_EXPORT GFDurableCacheSave(const char* key, const char* value)
    -> int;

/**
 * @brief
 *
 * @param key
 * @return const char*
 */
auto GF_SDK_EXPORT GFCacheGet(const char* key) -> const char*;

/**
 * @brief
 *
 * @return true
 * @return false
 */
auto GF_SDK_EXPORT GFIsFlatpakENV() -> bool;

/**
 * @brief
 *
 * @return true
 * @return false
 */
auto GF_SDK_EXPORT GFIsCheckReleaseCommitHash() -> bool;
}