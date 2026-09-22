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

#include "GFSDKPgp.h"

#include "GFSdkInternal.h"

/**
 * @file GFSdkPgp.cpp
 * @brief Packet-structure inspection, module-side.
 *
 * The reference case for the whole arrangement, and worth reading as one.
 * This function is public SDK and lives module-side. It calls exactly one
 * primitive. The primitive, in src/sdk/host/, is the only code that may name
 * GpgFrontend::InspectOpenPGPData. There is no path from here to Core.
 */

auto GFPgpInspectData(GFSDKContext* ctx, GFBufferView in, GFBufferRef* out)
    -> int {
  GF_SDK_REQUIRE(ctx, pgp, "GFPgpInspectData", -1);
  return g->inspect(hctx, in, out);
}
