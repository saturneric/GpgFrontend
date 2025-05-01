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

// MAX STRLEN -> 64 MB
constexpr int32_t kGfStrlenMax = static_cast<const int32_t>(1024 * 1024 * 32);

using GFCommandExecuteCallback = void (*)(void* data, int errcode,
                                          const char* out, const char* err);

using GFCommandExecuteContext = struct {
  char* cmd;
  int32_t argc;
  char** argv;
  GFCommandExecuteCallback cb;
  void* data;  ///< must free by user
};

using GFTranslatorDataReader = int (*)(const char* locale, char** data);
}