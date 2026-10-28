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

#include "GFSDKArchive.h"

#include "GFSdkInternal.h"

auto GFArchiveZipWrite(GFSDKContext* ctx, const GFArchiveEntry* entries,
                       size_t count, GFBufferRef* out) -> int {
  GF_SDK_REQUIRE(ctx, archive, "GFArchiveZipWrite", -1);
  return g->zip_write(hctx, entries, count, out);
}

auto GFArchiveZipRead(GFSDKContext* ctx, GFBufferView in,
                      const GFArchiveLimits* limits, GFArchiveEntryFn fn,
                      void* user, GFBufferRef* reason) -> int {
  GF_SDK_REQUIRE(ctx, archive, "GFArchiveZipRead", -1);
  return g->zip_read(hctx, in, limits, fn, user, reason);
}
