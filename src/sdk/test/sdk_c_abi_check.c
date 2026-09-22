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

/**
 * @file sdk_c_abi_check.c
 * @brief Proves the public SDK headers are consumable from C.
 *
 * Not a behavioural test: it never runs. It is compiled as C11 by its own
 * target so that a C++-only construct creeping into a public header -- a
 * trailing return type, `bool`, `constexpr`, `using X = ...`, an unguarded
 * `extern "C"` -- fails the BUILD rather than being noticed in review.
 *
 * There is no export macro to stub any more, following the precedent in
 * modules/src/m_email/test/CMakeLists.txt, so new headers stay stub-able.
 */

/* Every public header, so that one of them failing to stand on its own in C
   is a build failure here rather than a surprise for an out-of-tree module
   that does not happen to include them in the order this tree does. */
#include "GFSDKApp.h"
#include "GFSDKBuffer.h"
#include "GFSDKContext.h"
#include "GFSDKEditor.h"
#include "GFSDKGpg.h"
#include "GFSDKGpgList.h"
#include "GFSDKGpgResult.h"
#include "GFSDKHostApi.h"
#include "GFSDKLog.h"
#include "GFSDKModuleApi.h"
#include "GFSDKPgp.h"
#include "GFSDKProcess.h"
#include "GFSDKStorage.h"
#include "GFSDKTypes.h"
#include "GFSDKUI.h"

/* Force the compiler to actually instantiate a few declarations rather than
   skipping over them: taking a function's address needs its full type. */
int gf_sdk_c_abi_check(void);

int gf_sdk_c_abi_check(void) {
  void* fns[5];
  fns[0] = (void*)&GFMemAlloc;
  fns[1] = (void*)&GFBufferNewFromBytes;
  fns[2] = (void*)&GFBufferRelease;
  fns[3] = (void*)&GFLogAt;
  /* A pure helper too: it must be callable with no context at all, which is
     the property that makes "pure" mean something here. */
  fns[4] = (void*)&GFCompareSoftwareVersion;
  return fns[0] != NULL ? 0 : 1;
}
