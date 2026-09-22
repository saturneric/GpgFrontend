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

#include "GFSDKContext.h"

/**
 * @file GFSDKPgp.h
 * @brief Reading OpenPGP structure with no keyring and no engine.
 *
 * Its own capability, separate from "gpg": inspecting packet structure
 * touches no key material and runs no engine, so a module that only needs to
 * describe a message should not have to ask for the ability to decrypt one.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Describe @p in as a packet structure.
 *
 * JSON shape:
 *
 *   { "format": "binary"|"armored"|"cleartext", "size": N, "errors": [...],
 *     "blocks": [ { "kind": ..., "offset": N, "armor": {...}|absent,
 *                   "cleartext": {...}|absent, "error": null|"...",
 *                   "packets": [ { "tag": N, "tagName": "...", "offset": N,
 *                                  "headerVersion": "old"|"new",
 *                                  "headerLength": N, "bodyLength": N,
 *                                  "lengthType": "fixed"|"partial"
 *                                                |"indeterminate",
 *                                  "version": N, "fields": [{label,value}],
 *                                  "children": [ ...same shape... ],
 *                                  "error": null|"..." } ] } ] }
 *
 * A packet's `offset` is relative to the start of the packet stream it lives
 * in, the dearmored octets for an armored block, and the framing figures
 * describe the bytes as they appear there, so consecutive packets always
 * satisfy `offset + headerLength + bodyLength == next offset`.
 *
 * @param in borrowed
 * @param out owned on success; release with GFBufferRelease
 * @return 0 on success, negative on failure
 */
int GFPgpInspectData(GFSDKContext* ctx, GFBufferView in, GFBufferRef* out);

#ifdef __cplusplus
}
#endif
