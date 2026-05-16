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

extern "C" {

/// Maximum allowed string length for SDK string operations (32 MiB).
constexpr int32_t kGfStrlenMax = static_cast<const int32_t>(1024 * 1024 * 32);

/**
 * @brief Callback invoked after a command finishes executing.
 * @param data   User-supplied context pointer passed to GFExecuteCommandSync.
 * @param errcode Exit code returned by the command process.
 * @param out    Null-terminated standard output of the command.
 * @param err    Null-terminated standard error of the command.
 */
using GFCommandExecuteCallback = void (*)(void* data, int errcode,
                                          const char* out, const char* err);

/**
 * @brief Execution context for a single command in a batch operation.
 *
 * Used with GFExecuteCommandBatchSync to submit multiple commands at once.
 * The @p data pointer must be freed by the caller after the callback returns.
 */
using GFCommandExecuteContext = struct {
  char* cmd;                  ///< Command path or name to execute.
  int32_t argc;               ///< Number of arguments in @p argv.
  char** argv;                ///< Argument array of length @p argc.
  GFCommandExecuteCallback cb; ///< Callback invoked on completion.
  void* data;                 ///< User context pointer; must be freed by caller.
};

/**
 * @brief Callback that supplies translation data for a given locale.
 * @param locale BCP-47 locale string (e.g. "en_US").
 * @param data   Output pointer set to a caller-owned buffer with the data.
 * @return 0 on success, non-zero on failure.
 */
using GFTranslatorDataReader = int (*)(const char* locale, char** data);
}