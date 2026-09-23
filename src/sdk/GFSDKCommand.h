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

#include <stddef.h>
#include <stdint.h>

#include "GFSDKContext.h"

/**
 * @file GFSDKCommand.h
 * @brief Commands: the one way to ask the Host, or another module, to DO
 *        something.
 *
 * A command is a semantic action with a stable id -- "encrypt this document",
 * "open this view" -- and never a method call on an object. Arguments and
 * results are CBOR maps described by the command's schema; secret bytes do
 * not go into the CBOR at all but travel beside it as buffer handles, and the
 * CBOR refers to them by index (`{"$blob": n}`), so plaintext is never copied
 * into memory the Host cannot wipe.
 *
 * Every call goes through the same Host registry, whoever makes it: the
 * Host's own menus, a module in C++, or a module's Lua UI script. The
 * registry checks, in this order, that the command exists, that the caller
 * may invoke it, that the caller holds the capabilities the command needs,
 * that it is enabled, and then routes it to the right thread.
 *
 * Ownership. Every GFBufferRef handed to a function here -- arguments,
 * blobs, results -- is TRANSFERRED to the callee, on success and failure
 * alike. The blob ARRAY is borrowed; its elements are not. No `char*`
 * changes owner in either direction.
 *
 * Most modules never call these directly: GFSDKCommand.hpp derives all of it
 * from a typed C++ command definition.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* The status codes, state bits, callback types and GFCommandSpec are in
 * GFSDKTypes.h, beside the other spec structs, because GFSDKHostApi.h needs
 * them too. */

/* --- the calls ------------------------------------------------------------- */

/** Provide a command. Every one is withdrawn automatically at deactivation. */
int GFCommandRegister(GFSDKContext* ctx, const GFCommandSpec* spec);
int GFCommandUnregister(GFSDKContext* ctx, const char* id);

/**
 * @brief Invoke a command by id. Never blocks, never waits for a thread.
 *
 * @p flags is reserved and must be 0.
 *
 * @return GF_CMD_OK when the call was accepted; @p done then reports the
 *         outcome. Any other value is the outcome, and @p done is not called.
 */
int GFCommandInvoke(GFSDKContext* ctx, const char* id, uint32_t flags,
                    GFBufferRef args_cbor, GFBufferRef* blobs,
                    size_t blob_count, GFCommandDoneFn done, void* user,
                    uint64_t* out_call_id);

/** Finish a call this module's handler received. Exactly once per call. */
int GFCommandComplete(GFSDKContext* ctx, uint64_t call_id, int status,
                      GFBufferRef result_cbor, GFBufferRef* blobs,
                      size_t blob_count, const char* error);

/** Cancel a call this module made. After it returns, `done` never runs. */
int GFCommandCancel(GFSDKContext* ctx, uint64_t call_id);

/** For a handler: whether the caller has cancelled @p call_id. 1 or 0. */
int GFCommandIsCancelled(GFSDKContext* ctx, uint64_t call_id);

/** The CBOR descriptor of a command. Owned; release with GFBufferRelease. */
int GFCommandDescribe(GFSDKContext* ctx, const char* id, GFBufferRef* out);

/** Every registered command id starting with @p prefix ("" for all). */
int GFCommandList(GFSDKContext* ctx, const char* prefix, GFStringListRef* out);

/** A command's state for this caller, as GF_CMD_STATE_* bits. */
int GFCommandQueryState(GFSDKContext* ctx, const char* id, uint32_t* out_bits);

#ifdef __cplusplus
}
#endif
