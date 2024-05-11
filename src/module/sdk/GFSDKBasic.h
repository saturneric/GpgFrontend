/**
 * Copyright (C) 2021 Saturneric <eric@bktus.com>
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

#include "GFSDKExport.h"

extern "C" {

constexpr int32_t kGfStrlenMax = static_cast<const int32_t>(1024 * 8);

using GFCommandExeucteCallback = void (*)(void* data, int errcode,
                                          const char* out, const char* err);

using GFCommandExecuteContext = struct {
  const char* cmd;
  int32_t argc;
  const char** argv;
  GFCommandExeucteCallback cb;
  void* data;
};

/**
 * @brief
 *
 * @param size
 * @return void*
 */
auto GPGFRONTEND_MODULE_SDK_EXPORT GFAllocateMemory(uint32_t size) -> void*;

/**
 * @brief
 *
 * @return const char*
 */
auto GPGFRONTEND_MODULE_SDK_EXPORT GFProjectVersion() -> const char*;

/**
 * @brief
 *
 * @return const char*
 */
auto GPGFRONTEND_MODULE_SDK_EXPORT GFQtEnvVersion() -> const char*;

/**
 * @brief
 *
 */
void GPGFRONTEND_MODULE_SDK_EXPORT GFFreeMemory(void*);

/**
 * @brief
 *
 * @param cmd
 * @param argc
 * @param argv
 * @param cb
 * @param data
 */
void GPGFRONTEND_MODULE_SDK_EXPORT
GFExecuteCommandSync(const char* cmd, int32_t argc, const char** argv,
                     GFCommandExeucteCallback cb, void* data);

/**
 * @brief
 *
 * @param context_size
 * @param context
 */
void GPGFRONTEND_MODULE_SDK_EXPORT GFExecuteCommandBatchSync(
    int32_t context_size, const GFCommandExecuteContext* context);

/**
 * @brief
 *
 * @return char*
 */
auto GPGFRONTEND_MODULE_SDK_EXPORT GFModuleStrDup(const char*) -> char*;
}