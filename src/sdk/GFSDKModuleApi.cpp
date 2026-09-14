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

#include "GFSDKModuleApi.h"

#include "GFSDKBuffer.h"
#include "GFSDKBuildInfo.h"
#include "GFSDKGpgResult.h"
#include "GFSDKLog.h"

/**
 * @file GFSDKModuleApi.cpp
 * @brief The host's side of the runtime ABI.
 *
 * Nothing here does any work of its own: it is deliberately a plain table of
 * the real entry points, so that "what a module can call" and "what the SDK
 * exports" cannot drift apart. Adding an entry point to the SDK without
 * adding it here simply means modules cannot reach it, which is a visible
 * omission rather than a silent inconsistency.
 */

auto GFGetHostApi() -> const GFHostApi* {
  // Static, so the pointer handed to a module stays valid for the life of the
  // process and the module never has to think about its lifetime.
  static const GFHostApi kHostApi = {
      sizeof(GFHostApi),
      GF_SDK_ABI_VERSION,

      &GFBufferNewFromBytes,
      &GFBufferData,
      &GFBufferSize,
      &GFBufferZeroize,
      &GFBufferRelease,

      &GFGpgSign,
      &GFGpgEncrypt,
      &GFGpgDecrypt,
      &GFGpgVerify,

      &GFGpgResultStatusOf,
      &GFGpgResultError,
      &GFGpgResultData,
      &GFGpgResultCapsuleId,
      &GFGpgResultErrorString,
      &GFGpgResultHashAlgo,
      &GFGpgResultTakeData,
      &GFGpgResultRelease,

      &GFModuleLogDebug,
      &GFModuleLogInfo,
      &GFModuleLogWarn,
      &GFModuleLogError,
  };

  return &kHostApi;
}
