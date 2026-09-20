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

#include "GFSDKBuffer.h"
#include "GFSDKVisibility.h"

/**
 * @file GFSDKPgp.h
 * @brief Reading OpenPGP data as a structure, without a key database.
 *
 * Deliberately separate from GFSDKGpg.h: everything there addresses a keyring
 * through a channel, and nothing here does. Inspection answers "what is this
 * blob made of", which needs no keys, no engine and no channel.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Describe the packet structure of an OpenPGP blob as JSON.
 *
 * Accepts anything: armored or binary, message, detached signature,
 * certificate or cleartext-signed text. Encrypted payloads are NEVER
 * decrypted -- an encrypted container is reported by its envelope alone.
 * Compressed containers are recursed into, under a size and depth cap.
 *
 * Input that is not OpenPGP at all still succeeds: the document that comes
 * back says so in its `errors` array. A non-zero return means the inspector
 * itself could not run, which is a different thing from unparsable input.
 *
 * The document is UTF-8 JSON:
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
 * in -- the dearmored octets, for an armored block -- and the framing figures
 * describe the bytes as they appear there, so consecutive packets always
 * satisfy `offset + headerLength + bodyLength == next offset`.
 *
 * @param in the data to inspect. BORROWED; the caller keeps it.
 * @param[out] out_json receives a caller-owned UTF-8 JSON string on success.
 *             Free it with GFFreeMemory. Untouched on failure.
 * @return 0 on success, negative on failure.
 */
GF_SDK_EXPORT int GFPgpInspectData(GFBufferView in, char** out_json);

#ifdef __cplusplus
}
#endif
